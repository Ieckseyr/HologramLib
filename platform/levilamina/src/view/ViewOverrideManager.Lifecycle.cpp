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

std::string ViewOverrideManager::describeFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    std::size_t     entities = mGlobal.entities.size();
    std::size_t     blocks   = mGlobal.blocks.size();
    if (auto const* table = tableOfLocked(playerName)) {
        entities += table->entities.size();
        blocks += table->blocks.size();
    }
    auto const counters = hookCounters();
    return std::format(
        "entities={} blocks={} hooks(calls={} spawn={} meta={} block={} chunk={}) sendFail={} ctrlSteps={} skinnedSpawn={}",
        entities,
        blocks,
        counters.calls,
        counters.spawn,
        counters.meta,
        counters.block,
        counters.chunk,
        protoSendFailureCount().load(std::memory_order_relaxed),
        mControlSteps.load(std::memory_order_relaxed),
        mSkinnedSpawnCounter.load(std::memory_order_relaxed));
}

void ViewOverrideManager::noteHookCall() {
    mHookCalls.fetch_add(1, std::memory_order_relaxed);
}

void ViewOverrideManager::onPlayerLeave(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto const      it = mPlayers.find(playerName);
    if (it == mPlayers.end())
        return;

    it->second.byRuntime.clear();
    for (auto& [uniqueId, record] : it->second.entities)
        record.runtimeKnown = false;
    mPulse.erase(playerName);
    mInputs.erase(playerName);
}

void ViewOverrideManager::shutdown() {
    std::lock_guard lock(mMutex);
    mPlayers.clear();
    mPulse.clear();
    mInputs.clear();
    mGlobal = Table{};
    refreshActiveLocked();
}

} // namespace debugshape_export::view
