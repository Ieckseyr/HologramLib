#pragma once

#include "ProtoPacketSend.h"

#include "DiagLog.h"

#include "ViewOverrideLogic.h"

#include "hologramlib/HologramLib.h"
#include "playernpc/NpcProtocol.h"
#include "playernpc/NpcSkinRegistry.h"

#include <atomic>
#include <cstdint>
#include <string>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/ActorDataIDs.h>
#include <mc/world/actor/ActorFlags.h>
#include <mc/world/actor/DataItem.h>
#include <mc/world/actor/player/Player.h>

#include "core/proto/Version.h"
#include "MetaWire.h"

namespace debugshape_export::view {

namespace proto = ::debugshape_export::core::proto;

template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return (value.get());
    } else {
        return (value);
    }
}
template <typename T>
constexpr decltype(auto) unwrap(T& value) {
    if constexpr (requires { value.get(); }) {
        return (value.get());
    } else {
        return (value);
    }
}

inline constexpr std::uint8_t nametagAlwaysShowWire(bool alwaysShow) {
    return static_cast<std::uint8_t>(alwaysShow ? 0 : 1);
}

inline void sendRemoveActor(::Player& player, std::int64_t uniqueId) {
    proto::RemoveActorPacket packet;
    packet.entity_id = proto::ActorUniqueID{uniqueId};
    sendProtoPacketToPlayer(player, packet);
}

inline std::uint64_t& viewTickRef() {
    static std::uint64_t tick = 0;
    return tick;
}

[[nodiscard]] inline std::uint64_t currentViewTick() {
    return viewTickRef();
}

inline void pushMetaItems(proto::ActorDataList& data, logic::MetaPlan const& plan) {
    using ids = proto::ActorDataIDs;
    if (plan.setName) {
        data.push_back(meta_wire::string(ids::Name, plan.name));
    }
    if (plan.setAlwaysShow) {
        data.push_back(meta_wire::byte(ids::NametagAlwaysShow, nametagAlwaysShowWire(plan.alwaysShow)));
    }
    if (plan.setPose) {
        data.push_back(meta_wire::int64(ids::PoseIndex, static_cast<std::int64_t>(logic::clampPose(plan.pose))));
    }
    if (plan.setScale) {

        if (logic::scaleAcceptable(plan.scale)) {
            data.push_back(meta_wire::float32(ids::Reserved038, plan.scale));
            data.push_back(meta_wire::float32(ids::Reserved053, logic::hitboxWidthOf(plan.scale)));
            data.push_back(meta_wire::float32(ids::Reserved054, logic::hitboxHeightOf(plan.scale)));
        }
    }
    if (plan.setVariant) {
        data.push_back(meta_wire::int32(ids::Variant, static_cast<std::int32_t>(plan.variant)));
    }
    if (plan.setMarkVariant) {
        data.push_back(meta_wire::int32(ids::MarkVariant, static_cast<std::int32_t>(plan.markVariant)));
    }

    if (plan.setColorIndex && logic::colorAcceptable(plan.colorIndex)) {
        data.push_back(meta_wire::byte(ids::ColorIndex, static_cast<std::uint8_t>(plan.colorIndex)));
    }
    if (plan.setColor2 && logic::colorAcceptable(plan.color2)) {
        data.push_back(meta_wire::byte(ids::Color2Index, static_cast<std::uint8_t>(plan.color2)));
    }
}

inline logic::MetaPlan planOf(hologramlib::EntityView const& view) {
    auto plan = logic::metaPlanOf(
        view.hasNametag,
        view.nametag,
        view.nametagAlwaysShow,
        view.hasPose,
        view.pose,
        view.hasMetaScale,
        view.metaScale,
        view.hasVariant,
        view.variant,
        view.hasMarkVariant,
        view.markVariant);
    return logic::withColors(plan, view.hasColorIndex, view.colorIndex, view.hasColor2, view.color2);
}

inline std::string describeMetaItems(hologramlib::EntityView const& view) {
    auto plan = planOf(view);
    if (plan.empty())
        return "items=0 plan=empty";

    proto::SetActorDataPacket probe;
    probe.id = proto::bp::ActorRuntimeID{0};
    probe.tick = proto::bp::PlayerInputTick{0};
    pushMetaItems(probe.packed_items.data, plan);

    std::string      buffer;
    proto::bp::BinaryWriter writer{buffer};
    proto::bp::serialize(writer, probe);

    std::string hex;
    for (unsigned char const c : buffer) {
        hex += std::format("{:02x}", static_cast<unsigned>(c));
    }
    return std::format("items={} bytes={} hex={}", probe.packed_items.data.size(), buffer.size(), hex);
}

