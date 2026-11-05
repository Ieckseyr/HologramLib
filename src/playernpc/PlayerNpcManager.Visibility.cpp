#include "PlayerNpcManager.h"

#include "DiagFlags.h"
#include "DiagLog.h"
#include "PlayerNpcRuntimeOps.h"
#include "core/TickDriver.h"
#include "playernpc/NpcProtocol.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {
using namespace npc_ops;

void PlayerNpcManager::refreshLocked(int64_t id) {
    auto* it  = configOf(id);
    auto* rit = runtimeOf(id);
    if (it == nullptr || rit == nullptr)
        return;

    auto& cfg = *it;
    auto& rt  = *rit;

    std::vector<Player*> toRefresh;
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* player = findPlayerByUuid(uuid))
            toRefresh.push_back(player);
    }
    if (toRefresh.empty())
        return;

    rt.runtimeId = mNextRuntimeId++;

    for (auto* player : toRefresh) {
        sculk::protocol::RemoveActorPacket rm;
        rm.mActorUniqueId = static_cast<std::int64_t>(rt.uniqueId);
        npc_protocol::sendToPlayer(*player, rm, wire::Reliability::Reliable);
    }
    rt.shownPlayers.clear();

    for (auto* player : toRefresh) {
        sculk::protocol::SerializedSkin skin;
        if (!NpcSkinRegistry::getInstance().getSkin(cfg.skinId, skin))
            continue;
        Vec3 pos{cfg.x, cfg.y, cfg.z};
        if (npc_protocol::spawnPlayerList(*player, id, rt.uniqueId, cfg.name, skin) && npc_protocol::spawnPlayerBody(*player, id, rt.runtimeId, rt.uniqueId, pos, effectiveYaw(rt, player->getUuid(), cfg), cfg.name, cfg.scale) && npc_protocol::sendPlayerSkin(*player, id, skin)) {
            rt.shownPlayers.insert(player->getUuid());
            afterSpawnLocked(*player, id, rt);
            mTabRemovals[id].push_back({player->getUuid(), currentTick() + 20});
            notifySpawn(id, player->getRealName());
        }
    }
}

void PlayerNpcManager::dropDisallowedLocked(int64_t id) {
    auto* rt = runtimeOf(id);
    if (rt == nullptr)
        return;

    auto fit = mVisibleFilter.find(id);
    if (fit == mVisibleFilter.end() || fit->second.empty()) {
        return; // 白名单放开：交给 tick 的可见性计算去补
    }

    std::vector<mce::UUID> drop;
    for (auto const& uuid : rt->shownPlayers) {
        auto* player = findPlayerByUuid(uuid);
        if (player == nullptr || !fit->second.contains(player->getRealName())) {
            drop.push_back(uuid);
        }
    }
    for (auto const& uuid : drop) {
        if (auto* player = findPlayerByUuid(uuid); player != nullptr) {
            npc_protocol::remove(*player, id, rt->uniqueId);
        }
        rt->shownPlayers.erase(uuid);
    }
}

namespace {

constexpr int    kMaxSpawnsPerPass = 2;
constexpr double kViewHysteresis   = 4.0;

} // namespace

void PlayerNpcManager::syncVisibilityLocked() {
    core::SpawnBudget budget{kMaxSpawnsPerPass};
    mEntries.forEachLocked([&](int64_t id, Entry&) {
        syncOneLocked(id, budget);
    });
}

void PlayerNpcManager::syncOneLocked(int64_t id, core::SpawnBudget& budget) {
    if (!mGrid.ensureFresh())
        return;

    auto* config = configOf(id);
    auto* rt     = runtimeOf(id);
    if (config == nullptr || rt == nullptr)
        return;

    core::VisibilityRule const rule{
        .enabled      = config->enabled,
        .dimension    = config->dimension,
        .x            = config->x,
        .y            = config->y,
        .z            = config->z,
        .viewDistance = config->viewDistance,
        .hysteresis   = kViewHysteresis,
        .allowList    = filterOf(id),
    };

    core::syncEntryVisibility(
        mGrid,
        mInitializedPlayers,
        rule,
        *rt,
        mSeenScratch,
        [](mce::UUID const& uuid) {
            return findPlayerByUuid(uuid);
        },
        [&](Player& player) {
            return spawnForViewerLocked(id, *config, *rt, player, budget);
        },
        [&](Player& player) {
            npc_protocol::remove(player, id, rt->uniqueId);
        });
}

