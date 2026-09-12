// NpcProtocol.h - 假玩家 NPC 协议层（1.16.0）
//
// 移植自 SCustomNpc VisualPacket.h, 适配 HologramLib 发送通道
// （MinecraftPackets 校验 + NetworkSystem peer 发送, 与 ItemDisplay 同路径）:
//   spawnPlayer:  PlayerListPacket(Add, 携带皮肤) → AddPlayerPacket → 20 tick 后 Tab 移除
//   remove:       PlayerListPacket(Remove) + RemoveActorPacket
//   move:         MoveActorAbsolutePacket（UnreliableSequenced 免发放心跳）
#pragma once

#include <cmath>
#include <format>
#include <cstdint>
#include <atomic>
#include <string>
#include <type_traits>
#include <vector>

#include <ll/api/io/LoggerRegistry.h>
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
#include <sculk/protocol/codec/packet/PlayerSkinPacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include "NpcSkinRegistry.h"

namespace debugshape_export::npc_protocol {

// sculk 协议包通用发送（与 ItemDisplayManager::sendSculkToPlayer 同配方:
// vanilla 反序列化校验 + varint 头部封装 + NetworkSystem peer 发送）
//
// 26.40 的 BDS 校验对 PlayerListPacket / AddPlayerPacket 会误判（拒绝完全合法的数据），
// 这两型跳过校验直接发送；其余包（AddActor/RemoveActor 等）保留校验作为防线。
template <typename PacketT>
bool sendToPlayer(Player& player, PacketT const& packet, NetworkPeer::Reliability reliability) {
    std::vector<std::byte>        bodyBuffer;
    sculk::protocol::BinaryStream bodyStream(bodyBuffer);
    packet.write(bodyStream);

    constexpr bool kSkipBdsReadCheck =
        std::is_same_v<PacketT, sculk::protocol::PlayerListPacket>
        || std::is_same_v<PacketT, sculk::protocol::AddPlayerPacket>;

    if constexpr (!kSkipBdsReadCheck) {
        std::string          checkBuffer(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());
        ReadOnlyBinaryStream checkStream(checkBuffer, true);
        auto checkPacket = MinecraftPackets::createPacket(static_cast<MinecraftPacketIds>(packet.getId()));
        if (!checkPacket || !checkPacket->read(checkStream)) {
            // 校验失败: 不发, 调用方保留 shown 状态下轮重试。
            // 首次失败打 warn（此后静默, 防每秒重试刷屏）——否则 NPC 链路断点完全不可见。
            static std::atomic<bool> warned{false};
            if (!warned.exchange(true)) {
                ll::io::LoggerRegistry::getInstance()
                    .getOrCreate("HologramLib")
                    ->warn(
                        "[PlayerNpc] {} BDS read 校验失败, 丢弃 (read {}/{} bytes) —— 此类失败不再重复记录",
                        std::string(packet.getName()),
                        checkStream.mReadPointer,
                        checkStream.mView.size()
                    );
            }
            return false;
        }
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
    // xuid 不能留空，客户端会报错断线，固定写 "0"。
    entry.mXuid            = "0";
    entry.mPlatformChatId = "";
    entry.mSerializedSkin = skin;
    entry.mBuildPlatform  = 1;
    entry.mSkinTrusted    = true;
    entry.mColor          = 0;
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
// scale: 模型缩放（1.19.0; Reserved38=SCALE 元数据, 53/54 碰撞箱随玩家默认 0.6x1.8 等比）
inline bool spawnPlayerBody(
    Player&            player,
    std::int64_t       id,
    std::uint64_t      runtimeId,
    std::uint64_t      uniqueId,
    Vec3 const&        position,
    float              yaw,
    std::string const& name,
    float              scale = 1.0f
) {
    float const s = std::clamp(scale, 0.0625f, 10.0f); // 客户端硬限, 与实体 scale 同域
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
        {sculk::protocol::ActorDataIDs::Reserved38, s},
        {sculk::protocol::ActorDataIDs::Reserved53, 0.6f * s},
        {sculk::protocol::ActorDataIDs::Reserved54, 1.8f * s},
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
    packet.mFlags         = sculk::protocol::MoveActorAbsolutePacket::Flags::OnGround; // 纯视觉实体无物理
    packet.mPosition      = {position.x, position.y, position.z};
    packet.mRotationX     = 0;
    packet.mRotationY     = rotationByte(yaw);
    packet.mRotationYHead = rotationByte(yaw);
    return sendToPlayer(player, packet, NetworkPeer::Reliability::UnreliableSequenced);
}

// 假玩家移除：只发 RemoveActor。
// 不发 PlayerList Remove —— 26.40 客户端在皮肤条目仍活跃时移除玩家列表条目会崩
// （实体照常消失, 只是玩家列表里会留下这个名字）。
inline bool remove(Player& player, std::int64_t /*id*/, std::uint64_t uniqueId) {
    sculk::protocol::RemoveActorPacket packet;
    packet.mActorUniqueId = static_cast<std::int64_t>(uniqueId);
    return sendToPlayer(player, packet, NetworkPeer::Reliability::Reliable);
}

} // namespace debugshape_export::npc_protocol
