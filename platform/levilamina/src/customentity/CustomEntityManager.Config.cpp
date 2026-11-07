#include "CustomEntityManager.h"
#include "core/DiagFlags.h"
#include "kernel/EventIds.h"

#include "DiagLog.h"

#include "core/TickDriver.h"

#include <random>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/deps/nbt/CompoundTag.h>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/packet/MobArmorEquipmentPacket.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>


#include "ProtoPacketSend.h"

#include "sulfur/SulfurDisplayPackets.h"
#include <algorithm>
#include <memory>
#include <string>

#include "customentity/CustomEntityRuntimeOps.h"

namespace debugshape_export {

bool CustomEntityManager::setPlayerEquipmentSlot(
    int64_t            id,
    std::string const& playerName,
    int                slot,
    std::string const& name,
    int                aux,
    std::string const& nbt) {
    std::lock_guard lock(mMutex);
    if (slot < 0 || slot > 5)
        return false;
    if (configOf(id) == nullptr || runtimeOf(id) == nullptr)
        return false;
    auto* player = ce_ops::findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto& rt = *runtimeOf(id);
    if (!rt.shownPlayers.contains(player->getUuid()))
        return false;

    if (name.empty()) {

        auto it = rt.playerEquip.find(player->getUuid());
        if (it != rt.playerEquip.end()) {
            it->second[static_cast<std::size_t>(slot)] = {};
            bool allEmpty                              = true;
            for (auto const& e : it->second) {
                if (!e.name.empty()) {
                    allEmpty = false;
                    break;
                }
            }
            if (allEmpty)
                rt.playerEquip.erase(it);
        }
    } else {
        auto& entry = rt.playerEquip[player->getUuid()][static_cast<std::size_t>(slot)];
        entry.name  = name;
        entry.aux   = aux;
        entry.nbt   = nbt;
    }

    auto& data = *configOf(id);
    ce_ops::sendEffectiveEquipment(*player, data, rt);
    return true;
}

bool CustomEntityManager::setMobProperty(int64_t id, std::string const& name, std::string const& value) {
    if (name.empty())
        return false;
    CustomEntityConfig           data;
    CustomEntityManager::Runtime rt{};
    {
        std::lock_guard lock(mMutex);
        auto*           it = configOf(id);
        if (it == nullptr)
            return false;
        bool found = false;
        for (auto& prop : it->mobProperties) {
            if (prop.name == name) {
                prop.value = value;
                found      = true;
                break;
            }
        }
        if (!found)
            it->mobProperties.push_back(hologramlib::EntityMobProperty{name, value});
        data = (*it);
        if (auto* rit = runtimeOf(id); rit != nullptr)
            rt = (*rit);
    }

    if (rt.uniqueId == 0)
        return true;
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* player = ce_ops::findPlayerByUuid(uuid)) {
            sendProtoPacketToPlayer(
                *player,
                sulfur::makeMobProperty(static_cast<std::int64_t>(rt.uniqueId), name, value));
        }
    }
    return true;
}

std::string CustomEntityManager::getDebugInfo(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return "not_found";

    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return "no_runtime";

    auto        fit = mVisibleFilter.find(id);
    std::string filterList;
    if (fit != mVisibleFilter.end()) {
        for (auto const& n : fit->second) {
            if (!filterList.empty())
                filterList += ",";
            filterList += n;
        }
    }
    auto& d          = (*it);
    int   equipCount = 0;
    for (auto const& eq : d.equipment) {
        if (!eq.name.empty())
            ++equipCount;
    }
    std::string ride = "none";
    if (!d.ridePlayerName.empty()) {
        ride = "player:" + d.ridePlayerName;
    } else if (d.rideEntityId != 0) {
        ride = "entity:" + std::to_string(d.rideEntityId);
    }
    return std::format(
        "id={} dim={} pos=({:.1f},{:.1f},{:.1f}) identifier='{}' enabled={} scale={:.3f} pose={} "
        "equip={} view={} filter=[{}] shown={} ride={}",
        id,
        d.dimension,
        d.x,
        d.y,
        d.z,
        d.identifier,
        d.enabled,
        d.scale,
        d.pose,
        equipCount,
        d.viewDistance,
        filterList,
        rit->shownPlayers.size(),
        ride);
}

bool CustomEntityManager::setRidePlayer(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    if (playerName.empty()) {

        if (it->ridePlayerName.empty() && it->rideEntityId == 0)
            return true;
        it->ridePlayerName.clear();
        it->rideEntityId = 0;
        mDirtyIds.insert(id);
        return true;
    }

    auto level = ll::service::getLevel();
    if (!level || !level->getPlayer(playerName))
        return false;

    it->ridePlayerName = playerName;
    it->rideEntityId   = 0;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setRideEntity(int64_t id, int64_t vehicleEntityId) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    if (vehicleEntityId == 0) {

        if (it->ridePlayerName.empty() && it->rideEntityId == 0)
            return true;
        it->ridePlayerName.clear();
        it->rideEntityId = 0;
        mDirtyIds.insert(id);
        return true;
    }

    if (vehicleEntityId == id)
        return false;
    if (runtimeOf(vehicleEntityId) == nullptr)
        return false;

    it->rideEntityId = vehicleEntityId;
    it->ridePlayerName.clear();
    mDirtyIds.insert(id);
    return true;
}

} // namespace debugshape_export
