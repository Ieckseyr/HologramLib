#pragma once

#include "PlayerNpcManager.h"

#include "SculkPacketSend.h"
#include "core/EventIds.h"
#include "playernpc/NpcProtocol.h"

#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/math/Vec2.h>
#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/network/packet/MobArmorEquipmentPacket.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/ContainerID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>
#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorLinkPacket.hpp>

namespace debugshape_export {
namespace npc_ops {

inline std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
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

inline float effectiveYaw(PlayerNpcManager::Runtime const& rt, mce::UUID const& uuid, PlayerNpcConfig const& cfg) {
    auto it = rt.playerRot.find(uuid);
    return it == rt.playerRot.end() ? cfg.yaw : it->second.yaw;
}

inline ::ItemStack buildNpcEquip(PlayerNpcManager::Runtime::NpcEquip const& eq) {
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
    } catch (...) {
        return {};
    }
}

inline void sendNpcEquipment(Player& player, PlayerNpcManager::Runtime const& rt) {
    for (int slot = 0; slot <= 1; ++slot) {
        auto stack = buildNpcEquip(rt.equip[slot]);
        if (stack.isNull())
            continue;
        auto                 cid = (slot == 1) ? ::ContainerID::Offhand : ::ContainerID::Inventory;
        ::MobEquipmentPacket pkt{::ActorRuntimeID{rt.runtimeId}, stack, static_cast<unsigned char>(slot), 0, cid};
        pkt.sendTo(player);
    }
    auto head  = buildNpcEquip(rt.equip[2]);
    auto torso = buildNpcEquip(rt.equip[3]);
    auto legs  = buildNpcEquip(rt.equip[4]);
    auto feet  = buildNpcEquip(rt.equip[5]);
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

inline void sendNpcPose(Player& player, PlayerNpcManager::Runtime const& rt) {
    if (rt.pose == 0 && rt.flags == 0)
        return;
    std::int64_t flags = rt.flags;
    if (rt.pose == 1)
        flags |= (std::int64_t(1) << 1);
    if (rt.pose == 3)
        flags |= (std::int64_t(1) << 57);

    sculk::protocol::SetActorDataPacket pkt;
    pkt.mActorRuntimeId      = rt.runtimeId;
    pkt.mSynchedProperties   = {};
    pkt.mTick                = currentTick();
    pkt.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0, flags},
        {sculk::protocol::ActorDataIDs::PoseIndex, static_cast<std::int32_t>(rt.pose)},
    };
    sendSculkPacketToPlayer(player, pkt);
}

inline void sendNpcRide(Player& player, PlayerNpcManager::Runtime const& rt) {
    if (rt.rideVehicleUniqueId == 0)
        return;
    sculk::protocol::SetActorLinkPacket pkt;
    pkt.mLink.mType                   = sculk::protocol::ActorLinkType::Riding;
    pkt.mLink.mTargetAUniqueId        = rt.rideVehicleUniqueId;
    pkt.mLink.mTargetBUniqueId        = static_cast<std::int64_t>(rt.uniqueId);
    pkt.mLink.mImmediate              = true;
    pkt.mLink.mPassengerInitiated     = false;
    pkt.mLink.mVehicleAngularVelocity = 0.0f;
    sendSculkPacketToPlayer(player, pkt);
}

} // namespace npc_ops
} // namespace debugshape_export
