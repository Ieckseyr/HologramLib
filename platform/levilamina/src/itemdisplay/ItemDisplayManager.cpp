#include "ItemDisplayManager.h"
#include "ItemDisplayRuntimeOps.h"
#include "core/DiagFlags.h"
#include "kernel/EventIds.h"

#include "DiagLog.h"

#include "kernel/PacketSink.h"

#include "core/TickDriver.h"

#include <random>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/deps/nbt/ShortTag.h>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/enchanting/Enchant.h>
#include <mc/world/item/enchanting/EnchantmentInstance.h>
#include <mc/world/item/enchanting/ItemEnchants.h>
#include <mc/world/item/registry/ItemRegistryRef.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>


#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <map>
#include <optional>

namespace debugshape_export {
using namespace ide_ops;

ItemDisplayManager& ItemDisplayManager::getInstance() {
    static ItemDisplayManager instance;
    return instance;
}

void ItemDisplayManager::init() {
    std::lock_guard lock(mMutex);
    if (mJoinListener)
        return;

    mJoinListener       = debugshape_export::core::listen<ll::event::PlayerJoinEvent>(core::kPlayerJoinId,
                                                                                      [this](ll::event::PlayerJoinEvent& ev) {
                                                                                    if (hlibDomainDisabled("HLIB_NO_DISPLAY"))
                                                                                        return;
                                                                                    std::lock_guard lock(mMutex);
                                                                                    mInitializedPlayers.insert(ev.self().getUuid());

                                                                                    syncVisibilityLocked();
                                                                                      });
    mDisconnectListener = debugshape_export::core::listen<ll::event::PlayerDisconnectEvent>(core::kPlayerDisconnectId,
                                                                                            [this](ll::event::PlayerDisconnectEvent& ev) {
                                                                                                std::lock_guard lock(mMutex);
                                                                                                auto const&     uuid = ev.self().getUuid();
                                                                                                mInitializedPlayers.erase(uuid);
                                                                                                mEntries.forEachLocked([&](int64_t, Entry& entry) {
                                                                                                    entry.runtime.shownPlayers.erase(uuid);
                                                                                                });
                                                                                            });
}

void ItemDisplayManager::shutdown() {
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
    mVisibleFilter.clear();
    mFollowers.clear();
    mInitializedPlayers.clear();
    animQueue().clear();
}

int64_t ItemDisplayManager::create(ItemDisplayConfig const& config) {
    std::lock_guard lock(mMutex);

    int64_t const id = mEntries.insert(std::make_unique<Entry>());
    if (int64_t const created = createLocked(config, id); created < 0) {
        mEntries.erase(id);
        return created;
    }
    return id;
}

int64_t ItemDisplayManager::createRandom(ItemDisplayConfig const& config) {

    static std::mt19937_64 rng{std::random_device{}()};
    std::lock_guard        lock(mMutex);
    constexpr std::int64_t kBase = core::actorIds::randomIdBase, kSpan = core::actorIds::randomIdSpan;
    for (int tries = 0; tries < 128; ++tries) {
        auto const id = kBase + static_cast<std::int64_t>(rng() % static_cast<std::uint64_t>(kSpan));
        if (configOf(id) != nullptr)
            continue;
        return createLocked(config, id);
    }
    return -1;
}

int64_t ItemDisplayManager::createWithId(ItemDisplayConfig const& config, int64_t desiredId) {
    if (desiredId <= 0)
        return -2;
    std::lock_guard lock(mMutex);
    if (configOf(desiredId) != nullptr)
        return -2;
    return createLocked(config, desiredId);
}

int64_t ItemDisplayManager::createSeamless(
    ItemDisplayConfig const& config,
    int                      mode,
    double                   viewDistance,
    std::string const&       visiblePlayer) {
    std::lock_guard lock(mMutex);
    if (hlibDomainDisabled("HLIB_NO_DISPLAY"))
        return -1;
    ItemDisplayConfig cfg = config;
    if (mode == 1 || mode == 2)
        cfg.mode = mode;
    if (viewDistance >= 0)
        cfg.viewDistance = viewDistance;

    int64_t const id = mEntries.insert(std::make_unique<Entry>());

    if (!visiblePlayer.empty()) {
        mVisibleFilter.emplace(id, std::unordered_set<std::string>{visiblePlayer});
    }

    if (int64_t const created = createLocked(cfg, id); created < 0) {
        mEntries.erase(id);
        mVisibleFilter.erase(id);
        return created;
    }
    return id;
}

bool ItemDisplayManager::isIdUsed(int64_t id) const {
    std::lock_guard lock(mMutex);
    return (configOf(id) != nullptr);
}

int64_t ItemDisplayManager::createLocked(ItemDisplayConfig const& config, int64_t id) {
    if (hlibDomainDisabled("HLIB_NO_DISPLAY"))
        return -1;

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

std::uint64_t ItemDisplayManager::allocUniqueIdLocked() {
    return mNextActorUniqueId++;
}

std::uint64_t ItemDisplayManager::allocRuntimeIdLocked() {
    return mNextRuntimeId++;
}

bool ItemDisplayManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);