inline void sendViewMeta(::Player& player, std::uint64_t runtimeId, logic::MetaPlan const& plan) {
    if (plan.empty())
        return;
    proto::SetActorDataPacket packet;
    packet.id   = proto::bp::ActorRuntimeID{runtimeId};
    packet.tick = proto::bp::PlayerInputTick{currentViewTick()};
    pushMetaItems(packet.packed_items.data, plan);
    if (packet.packed_items.data.empty())
        return;
    sendProtoPacketToPlayer(player, packet);
}

inline proto::AttributeData makeAttribute(
    std::string   name,
    float         minV,
    float         maxV,
    float         curV,
    float         defV,
    std::uint64_t tick) {
    (void)tick; // 2225 的 AttributeData 不再带 per-attribute tick
    proto::AttributeData a;
    a.name              = std::move(name);
    a.min_value         = minV;
    a.max_value         = maxV;
    a.current_value     = curV;
    a.default_min_value = minV;
    a.default_max_value = maxV;
    a.default_value     = defV;
    return a;
}

inline std::int64_t visualFlagsOf(::Actor& actor) {
    std::int64_t flags = 0;
    auto const   set   = [&](::ActorFlags flag, int bit) {
        if (actor.getStatusFlag(flag))
            flags |= (std::int64_t(1) << bit);
    };
    set(::ActorFlags::Onfire, 0);
    set(::ActorFlags::Sneaking, 1);
    set(::ActorFlags::Sprinting, 3);
    set(::ActorFlags::Invisible, 5);
    set(::ActorFlags::CanShowName, 14);
    set(::ActorFlags::AlwaysShowName, 15);
    return flags;
}

inline void sendActorSpawn(
    ::Player&              player,
    ::Actor&               actor,
    std::string const&     identifier,
    logic::MetaPlan const& plan) {
    auto const runtimeId = static_cast<std::uint64_t>(actor.getRuntimeID());

    proto::AddActorPacket packet;
    packet.runtime_id = proto::bp::ActorRuntimeID{runtimeId};
    packet.entity_id  = proto::ActorUniqueID{actor.getOrCreateUniqueID().rawID};
    packet.actor_type = identifier;

    auto const& pos = actor.getPosition();
    packet.pos.x    = pos.x;
    packet.pos.y    = pos.y;
    packet.pos.z    = pos.z;

    auto const velocity = actor.getVelocity();
    packet.velocity.x   = velocity.x;
    packet.velocity.y   = velocity.y;
    packet.velocity.z   = velocity.z;

    auto const& rot     = actor.getRotation();
    packet.rot.x        = rot.x;
    packet.rot.y        = rot.y;
    packet.y_head_rotation = actor.getYHeadRot();
    packet.y_body_rotation = actor.getYHeadRot();
    packet.links           = {};

    packet.data.data.push_back(meta_wire::int64(proto::ActorDataIDs::Reserved0, visualFlagsOf(actor)));
    pushMetaItems(packet.data.data, plan);

    float const health = static_cast<float>(actor.getMaxHealth());
    packet.attributes  = {
        {"minecraft:health", 0.0f, health, health},
        {"minecraft:scale", 0.0625f, 10.0f, 1.0f},
    };
    packet.synched_properties = {};
    sendProtoPacketToPlayer(player, packet);

    proto::UpdateAttributesPacket attributes;
    attributes.runtime_id     = proto::bp::ActorRuntimeID{runtimeId};
    attributes.tick           = proto::bp::PlayerInputTick{currentViewTick()};
    attributes.attribute_data = {
        makeAttribute("minecraft:health", 0.0f, health, health, health, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendProtoPacketToPlayer(player, attributes);
}

inline void sendItemActorSpawn(
    ::Player&              player,
    std::uint64_t          runtimeId,
    std::int64_t           uniqueId,
    std::string const&     identifier,
    float                  x,
    float                  y,
    float                  z,
    float                  vx,
    float                  vy,
    float                  vz,
    float                  yaw,
    float                  pitch,
    logic::MetaPlan const& plan) {

    sendRemoveActor(player, uniqueId);

    proto::AddActorPacket packet;
    packet.runtime_id      = proto::bp::ActorRuntimeID{runtimeId};
    packet.entity_id       = proto::ActorUniqueID{uniqueId};
    packet.actor_type      = identifier;
    packet.pos.x           = x;
    packet.pos.y           = y;
    packet.pos.z           = z;
    packet.velocity.x      = vx;
    packet.velocity.y      = vy;
    packet.velocity.z      = vz;
    packet.rot.x           = pitch;
    packet.rot.y           = yaw;
    packet.y_head_rotation = yaw;
    packet.y_body_rotation = yaw;
    packet.links           = {};

    packet.data.data.push_back(meta_wire::int64(proto::ActorDataIDs::Reserved0, std::int64_t(0)));
    pushMetaItems(packet.data.data, plan);

    packet.attributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale", 0.0625f, 10.0f, 1.0f},
    };
    packet.synched_properties = {};
    sendProtoPacketToPlayer(player, packet);

    proto::UpdateAttributesPacket attributes;
    attributes.runtime_id     = proto::bp::ActorRuntimeID{runtimeId};
    attributes.tick           = proto::bp::PlayerInputTick{currentViewTick()};
    attributes.attribute_data = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendProtoPacketToPlayer(player, attributes);

    sendViewMeta(player, runtimeId, plan);
}

template <typename PayloadT>
inline void sendItemSpawnRewritten(
    ::Player&              player,
    PayloadT const&        payload,
    std::string const&     identifier,
    logic::MetaPlan const& plan) {
    auto const pos = unwrap(payload.mPos);
    auto const vel = unwrap(payload.mVelocity);
    sendItemActorSpawn(
        player,
        static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID),
        unwrap(payload.mId).rawID,
        identifier,
        pos.x,
        pos.y,
        pos.z,
        vel.x,
        vel.y,
        vel.z,
        0.0f,
        0.0f,
        plan);
}

