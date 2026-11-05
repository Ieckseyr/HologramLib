#pragma once

#include "SculkPacketSend.h"

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

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/actor/attribute/Attribute.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateAttributesPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateBlockPacket.hpp>

namespace debugshape_export::view {

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
    sculk::protocol::RemoveActorPacket packet;
    packet.mActorUniqueId = uniqueId;
    sendSculkPacketToPlayer(player, packet);
}

inline std::uint64_t& viewTickRef() {
    static std::uint64_t tick = 0;
    return tick;
}

[[nodiscard]] inline std::uint64_t currentViewTick() {
    return viewTickRef();
}

template <typename MetaDataT>
inline void pushMetaItems(MetaDataT& meta, logic::MetaPlan const& plan) {
    using ids = sculk::protocol::ActorDataIDs;
    if (plan.setName) {
        meta.mDataItems.push_back(sculk::protocol::MetaData::DataItem{ids::Name, plan.name});
    }
    if (plan.setAlwaysShow) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::NametagAlwaysShow, nametagAlwaysShowWire(plan.alwaysShow)});
    }
    if (plan.setPose) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::PoseIndex, static_cast<std::int64_t>(logic::clampPose(plan.pose))});
    }
    if (plan.setScale) {

        if (logic::scaleAcceptable(plan.scale)) {
            meta.mDataItems.push_back(sculk::protocol::MetaData::DataItem{ids::Reserved38, plan.scale});
            meta.mDataItems.push_back(
                sculk::protocol::MetaData::DataItem{ids::Reserved53, logic::hitboxWidthOf(plan.scale)});
            meta.mDataItems.push_back(
                sculk::protocol::MetaData::DataItem{ids::Reserved54, logic::hitboxHeightOf(plan.scale)});
        }
    }
    if (plan.setVariant) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::Variant, static_cast<std::int32_t>(plan.variant)});
    }
    if (plan.setMarkVariant) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::MarkVariant, static_cast<std::int32_t>(plan.markVariant)});
    }

    if (plan.setColorIndex && logic::colorAcceptable(plan.colorIndex)) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::ColorIndex, static_cast<std::uint8_t>(plan.colorIndex)});
    }
    if (plan.setColor2 && logic::colorAcceptable(plan.color2)) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::Color2Index, static_cast<std::uint8_t>(plan.color2)});
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

    sculk::protocol::MetaData meta;
    pushMetaItems(meta, plan);
    std::vector<std::byte>        buffer;
    sculk::protocol::BinaryStream stream{buffer};
    meta.write(stream);

    std::string hex;
    for (std::size_t i = 0; i < stream.size(); ++i) {
        hex += std::format("{:02x}", static_cast<unsigned>(stream.data()[i]));
    }
    return std::format("items={} bytes={} hex={}", meta.mDataItems.size(), stream.size(), hex);
}

inline void sendViewMeta(::Player& player, std::uint64_t runtimeId, logic::MetaPlan const& plan) {
    if (plan.empty())
        return;
    sculk::protocol::SetActorDataPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mTick           = currentViewTick();
    pushMetaItems(packet.mMetaData, plan);
    if (packet.mMetaData.mDataItems.empty())
        return;
    sendSculkPacketToPlayer(player, packet);
}

inline sculk::protocol::Attribute makeAttribute(
    std::string   name,
    float         minV,
    float         maxV,
    float         curV,
    float         defV,
    std::uint64_t tick) {
    sculk::protocol::Attribute a;
    a.mName            = std::move(name);
    a.mMinValue        = minV;
    a.mMaxValue        = maxV;
    a.mCurrentValue    = curV;
    a.mDefaultMinValue = minV;
    a.mDefaultMaxValue = maxV;
    a.mDefaultValue    = defV;
    a.mTick            = tick;
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

    sculk::protocol::AddActorPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mActorUniqueId  = actor.getOrCreateUniqueID().rawID;
    packet.mIdentifier     = identifier;

    auto const& pos     = actor.getPosition();
    packet.mPosition.mX = pos.x;
    packet.mPosition.mY = pos.y;
    packet.mPosition.mZ = pos.z;

    auto const velocity = actor.getVelocity();
    packet.mVelocity.mX = velocity.x;
    packet.mVelocity.mY = velocity.y;
    packet.mVelocity.mZ = velocity.z;

    auto const& rot       = actor.getRotation();
    packet.mRotation.mX   = rot.x;
    packet.mRotation.mY   = rot.y;
    packet.mYHeadRotation = actor.getYHeadRot();
    packet.mYBodyRotation = actor.getYHeadRot();
    packet.mActorLinks    = {};

    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, visualFlagsOf(actor)});
    pushMetaItems(packet.mMetaData, plan);

    float const health = static_cast<float>(actor.getMaxHealth());
    packet.mAttributes = {
        {"minecraft:health", 0.0f, health, health},
        {"minecraft:scale", 0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(player, packet);

    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = runtimeId;
    attributes.mTick           = currentViewTick();
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, health, health, health, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(player, attributes);
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

    sculk::protocol::AddActorPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mActorUniqueId  = uniqueId;
    packet.mIdentifier     = identifier;
    packet.mPosition.mX    = x;
    packet.mPosition.mY    = y;
    packet.mPosition.mZ    = z;
    packet.mVelocity.mX    = vx;
    packet.mVelocity.mY    = vy;
    packet.mVelocity.mZ    = vz;
    packet.mRotation.mX    = pitch;
    packet.mRotation.mY    = yaw;
    packet.mYHeadRotation  = yaw;
    packet.mYBodyRotation  = yaw;
    packet.mActorLinks     = {};

    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, std::int64_t(0)});
    pushMetaItems(packet.mMetaData, plan);

    packet.mAttributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale", 0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(player, packet);

    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = runtimeId;
    attributes.mTick           = currentViewTick();
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(player, attributes);

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
    sculk::protocol::UpdateBlockPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mRuntimeId     = runtimeId;
    packet.mFlag          = 3;
    packet.mLayer         = 0;
    sendSculkPacketToPlayer(player, packet);
}

