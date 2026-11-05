#include "CustomEntityManager.h"
#include "../DiagFlags.h"
#include "core/EventIds.h"

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

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/level/MolangVersion.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/AnimateEntityPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorLinkPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateAttributesPacket.hpp>

#include "SculkPacketSend.h"

#include "sulfur/SulfurDisplayPackets.h"
#include <algorithm>
#include <memory>
#include <string>

#include "customentity/CustomEntityRuntimeOps.h"

namespace debugshape_export {
using namespace ce_ops;

CustomEntityManager& CustomEntityManager::getInstance() {
    static CustomEntityManager instance;
    return instance;
}

void CustomEntityManager::init() {
    std::lock_guard lock(mMutex);
    if (mJoinListener)
        return;

    mJoinListener       = debugshape_export::core::listen<ll::event::PlayerJoinEvent>(core::kPlayerJoinId,
                                                                                      [this](ll::event::PlayerJoinEvent& ev) {
                                                                                    if (hlibDomainDisabled("HLIB_NO_ENTITY"))
                                                                                        return;
                                                                                    std::lock_guard lock(mMutex);
                                                                                    mInitializedPlayers.insert(ev.self().getUuid());
                                                                                      });
    mDisconnectListener = debugshape_export::core::listen<ll::event::PlayerDisconnectEvent>(core::kPlayerDisconnectId,
                                                                                            [this](ll::event::PlayerDisconnectEvent& ev) {
                                                                                                std::lock_guard lock(mMutex);
                                                                                                auto const&     uuid = ev.self().getUuid();
                                                                                                mInitializedPlayers.erase(uuid);
                                                                                                mEntries.forEachLocked([&](int64_t, Entry& entry) {
                                                                                                    entry.runtime.shownPlayers.erase(uuid);
                                                                                                    entry.runtime.playerRot.erase(uuid);
                                                                                                });
                                                                                            });
}

void CustomEntityManager::shutdown() {
    std::lock_guard lock(mMutex);
    if (mJoinListener) {
        ll::event::EventBus::getInstance().removeListener(mJoinListener);
        mJoinListener.reset();
    }
    if (mDisconnectListener) {
        ll::event::EventBus::getInstance().removeListener(mDisconnectListener);
        mDisconnectListener.reset();
    }
    mEntries.clear();
    mDirtyIds.clear();
    mLightDirtyIds.clear();
    mVisibleFilter.clear();
    mAnimQueue.clear();
    mInitializedPlayers.clear();
}

int64_t CustomEntityManager::create(CustomEntityConfig const& config) {
    std::lock_guard lock(mMutex);

    int64_t const id = mEntries.insert(std::make_unique<Entry>());
    if (int64_t const created = createLocked(config, id); created < 0) {
        mEntries.erase(id);
        return created;
    }
    return id;
}

int64_t CustomEntityManager::createRandom(CustomEntityConfig const& config) {

    static std::mt19937_64 rng{std::random_device{}()};
    std::lock_guard        lock(mMutex);
    constexpr std::int64_t kBase = core::actorIds::randomIdBase, kSpan = core::actorIds::randomIdSpan;
    for (int tries = 0; tries < 128; ++tries) {
        auto const id = kBase + static_cast<std::int64_t>(rng() % static_cast<std::uint64_t>(kSpan));
        if ((configOf(id) != nullptr))
            continue;
        return createLocked(config, id);
    }
    return -1;
}

int64_t CustomEntityManager::createWithId(CustomEntityConfig const& config, int64_t desiredId) {
    if (desiredId <= 0)
        return -2;
    std::lock_guard lock(mMutex);
    if ((configOf(desiredId) != nullptr))
        return -2;

    return createLocked(config, desiredId);
}

bool CustomEntityManager::isIdUsed(int64_t id) const {
    std::lock_guard lock(mMutex);
    return (configOf(id) != nullptr);
}

int64_t CustomEntityManager::createLocked(CustomEntityConfig const& config, int64_t id) {
    if (hlibDomainDisabled("HLIB_NO_ENTITY"))
        return -1;

    if (config.identifier.empty()) {
        HLIB_LOG_WARN("[CustomEntity] create rejected: empty identifier");
        return -1;
    }

    Runtime rt{};
    rt.uniqueId  = allocUniqueIdLocked();
    rt.runtimeId = allocRuntimeIdLocked();

    if (auto* existing = mEntries.peek(id)) {
        existing->config  = config;
        existing->runtime = std::move(rt);
    } else {
        mEntries.insertWithId(id, std::make_unique<Entry>(Entry{config, std::move(rt)}));
    }

    syncOneLocked(id);
    return id;
}

std::uint64_t CustomEntityManager::allocUniqueIdLocked() {
    return mNextActorUniqueId++;
}

std::uint64_t CustomEntityManager::allocRuntimeIdLocked() {
    return mNextRuntimeId++;
}

bool CustomEntityManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);

    mDirtyIds.erase(id);
    mLightDirtyIds.erase(id);
    mVisibleFilter.erase(id);

    for (auto it = mAnimQueue.begin(); it != mAnimQueue.end();) {
        if (it->second.entityId == id)
            it = mAnimQueue.erase(it);
        else
            ++it;
    }
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;

    auto uuids = std::vector<mce::UUID>{rit->shownPlayers.begin(), rit->shownPlayers.end()};
    for (auto const& uuid : uuids) {
        if (auto* player = findPlayerByUuid(uuid)) {
            despawnForPlayer(*rit, *player);
        }
    }
    mEntries.erase(id);
    return true;
}

