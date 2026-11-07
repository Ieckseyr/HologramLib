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

#include "core/proto/Version.h"
#include "kernel/PacketSink.h"

#include "MetaWire.h"
#include "NpcPlayerList.h"
#include "NpcSkinRegistry.h"

namespace debugshape_export::npc_protocol {

namespace proto = ::debugshape_export::core::proto;

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
    proto::PlayerListPacket packet;
    packet.entries.push_back(playerListRemoveEntry(npcUuid(id)));
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

inline bool spawnPlayerList(
    Player&                         player,
    std::int64_t                    id,
    std::uint64_t                   uniqueId,
    std::string const&              name,
    proto::SerializedSkinRef const& skin) {
    proto::PlayerListPacket packet;
    packet.entries.push_back(
        playerListEntry(npcUuid(id), static_cast<std::int64_t>(uniqueId), name, skin));
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

// 内容与收件人无关的包：先编码一次，多个收件人共用一个 body（P03）
inline std::string buildPlayerListBody(
    std::int64_t                    id,
    std::uint64_t                   uniqueId,
    std::string const&              name,
    proto::SerializedSkinRef const& skin) {
    proto::PlayerListPacket packet;
    packet.entries.push_back(
        playerListEntry(npcUuid(id), static_cast<std::int64_t>(uniqueId), name, skin));
    return codec::body(packet);
}

inline std::string buildPlayerSkinBody(std::int64_t id, proto::SerializedSkinRef const& skin) {
    proto::PlayerSkinPacket packet;
    packet.uuid                     = npcUuid(id);
    packet.skin                     = skin;
    packet.skin.trusted_skin_flag   = static_cast<decltype(packet.skin.trusted_skin_flag)>(2);
    packet.localized_new_skin_name  = skin.id.empty() ? skin.full_id : skin.id;
    packet.localized_old_skin_name  = packet.localized_new_skin_name;
    return codec::body(packet);
}

inline bool sendBody(Player& player, std::uint32_t packetId, std::string const& body) {
    return wire::sendBody(player, packetId, body, wire::Reliability::Reliable, "PlayerNpc");
}

inline proto::PlayerListAddEntry playerListEntryForPlayer(
    Player const&                   player,
    proto::SerializedSkinRef const& skin) {
    auto const  uuid = player.getUuid();
    std::string xuid = player.getXuid();
    if (xuid.empty())
        xuid = "0";

    proto::PlayerListAddEntry entry;
    entry.uuid               = proto::UUID{uuid.a, uuid.b};
    entry.id                 = proto::ActorUniqueID{static_cast<std::int64_t>(player.getOrCreateUniqueID().rawID)};
    entry.action             = proto::PlayerListPacketType::Add;
    entry.name               = player.getRealName();
    entry.xuid               = std::move(xuid);
    entry.play_fab_id        = {};
    entry.platform_online_id = {};
    entry.build_platform     = static_cast<decltype(entry.build_platform)>(1);
    entry.skin               = skin;
    entry.skin.trusted_skin_flag =
        static_cast<decltype(entry.skin.trusted_skin_flag)>(kSkinTrustedTrue);
    entry.is_teacher    = false;
    entry.is_host       = false;
    entry.is_sub_client = false;
    entry.color         = proto::Color{0};
    return entry;
}

inline bool sendPlayerSkin(Player& player, std::int64_t id, proto::SerializedSkinRef const& skin) {
    proto::PlayerSkinPacket packet;
    packet.uuid                    = npcUuid(id);
    packet.skin                    = skin;
    packet.skin.trusted_skin_flag  = static_cast<decltype(packet.skin.trusted_skin_flag)>(2);
    packet.localized_new_skin_name = skin.id.empty() ? skin.full_id : skin.id;
    packet.localized_old_skin_name = packet.localized_new_skin_name;
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
    using ids = proto::ActorDataIDs;

    float const                     s = std::clamp(scale, 0.0625f, 10.0f);
    proto::AddPlayerPacket          packet;
    packet.uuid             = npcUuid(id);
    packet.name             = name;
    packet.runtime_id       = proto::bp::ActorRuntimeID{runtimeId};
    packet.pos              = {position.x, position.y, position.z};
    packet.velocity         = {0.0f, 0.0f, 0.0f};
    packet.rot              = {0.0f, yaw};
    packet.y_head_rot       = yaw;
    packet.player_game_type = proto::GameType::Survival;
    packet.unpack.data      = {
        meta_wire::int64(ids::Reserved0, std::int64_t{0}),
        meta_wire::string(ids::Name, name),
        meta_wire::int32(ids::NametagAlwaysShow, std::int32_t{1}),
        meta_wire::float32(ids::Reserved038, s),
        meta_wire::float32(ids::Reserved053, 0.6f * s),
        meta_wire::float32(ids::Reserved054, 1.8f * s),
    };
    packet.abilities_data.target_player = proto::ActorUniqueID{static_cast<std::int64_t>(uniqueId)};
    packet.abilities_data.player_permissions =
        static_cast<decltype(packet.abilities_data.player_permissions)>(1);
    packet.abilities_data.layers = {{
        proto::bp::v776::SerializedAbilitiesData::SerializedAbilitiesLayer::CustomCache,
        0,
        0,
        0.05f,
        0.1f,
        0.1f,
    }};
    packet.build_platform = static_cast<decltype(packet.build_platform)>(1);
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

inline bool move(Player& player, std::uint64_t runtimeId, Vec3 const& position, float yaw) {
    proto::MoveActorAbsolutePacket packet;
    packet.move_data.runtime_id = proto::bp::ActorRuntimeID{runtimeId};
    packet.move_data.header     = proto::move_flags::kOnGround;
    packet.move_data.pos        = {position.x, position.y, position.z};
    packet.move_data.rot_x      = 0;
    packet.move_data.rot_y      = rotationByte(yaw);
    packet.move_data.rot_y_head = rotationByte(yaw);
    return sendToPlayer(player, packet, wire::Reliability::UnreliableSequenced);
}

inline bool remove(Player& player, std::int64_t, std::uint64_t uniqueId) {
    proto::RemoveActorPacket packet;
    packet.entity_id = proto::ActorUniqueID{static_cast<std::int64_t>(uniqueId)};
    return sendToPlayer(player, packet, wire::Reliability::Reliable);
}

} // namespace debugshape_export::npc_protocol