inline bool sendPlayerSkin(::Player& viewer, ::Player const& target, std::string const& sourcePlayer) {
    auto&             npcs   = hologramlib::IHologramLib::getInstance().playerNpcs();
    std::string const skinId = "view:" + sourcePlayer;
    if (!npcs.captureSkin(skinId, sourcePlayer))
        return false;

    sculk::protocol::SerializedSkin skin;
    if (!NpcSkinRegistry::getInstance().getSkin(skinId, skin))
        return false;

    auto const&                       uuid = target.getUuid();
    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
    packet.mPlayerEntryList = {npc_protocol::playerListEntry(
        sculk::protocol::UUID{uuid.a, uuid.b},
        static_cast<std::int64_t>(target.getOrCreateUniqueID().rawID),
        target.getRealName(),
        skin)};
    return npc_protocol::sendToPlayer(viewer, packet, wire::Reliability::Reliable);
}

inline bool captureOwnSkin(::Player& target, sculk::protocol::SerializedSkin& out) {
    std::string const skinId = "view:" + target.getRealName();
    if (!hologramlib::IHologramLib::getInstance().playerNpcs().captureSkin(skinId, target.getRealName())) {
        return false;
    }
    return NpcSkinRegistry::getInstance().getSkin(skinId, out);
}

inline sculk::protocol::GameType gameTypeOfPlayMode(int playMode) {
    switch (playMode) {
    case 1:
        return sculk::protocol::GameType::Creative;
    case 2:
        return sculk::protocol::GameType::Adventure;
    case 5:
        return sculk::protocol::GameType::Default;
    case 6:
        return sculk::protocol::GameType::Spectator;
    default:
        return sculk::protocol::GameType::Survival;
    }
}

inline bool sendRealPlayerBody(
    ::Player&                              viewer,
    ::Player&                              target,
    sculk::protocol::SerializedSkin const& skin,
    int                                    playMode) {
    auto const                  uuid      = target.getUuid();
    auto const                  runtimeId = static_cast<std::uint64_t>(target.getRuntimeID());
    auto const                  uniqueId  = static_cast<std::int64_t>(target.getOrCreateUniqueID().rawID);
    auto const                  pos       = target.getPosition();
    auto const                  rot       = target.getRotation();
    auto const                  velocity  = target.getVelocity();
    std::string const           name      = target.getRealName();
    sculk::protocol::UUID const uuidWire{uuid.a, uuid.b};

    {
        sculk::protocol::PlayerListPacket list;
        list.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
        list.mPlayerEntryList = {npc_protocol::playerListEntry(uuidWire, uniqueId, name, skin)};
        sendSculkPacketToPlayer(viewer, list);
    }

    sculk::protocol::AddPlayerPacket body;
    body.mUuid                        = uuidWire;
    body.mName                        = name;
    body.mActorRuntimeId              = runtimeId;
    body.mPos                         = {pos.x, pos.y, pos.z};
    body.mVelocity                    = {velocity.x, velocity.y, velocity.z};
    body.mRot                         = {rot.x, rot.y};
    body.mYHeadRot                    = target.getYHeadRot();
    body.mGameType                    = gameTypeOfPlayMode(playMode);
    body.mAbilities.mPlayerRawId      = uniqueId;
    body.mAbilities.mPlayerPermission = 1;
    body.mAbilities.mLayers           = {{0, 0, 0, 0.05f, 0.1f, 0.1f}};
    body.mBuildPlatform               = 1;
    body.mMetaData.mDataItems         = {
        {sculk::protocol::ActorDataIDs::Reserved0, visualFlagsOf(target)},
        {sculk::protocol::ActorDataIDs::Name, name},
        {sculk::protocol::ActorDataIDs::NametagAlwaysShow, nametagAlwaysShowWire(true)},
        {sculk::protocol::ActorDataIDs::Reserved38, 1.0f},
        {sculk::protocol::ActorDataIDs::Reserved53, 0.6f},
        {sculk::protocol::ActorDataIDs::Reserved54, 1.8f},
    };

    std::uint64_t const before = sculkSendFailureCount().load(std::memory_order_relaxed);
    sendSculkPacketToPlayer(viewer, body);
    return sculkSendFailureCount().load(std::memory_order_relaxed) == before;
}

