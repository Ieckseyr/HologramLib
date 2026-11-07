#pragma once

#include "CustomEntityManager.h"

#include "DiagLog.h"
#include "ProtoPacketSend.h"
#include "sulfur/SulfurDisplayPackets.h"

#include <ll/api/service/Bedrock.h>
#include <exception>

#include <mc/deps/nbt/CompoundTag.h>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/packet/MobArmorEquipmentPacket.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>

#include "core/proto/Version.h"
#include "MetaWire.h"

#include <algorithm>
#include <memory>
#include <string>

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

namespace ce_ops {

inline proto::AttributeData makeAttribute(
    std::string   name,
    float         minV,
    float         maxV,
    float         curV,
    float         defV = 0.0f,
    std::uint64_t tick = 0) {
    (void)tick; // 2225 的 AttributeData 不再带 per-attribute tick
    proto::AttributeData a;
    a.name              = std::move(name);
    a.min_value         = minV;
    a.max_value         = maxV;
    a.current_value     = curV;
    a.default_min_value = minV;
    a.default_max_value = maxV;
    a.default_value     = (defV == 0.0f) ? curV : defV;
    a.modifiers         = {};
    return a;
}

inline std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

constexpr char const* kStopAnimation = "animation.player.attack.positions";
constexpr char const* kStopExpr      = "1";

inline bool isLegacyStopExpression(std::string const& e) {
    return e == "query.any_animation" || e == "query.any_animation()";
}

inline Player* findPlayerByUuid(mce::UUID const& uuid) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    return level->getPlayer(uuid);
}

inline Player* findPlayerByName(std::string const& realName) {
    if (realName.empty())
        return nullptr;
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) {
        if (found)
            return true;
        if (p.getRealName() == realName) {
            found = &p;
            return false;
        }
        return true;
    });
    return found;
}

struct PitchYaw {
    float pitch{0};
    float yaw{0};
};
inline PitchYaw effectiveRot(
    CustomEntityManager::Runtime const& rt,
    mce::UUID const&                    uuid,
    CustomEntityConfig const&           data) {
    auto it = rt.playerRot.find(uuid);
    if (it == rt.playerRot.end())
        return PitchYaw{data.pitch, data.yaw};
    return PitchYaw{it->second.pitch, it->second.yaw};
}

inline std::string normalizeIdentifier(std::string const& raw) {
    if (raw.empty())
        return {};
    if (raw.find(':') != std::string::npos)
        return raw;
    return "minecraft:" + raw;
}

using Runtime = CustomEntityManager::Runtime;

inline ::ItemStack buildEquipmentItem(hologramlib::CustomEntityEquipment const& eq) {
    if (eq.name.empty())
        return {};
    auto level = ll::service::getLevel();
    if (!level)
        return {};
    try {
        std::unique_ptr<::CompoundTag> tag;
        if (!eq.nbt.empty()) {
            if (auto parsed = ::CompoundTag::fromSnbt(eq.nbt)) {
                tag = std::make_unique<::CompoundTag>(std::move(*parsed));
            }
        }
        auto tryGet = [&](std::string const& name) -> ::ItemStack {
            auto weak = level->getItemRegistry().getItem(::HashedString(name));
            if (!weak)
                return {};

            ::ItemStack stack{};
            stack.reinit(*weak, 1, eq.aux);
            if (tag)
                stack.setUserData(std::make_unique<::CompoundTag>(*tag));
            return stack;
        };
        ::ItemStack stack = tryGet(eq.name);
        if (stack.isNull()) {

            auto normalized = eq.name;
            if (normalized.find(':') == std::string::npos)
                normalized = "minecraft:" + normalized;
            stack = tryGet(std::move(normalized));
        }
        return stack;
    } catch (std::exception const&) {
        return {};
    }
}

inline std::string const& effectiveNametag(CustomEntityConfig const& data, CustomEntityManager::Runtime const& rt, mce::UUID uuid) {
    static std::string const kEmpty;
    auto                     it = rt.playerNameTag.find(uuid);
    return it != rt.playerNameTag.end() ? it->second : (data.nametag.empty() ? kEmpty : data.nametag);
}

