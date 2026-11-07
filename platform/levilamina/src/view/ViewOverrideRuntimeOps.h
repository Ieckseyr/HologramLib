#pragma once

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

bool markSpawnSeen(std::int64_t uniqueId);

namespace vo_ops {

template <typename PayloadT>
PayloadT const& payloadOf(::Packet const& packet) {
    return static_cast<PayloadT const&>(static_cast<::ll::PayloadPacket<PayloadT> const&>(packet));
}

inline std::optional<std::uint32_t> networkIdOfBlockType(std::string const& type) {
    auto block = ::Block::tryGetFromRegistry(::HashedString{type});
    if (!block)
        return std::nullopt;
    return static_cast<std::uint32_t>(unwrap(block->mNetworkId));
}

inline std::uint32_t realBlockNetworkIdAt(::Player& player, int x, int y, int z) {
    auto const& block = player.getDimensionBlockSource().getBlock(::BlockPos{x, y, z});
    return static_cast<std::uint32_t>(unwrap(block.mNetworkId));
}

template <typename Fn>
inline void forEachTargetPlayer(std::string const& playerName, Fn&& fn) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (playerName.empty() || player.getRealName() == playerName)
            fn(player);
        return true;
    });
}

inline int dimensionOfPlayer(::Player& player) {
    return static_cast<int>(player.getDimensionId());
}

inline int dimensionOfTarget(std::string const& playerName) {
    if (playerName.empty())
        return logic::kAnyDimension;
    int dim = logic::kAnyDimension;
    forEachTargetPlayer(playerName, [&](::Player& player) {
        dim = dimensionOfPlayer(player);
    });
    return dim;
}

inline ::Actor* actorByRuntimeId(std::uint64_t runtimeId) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return nullptr;
    return level->getRuntimeEntity(::ActorRuntimeID{runtimeId}, false);
}

// 引擎侧校验：这个 runtimeId 现在是否仍属于 uniqueId。查实体是引擎调用，只能在锁外做。
inline bool runtimeBelongsTo(std::uint64_t runtimeId, std::int64_t uniqueId) {
    auto* actor = actorByRuntimeId(runtimeId);
    return actor != nullptr && actor->getOrCreateUniqueID().rawID == uniqueId;
}

constexpr float kProxyTrim = 0.0f;

inline float feetAnchorY(::Actor const& actor) {
    return actor.getAABB().min.y + kProxyTrim;
}

inline ::Player* playerByUniqueId(std::int64_t uniqueId) {
    ::Player* found = nullptr;
    forEachTargetPlayer({}, [&](::Player& p) {
        if (p.getOrCreateUniqueID().rawID == uniqueId)
            found = &p;
    });
    return found;
}

template <typename Fn>
inline void forEachOverrideViewer(std::string const& playerName, std::int64_t uniqueId, Fn&& fn) {
    ::Player* self = playerByUniqueId(uniqueId);
    forEachTargetPlayer(playerName, [&](::Player& player) {
        if (self != nullptr && &player == self)
            return;
        fn(player);
    });
}

template <typename Fn>
inline void forEachRestoreViewer(std::string const& playerName, std::int64_t uniqueId, bool includeSelf, Fn&& fn) {
    ::Player* self = playerByUniqueId(uniqueId);
    forEachTargetPlayer(playerName, [&](::Player& player) {
        if (self != nullptr && &player == self && !includeSelf)
            return;
        fn(player);
    });
}

inline float feetAnchorYFromInput(::Actor const& actor, float inputY) {
    float const eye  = actor.getPosition().y;
    float const feet = actor.getAABB().min.y;
    return (eye - feet > 0.01f) ? inputY - (eye - feet) : inputY;
}

inline ::Actor* liveActorNear(::Player& player, std::int64_t uniqueId, float maxDistance = 64.0f) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return nullptr;
    auto* actor = level->fetchEntity(::ActorUniqueID{uniqueId}, false);
    if (actor == nullptr)
        return nullptr;
    return player.distanceTo(*actor) <= maxDistance ? actor : nullptr;
}

using view::planOf;

inline logic::MetaPlan const& noPlan() {
    static logic::MetaPlan const empty{};
    return empty;
}

} // namespace vo_ops
} // namespace debugshape_export::view