void CustomEntityManager::destroyAll() {
    std::lock_guard lock(mMutex);
    mDirtyIds.clear();
    mLightDirtyIds.clear();
    mVisibleFilter.clear();
    mAnimQueue.clear();
    mEntries.forEachLocked([&](int64_t, Entry& entry) {
        auto& rt    = entry.runtime;
        auto  uuids = std::vector<mce::UUID>{rt.shownPlayers.begin(), rt.shownPlayers.end()};
        for (auto const& uuid : uuids) {
            if (auto* player = findPlayerByUuid(uuid)) {
                despawnForPlayer(rt, *player);
            }
        }
    });
    mEntries.clear();
}

bool CustomEntityManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return (configOf(id) != nullptr);
}

bool CustomEntityManager::get(int64_t id, CustomEntityConfig& out) const {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    out = (*it);
    return true;
}

bool CustomEntityManager::setIdentifier(int64_t id, std::string const& identifier) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (identifier.empty())
        return false;
    it->identifier = identifier;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setPosition(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->x                 = x;
    it->y                 = y;
    it->z                 = z;
    bool const dimChanged = (dim >= 0 && it->dimension != dim);
    if (dim >= 0)
        it->dimension = dim;

    if (dimChanged)
        mDirtyIds.insert(id);
    else
        mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setRotation(int64_t id, float yaw, float pitch) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->yaw   = yaw;
    it->pitch = pitch;
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setPlayerRotation(int64_t id, std::string const& playerName, float yaw, float pitch) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr || runtimeOf(id) == nullptr)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto& rt = *runtimeOf(id);
    if (!rt.shownPlayers.contains(player->getUuid()))
        return false;
    rt.playerRot[player->getUuid()] = hologramlib::PerPlayerRotation{yaw, pitch};
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::clearPlayerRotation(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    if (rit->playerRot.erase(player->getUuid()) == 0)
        return false;
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setPlayerNametag(int64_t id, std::string const& playerName, std::string const& text) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr || runtimeOf(id) == nullptr)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto& rt = *runtimeOf(id);
    if (!rt.shownPlayers.contains(player->getUuid()))
        return false;

    if (text.empty()) {
        rt.playerNameTag.erase(player->getUuid());
    } else {
        rt.playerNameTag[player->getUuid()] = text;
    }
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setPlayerScale(int64_t id, std::string const& playerName, float scale) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr || runtimeOf(id) == nullptr)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto& rt = *runtimeOf(id);
    if (!rt.shownPlayers.contains(player->getUuid()))
        return false;

    if (scale <= 0.0f) {
        rt.playerScale.erase(player->getUuid());
    } else {
        rt.playerScale[player->getUuid()] = std::clamp(scale, 0.0625f, 10.0f);
    }
    mLightDirtyIds.insert(id);
    return true;
}

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
    auto* player = findPlayerByName(playerName);
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
    sendEffectiveEquipment(*player, data, rt);
    return true;
}