inline float effectiveScaleValue(CustomEntityConfig const& data, CustomEntityManager::Runtime const& rt, mce::UUID uuid) {
    auto it = rt.playerScale.find(uuid);
    return it != rt.playerScale.end() ? it->second : data.scale;
}

inline hologramlib::CustomEntityEquipment const& effectiveEquipment(
    CustomEntityConfig const&           data,
    CustomEntityManager::Runtime const& rt,
    mce::UUID                           uuid,
    int                                 slot) {
    static hologramlib::CustomEntityEquipment const kEmpty;
    auto                                            it = rt.playerEquip.find(uuid);
    if (it != rt.playerEquip.end()) {
        auto const& entry = it->second[static_cast<std::size_t>(slot)];
        if (!entry.name.empty())
            return entry;
    }
    return data.equipment[static_cast<std::size_t>(slot)];
}

inline void sendEffectiveEquipment(::Player& player, CustomEntityConfig const& data, CustomEntityManager::Runtime const& rt) {
    auto const uuid = player.getUuid();

    for (int slot = 0; slot <= 1; ++slot) {
        auto stack = buildEquipmentItem(effectiveEquipment(data, rt, uuid, slot));
        if (stack.isNull())
            continue;
        ::ActorRuntimeID     runtimeId{rt.runtimeId};
        auto                 cid = (slot == 1) ? ::ContainerID::Offhand : ::ContainerID::Inventory;
        ::MobEquipmentPacket equipPkt{runtimeId, stack, static_cast<unsigned char>(slot), 0, cid};
        equipPkt.sendTo(player);
    }

    {
        auto head  = buildEquipmentItem(effectiveEquipment(data, rt, uuid, 2));
        auto torso = buildEquipmentItem(effectiveEquipment(data, rt, uuid, 3));
        auto legs  = buildEquipmentItem(effectiveEquipment(data, rt, uuid, 4));
        auto feet  = buildEquipmentItem(effectiveEquipment(data, rt, uuid, 5));
        if (!head.isNull() || !torso.isNull() || !legs.isNull() || !feet.isNull()) {
            ::MobArmorEquipmentPacket armorPkt;
            armorPkt.mRuntimeId = ::ActorRuntimeID{rt.runtimeId};
            std::destroy_at(&armorPkt.mHead);
            std::construct_at(&armorPkt.mHead, head);
            std::destroy_at(&armorPkt.mTorso);
            std::construct_at(&armorPkt.mTorso, torso);
            std::destroy_at(&armorPkt.mLegs);
            std::construct_at(&armorPkt.mLegs, legs);
            std::destroy_at(&armorPkt.mFeet);
            std::construct_at(&armorPkt.mFeet, feet);
            armorPkt.sendTo(player);
        }
    }
}

inline void sendMobProperties(::Player& player, CustomEntityConfig const& data, CustomEntityManager::Runtime const& rt) {
    for (auto const& prop : data.mobProperties) {
        if (prop.name.empty())
            continue;
        sendProtoPacketToPlayer(
            player,
            sulfur::makeMobProperty(static_cast<std::int64_t>(rt.uniqueId), prop.name, prop.value));
    }
}

