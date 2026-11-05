#include "CustomEntityManager.h"

#include "CustomEntityRuntimeOps.h"
#include "DiagFlags.h"
#include "core/TickDriver.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {
using namespace ce_ops;

bool CustomEntityManager::playAnimation(
    int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (animation.empty())
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    if (!it->enabled || rit->shownPlayers.empty())
        return false;

    auto const now  = currentTick();
    auto const ctrl = "wiki.customentity." + std::to_string(id);

    if (isLegacyStopExpression(stopExpression)) {
        for (auto const& uuid : rit->shownPlayers) {
            mAnimQueue.emplace(
                now + 2,
                EntityAnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
        }
        return true;
    }
    for (auto const& uuid : rit->shownPlayers) {
        mAnimQueue.emplace(
            now + 2,
            EntityAnimEntry{uuid, rit->runtimeId, id, animation, ctrl, stopExpression});
    }

    if (durationTicks > 0) {
        for (auto const& uuid : rit->shownPlayers) {
            mAnimQueue.emplace(
                now + 2 + static_cast<std::uint64_t>(durationTicks),
                EntityAnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
        }
    }
    return true;
}

bool CustomEntityManager::playAnimationTo(
    int64_t id, std::string const& playerName,
    std::string const& animation, std::string const& stopExpression, int durationTicks) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (animation.empty())
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    if (!it->enabled)
        return false;

    auto level = ll::service::getLevel();
    if (!level)
        return false;
    Player*   target = nullptr;
    mce::UUID uuid{};
    level->forEachPlayer([&](Player& p) {
        if (p.getRealName() == playerName) {
            target = &p;
            uuid   = p.getUuid();
            return false;
        }
        return true;
    });
    if (!target)
        return false;
    if (!rit->shownPlayers.contains(uuid))
        return false;

    auto const now  = currentTick();
    auto const ctrl = "wiki.customentity." + std::to_string(id);
    if (isLegacyStopExpression(stopExpression)) {
        mAnimQueue.emplace(now + 2, EntityAnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
        return true;
    }
    mAnimQueue.emplace(now + 2, EntityAnimEntry{uuid, rit->runtimeId, id, animation, ctrl, stopExpression});
    if (durationTicks > 0) {
        mAnimQueue.emplace(
            now + 2 + static_cast<std::uint64_t>(durationTicks),
            EntityAnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
    }
    return true;
}

bool CustomEntityManager::stopAnimation(int64_t id) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr || rit->shownPlayers.empty())
        return false;
    auto const now  = currentTick();
    auto const ctrl = "wiki.customentity." + std::to_string(id);
    for (auto const& uuid : rit->shownPlayers) {
        mAnimQueue.emplace(now + 2, EntityAnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
    }
    return true;
}

bool CustomEntityManager::stopAnimationTo(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto level = ll::service::getLevel();
    if (!level)
        return false;
    Player*   target = nullptr;
    mce::UUID uuid{};
    level->forEachPlayer([&](Player& p) {
        if (p.getRealName() == playerName) {
            target = &p;
            uuid   = p.getUuid();
            return false;
        }
        return true;
    });
    if (!target || !rit->shownPlayers.contains(uuid))
        return false;
    mAnimQueue.emplace(
        currentTick() + 2,
        EntityAnimEntry{uuid, rit->runtimeId, id, kStopAnimation, "wiki.customentity." + std::to_string(id), kStopExpr});
    return true;
}

void CustomEntityManager::setEntitySpawnCallback(EntitySpawnCallback callback) {
    std::lock_guard lock(mMutex);
    mSpawnCallback = std::move(callback);
}

void CustomEntityManager::notifySpawnLocked(int64_t id, Player& player) {
    // 只入队：调用点可能正处在 forEachLocked 迭代里，同步回调里 destroy 会让迭代器失效
    constexpr std::size_t kSpawnQueueCap = 512;
    if (mSpawnQueue.size() >= kSpawnQueueCap)
        mSpawnQueue.pop_front();
    mSpawnQueue.emplace_back(id, player.getRealName());
    if (mSpawnCallback) {
        mPendingSpawnCallbacks.emplace_back(id, player.getRealName());
    }
}

void CustomEntityManager::drainSpawnCallbacks() {
    std::vector<std::pair<int64_t, std::string>> pending;
    EntitySpawnCallback                          cb;
    {
        std::lock_guard lock(mMutex);
        pending.swap(mPendingSpawnCallbacks);
        cb = mSpawnCallback;
    }
    if (!cb) {
        return;
    }
    for (auto const& [id, name] : pending) {
        cb(id, name);
    }
}

std::vector<std::string> CustomEntityManager::pollSpawns() {
    std::lock_guard          lock(mMutex);
    std::vector<std::string> out;
    out.reserve(mSpawnQueue.size());
    for (auto const& [id, playerName] : mSpawnQueue) {
        out.push_back("player=" + playerName + " id=" + std::to_string(id));
    }
    mSpawnQueue.clear();
    return out;
}

void CustomEntityManager::clearSpawns() {
    std::lock_guard lock(mMutex);
    mSpawnQueue.clear();
}

bool CustomEntityManager::getIdPair(int64_t id, std::uint64_t& outUniqueId, std::uint64_t& outRuntimeId) const {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    outUniqueId  = rit->uniqueId;
    outRuntimeId = rit->runtimeId;
    return true;
}

bool CustomEntityManager::findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const {
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

std::vector<int64_t> CustomEntityManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    return mEntries.snapshotAll();
}

int64_t CustomEntityManager::findNearest(float x, float y, float z, int dim, double maxDist) const {
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

std::optional<std::uint64_t> CustomEntityManager::resolveVehicleUniqueIdLocked(CustomEntityConfig const& data) {
    if (!data.ridePlayerName.empty()) {
        auto level = ll::service::getLevel();
        if (!level)
            return std::nullopt;
        if (auto* player = level->getPlayer(data.ridePlayerName)) {
            return static_cast<std::uint64_t>(player->getOrCreateUniqueID().rawID);
        }
        return std::nullopt;
    }
    if (data.rideEntityId != 0) {
        auto* rt = runtimeOf(data.rideEntityId);
        if (rt != nullptr)
            return rt->uniqueId;
    }
    return std::nullopt;
}

void CustomEntityManager::flushAnimsLocked() {
    if (mAnimQueue.empty())
        return;
    auto const now = currentTick();
    for (auto it = mAnimQueue.begin(); it != mAnimQueue.end() && it->first <= now;) {
        auto& e = it->second;

        bool  alive = false;
        auto* rit   = runtimeOf(e.entityId);
        if (rit != nullptr) {
            alive = rit->runtimeId == e.runtimeId && rit->shownPlayers.contains(e.playerUuid);
        }
        if (alive) {
            if (auto* player = findPlayerByUuid(e.playerUuid)) {
                sculk::protocol::AnimateEntityPacket pkt;
                pkt.mAnimation                   = e.animation;
                pkt.mNextState                   = "none";
                pkt.mStopExpression              = e.stopExpression;
                pkt.mStopExpressionMolangVersion = sculk::protocol::MolangVersion::Initial;
                pkt.mController                  = e.controller;
                pkt.mBlendOutTime                = 0;
                pkt.mRuntimeIds                  = {e.runtimeId};
                sendSculkPacketToPlayer(*player, pkt);
            }
        }
        it = mAnimQueue.erase(it);
    }
}

void CustomEntityManager::refreshLocked(int64_t id) {

    auto* it  = configOf(id);
    auto* rit = runtimeOf(id);
    if (it == nullptr || rit == nullptr)
        return;

    auto& rt    = (*rit);
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
        if (spawnForPlayer(id, (*it), rt, *player, resolveVehicleUniqueIdLocked((*it)))) {
            rt.shownPlayers.insert(player->getUuid());
            notifySpawnLocked(id, *player);
        }
    }
}

void CustomEntityManager::refreshLightLocked(int64_t id) {
    auto* it  = configOf(id);
    auto* rit = runtimeOf(id);
    if (it == nullptr || rit == nullptr)
        return;
    auto const& data = (*it);
    auto&       rt   = (*rit);

    if (!data.enabled || rt.shownPlayers.empty())
        return;

    auto const tick = currentTick();

    auto rotToByte = [](float deg) -> std::uint8_t {
        float d = std::fmod(deg, 360.0f);
        if (d < 0)
            d += 360.0f;
        return static_cast<std::uint8_t>((d * 256.0f) / 360.0f);
    };

    for (auto const& uuid : rt.shownPlayers) {
        auto* player = findPlayerByUuid(uuid);
        if (!player)
            continue;

        PitchYaw const rot = effectiveRot(rt, uuid, data);

        {
            sculk::protocol::MoveActorAbsolutePacket maap;
            maap.mActorRuntimeId = rt.runtimeId;
            maap.mFlags          = sculk::protocol::MoveActorAbsolutePacket::Flags::Teleport;
            maap.mPosition       = {data.x, data.y, data.z};
            maap.mRotationX      = rotToByte(rot.pitch);
            maap.mRotationY      = rotToByte(rot.yaw);
            maap.mRotationYHead  = rotToByte(rot.yaw);
            sendSculkPacketToPlayer(*player, maap);
        }

        {
            sculk::protocol::SetActorDataPacket sadp;
            sadp.mActorRuntimeId     = rt.runtimeId;
            sadp.mTick               = tick;
            std::int64_t const flags = data.flags | (data.invisible ? (std::int64_t(1) << 5) : 0);
            sadp.mMetaData.mDataItems.push_back(
                {sculk::protocol::ActorDataIDs::Reserved0, flags});
            {
                auto const& name = effectiveNametag(data, rt, uuid);
                if (!name.empty()) {
                    sadp.mMetaData.mDataItems.push_back(
                        {sculk::protocol::ActorDataIDs::Name, name});
                }
            }
            sadp.mMetaData.mDataItems.push_back(
                {sculk::protocol::ActorDataIDs::NametagAlwaysShow,
                 static_cast<std::int32_t>(data.nametagAlwaysShow ? 0 : 1)});
            sendSculkPacketToPlayer(*player, sadp);
        }

        {
            float const                             s = std::clamp(effectiveScaleValue(data, rt, uuid), 0.0625f, 10.0f);
            sculk::protocol::UpdateAttributesPacket uap;
            uap.mActorRuntimeId = rt.runtimeId;
            uap.mAttributes     = {
                makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, tick),
                makeAttribute("minecraft:scale", 0.0625f, 10.0f, s, 1.0f, tick),
            };
            uap.mTick = tick;
            sendSculkPacketToPlayer(*player, uap);
        }
    }
}

void CustomEntityManager::processDirtyLocked() {
    flushAnimsLocked();

    auto dirty = std::move(mDirtyIds);
    mDirtyIds.clear();

    auto light = std::move(mLightDirtyIds);
    mLightDirtyIds.clear();

    for (auto id : dirty) {
        if (configOf(id) == nullptr)
            continue;
        light.erase(id);
        refreshLocked(id);
    }
    for (auto id : light) {
        if (configOf(id) == nullptr)
            continue;
        refreshLightLocked(id);
    }
    if (dirty.empty())
        return;

    for (auto id : dirty) {
        if (configOf(id) == nullptr)
            continue;
        syncOneLocked(id);
    }
}

void CustomEntityManager::syncVisibilityLocked() {
    mEntries.forEachLocked([&](int64_t id, Entry&) {
        syncOneLocked(id);
    });
}

void CustomEntityManager::syncOneLocked(int64_t id) {
    if (!mGrid.ensureFresh())
        return;

    auto* cfg = configOf(id);
    auto* rt  = runtimeOf(id);
    if (cfg == nullptr || rt == nullptr)
        return;

    core::VisibilityRule const rule{
        .enabled      = cfg->enabled,
        .dimension    = cfg->dimension,
        .x            = cfg->x,
        .y            = cfg->y,
        .z            = cfg->z,
        .viewDistance = cfg->viewDistance,
        .hysteresis   = 0.0,
        .allowList    = filterOf(id),
    };

    core::syncEntryVisibility(
        mGrid,
        mInitializedPlayers,
        rule,
        *rt,
        mSeenScratch,
        [](mce::UUID const& uuid) {
            return core::findOnlinePlayer(uuid);
        },
        [&](Player& player) {
            return spawnOneLocked(id, *cfg, *rt, player);
        },
        [&](Player& player) {
            despawnForPlayer(*rt, player);
        });
}

bool CustomEntityManager::spawnOneLocked(int64_t id, CustomEntityConfig const& data, Runtime& rt, Player& player) {
    if (!spawnForPlayer(id, data, rt, player, resolveVehicleUniqueIdLocked(data)))
        return false;
    notifySpawnLocked(id, player);
    return true;
}

std::unordered_set<std::string> const* CustomEntityManager::filterOf(int64_t id) const {
    auto const it = mVisibleFilter.find(id);
    return it == mVisibleFilter.end() ? nullptr : &it->second;
}

struct CustomEntityTickHookAccess {
    static void sync(CustomEntityManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.syncVisibilityLocked();
    }
    static void processDirty(CustomEntityManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.processDirtyLocked();
    }
};

void customEntityTick() {
    if (hlibDomainDisabled("HLIB_NO_ENTITY"))
        return;

    CustomEntityTickHookAccess::processDirty(CustomEntityManager::getInstance());

    auto const           now          = currentTick();
    static std::uint64_t lastSyncTick = 0;
    if (now - lastSyncTick >= 20) {
        lastSyncTick = now;
        CustomEntityTickHookAccess::sync(CustomEntityManager::getInstance());
    }

    CustomEntityManager::getInstance().drainSpawnCallbacks();
}

tick::AutoRegister gCustomEntityTickHookRegistrar{"customentity", &customEntityTick, tick::Order::CustomEntity};

} // namespace debugshape_export