    mDirtyIds.erase(id);
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
    mVisibleFilter.erase(id);
    mFollowers.erase(id);
    return true;
}

void ItemDisplayManager::destroyAll() {
    std::lock_guard lock(mMutex);
    mDirtyIds.clear();
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
    mVisibleFilter.clear();
    mFollowers.clear();
}

bool ItemDisplayManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return (configOf(id) != nullptr);
}

bool ItemDisplayManager::get(int64_t id, ItemDisplayConfig& out) const {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    out = *it;
    return true;
}

bool ItemDisplayManager::setItem(int64_t id, std::string const& item, int aux) {
    return setItemWithNbt(id, item, aux, "");
}

bool ItemDisplayManager::setItemWithNbt(
    int64_t            id,
    std::string const& item,
    int                aux,
    std::string const& nbt) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->item    = item;
    it->itemAux = aux;
    it->itemNbt = nbt;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setGlint(int64_t id, bool on) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->itemGlint == on)
        return true;
    it->itemGlint = on;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setPosition(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    mFollowers.erase(id);
    it->x = x;
    it->y = y;
    it->z = z;
    if (dim >= 0)
        it->dimension = dim;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->offsetX = ox;
    it->offsetY = oy;
    it->offsetZ = oz;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->baseOffsetX = ox;
    it->baseOffsetY = oy;
    it->baseOffsetZ = oz;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->rotX = rx;
    it->rotY = ry;
    it->rotZ = rz;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setScale(int64_t id, std::string const& scale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->scale = scale;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->extendScale = scale;
    it->extendRotX  = rx;
    it->extendRotY  = ry;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setMode(int64_t id, int mode) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->mode == mode)
        return true;
    it->mode = mode;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setEnabled(int64_t id, bool enabled) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->enabled = enabled;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setViewDistance(int64_t id, double dist) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->viewDistance == dist)
        return true;
    it->viewDistance = dist;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;

    if (playerNames.empty()) {
        mVisibleFilter.erase(id);
    } else {
        std::unordered_set<std::string> names(playerNames.begin(), playerNames.end());
        mVisibleFilter.insert_or_assign(id, std::move(names));
    }

    syncOneLocked(id);
    return true;
}

bool ItemDisplayManager::clearVisiblePlayers(int64_t id) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;
    if (mVisibleFilter.erase(id) == 0)
        return true;
    syncOneLocked(id);
    return true;
}

bool ItemDisplayManager::setVisiblePlayer(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;

    mVisibleFilter.insert_or_assign(id, std::unordered_set<std::string>{playerName});

    syncOneLocked(id);
    return true;
}

