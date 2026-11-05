#include "ViewOverrideManager.h"
#include "DiagLog.h"
#include "ViewOverrideRuntimeOps.h"
#include "ViewPackets.h"

#include <mc/world/actor/player/Player.h>

namespace debugshape_export::view {
using namespace vo_ops;

bool ViewOverrideManager::applyToOutbound(::Player& target, ::Packet const& packet, Applied& applied) {
    std::string const& playerName = target.getRealName();

    // 引擎调用一律先在锁外取好：锁内只改表、只取快照（CONTRIBUTING「锁内不调游戏 API」）
    std::int64_t selfUniqueId = 0;
    if (packet.getId() == ::MinecraftPacketIds::SetActorData || packet.getId() == ::MinecraftPacketIds::MovePlayer) {
        selfUniqueId = target.getOrCreateUniqueID().rawID;
    }
    std::int64_t addPlayerUniqueId = 0;
    if (packet.getId() == ::MinecraftPacketIds::AddPlayer) {
        auto const runtimeId =
            static_cast<std::uint64_t>(unwrap(payloadOf<::AddPlayerPacketPayload>(packet).mRuntimeId).rawID);
        auto* actor = actorByRuntimeId(runtimeId);
        if (actor == nullptr)
            return false;
        addPlayerUniqueId = actor->getOrCreateUniqueID().rawID;
    }
    int const targetDim = dimensionOfPlayer(target);

    std::unique_lock lock(mMutex);
    Table*           playerTable = nullptr;
    if (auto const it = mPlayers.find(playerName); it != mPlayers.end() && !it->second.empty()) {
        playerTable = &it->second;
    }
    if (playerTable == nullptr && mGlobal.empty())
        return false;

    // 反查索引要写进记录真正所在的那张表：记在全局表却写进玩家表，全局覆盖的四条包改写会全部查不到
    auto indexRuntimeLocked = [&](std::int64_t uniqueId, std::uint64_t runtimeId) {
        auto& owner = (playerTable != nullptr && playerTable->entities.contains(uniqueId)) ? *playerTable : mGlobal;
        owner.byRuntime[runtimeId] = uniqueId;
    };

    switch (static_cast<int>(packet.getId())) {
    case logic::kAddActor: {
        mSpawnSeen.fetch_add(1, std::memory_order_relaxed);
        auto& payload  = payloadOf<::AddActorPacketPayload>(packet);
        auto  uniqueId = unwrap(payload.mEntityId).rawID;
        if (markSpawnSeen(uniqueId)) {
            auto const& sp = unwrap(payload.mPos);
            pushSpawnEvent(
                "actor|" + std::to_string(uniqueId) + "|" + std::to_string(targetDim) + "|" + std::to_string(sp.x) + "|" + std::to_string(sp.y) + "|" + std::to_string(sp.z) + "|" + unwrap(payload.mActorType));
        }
        auto* record = findEntityLocked(playerName, uniqueId);
        if (record == nullptr)
            return false;
        record->runtimeKnown = true;
        record->runtimeId    = static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID);
        indexRuntimeLocked(uniqueId, record->runtimeId);

        applied.hit = true;
        if (record->view.hidden) {
            applied.drop = true;
            return true;
        }

        // 往下要么发包要么等 tab 移除，先取快照再解锁
        hologramlib::EntityView const view = record->view;
        std::uint64_t const           rtId = record->runtimeId;
        lock.unlock();

        if (!view.skinId.empty()) {
            auto const pos = unwrap(payload.mPos);
            bool const ok  = sendSkinnedSpawn(
                target,
                rtId,
                uniqueId,
                static_cast<float>(pos.x),
                static_cast<float>(pos.y),
                static_cast<float>(pos.z),
                static_cast<float>(unwrap(payload.mRot).y),
                view.skinId,
                view.hasNametag,
                view.nametag,
                view.scale,
                view.yOffset);
            if (ok) {
                scheduleTabRemoval(playerName, uniqueId);
                applied.drop = true;
            } else {

                HLIB_LOG_WARN(
                    "HologramLib: 换皮肤失败, 出生包按原样放行: entity={} skin={}",
                    uniqueId,
                    view.skinId);
            }
            return true;
        }
        if (!view.identifier.empty()) {

            sendActorSpawnRewritten(target, payload, view.identifier, planOf(view));
            applied.metaPush.push_back(uniqueId);
            applied.drop = true;
            return true;
        }
        if (view.hasNametag) {
            applied.nametagPush.emplace_back(uniqueId, rtId);
        }
        return true;
    }
    case logic::kAddItemActor: {

        mSpawnSeen.fetch_add(1, std::memory_order_relaxed);
        auto& payload  = payloadOf<::AddItemActorPacketPayload>(packet);
        auto  uniqueId = unwrap(payload.mId).rawID;
        if (markSpawnSeen(uniqueId)) {
            auto const& sp = unwrap(payload.mPos);
            pushSpawnEvent(
                "drop|" + std::to_string(uniqueId) + "|" + std::to_string(targetDim) + "|" + std::to_string(sp.x) + "|" + std::to_string(sp.y) + "|" + std::to_string(sp.z) + "|");
        }
        auto* record = findEntityLocked(playerName, uniqueId);
        if (record == nullptr)
            return false;
        record->runtimeKnown = true;
        record->runtimeId    = static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID);
        indexRuntimeLocked(uniqueId, record->runtimeId);

        applied.hit = true;

        hologramlib::EntityView const view = record->view;
        lock.unlock();
        switch (logic::planItemSpawn(view.hidden, !view.identifier.empty())) {
        case logic::ItemSpawnPlan::Drop:
            applied.drop = true;
            return true;
        case logic::ItemSpawnPlan::Rewrite:
            view::sendItemSpawnRewritten(target, payload, view.identifier, planOf(view));
            applied.metaPush.push_back(uniqueId);
            applied.drop = true;
            return true;
        case logic::ItemSpawnPlan::Pass:
        default:
            return true;
        }
    }
    case logic::kSetActorMotion: {

        auto const runtimeId =
            static_cast<std::uint64_t>(unwrap(payloadOf<::SetActorMotionPacketPayload>(packet).mRuntimeId).rawID);
        auto* record = findEntityByRuntimeLocked(playerName, runtimeId);
        if (record == nullptr)
            return false;
        std::int64_t const uniqueId = record->uniqueId;
        bool const         skinned  = !record->view.skinId.empty();
        lock.unlock();
        if (!skinned || !runtimeBelongsTo(runtimeId, uniqueId))
            return false;
        applied.hit  = true;
        applied.drop = true;
        return true;
    }
    case logic::kMoveAbsoluteActor:
    case logic::kMoveDeltaActor: {

        std::uint64_t runtimeId = 0;
        if (packet.getId() == ::MinecraftPacketIds::MoveAbsoluteActor) {
            runtimeId = static_cast<std::uint64_t>(
                unwrap(unwrap(payloadOf<::MoveActorAbsolutePacketPayload>(packet).mMoveData).mRuntimeId).rawID);
        } else {
            runtimeId =
                static_cast<std::uint64_t>(unwrap(unwrap(payloadOf<::MoveActorDeltaPacketPayload>(packet).mMoveData).mRuntimeId).rawID);
        }
        auto* record = findEntityByRuntimeLocked(playerName, runtimeId);
        if (record == nullptr)
            return false;
        std::int64_t const uniqueId = record->uniqueId;
        bool const         skinned  = !record->view.skinId.empty();
        lock.unlock();
        if (!skinned || !runtimeBelongsTo(runtimeId, uniqueId))
            return false;
        applied.hit  = true;
        applied.drop = true;
        return true;
    }
    case logic::kAddPlayer: {
        mSpawnSeen.fetch_add(1, std::memory_order_relaxed);
        auto& payload   = payloadOf<::AddPlayerPacketPayload>(packet);
        auto  runtimeId = static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID);
        auto* record    = findEntityLocked(playerName, addPlayerUniqueId);
        if (record == nullptr)
            return false;
        record->runtimeKnown = true;
        record->runtimeId    = runtimeId;
        record->playerName   = unwrap(payload.mName);
        indexRuntimeLocked(addPlayerUniqueId, runtimeId);