inline void sendMoveActorAbsolute(::Player& viewer, std::uint64_t runtimeId, float x, float y, float z, float pitch, float yaw, float headYaw) {
    sculk::protocol::MoveActorAbsolutePacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mFlags          = sculk::protocol::MoveActorAbsolutePacket::Flags::Teleport;
    packet.mPosition       = sculk::protocol::Vec3{x, y, z};
    packet.mRotationX      = npc_protocol::rotationByte(pitch);
    packet.mRotationY      = npc_protocol::rotationByte(yaw);
    packet.mRotationYHead  = npc_protocol::rotationByte(headYaw);
    sendSculkPacketToPlayer(viewer, packet);
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
    sculk::protocol::SerializedSkin skin;
    if (!NpcSkinRegistry::getInstance().getSkin(skinId, skin))
        return false;

    {
        std::string const perEntity = "mskin_" + std::to_string(static_cast<unsigned long long>(uniqueId));
        skin.mId                    = "HoloLibNpcSkin_" + perEntity;
        skin.mFullId                = skin.mId;
    }

    if (skin.mSkinImageBytes.empty() || skin.mGeometryData.empty()) {
        HLIB_LOG_WARN(
            "[视图] 皮肤数据不完整: 皮肤={} 贴图={} 字节 几何={} 字节 资源补丁={} 字节 id='{}' —— 客户端会显示默认模型",
            skinId,
            skin.mSkinImageBytes.size(),
            skin.mGeometryData.size(),
            skin.mResourcePatch.size(),
            skin.mId);
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
        skin.mSkinImageBytes.size(),
        skin.mGeometryData.size(),
        skin.mId);
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
    sculk::protocol::AddActorPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mActorUniqueId  = uniqueId;
    packet.mIdentifier     = identifier;
    packet.mPosition       = sculk::protocol::Vec3{eyeX, eyeY - logic::kPlayerEyeHeight, eyeZ};
    packet.mVelocity       = sculk::protocol::Vec3{0.0f, 0.0f, 0.0f};
    packet.mRotation       = sculk::protocol::Vec2{pitch, yaw};
    packet.mYHeadRotation  = headYaw;
    packet.mYBodyRotation  = yaw;
    packet.mActorLinks     = {};

    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, std::int64_t(0)});
    pushMetaItems(packet.mMetaData, plan);
    packet.mAttributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale", 0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(viewer, packet);

    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = runtimeId;
    attributes.mTick           = currentViewTick();
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(viewer, attributes);
}

template <typename PayloadT>
inline void sendActorSpawnRewritten(
    ::Player&              viewer,
    PayloadT const&        payload,
    std::string const&     identifier,
    logic::MetaPlan const& plan) {
    sculk::protocol::AddActorPacket packet;
    packet.mActorRuntimeId = static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID);
    packet.mActorUniqueId  = unwrap(payload.mEntityId).rawID;
    packet.mIdentifier     = identifier;
    packet.mPosition       = sculk::protocol::Vec3{unwrap(payload.mPos).x, unwrap(payload.mPos).y, unwrap(payload.mPos).z};
    packet.mVelocity       = sculk::protocol::Vec3{unwrap(payload.mVelocity).x, unwrap(payload.mVelocity).y, unwrap(payload.mVelocity).z};
    packet.mRotation       = sculk::protocol::Vec2{unwrap(payload.mRot).x, unwrap(payload.mRot).y};
    packet.mYHeadRotation  = unwrap(payload.mYHeadRotation);
    packet.mYBodyRotation  = unwrap(payload.mYBodyRotation);

    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, std::int64_t(0)});
    pushMetaItems(packet.mMetaData, plan);

    packet.mAttributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale", 0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(viewer, packet);

    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = packet.mActorRuntimeId;
    attributes.mTick           = currentViewTick();
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(viewer, attributes);
}

} // namespace debugshape_export::view