inline void sendItemActorAsOther(
    ::Player&              player,
    ::Actor&               actor,
    std::string const&     identifier,
    logic::MetaPlan const& plan) {
    auto const pos = actor.getPosition();
    auto const vel = actor.getVelocity();
    auto const rot = actor.getRotation();
    sendItemActorSpawn(
        player,
        static_cast<std::uint64_t>(actor.getRuntimeID()),
        actor.getOrCreateUniqueID().rawID,
        identifier,
        pos.x,
        pos.y,
        pos.z,
        vel.x,
        vel.y,
        vel.z,
        rot.y,
        rot.x,
        plan);
}

inline void sendBlockUpdate(::Player& player, int x, int y, int z, std::uint32_t runtimeId) {
    proto::UpdateBlockPacket packet;
    packet.pos          = proto::BlockPos{x, y, z};
    packet.runtime_id   = proto::BlockRuntimeId{runtimeId};
    packet.update_flags = 3;
    packet.layer        = 0;
    sendProtoPacketToPlayer(player, packet);
}

inline bool sendPlayerSkin(::Player& viewer, ::Player const& target, std::string const& sourcePlayer) {
    auto&             npcs   = hologramlib::IHologramLib::getInstance().playerNpcs();
    std::string const skinId = "view:" + sourcePlayer;
    if (!npcs.captureSkin(skinId, sourcePlayer))
        return false;

    proto::SerializedSkinRef skin;
    if (!NpcSkinRegistry::getInstance().getSkin(skinId, skin))
        return false;

    auto const&                uuid = target.getUuid();
    proto::PlayerListPacket    packet;
    packet.entries.push_back(npc_protocol::playerListEntry(
        proto::UUID{uuid.a, uuid.b},
        static_cast<std::int64_t>(target.getOrCreateUniqueID().rawID),
        target.getRealName(),
        skin));
    return npc_protocol::sendToPlayer(viewer, packet, wire::Reliability::Reliable);
}

inline bool captureOwnSkin(::Player& target, proto::SerializedSkinRef& out) {
    std::string const skinId = "view:" + target.getRealName();
    if (!hologramlib::IHologramLib::getInstance().playerNpcs().captureSkin(skinId, target.getRealName())) {
        return false;
    }
    return NpcSkinRegistry::getInstance().getSkin(skinId, out);
}

inline proto::GameType gameTypeOfPlayMode(int playMode) {
    switch (playMode) {
    case 1:
        return proto::GameType::Creative;
    case 2:
        return proto::GameType::Adventure;
    case 5:
        return proto::GameType::Default;
    case 6:
        return proto::GameType::Spectator;
    default:
        return proto::GameType::Survival;
    }
}