        applied.hit = true;
        if (record->view.hidden) {
            applied.drop = true;
            return true;
        }

        hologramlib::EntityView const view       = record->view;
        bool const                    substitute = !view.identifier.empty() && view.identifier != "minecraft:player";
        InputSnapshot                 snap{};
        bool const                    haveSnap = substitute && inputSnapshotOfLocked(addPlayerUniqueId, snap);
        // 拿不到输入快照就拼不出替身包：此时放行原包，否则这个玩家在对方视野里彻底消失
        bool const willRewrite = substitute && haveSnap;
        if (willRewrite)
            record->substituted = true;
        lock.unlock();

        if (willRewrite) {
            sendActorSpawnFromInput(
                target,
                runtimeId,
                addPlayerUniqueId,
                view.identifier,
                snap.x,
                snap.y,
                snap.z,
                snap.pitch,
                snap.yaw,
                snap.headYaw,
                planOf(view));
            applied.drop = true;
            return true;
        }
        if (view.hasNametag)
            applied.nametagPush.emplace_back(addPlayerUniqueId, runtimeId);
        return true;
    }
    case logic::kSetActorData: {
        mMetaSeen.fetch_add(1, std::memory_order_relaxed);
        auto& payload   = payloadOf<::SetActorDataPacketPayload>(packet);
        auto  runtimeId = static_cast<std::uint64_t>(unwrap(payload.mId).rawID);
        auto* record    = findEntityByRuntimeLocked(playerName, runtimeId);
        if (record == nullptr)
            return false;

        std::int64_t const uniqueId   = record->uniqueId;
        bool const         hidden     = record->view.hidden;
        bool const         hasNametag = record->view.hasNametag;
        lock.unlock();

        if (!runtimeBelongsTo(runtimeId, uniqueId))
            return false;
        if (uniqueId == selfUniqueId)
            return false;

        applied.hit = true;
        if (hidden) {
            applied.drop = true;
            return true;
        }

        if (hasNametag)
            applied.nametagPush.emplace_back(uniqueId, runtimeId);
        return true;
    }
    case logic::kUpdateBlock:
    case logic::kUpdateBlockSynced: {
        mBlockSeen.fetch_add(1, std::memory_order_relaxed);
        bool const synced  = packet.getId() == ::MinecraftPacketIds::UpdateBlockSynced;
        auto const pos     = synced ? unwrap(payloadOf<::UpdateBlockSyncedPacketPayload>(packet).mPos)
                                    : unwrap(payloadOf<::UpdateBlockPacketPayload>(packet).mPos);
        auto const current = synced
                                 ? static_cast<std::uint32_t>(unwrap(payloadOf<::UpdateBlockSyncedPacketPayload>(packet).mRuntimeId))
                                 : static_cast<std::uint32_t>(unwrap(payloadOf<::UpdateBlockPacketPayload>(packet).mRuntimeId));

        auto* record = findBlockLocked(playerName, logic::packPos(pos.x, pos.y, pos.z));
        if (record == nullptr)
            return false;
        if (!logic::dimensionMatches(record->dim, targetDim))
            return false;
        if (!logic::needsRewrite(current, record->viewId))
            return false;

        std::uint32_t const viewId = record->viewId;
        lock.unlock();
        sendBlockUpdate(target, pos.x, pos.y, pos.z, viewId);
        applied.hit  = true;
        applied.drop = true;
        return true;
    }
    case logic::kMovePlayer: {
        auto& payload   = payloadOf<::MovePlayerPacketPayload>(packet);
        auto  runtimeId = static_cast<std::uint64_t>(unwrap(payload.mPlayerID).rawID);
        auto* record    = findEntityByRuntimeLocked(playerName, runtimeId);
        if (record == nullptr)
            return false;

        std::int64_t const uniqueId    = record->uniqueId;
        bool const         substituted = record->substituted;
        lock.unlock();

        if (!substituted || !runtimeBelongsTo(runtimeId, uniqueId))
            return false;
        if (uniqueId == selfUniqueId)
            return false;

        applied.hit  = true;
        applied.drop = true;
        return true;
    }
    case logic::kLevelChunk: {
        mChunkSeen.fetch_add(1, std::memory_order_relaxed);
        auto&      payload = payloadOf<::LevelChunkPacketPayload>(packet);
        auto const pos     = unwrap(payload.mPos);
        auto const dim     = static_cast<int>(unwrap(payload.mDimensionId));
        auto       collect = [&](Table const& table) {
            for (auto const& [key, record] : table.blocks) {
                if (!logic::inChunk(key, pos.x, pos.z))
                    continue;
                if (!logic::dimensionMatches(record.dim, dim))
                    continue;
                applied.repush.emplace_back(key, record.viewId);
            }
        };
        if (playerTable != nullptr)
            collect(*playerTable);
        collect(mGlobal);
        if (applied.repush.empty())
            return false;
        applied.hit = true;
        return true;
    }
    default:
        return false;
    }
}

