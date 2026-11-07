#include "ItemDisplayManager.h"

#include "core/DiagFlags.h"
#include "ItemDisplayRuntimeOps.h"
#include "core/TickDriver.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;
using namespace ide_ops;

bool ItemDisplayManager::follow(int64_t id, std::string const& playerName, float offX, float offY, float offZ) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;
    FollowState st{};
    st.playerName = playerName;
    st.offX       = offX;
    st.offY       = offY;
    st.offZ       = offZ;
    st.synced     = false;
    mFollowers.insert_or_assign(id, std::move(st));
    return true;
}

int ItemDisplayManager::emitForEntity(std::string const& playerName, std::int64_t uniqueId, ItemDisplayConfig const& cfg) {
    auto level = ll::service::getLevel();
    if (!level)
        return -1;
    Player* player = nullptr;
    level->forEachPlayer([&](Player& p) {
        if (p.getRealName() == playerName) {
            player = &p;
            return false;
        }
        return true;
    });
    if (!player)
        return -1;
    auto* ent = level->fetchEntity(ActorUniqueID(uniqueId), false);
    if (!ent)
        return -1;
    auto stack = buildItemStack(cfg.item, cfg.itemAux, cfg.itemNbt, cfg.itemGlint);
    if (!stack)
        return -1;

    auto const  runtimeId = static_cast<std::uint64_t>(ent->getRuntimeID().rawID);
    auto const& pos       = ent->getPosition();
    auto const& vel       = ent->getVelocity();
    int const   mode      = effectiveMode(cfg, *stack);

    ItemDisplayConfig data = cfg;
    data.x                 = pos.x;
    data.y                 = pos.y;
    data.z                 = pos.z;
    if (data.rotY == "0") {
        data.rotY = std::format("{:.2f}", std::atan2(vel.x, -vel.z) * 57.2957795f + 90.0f);
    }

    {
        proto::RemoveActorPacket rm;
        rm.entity_id = proto::ActorUniqueID{uniqueId};
        wire::send(*player, rm);
    }

    Runtime tmpRt{};
    tmpRt.uniqueId    = static_cast<std::uint64_t>(uniqueId);
    tmpRt.runtimeId   = runtimeId;
    tmpRt.cachedStack = *stack;
    sendFoxActor(*player, data, tmpRt);
    sendDataPacket(*player, runtimeId, data.hitboxWidth, data.hitboxHeight);
    scheduleAnims(uniqueId, data, tmpRt, *player, mode, true);
    return mode;
}

bool ItemDisplayManager::unfollow(int64_t id) {
    std::lock_guard lock(mMutex);
    mFollowers.erase(id);
    return true;
}

bool ItemDisplayManager::setHitbox(int64_t id, float width, float height) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->hitboxWidth  = width > 0 ? width : 0.0f;
    it->hitboxHeight = height > 0 ? height : 0.0f;

    auto* rit = runtimeOf(id);
    if (rit != nullptr) {
        for (auto const& uuid : rit->shownPlayers) {
            if (auto* player = findPlayerByUuid(uuid)) {
                sendHitboxPacket(*player, rit->runtimeId, it->hitboxWidth, it->hitboxHeight);
            }
        }
    }
    return true;
}

void ItemDisplayManager::followTickLocked() {
    if (mFollowers.empty())
        return;
    auto level = ll::service::getLevel();
    if (!level)
        return;

    for (auto it = mFollowers.begin(); it != mFollowers.end();) {
        auto& fs = it->second;

        Player* target = nullptr;
        level->forEachPlayer([&](Player& p) {
            if (p.getRealName() == fs.playerName) {
                target = &p;
                return false;
            }
            return true;
        });
        auto* cit = configOf(it->first);
        auto* rit = runtimeOf(it->first);
        if (!target || cit == nullptr || rit == nullptr) {
            it = mFollowers.erase(it);
            continue;
        }

        auto&       cfg  = *cit;
        auto const& tpos = target->getPosition();
        float const nx   = tpos.x + fs.offX;
        float const ny   = tpos.y + fs.offY;
        float const nz   = tpos.z + fs.offZ;

        if (!fs.synced || target->getDimensionId() != DimensionType(cfg.dimension)) {

            cfg.dimension = ((int)target->getDimensionId());
            cfg.x         = nx;
            cfg.y         = ny;
            cfg.z         = nz;
            mDirtyIds.insert(it->first);
            fs.synced = true;
            ++it;
            continue;
        }

        cfg.x = nx;
        cfg.y = ny;
        cfg.z = nz;
        for (auto const& uuid : rit->shownPlayers) {
            if (auto* player = findPlayerByUuid(uuid)) {
                sendFollowMove(*player, rit->runtimeId, nx, ny, nz);
            }
        }
        ++it;
    }
}

