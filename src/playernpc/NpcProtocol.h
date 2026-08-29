// NpcProtocol.h - 假玩家 NPC 协议层（1.16.0）
//
// 移植自 SCustomNpc VisualPacket.h, 适配 HologramLib 发送通道
// （MinecraftPackets 校验 + NetworkSystem peer 发送, 与 ItemDisplay 同路径）:
//   spawnPlayer:  PlayerListPacket(Add, 携带皮肤) → AddPlayerPacket → 20 tick 后 Tab 移除
//   remove:       PlayerListPacket(Remove) + RemoveActorPacket
//   move:         MoveActorAbsolutePacket（UnreliableSequenced 免发放心跳）
#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/world/actor/player/Player.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/packet/AddPlayerPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/PlayerListPacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/utility/deps/BinaryStream.hpp>

#include "NpcSkinRegistry.h"

namespace debugshape_export::npc_protocol {

// sculk 协议包通用发送（与 ItemDisplayManager::sendSculkToPlayer 同配方:
// vanilla 反序列化校验 + varint 头部封装 + NetworkSystem peer 发送）
template <typename PacketT>
bool sendToPlayer(Player& player, PacketT const& packet, NetworkPeer::Reliability reliability) {
    std::vector<std::byte>        bodyBuffer;
    sculk::protocol::BinaryStream bodyStream(bodyBuffer);
    packet.write(bodyStream);

    std::string          checkBuffer(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());
    ReadOnlyBinaryStream checkStream(checkBuffer, true);
    auto                 checkPacket = MinecraftPackets::createPacket(static_cast<MinecraftPacketIds>(packet.getId()));
    if (!checkPacket || !checkPacket->read(checkStream)) {
        return false; // 校验失败: 不发, 调用方保留 shown 状态下轮重试
    }

    BinaryStream sendStream;
    sendStream.writeUnsignedVarInt(
        (static_cast<int>(packet.getId()) & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12),
        nullptr,
        nullptr
    );
    sendStream.mBuffer.append(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());

    auto networkSystem = ll::service::getNetworkSystem();
    if (!networkSystem) return false;
    auto* peer = networkSystem->getPeerForUser(player.getNetworkIdentifier());
    if (peer == nullptr) return false;
    peer->sendPacket(sendStream.mBuffer, reliability, Compressibility::Compressible);
    return true;
}

inline std::uint8_t rotationByte(float degrees) {
    auto wrapped = std::fmod(degrees, 360.0f);
    if (wrapped < 0.0f) wrapped += 360.0f;
    return static_cast<std::uint8_t>(std::lround(wrapped * (256.0f / 360.0f)));
}

// NPC 协议 UUID: 固定高位段（与真实玩家 UUID 空间天然隔离, 与库内 id 一一对应）
inline sculk::protocol::UUID npcUuid(std::int64_t id) {
    return {0xF0B3'4E50'4300'0000ULL, static_cast<std::uint64_t>(id)};
}

inline sculk::protocol::PlayerListEntry playerListEntry(
    sculk::protocol::UUID const&           uuid,
    std::int64_t                           uniqueId,
    std::string const&                     name,
    sculk::protocol::SerializedSkin const&  skin
) {
    sculk::protocol::PlayerListEntry entry;
    entry.mUUID            = uuid;
    entry.mActorUniqueId   = uniqueId;
    entry.mPlayerName     = name;
    entry.mSerializedSkin = skin;
    entry.mBuildPlatform  = 1;
    entry.mSkinTrusted    = true;
    return entry;
}

// Tab 列表移除条目（不需要皮肤/名字; AddPlayer 后 20 tick 调用）
inline bool removePlayerList(Player& player, std::int64_t id) {
    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Remove;
    packet.mPlayerEntryList = {playerListEntry(npcUuid(id), 0, "", {})};
    return sendToPlayer(player, packet, NetworkPeer::Reliability::Reliable);
}

// 假玩家生成（完整序列的第一步: 先注册皮肤到客户端 PlayerList）
// 返回 false = 发送失败, 调用方不得标记 shown
inline bool spawnPlayerList(
    Player&                               player,
    std::int64_t                          id,
    std::uint64_t                         uniqueId,
    std::string const&                    name,
    sculk::protocol::SerializedSkin const& skin
) {
    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
    packet.mPlayerEntryList = {playerListEntry(npcUuid(id), static_cast<std::int64_t>(uniqueId), name, skin)};
    return sendToPlayer(player, packet, NetworkPeer::Reliability::Reliable);
}

// 假玩家生成第二步: AddPlayer 实体化（时序上必须在 PlayerList Add 之后）
inline bool spawnPlayerBody(
    Player&            player,
    std::int64_t       id,
    std::uint64_t      runtimeId,
    std::uint64_t      uniqueId,
    Vec3 const&        position,
    float              yaw,
    std::string const& name
) {
    sculk::protocol::AddPlayerPacket packet;
    packet.mUuid               = npcUuid(id);
    packet.mName               = name;
    packet.mActorRuntimeId     = runtimeId;
    packet.mPos                = {position.x, position.y, position.z};
    packet.mVelocity           = {0.0f, 0.0f, 0.0f};
    packet.mRot                = {0.0f, yaw};
    packet.mYHeadRot           = yaw;
    packet.mGameType           = sculk::protocol::GameType::Survival;
    packet.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0, std::int64_t{0}},
        {sculk::protocol::ActorDataIDs::Name, name},
        {sculk::protocol::ActorDataIDs::NametagAlwaysShow, std::int32_t{1}},
    };
    packet.mAbilities.mPlayerRawId = static_cast<std::int64_t>(uniqueId);
    packet.mAbilities.mPlayerPermission = 1; // Member
    packet.mAbilities.mLayers      = {{0, 0, 0, 0.05f, 0.1f, 0.1f}};
    packet.mBuildPlatform          = 1;
    return sendToPlayer(player, packet, NetworkPeer::Reliability::Reliable);
}

// 假玩家移动（respawn 之外的轻量位置更新; UnreliableSequenced 免心跳）
inline bool move(Player& player, std::uint64_t runtimeId, Vec3 const& position, float yaw) {
    sculk::protocol::MoveActorAbsolutePacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mHeader        = 1; // On ground（纯视觉实体无物理）
    packet.mPosition      = {position.x, position.y, position.z};
    packet.mRotationX     = 0;
    packet.mRotationY     = rotationByte(yaw);
    packet.mRotationYHead = rotationByte(yaw);
    return sendToPlayer(player, packet, NetworkPeer::Reliability::UnreliableSequenced);
}

// 假玩家移除（PlayerList Remove + RemoveActor 双包; 与 despawn 语义一致）
inline bool remove(Player& player, std::int64_t id, std::uint64_t uniqueId) {
    removePlayerList(player, id);
    sculk::protocol::RemoveActorPacket packet;
    packet.mActorUniqueId = static_cast<std::int64_t>(uniqueId);
    return sendToPlayer(player, packet, NetworkPeer::Reliability::Reliable);
}

} // namespace debugshape_export::npc_protocol