inline bool sendRealPlayerBody(
    ::Player&                              viewer,
    ::Player&                              target,
    proto::SerializedSkinRef const& skin,
    int                             playMode) {
    auto const                  uuid      = target.getUuid();
    auto const                  runtimeId = static_cast<std::uint64_t>(target.getRuntimeID());
    auto const                  uniqueId  = static_cast<std::int64_t>(target.getOrCreateUniqueID().rawID);
    auto const                  pos       = target.getPosition();
    auto const                  rot       = target.getRotation();
    auto const                  velocity  = target.getVelocity();
    std::string const           name      = target.getRealName();
    proto::UUID const uuidWire{uuid.a, uuid.b};

    {
        proto::PlayerListPacket list;
        list.entries.push_back(npc_protocol::playerListEntry(uuidWire, uniqueId, name, skin));
        sendProtoPacketToPlayer(viewer, list);
    }

    using ids = proto::ActorDataIDs;

    proto::AddPlayerPacket body;
    body.uuid                                  = uuidWire;
    body.name                                  = name;
    body.runtime_id                            = proto::bp::ActorRuntimeID{runtimeId};
    body.pos                                   = {pos.x, pos.y, pos.z};
    body.velocity                              = {velocity.x, velocity.y, velocity.z};
    body.rot                                   = {rot.x, rot.y};
    body.y_head_rot                            = target.getYHeadRot();
    body.player_game_type                      = gameTypeOfPlayMode(playMode);
    body.abilities_data.target_player          = proto::ActorUniqueID{uniqueId};
    body.abilities_data.player_permissions     = static_cast<decltype(body.abilities_data.player_permissions)>(1);
    body.abilities_data.layers                 = {{proto::bp::v776::SerializedAbilitiesData::SerializedAbilitiesLayer::CustomCache, 0, 0, 0.05f, 0.1f, 0.1f}};
    body.build_platform                        = static_cast<decltype(body.build_platform)>(1);
    body.unpack.data                           = {
        meta_wire::int64(ids::Reserved0, visualFlagsOf(target)),
        meta_wire::string(ids::Name, name),
        meta_wire::byte(ids::NametagAlwaysShow, nametagAlwaysShowWire(true)),
        meta_wire::float32(ids::Reserved038, 1.0f),
        meta_wire::float32(ids::Reserved053, 0.6f),
        meta_wire::float32(ids::Reserved054, 1.8f),
    };

    std::uint64_t const before = protoSendFailureCount().load(std::memory_order_relaxed);
    sendProtoPacketToPlayer(viewer, body);
    return protoSendFailureCount().load(std::memory_order_relaxed) == before;
}

inline void sendMoveActorAbsolute(::Player& viewer, std::uint64_t runtimeId, float x, float y, float z, float pitch, float yaw, float headYaw) {
    proto::MoveActorAbsolutePacket packet;
    packet.move_data.runtime_id = proto::bp::ActorRuntimeID{runtimeId};
    packet.move_data.header     = proto::move_flags::kTeleport;
    packet.move_data.pos        = proto::Vec3{x, y, z};
    packet.move_data.rot_x      = npc_protocol::rotationByte(pitch);
    packet.move_data.rot_y      = npc_protocol::rotationByte(yaw);
    packet.move_data.rot_y_head = npc_protocol::rotationByte(headYaw);
    sendProtoPacketToPlayer(viewer, packet);
}

inline std::atomic<std::uint64_t> mSkinnedSpawnCounter{0};

inline bool sendSkinnedSpawn(
    ::Player&          viewer,
    std::uint64_t      runtimeId,
    std::int64_t       uniqueId,
    float              x,
    float              y,
    float              z,
    float              yaw,
    std::string const& skinId,
    bool               hasNametag,
    std::string const& nametag,
    float              scale,
    float              yOffset) {
    proto::SerializedSkinRef skin;
    if (!NpcSkinRegistry::getInstance().getSkin(skinId, skin))
        return false;

    {
        std::string const perEntity = "mskin_" + std::to_string(static_cast<unsigned long long>(uniqueId));
        skin.id                     = "HoloLibNpcSkin_" + perEntity;
        skin.full_id                = skin.id;
    }

    if (skin.image_data.image_bytes.empty() || skin.geometry_data.empty()) {
        HLIB_LOG_WARN(
            "[视图] 皮肤数据不完整: 皮肤={} 贴图={} 字节 几何={} 字节 资源补丁={} 字节 id='{}' —— 客户端会显示默认模型",
            skinId,
            skin.image_data.image_bytes.size(),
            skin.geometry_data.size(),
            skin.resource_patch.size(),
            skin.id);
    }

    std::string const name = hasNametag ? nametag : std::string{" "};

    if (!npc_protocol::spawnPlayerList(viewer, uniqueId, static_cast<std::uint64_t>(uniqueId), name, skin)) {
        return false;
    }
    float const s      = scale > 0.0f ? std::clamp(scale, 0.0625f, 10.0f) : 1.0f;
    bool const  bodyOk = npc_protocol::spawnPlayerBody(
        viewer,
        uniqueId,
        runtimeId,
        static_cast<std::uint64_t>(uniqueId),
        Vec3{x, y + yOffset, z},
        yaw,
        name,
        s);

    HLIB_LOG_INFO(
        "[视图] 换皮包已发: 观看者={} 实体 uniqueId={} runtimeId={} 皮肤={} 缩放={:.2f} 身体包={} "
        "（贴图={} B 几何={} B 皮肤id='{}'）",
        viewer.getRealName(),
        uniqueId,
        runtimeId,
        skinId,
        s,
        bodyOk,
        skin.image_data.image_bytes.size(),
        skin.geometry_data.size(),
        skin.id);
    ++mSkinnedSpawnCounter;
    return bodyOk;
}