void ViewOverrideManager::runPostActions(::Player& target, Applied const& applied) {

    for (auto const& [uniqueId, runtimeId] : applied.nametagPush) {
        std::string nametag;
        bool        alwaysShow = false;
        {
            std::lock_guard lock(mMutex);
            auto const      lookup = [&](Table const& t) -> EntityRecord const* {
                auto const e = t.entities.find(uniqueId);
                return e == t.entities.end() ? nullptr : &e->second;
            };
            auto const* record = lookup(mGlobal);
            if (auto const it = mPlayers.find(target.getRealName()); it != mPlayers.end()) {
                if (auto const* local = lookup(it->second); local != nullptr)
                    record = local;
            }
            if (record == nullptr)
                continue;
            nametag    = record->view.nametag;
            alwaysShow = record->view.nametagAlwaysShow;
        }
        view::sendViewMeta(target, runtimeId, logic::metaPlanOf(true, nametag, alwaysShow));
    }

    for (auto const& [key, viewId] : applied.repush) {
        sendBlockUpdate(target, logic::unpackX(key), logic::unpackY(key), logic::unpackZ(key), viewId);
    }

    for (auto const uniqueId : applied.metaPush) {
        scheduleMetaPush(target.getRealName(), uniqueId);
    }
}

void ViewOverrideManager::pulseForPlayer(::Player& player) {

    constexpr float kVisibleRange = 40.0f;

    std::string const playerName = player.getRealName();

    constexpr std::uint32_t kJoinGraceTicks = 40;
    {
        std::lock_guard lock(mMutex);
        auto&           state = mPulse[playerName];
        if (state.ticks < kJoinGraceTicks) {
            ++state.ticks;
            return;
        }
    }

    std::vector<std::pair<std::int64_t, EntityRecord>>  entities;
    std::vector<std::pair<std::int64_t, std::uint32_t>> repushBlocks;
    // 取脚位与维度都是引擎调用，放锁外
    auto const pos = player.getFeetBlockPos();
    auto const dim = dimensionOfPlayer(player);
    {
        std::lock_guard lock(mMutex);
        auto&           state = mPulse[playerName];
        bool const      moved = !state.hasChunk || state.chunkX != (pos.x >> 4) || state.chunkZ != (pos.z >> 4);
        state.hasChunk        = true;
        state.chunkX          = pos.x >> 4;
        state.chunkZ          = pos.z >> 4;

        auto collect = [&](Table const& table) {
            for (auto const& [uniqueId, record] : table.entities)
                entities.emplace_back(uniqueId, record);
            if (!moved)
                return;

            for (auto const& [key, record] : table.blocks) {
                if (logic::dimensionMatches(record.dim, dim))
                    repushBlocks.emplace_back(key, record.viewId);
            }
        };
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end())
            collect(it->second);
        collect(mGlobal);
    }

    for (auto const& [uniqueId, record] : entities) {
        auto*      actor      = liveActorNear(player, uniqueId, kVisibleRange);
        bool const nowVisible = actor != nullptr;
        bool       wasVisible = false;
        {
            std::lock_guard lock(mMutex);
            auto&           state   = mPulse[playerName];
            auto const      it      = state.visible.find(uniqueId);
            wasVisible              = it != state.visible.end() && it->second;
            state.visible[uniqueId] = nowVisible;
        }
        if (!nowVisible || actor == nullptr)
            continue;

        if (record.view.skinId.empty())
            continue;

        auto const  pos = actor->getPosition();
        auto const  rot = actor->getRotation();
        float const yaw = rot.y;

        if (!wasVisible) {
            std::string const skinId = record.view.skinId;
            if (NpcSkinRegistry::getInstance().hasSkin(skinId)) {
                sendRemoveActor(player, uniqueId);
                sendSkinnedSpawn(
                    player,
                    static_cast<std::uint64_t>(actor->getRuntimeID()),
                    uniqueId,
                    pos.x,
                    pos.y,
                    pos.z,
                    yaw,
                    skinId,
                    record.view.hasNametag,
                    record.view.nametag,
                    record.view.scale,
                    record.view.yOffset);
            }
        }

        npc_protocol::move(
            player,
            static_cast<std::uint64_t>(actor->getRuntimeID()),
            Vec3{pos.x, pos.y + record.view.yOffset, pos.z},
            yaw);
    }

    for (auto const& [key, viewId] : repushBlocks) {
        sendBlockUpdate(player, logic::unpackX(key), logic::unpackY(key), logic::unpackZ(key), viewId);
    }
}

} // namespace debugshape_export::view