bool ItemDisplayManager::scaleTo(int64_t id, double targetScale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (!std::isfinite(targetScale) || targetScale < 0.01)
        targetScale = 0.01;

    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;

    it->scale = std::format("{:.4f}", targetScale);

    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setStretch(int64_t id, std::string const& xz, std::string const& y) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    rit->stretchXZ = xz;
    rit->stretchY  = y;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setStretchLive(int64_t id, std::string const& xz, std::string const& y) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    rit->stretchXZ = xz;
    rit->stretchY  = y;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setRotationLive(
    int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    it->rotX = rx;
    it->rotY = ry;
    it->rotZ = rz;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setOffsetLive(
    int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    it->offsetX = ox;
    it->offsetY = oy;
    it->offsetZ = oz;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setScaleLive(int64_t id, std::string const& scale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    it->scale = scale;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setTransformLive(
    int64_t            id,
    std::string const& ox, std::string const& oy, std::string const& oz,
    std::string const& rx, std::string const& ry, std::string const& rz,
    std::string const& scale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    it->offsetX = ox;
    it->offsetY = oy;
    it->offsetZ = oz;
    it->rotX    = rx;
    it->rotY    = ry;
    it->rotZ    = rz;
    it->scale   = scale;
    return resendFmbeLocked(id, (*it), (*rit));
}

std::string ItemDisplayManager::getDebugInfo(int64_t id) const {
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
    auto& d  = *it;
    auto& rt = *rit;
    return std::format(
        "id={} dim={} pos=({:.1f},{:.1f},{:.1f}) mode={} enabled={} item='{}' view={} filter=[{}] shown={}",
        id,
        d.dimension,
        d.x,
        d.y,
        d.z,
        d.mode,
        d.enabled,
        d.item,
        d.viewDistance,
        filterList,
        rt.shownPlayers.size());
}

bool ItemDisplayManager::rotateY(int64_t id, float delta) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    auto& ry = it->rotY;
    float current{};
    auto  parsed = std::from_chars(ry.data(), ry.data() + ry.size(), current);
    if (parsed.ec == std::errc{} && parsed.ptr == ry.data() + ry.size()) {
        ry = std::format("{}", current + delta);
    } else {
        ry = std::format("({})+{}", ry, delta);
    }
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::scaleBy(int64_t id, double factor) {
    if (factor <= 0)
        return false;
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    auto& s = it->scale;
    float current{};
    auto  parsed = std::from_chars(s.data(), s.data() + s.size(), current);
    if (parsed.ec == std::errc{} && parsed.ptr == s.data() + s.size()) {
        if (current * factor <= 0)
            return false;
        s = std::format("{}", current * factor);
    } else {
        s = std::format("({})*{}", s, factor);
    }
    mDirtyIds.insert(id);
    return true;
}

std::vector<int64_t> ItemDisplayManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    return mEntries.snapshotAll();
}

std::uint64_t ItemDisplayManager::runtimeIdOf(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto*           rt = runtimeOf(id);
    return rt == nullptr ? 0 : rt->runtimeId;
}

bool ItemDisplayManager::findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const {
    std::lock_guard lock(mMutex);
    bool            found = false;
    mEntries.forEachLocked([&](int64_t id, Entry const& entry) {
        if (!found && entry.runtime.runtimeId == runtimeId) {
            outId = id;
            found = true;
        }
    });
    return found;
}

int64_t ItemDisplayManager::findNearest(float x, float y, float z, int dim, double maxDist) const {
    std::lock_guard lock(mMutex);
    int64_t         best      = -1;
    double          bestDist2 = (maxDist > 0) ? maxDist * maxDist : 1e300;
    mEntries.forEachLocked([&](int64_t id, Entry const& entry) {
        auto const& cfg = entry.config;
        if (cfg.dimension != dim)
            return;
        double const dx    = cfg.x - x;
        double const dy    = cfg.y - y;
        double const dz    = cfg.z - z;
        double const dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            best      = id;
        }
    });
    return best;
}

void ItemDisplayManager::refreshLocked(int64_t id) {

    auto* it  = configOf(id);
    auto* rit = runtimeOf(id);
    if (it == nullptr || rit == nullptr)
        return;

    auto& rt    = *rit;
    auto  uuids = std::vector<mce::UUID>{rt.shownPlayers.begin(), rt.shownPlayers.end()};

    std::vector<Player*> toSpawn;
    bool                 replaced = false;
    for (auto const& uuid : uuids) {
        auto* player = findPlayerByUuid(uuid);
        if (!player) {
            rt.shownPlayers.erase(uuid);
            continue;
        }
        despawnForPlayer(rt, *player);
        rt.shownPlayers.erase(uuid);
        replaced = true;
        if (it->enabled)
            toSpawn.push_back(player);
    }

    if (replaced) {
        rt.uniqueId  = allocUniqueIdLocked();
        rt.runtimeId = allocRuntimeIdLocked();
    }

    for (auto* player : toSpawn) {
        if (spawnForPlayer(id, (*it), rt, *player)) {
            rt.shownPlayers.insert(player->getUuid());
        }
    }
}

void ItemDisplayManager::processDirtyLocked() {

    if (mDirtyIds.empty())
        return;
    auto dirty = std::move(mDirtyIds);
    mDirtyIds.clear();

    for (auto id : dirty) {
        if (configOf(id) == nullptr)
            continue;
        refreshLocked(id);
        syncOneLocked(id);
    }
}

} // namespace debugshape_export