inline void sendActorSpawnFromInput(
    ::Player&              viewer,
    std::uint64_t          runtimeId,
    std::int64_t           uniqueId,
    std::string const&     identifier,
    float                  eyeX,
    float                  eyeY,
    float                  eyeZ,
    float                  pitch,
    float                  yaw,
    float                  headYaw,
    logic::MetaPlan const& plan) {
    proto::AddActorPacket packet;
    packet.runtime_id      = proto::bp::ActorRuntimeID{runtimeId};
    packet.entity_id       = proto::ActorUniqueID{uniqueId};
    packet.actor_type      = identifier;
    packet.pos             = proto::Vec3{eyeX, eyeY - logic::kPlayerEyeHeight, eyeZ};
    packet.velocity        = proto::Vec3{0.0f, 0.0f, 0.0f};
    packet.rot             = proto::Vec2{pitch, yaw};
    packet.y_head_rotation = headYaw;
    packet.y_body_rotation = yaw;
    packet.links           = {};

    packet.data.data.push_back(meta_wire::int64(proto::ActorDataIDs::Reserved0, std::int64_t(0)));
    pushMetaItems(packet.data.data, plan);
    packet.attributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale", 0.0625f, 10.0f, 1.0f},
    };
    packet.synched_properties = {};
    sendProtoPacketToPlayer(viewer, packet);

    proto::UpdateAttributesPacket attributes;
    attributes.runtime_id     = proto::bp::ActorRuntimeID{runtimeId};
    attributes.tick           = proto::bp::PlayerInputTick{currentViewTick()};
    attributes.attribute_data = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendProtoPacketToPlayer(viewer, attributes);
}

template <typename PayloadT>
inline void sendActorSpawnRewritten(
    ::Player&              viewer,
    PayloadT const&        payload,
    std::string const&     identifier,
    logic::MetaPlan const& plan) {
    proto::AddActorPacket packet;
    packet.runtime_id      = proto::bp::ActorRuntimeID{static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID)};
    packet.entity_id       = proto::ActorUniqueID{unwrap(payload.mEntityId).rawID};
    packet.actor_type      = identifier;
    packet.pos             = proto::Vec3{unwrap(payload.mPos).x, unwrap(payload.mPos).y, unwrap(payload.mPos).z};
    packet.velocity        = proto::Vec3{unwrap(payload.mVelocity).x, unwrap(payload.mVelocity).y, unwrap(payload.mVelocity).z};
    packet.rot             = proto::Vec2{unwrap(payload.mRot).x, unwrap(payload.mRot).y};
    packet.y_head_rotation = unwrap(payload.mYHeadRotation);
    packet.y_body_rotation = unwrap(payload.mYBodyRotation);

    packet.data.data.push_back(meta_wire::int64(proto::ActorDataIDs::Reserved0, std::int64_t(0)));
    pushMetaItems(packet.data.data, plan);

    packet.attributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale", 0.0625f, 10.0f, 1.0f},
    };
    packet.synched_properties = {};
    sendProtoPacketToPlayer(viewer, packet);

    proto::UpdateAttributesPacket attributes;
    attributes.runtime_id     = packet.runtime_id;
    attributes.tick           = proto::bp::PlayerInputTick{currentViewTick()};
    attributes.attribute_data = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendProtoPacketToPlayer(viewer, attributes);
}

} // namespace debugshape_export::view