inline void sendCustomActor(
    Player&                             player,
    CustomEntityConfig const&           data,
    Runtime const&                      rt,
    std::optional<std::uint64_t> const& vehicleUniqueId) {

    PitchYaw const rot = effectiveRot(rt, player.getUuid(), data);

    using ids = proto::ActorDataIDs;

    proto::AddActorPacket pkt;
    pkt.entity_id        = proto::ActorUniqueID{static_cast<std::int64_t>(rt.uniqueId)};
    pkt.runtime_id       = proto::bp::ActorRuntimeID{rt.runtimeId};
    pkt.actor_type       = normalizeIdentifier(data.identifier);
    pkt.pos              = {data.x, data.y, data.z};
    pkt.velocity         = {0, 0, 0};
    pkt.rot              = {rot.pitch, rot.yaw};
    pkt.y_head_rotation  = rot.yaw;
    pkt.y_body_rotation  = rot.yaw;
    pkt.links            = {};

    std::int64_t const flags = data.flags | (data.invisible ? (std::int64_t(1) << 5) : 0);

    pkt.data.data.clear();
    pkt.data.data.push_back(meta_wire::int64(ids::Reserved0, flags));
    {
        auto const& name = effectiveNametag(data, rt, player.getUuid());
        if (!name.empty()) {
            pkt.data.data.push_back(meta_wire::string(ids::Name, name));
        }
    }

    pkt.data.data.push_back(
        meta_wire::int32(ids::NametagAlwaysShow, static_cast<std::int32_t>(data.nametagAlwaysShow ? 0 : 1)));
    if (data.variant != 0) {
        pkt.data.data.push_back(meta_wire::int32(ids::Variant, static_cast<std::int32_t>(data.variant)));
    }
    if (data.markVariant != 0) {
        pkt.data.data.push_back(meta_wire::int32(ids::MarkVariant, static_cast<std::int32_t>(data.markVariant)));
    }
    if (data.colorIndex != 0) {
        pkt.data.data.push_back(meta_wire::byte(ids::ColorIndex, static_cast<std::uint8_t>(data.colorIndex)));
    }

    if (data.pose != 0) {
        pkt.data.data.push_back(meta_wire::int32(ids::PoseIndex, static_cast<std::int32_t>(data.pose)));
    }

    float const scale = std::clamp(effectiveScaleValue(data, rt, player.getUuid()), 0.0625f, 10.0f);
    pkt.attributes    = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale", 0.0625f, 10.0f, scale},
    };
    pkt.synched_properties = {};
    sendProtoPacketToPlayer(player, pkt);

    {
        float const                             s = std::clamp(effectiveScaleValue(data, rt, player.getUuid()), 0.0625f, 10.0f);
        proto::UpdateAttributesPacket uap;
        uap.runtime_id      = proto::bp::ActorRuntimeID{rt.runtimeId};
        uap.attribute_data  = {
            makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f),
            makeAttribute("minecraft:scale", 0.0625f, 10.0f, s, 1.0f),
        };
        uap.tick = proto::bp::PlayerInputTick{currentTick()};
        sendProtoPacketToPlayer(player, uap);
    }

    sendEffectiveEquipment(player, data, rt);

    sendMobProperties(player, data, rt);

    if (vehicleUniqueId) {
        proto::SetActorLinkPacket linkPkt;
        linkPkt.link.type                     = proto::ActorLinkType::Riding;
        linkPkt.link.a                        = proto::ActorUniqueID{static_cast<std::int64_t>(*vehicleUniqueId)};
        linkPkt.link.b                        = proto::ActorUniqueID{static_cast<std::int64_t>(rt.uniqueId)};
        linkPkt.link.immediate                = true;
        linkPkt.link.passenger_initiated      = false;
        linkPkt.link.vehicle_angular_velocity = 0.0f;
        sendProtoPacketToPlayer(player, linkPkt);
    }
}

inline void sendRemove(Player& player, Runtime const& rt) {
    proto::RemoveActorPacket pkt;
    pkt.entity_id = proto::ActorUniqueID{static_cast<std::int64_t>(rt.uniqueId)};
    sendProtoPacketToPlayer(player, pkt);
}

inline bool spawnForPlayer(
    int64_t                             id,
    CustomEntityConfig const&           data,
    Runtime&                            rt,
    Player&                             player,
    std::optional<std::uint64_t> const& vehicleUniqueId) {

    if (!ll::service::getLevel())
        return false;

    sendCustomActor(player, data, rt, vehicleUniqueId);
    HLIB_LOG_DEBUG(
        "[CustomEntity] spawned #{} '{}' at ({:.1f},{:.1f},{:.1f}) dim={} scale={:.3f} tag='{}'",
        id,
        data.identifier,
        data.x,
        data.y,
        data.z,
        data.dimension,
        data.scale,
        data.nametag);
    return true;
}

inline void despawnForPlayer(Runtime const& rt, Player& player) {
    sendRemove(player, rt);
}
} // namespace ce_ops
} // namespace debugshape_export
