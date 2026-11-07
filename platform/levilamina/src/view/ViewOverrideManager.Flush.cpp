#include "ViewOverrideRuntimeOps.h"
#include "core/DiagFlags.h"

#include "ViewOverrideManager.h"

#include "ViewPackets.h"

#include "customentity/CustomEntityManager.h"

#include "DiagLog.h"

#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/network/MinecraftPacketIds.h>
#include <mc/network/Packet.h>
#include <mc/network/packet/AddActorPacketPayload.h>
#include <mc/network/packet/AddItemActorPacketPayload.h>
#include <mc/network/packet/AddPlayerPacketPayload.h>
#include <mc/network/packet/LevelChunkPacketPayload.h>
#include <mc/network/packet/MoveActorAbsoluteData.h>
#include <mc/network/packet/MoveActorAbsolutePacketPayload.h>
#include <mc/network/packet/MoveActorDeltaData.h>
#include <mc/network/packet/MoveActorDeltaPacketPayload.h>
#include <mc/network/packet/MovePlayerPacketPayload.h>
#include <mc/network/packet/SetActorDataPacketPayload.h>
#include <mc/network/packet/SetActorMotionPacketPayload.h>
#include <mc/network/packet/UpdateBlockPacketPayload.h>
#include <mc/network/packet/UpdateBlockSyncedPacketPayload.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/ActorDataIDs.h>
#include <mc/world/actor/ActorFlags.h>
#include <mc/world/actor/ActorType.h>
#include <mc/world/actor/player/Inventory.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/block/Block.h>

#include <cmath>
#include <format>
#include <optional>
#include <utility>

namespace debugshape_export::view {

void ViewOverrideManager::pushSubstitutedMovement(::Player& player, InputSnapshot const& snapshot) {
    if (!mActive.load(std::memory_order_relaxed))
        return;
    auto level = ::ll::service::getLevel();
    if (level == nullptr)
        return;

    bool                     globalSub = false;
    std::vector<std::string> perViewerNames;
    {
        std::lock_guard lock(mMutex);
        if (auto const it = mGlobal.entities.find(snapshot.uniqueId); it != mGlobal.entities.end()) {
            globalSub = it->second.substituted;
        }
        for (auto const& [name, table] : mPlayers) {
            auto const it = table.entities.find(snapshot.uniqueId);
            if (it != table.entities.end() && it->second.substituted)
                perViewerNames.push_back(name);
        }
    }
    if (!globalSub && perViewerNames.empty())
        return;

    float const feetY = vo_ops::feetAnchorYFromInput(player, snapshot.y);

    auto push = [&](::Player& viewer) {
        sendMoveActorAbsolute(
            viewer,
            snapshot.runtimeId,
            snapshot.x,
            feetY,
            snapshot.z,
            snapshot.pitch,
            snapshot.yaw,
            snapshot.headYaw);
    };
    if (globalSub) {
        level->forEachPlayer([&](::Player& p) -> bool {
            if (&p != &player && p.distanceTo(player) <= 64.0f)
                push(p);
            return true;
        });
    } else {
        for (auto const& name : perViewerNames) {
            auto* p = level->getPlayer(name);
            if (p != nullptr && p != &player && p->distanceTo(player) <= 64.0f)
                push(*p);
        }
    }
}

void ViewOverrideManager::scheduleTabRemoval(std::string const& playerName, std::int64_t uniqueId) {
    std::lock_guard lock(mMutex);
    for (auto const& [due, entry] : mPendingTabRemoval) {
        if (entry.first == playerName && entry.second == uniqueId)
            return;
    }
    if (mPendingTabRemoval.size() > 512)
        mPendingTabRemoval.erase(mPendingTabRemoval.begin());
    mPendingTabRemoval.emplace_back(mServerTick + 20, std::make_pair(playerName, uniqueId));
}

void ViewOverrideManager::flushTabRemovals() {
    std::vector<std::pair<std::string, std::int64_t>> due;
    {
        std::lock_guard lock(mMutex);
        if (mPendingTabRemoval.empty())
            return;
        for (auto it = mPendingTabRemoval.begin(); it != mPendingTabRemoval.end();) {
            if (it->first <= mServerTick) {
                due.push_back(it->second);
                it = mPendingTabRemoval.erase(it);
            } else {
                ++it;
            }
        }
    }
    if (due.empty())
        return;
    auto level = ::ll::service::getLevel();
    if (!level)
        return;
    for (auto const& [name, uniqueId] : due) {
        if (auto* player = level->getPlayer(name)) {
            npc_protocol::removePlayerList(*player, uniqueId);
        }
    }
}

void ViewOverrideManager::scheduleMetaPush(std::string const& playerName, std::int64_t uniqueId) {
    std::lock_guard lock(mMutex);
    auto const*     record = findEntityLocked(playerName, uniqueId);
    if (record == nullptr || planOf(record->view).empty())
        return;
    for (auto const& [due, name, id] : mPendingMetaPush) {
        if (name == playerName && id == uniqueId)
            return;
    }
    if (mPendingMetaPush.size() > 512)
        mPendingMetaPush.erase(mPendingMetaPush.begin());
    mPendingMetaPush.emplace_back(mServerTick + 2, playerName, uniqueId);
}

void ViewOverrideManager::flushMetaPushes() {
    std::vector<std::pair<std::string, std::int64_t>> due;
    {
        std::lock_guard lock(mMutex);
        if (mPendingMetaPush.empty())
            return;
        for (auto it = mPendingMetaPush.begin(); it != mPendingMetaPush.end();) {
            if (std::get<0>(*it) <= mServerTick) {
                due.emplace_back(std::get<1>(*it), std::get<2>(*it));
                it = mPendingMetaPush.erase(it);
            } else {
                ++it;
            }
        }
    }
    if (due.empty())
        return;
    auto level = ::ll::service::getLevel();
    if (!level)
        return;
    for (auto const& [name, uniqueId] : due) {
        auto* player = level->getPlayer(name);
        if (player == nullptr)
            continue;

        std::optional<logic::MetaPlan> plan;
        {
            std::lock_guard lock(mMutex);
            if (auto const* record = findEntityLocked(name, uniqueId); record != nullptr) {
                plan = planOf(record->view);
            }
        }
        if (!plan || plan->empty())
            continue;
        auto* actor = vo_ops::liveActorNear(*player, uniqueId);
        if (actor == nullptr)
            continue;
        view::sendViewMeta(*player, static_cast<std::uint64_t>(actor->getRuntimeID()), *plan);
    }
}

void ViewOverrideManager::tickPulse() {
    ++mServerTick;
    view::viewTickRef() = mServerTick;
    flushTabRemovals();
    flushMetaPushes();
    if (!mActive.load(std::memory_order_relaxed))
        return;
    auto level = ::ll::service::getLevel();
    if (!level)
        return;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (hasOverridesFor(player.getRealName()))
            pulseForPlayer(player);
        return true;
    });
}

} // namespace debugshape_export::view