void ItemDisplayManager::syncVisibilityLocked() {
    mEntries.forEachLocked([&](int64_t id, Entry&) {
        syncOneLocked(id);
    });
}

void ItemDisplayManager::syncOneLocked(int64_t id) {
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

bool ItemDisplayManager::spawnOneLocked(int64_t id, ItemDisplayConfig const& data, Runtime& rt, Player& player) {
    return spawnForPlayer(id, data, rt, player);
}

std::unordered_set<std::string> const* ItemDisplayManager::filterOf(int64_t id) const {
    auto const it = mVisibleFilter.find(id);
    return it == mVisibleFilter.end() ? nullptr : &it->second;
}

struct ItemDisplayTickHookAccess {
    static void sync(ItemDisplayManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.syncVisibilityLocked();
    }

    static void processDirty(ItemDisplayManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.processDirtyLocked();
    }

    static void followTick(ItemDisplayManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.followTickLocked();
    }

    static bool alive(ItemDisplayManager& mgr, std::uint64_t runtimeId, mce::UUID const& uuid) {
        std::lock_guard lock(mgr.mMutex);
        bool            aliveFlag = false;
        mgr.mEntries.forEachLocked([&](int64_t, ItemDisplayManager::Entry const& entry) {
            if (entry.runtime.runtimeId == runtimeId)
                aliveFlag = entry.runtime.shownPlayers.contains(uuid);
        });
        return aliveFlag;
    }
};

void itemDisplayTick() {
    if (hlibDomainDisabled("HLIB_NO_DISPLAY"))
        return;

    ItemDisplayManager::getInstance().tickEasterEggs();

    ItemDisplayTickHookAccess::processDirty(ItemDisplayManager::getInstance());

    ItemDisplayTickHookAccess::followTick(ItemDisplayManager::getInstance());

    auto const now = currentTick();
    auto&      q   = animQueue();
    auto       it  = q.begin();
    while (it != q.end() && it->first <= now) {
        auto& entry = it->second;
        if (auto* player = findPlayerByUuid(entry.playerUuid)) {

            if (entry.noAliveCheck || ItemDisplayTickHookAccess::alive(
                                          ItemDisplayManager::getInstance(),
                                          entry.runtimeId,
                                          entry.playerUuid)) {
                if (entry.equip) {

                    if (entry.equipStack)
                        sendEquipment(*player, entry.runtimeId, *entry.equipStack);
                } else {
                    proto::AnimateEntityPacket pkt;
                    pkt.animation                = entry.animation;
                    pkt.next_state               = "none";
                    pkt.stop_expression          = entry.stopExpression;
                    pkt.stop_expression_version  = static_cast<std::int32_t>(proto::MolangVersion::Initial);
                    pkt.controller               = entry.controller;
                    pkt.blend_out_time           = 0;
                    pkt.runtime_ids              = {proto::bp::ActorRuntimeID{entry.runtimeId}};
                    wire::send(*player, pkt);
                }
            }
        }
        it = q.erase(it);
    }

    static std::uint64_t lastSyncTick = 0;
    if (now - lastSyncTick >= 20) {
        lastSyncTick = now;
        ItemDisplayTickHookAccess::sync(ItemDisplayManager::getInstance());
    }
}

tick::AutoRegister gTickHookRegistrar{"itemdisplay", &itemDisplayTick, tick::Order::ItemDisplay};

} // namespace debugshape_export