bool CustomEntityManager::clearPlayerAppearance(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto& rt = (*rit);

    bool const had = rt.playerNameTag.erase(player->getUuid()) > 0 || rt.playerScale.erase(player->getUuid()) > 0 || rt.playerEquip.erase(player->getUuid()) > 0;
    if (!had)
        return false;
    mLightDirtyIds.insert(id);
    if (auto* p2 = findPlayerByName(playerName)) {
        sendEffectiveEquipment(*p2, *configOf(id), rt);
    }
    return true;
}

bool CustomEntityManager::clearPlayerRotations(int64_t id) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    if (rit->playerRot.empty())
        return true;
    rit->playerRot.clear();
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setNametag(int64_t id, std::string const& text) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->nametag = text;
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setScale(int64_t id, float scale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (scale <= 0)
        return false;
    it->scale = scale;
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setVariant(int64_t id, int variant) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->variant = variant;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setMarkVariant(int64_t id, int markVariant) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->markVariant = markVariant;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setColorIndex(int64_t id, int colorIndex) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->colorIndex = colorIndex;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setFlags(int64_t id, int64_t flags) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->flags = flags;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setInvisible(int64_t id, bool on) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->invisible == on)
        return true;
    it->invisible = on;
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setEnabled(int64_t id, bool enabled) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->enabled = enabled;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setViewDistance(int64_t id, double dist) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->viewDistance = dist;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setPose(int64_t id, int pose) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (pose < 0)
        pose = 0;
    if (pose > 13)
        pose = 13;
    it->pose = pose;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setEquipmentSlot(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) {
    std::lock_guard lock(mMutex);
    if (slot < 0 || slot > 5)
        return false;
    auto* it = configOf(id);
    if (it == nullptr)
        return false;
    if (name.empty()) {
        it->equipment[slot] = {};
    } else {
        it->equipment[slot].name = name;
        it->equipment[slot].aux  = aux;
        it->equipment[slot].nbt  = nbt;
    }
    mDirtyIds.insert(id);
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
        if (auto* player = findPlayerByUuid(uuid)) {
            sendSculkPacketToPlayer(
                *player,
                sulfur::makeMobProperty(static_cast<std::int64_t>(rt.uniqueId), name, value));
        }
    }
    return true;
}

bool CustomEntityManager::clearMobProperties(int64_t id) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->mobProperties.clear();
    return true;
}

bool CustomEntityManager::scaleBy(int64_t id, double factor) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (factor <= 0)
        return false;
    it->scale =
        std::clamp(static_cast<float>(it->scale * factor), 0.0625f, 10.0f);
    mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;

    if (playerNames.empty()) {
        mVisibleFilter.erase(id);
    } else {
        mVisibleFilter.insert_or_assign(
            id, std::unordered_set<std::string>(playerNames.begin(), playerNames.end()));
    }

    syncOneLocked(id);
    return true;
}

bool CustomEntityManager::clearVisiblePlayers(int64_t id) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;
    if (mVisibleFilter.erase(id) == 0)
        return true;
    syncOneLocked(id);
    return true;
}

bool CustomEntityManager::setVisiblePlayer(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;

    mVisibleFilter.insert_or_assign(id, std::unordered_set<std::string>{playerName});

    syncOneLocked(id);
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

bool CustomEntityManager::clearRide(int64_t id) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->ridePlayerName.empty() && it->rideEntityId == 0)
        return true;
    it->ridePlayerName.clear();
    it->rideEntityId = 0;
    mDirtyIds.insert(id);
    return true;
}

} // namespace debugshape_export