bool PlayerNpcManager::spawnForViewerLocked(
    int64_t                id,
    PlayerNpcConfig const& config,
    Runtime&               rt,
    Player&                player,
    core::SpawnBudget&     budget) {
    if (!budget.available())
        return false;

    sculk::protocol::SerializedSkin skin;
    if (!NpcSkinRegistry::getInstance().getSkin(config.skinId, skin)) {
        if (!mWarnedMissingSkins.contains(config.skinId)) {
            mWarnedMissingSkins.insert(config.skinId);
            HLIB_LOG_WARN(
                "[PlayerNpc] npc #{} 的皮肤 '{}' 未注册, 跳过生成 (请 registerSkin/采集后再试)",
                id,
                config.skinId);
        }
        return false;
    }

    Vec3 const pos{config.x, config.y, config.z};

    auto const listBody = npc_protocol::buildPlayerListBody(id, rt.uniqueId, config.name, skin);
    auto const skinBody = npc_protocol::buildPlayerSkinBody(id, skin);

    if (!npc_protocol::sendBody(player, static_cast<std::uint32_t>(::MinecraftPacketIds::PlayerList), listBody))
        return false;
    if (!npc_protocol::spawnPlayerBody(
            player,
            id,
            rt.runtimeId,
            rt.uniqueId,
            pos,
            effectiveYaw(rt, player.getUuid(), config),
            config.name,
            config.scale))
        return false;
    if (!npc_protocol::sendBody(player, static_cast<std::uint32_t>(::MinecraftPacketIds::PlayerSkin), skinBody))
        return false;

    budget.consume();
    afterSpawnLocked(player, id, rt);
    mTabRemovals[id].push_back({player.getUuid(), currentTick() + 20});
    notifySpawn(id, player.getRealName());
    return true;
}

std::unordered_set<std::string> const* PlayerNpcManager::filterOf(int64_t id) const {
    auto const it = mVisibleFilter.find(id);
    return it == mVisibleFilter.end() ? nullptr : &it->second;
}

void PlayerNpcManager::processDirtyLocked() {
    if (mDirtyIds.empty())
        return;
    auto dirty = std::move(mDirtyIds);
    mDirtyIds.clear();

    core::SpawnBudget budget{kMaxSpawnsPerPass};
    for (auto id : dirty) {
        if (configOf(id) == nullptr)
            continue;
        refreshLocked(id);
        syncOneLocked(id, budget);
    }
}

struct PlayerNpcTickHookAccess {
    static void processDirty(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.processDirtyLocked();
    }

    static void processLightDirty(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        if (mgr.mLightDirtyIds.empty())
            return;
        auto light = std::move(mgr.mLightDirtyIds);
        mgr.mLightDirtyIds.clear();
        for (auto id : light) {
            if (mgr.configOf(id) == nullptr)
                continue;
            mgr.refreshLightLocked(id);
        }
    }
    static void flushAnims(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.flushAnimsLocked();
    }
    static void sync(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.syncVisibilityLocked();
    }
    static void processTabRemovals(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        auto const      now = currentTick();
        for (auto it = mgr.mTabRemovals.begin(); it != mgr.mTabRemovals.end();) {
            auto& removals = it->second;
            std::erase_if(removals, [&](PlayerNpcManager::TabRemoval const& r) {
                if (r.dueTick > now)
                    return false;

                return true;
            });
            if (removals.empty())
                it = mgr.mTabRemovals.erase(it);
            else
                ++it;
        }
    }
};

void playerNpcTick() {
    if (hlibDomainDisabled("HLIB_NO_NPC"))
        return;

    PlayerNpcTickHookAccess::processDirty(PlayerNpcManager::getInstance());
    PlayerNpcTickHookAccess::processLightDirty(PlayerNpcManager::getInstance());
    PlayerNpcTickHookAccess::flushAnims(PlayerNpcManager::getInstance());
    PlayerNpcTickHookAccess::processTabRemovals(PlayerNpcManager::getInstance());

    static std::uint64_t lastSyncTick = 0;
    auto const           now          = currentTick();
    if (now - lastSyncTick >= 20) {
        lastSyncTick = now;
        PlayerNpcTickHookAccess::sync(PlayerNpcManager::getInstance());
    }

    PlayerNpcManager::getInstance().drainSpawnCallbacks();
}

tick::AutoRegister gPlayerNpcTickHookRegistrar{"playernpc", &playerNpcTick, tick::Order::PlayerNpc};

bool PlayerNpcManager::findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const {
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

std::vector<std::string> PlayerNpcManager::pollSpawns() {
    std::lock_guard          lock(mMutex);
    std::vector<std::string> out;
    out.reserve(mSpawnQueue.size());
    for (auto const& [id, playerName] : mSpawnQueue) {
        out.push_back("player=" + playerName + " id=" + std::to_string(id));
    }
    mSpawnQueue.clear();
    return out;
}

void PlayerNpcManager::clearSpawns() {
    std::lock_guard lock(mMutex);
    mSpawnQueue.clear();
}

} // namespace debugshape_export
