#pragma once

#include <cmath>

#include "DiagLog.h"
#include <atomic>
#include <cstdint>
#include <format>
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

#include "NpcPlayerList.h"
#include "NpcSkinRegistry.h"
#include "kernel/PacketSink.h"

namespace debugshape_export::npc_protocol {

template <typename PacketT>
bool sendToPlayer(Player& player, PacketT const& packet, wire::Reliability reliability = wire::Reliability::Reliable) {
    return wire::send(player, packet, reliability);
}

inline std::uint8_t rotationByte(float degrees) {
    auto wrapped = std::fmod(degrees, 360.0f);
    if (wrapped < 0.0f)
        wrapped += 360.0f;
    return static_cast<std::uint8_t>(std::lround(wrapped * (256.0f / 360.0f)));
}

inline bool removePlayerList(Player& player, std::int64_t id) {
    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Remove;
    packet.mPlayerEntryList = {playerListEntry(npcUuid(id), 0, "", {})};
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

inline bool spawnPlayerList(
    Player&                                player,
    std::int64_t                           id,
    std::uint64_t                          uniqueId,
    std::string const&                     name,
    sculk::protocol::SerializedSkin const& skin) {
    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
    packet.mPlayerEntryList = {playerListEntry(npcUuid(id), static_cast<std::int64_t>(uniqueId), name, skin)};
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

// 内容与收件人无关的包：先编码一次，多个收件人共用一个 body（P03）
inline std::string buildPlayerListBody(
    std::int64_t                           id,
    std::uint64_t                          uniqueId,
    std::string const&                     name,
    sculk::protocol::SerializedSkin const& skin) {
    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
    packet.mPlayerEntryList = {playerListEntry(npcUuid(id), static_cast<std::int64_t>(uniqueId), name, skin)};
    return wire26::body26(packet);
}

inline std::string buildPlayerSkinBody(std::int64_t id, sculk::protocol::SerializedSkin const& skin) {
    sculk::protocol::PlayerSkinPacket packet;
    packet.mUUID                            = npcUuid(id);
    packet.mSerializedSkin                  = skin;
    packet.mSerializedSkin.mTrustedSkinFlag = 2;
    packet.mNewSkinName                     = skin.mId.empty() ? skin.mFullId : skin.mId;
    packet.mOldSkinName                     = packet.mNewSkinName;
    return wire26::body26(packet);
}

inline bool sendBody(Player& player, std::uint32_t packetId, std::string const& body) {
    return wire::sendBody(player, packetId, body, wire::Reliability::Reliable, "PlayerNpc");
}

inline sculk::protocol::PlayerListEntry playerListEntryForPlayer(
    Player const&                          player,
    sculk::protocol::SerializedSkin const& skin) {
    auto const  uuid = player.getUuid();
    std::string xuid = player.getXuid();
    if (xuid.empty())
        xuid = "0";

    sculk::protocol::PlayerListEntry entry;
    entry.mUUID                            = sculk::protocol::UUID{uuid.a, uuid.b};
    entry.mActorUniqueId                   = static_cast<std::int64_t>(player.getOrCreateUniqueID().rawID);
    entry.mPlayerName                      = player.getRealName();
    entry.mXuid                            = std::move(xuid);
    entry.mPlatformChatId                  = "";
    entry.mSerializedSkin                  = skin;
    entry.mSkinTrusted                     = true;
    entry.mSerializedSkin.mTrustedSkinFlag = kSkinTrustedTrue;
    entry.mBuildPlatform                   = 1;
    entry.mColor                           = 0;
    return entry;
}

inline bool sendPlayerSkin(Player& player, std::int64_t id, sculk::protocol::SerializedSkin const& skin) {
    sculk::protocol::PlayerSkinPacket packet;
    packet.mUUID                            = npcUuid(id);
    packet.mSerializedSkin                  = skin;
    packet.mSerializedSkin.mTrustedSkinFlag = 2;
    packet.mNewSkinName                     = skin.mId.empty() ? skin.mFullId : skin.mId;
    packet.mOldSkinName                     = packet.mNewSkinName;
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

inline bool spawnPlayerBody(
    Player&            player,
    std::int64_t       id,
    std::uint64_t      runtimeId,
    std::uint64_t      uniqueId,
    Vec3 const&        position,
    float              yaw,
    std::string const& name,
    float              scale = 1.0f) {
    float const                      s = std::clamp(scale, 0.0625f, 10.0f);
    sculk::protocol::AddPlayerPacket packet;
    packet.mUuid                = npcUuid(id);
    packet.mName                = name;
    packet.mActorRuntimeId      = runtimeId;
    packet.mPos                 = {position.x, position.y, position.z};
    packet.mVelocity            = {0.0f, 0.0f, 0.0f};
    packet.mRot                 = {0.0f, yaw};
    packet.mYHeadRot            = yaw;
    packet.mGameType            = sculk::protocol::GameType::Survival;
    packet.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0, std::int64_t{0}},
        {sculk::protocol::ActorDataIDs::Name, name},
        {sculk::protocol::ActorDataIDs::NametagAlwaysShow, std::int32_t{1}},
        {sculk::protocol::ActorDataIDs::Reserved38, s},
        {sculk::protocol::ActorDataIDs::Reserved53, 0.6f * s},
        {sculk::protocol::ActorDataIDs::Reserved54, 1.8f * s},
    };
    packet.mAbilities.mPlayerRawId      = static_cast<std::int64_t>(uniqueId);
    packet.mAbilities.mPlayerPermission = 1;
    packet.mAbilities.mLayers           = {{0, 0, 0, 0.05f, 0.1f, 0.1f}};
    packet.mBuildPlatform               = 1;
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

inline bool move(Player& player, std::uint64_t runtimeId, Vec3 const& position, float yaw) {
    sculk::protocol::MoveActorAbsolutePacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mFlags          = sculk::protocol::MoveActorAbsolutePacket::Flags::OnGround;
    packet.mPosition       = {position.x, position.y, position.z};
    packet.mRotationX      = 0;
    packet.mRotationY      = rotationByte(yaw);
    packet.mRotationYHead  = rotationByte(yaw);
    return sendToPlayer(player, packet, wire::Reliability::UnreliableSequenced);
}

inline bool remove(Player& player, std::int64_t, std::uint64_t uniqueId) {
    sculk::protocol::RemoveActorPacket packet;
    packet.mActorUniqueId = static_cast<std::int64_t>(uniqueId);
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

} // namespace debugshape_export::npc_protocol
