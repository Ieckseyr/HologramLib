#include "../DiagFlags.h"
#include "ViewOverrideRuntimeOps.h"

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
#include <sculk/protocol/codec/packet/ActorEventPacket.hpp>
#include <sculk/protocol/codec/packet/AnimatePacket.hpp>

#include <cmath>
#include <format>
#include <optional>
#include <utility>

namespace debugshape_export::view {
using namespace vo_ops;

ViewOverrideManager& ViewOverrideManager::getInstance() {
    static ViewOverrideManager instance;
    return instance;
}

ViewOverrideManager::Table& ViewOverrideManager::ensureTableLocked(std::string const& playerName) {
    if (playerName.empty())
        return mGlobal;
    return mPlayers[playerName];
}

ViewOverrideManager::Table const* ViewOverrideManager::tableOfLocked(std::string const& playerName) const {
    if (playerName.empty())
        return &mGlobal;
    auto const it = mPlayers.find(playerName);
    return it == mPlayers.end() ? nullptr : &it->second;
}

ViewOverrideManager::EntityRecord*
ViewOverrideManager::findEntityLocked(std::string const& playerName, std::int64_t uniqueId) {
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto const e = it->second.entities.find(uniqueId); e != it->second.entities.end())
                return &e->second;
        }
    }
    if (auto const e = mGlobal.entities.find(uniqueId); e != mGlobal.entities.end())
        return &e->second;
    return nullptr;
}

ViewOverrideManager::EntityRecord*
ViewOverrideManager::findEntityByRuntimeLocked(std::string const& playerName, std::uint64_t runtimeId) {
    // 只查表：引擎侧确认（runtimeId 是否仍属于这个 uniqueId）由调用方解锁后做，见 vo_ops::runtimeBelongsTo
    auto lookup = [&](Table& table) -> EntityRecord* {
        auto const idx = table.byRuntime.find(runtimeId);
        if (idx == table.byRuntime.end())
            return nullptr;
        auto const e = table.entities.find(idx->second);
        if (e == table.entities.end())
            return nullptr;
        return &e->second;
    };
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto* rec = lookup(it->second))
                return rec;
        }
    }
    return lookup(mGlobal);
}

ViewOverrideManager::BlockRecord* ViewOverrideManager::findBlockLocked(std::string const& playerName, std::int64_t posKey) {
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto const b = it->second.blocks.find(posKey); b != it->second.blocks.end())
                return &b->second;
        }
    }
    if (auto const b = mGlobal.blocks.find(posKey); b != mGlobal.blocks.end())
        return &b->second;
    return nullptr;
}

void ViewOverrideManager::refreshActiveLocked() {

    bool any = !mGlobal.empty();
    if (!any) {
        for (auto const& [name, table] : mPlayers) {
            if (!table.empty()) {
                any = true;
                break;
            }
        }
    }
    mActive.store(any, std::memory_order_relaxed);
}

bool ViewOverrideManager::hasOverridesFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    if (!mGlobal.empty())
        return true;
    auto const it = mPlayers.find(playerName);
    return it != mPlayers.end() && !it->second.empty();
}

bool ViewOverrideManager::overrideEntity(
    std::string const&             playerName,
    std::int64_t                   uniqueId,
    hologramlib::EntityView const& view) {
    if (hlibDomainDisabled("HLIB_NO_VIEW"))
        return false;
    auto normalised       = view;
    normalised.identifier = logic::normaliseType(view.identifier);

    normalised.skinId = logic::trimText(view.skinId);
    if (!logic::viewHasEffect(
            !normalised.identifier.empty(),
            !normalised.asPlayer.empty(),
            !normalised.skinId.empty(),
            normalised.hidden,
            normalised.hasNametag,
            normalised.hasPose,
            normalised.hasMetaScale,
            normalised.hasVariant,
            normalised.hasMarkVariant,
            normalised.hasColorIndex,
            normalised.hasColor2)) {
        return clearEntity(playerName, uniqueId);
    }

    EntityRecord record;
    bool         pushNametagNow = false;
    bool         needRuntimeId  = false;
    {
        std::lock_guard lock(mMutex);
        auto&           table = ensureTableLocked(playerName);
        auto&           rec   = table.entities[uniqueId];
        rec.uniqueId          = uniqueId;

        // 合并而不是整体覆盖：只改 pose 时不能把 identifier/skin/nametag 一起清掉（R06）
        auto merged = rec.view;
        if (!normalised.identifier.empty()) {
            merged.identifier = normalised.identifier;
        }
        if (!normalised.asPlayer.empty()) {
            merged.asPlayer = normalised.asPlayer;
        }
        if (!normalised.skinId.empty()) {
            merged.skinId = normalised.skinId;
        }
        if (normalised.hidden) {
            merged.hidden = true;
        }
        if (normalised.scale != 0.0f) {
            merged.scale = normalised.scale;
        }
        // 少了这条，换皮出生包第一次用 normalised.yOffset，之后每次重生都退回 0（模型陷进地里）
        if (normalised.yOffset != 0.0f) {
            merged.yOffset = normalised.yOffset;
        }
        if (normalised.hasNametag) {
            merged.hasNametag        = true;
            merged.nametag           = normalised.nametag;
            merged.nametagAlwaysShow = normalised.nametagAlwaysShow;
        }
        if (normalised.hasPose) {
            merged.hasPose = true;
            merged.pose    = normalised.pose;
        }
        if (normalised.hasMetaScale) {
            merged.hasMetaScale = true;
            merged.metaScale    = normalised.metaScale;
        }
        if (normalised.hasVariant) {
            merged.hasVariant = true;
            merged.variant    = normalised.variant;
        }
        if (normalised.hasMarkVariant) {
            merged.hasMarkVariant = true;
            merged.markVariant    = normalised.markVariant;
        }
        if (normalised.hasColorIndex) {
            merged.hasColorIndex = true;
            merged.colorIndex    = normalised.colorIndex;
        }
        if (normalised.hasColor2) {
            merged.hasColor2 = true;
            merged.color2    = normalised.color2;
        }
        rec.view = merged;
        if (rec.runtimeKnown) {
            table.byRuntime[rec.runtimeId] = uniqueId;
            pushNametagNow                 = !merged.hidden && normalised.hasNametag;
        }
        needRuntimeId = !rec.runtimeKnown;
        record        = rec;
        refreshActiveLocked();
    }

    if (needRuntimeId) {
        if (auto level = ::ll::service::getLevel(); level != nullptr) {
            if (auto* actor = level->fetchEntity(::ActorUniqueID{uniqueId}, false); actor != nullptr) {
                auto const      runtimeId = static_cast<std::uint64_t>(actor->getRuntimeID());
                std::lock_guard lock(mMutex);
                auto&           table      = ensureTableLocked(playerName);
                auto&           rec        = table.entities[uniqueId];
                rec.runtimeKnown           = true;
                rec.runtimeId              = runtimeId;
                table.byRuntime[runtimeId] = uniqueId;
                record                     = rec;
                pushNametagNow             = !record.view.hidden && normalised.hasNametag;
            }
        }
    }

    // 本次调用是否动了「出生形态」：只改 pose / nametag 时不重发，否则脚本每帧调一次 setPose 就会每帧刷一遍皮肤包
    bool const touchedShape = !normalised.asPlayer.empty() || !normalised.identifier.empty() || !normalised.skinId.empty();
    bool const wantHidden   = normalised.hidden;
    // 下面按「合并后的最终状态」发包：只改一个字段时不能把 scale/identifier 一起带丢
    normalised = record.view;
    if (wantHidden && touchedShape) {
        forEachOverrideViewer(playerName, uniqueId, [&](::Player& player) {
            sendRemoveActor(player, uniqueId);
        });

    } else if (touchedShape && (!normalised.asPlayer.empty() || !normalised.identifier.empty() || !normalised.skinId.empty())) {
        forEachTargetPlayer(playerName, [&](::Player& player) {
            auto* actor = liveActorNear(player, uniqueId);

            if (actor != nullptr && actor->isPlayer()) {
                auto& target = *static_cast<::Player*>(actor);
                if (!normalised.asPlayer.empty()) {

                    if (!sendPlayerSkin(player, target, normalised.asPlayer)) {
                        HLIB_LOG_WARN("HologramLib: 换皮肤失败（源玩家不在线且无留存快照?）: {}", normalised.asPlayer);
                    }
                    return;
                }
                if (&target == &player)
                    return;
                if (normalised.identifier != "minecraft:player") {

                    {
                        std::lock_guard lock(mMutex);
                        auto&           table = ensureTableLocked(playerName);
                        auto&           rec   = table.entities[uniqueId];
                        rec.substituted       = true;
                    }
                    sendRemoveActor(player, uniqueId);

                    InputSnapshot snap{};
                    if (inputSnapshotOf(uniqueId, snap)) {
                        sendActorSpawnFromInput(
                            player,
                            snap.runtimeId,
                            uniqueId,
                            normalised.identifier,
                            snap.x,
                            snap.y,
                            snap.z,
                            snap.pitch,
                            snap.yaw,
                            snap.headYaw,
                            planOf(normalised));
                    }
                    return;
                }
                if (normalised.hasNametag && record.runtimeKnown) {
                    view::sendViewMeta(player, record.runtimeId, logic::metaPlanOf(true, normalised.nametag, normalised.nametagAlwaysShow));
                }
                return;
            }

            if (!normalised.skinId.empty()) {
                if (actor != nullptr) {
                    auto const pos  = actor->getPosition();
                    auto const rot  = actor->getRotation();
                    auto const rtId = record.runtimeKnown ? record.runtimeId
                                                          : static_cast<std::uint64_t>(actor->getRuntimeID());
                    sendRemoveActor(player, uniqueId);
                    if (sendSkinnedSpawn(
                            player,
                            rtId,
                            uniqueId,
                            pos.x,
                            pos.y,
                            pos.z,
                            rot.y,
                            normalised.skinId,
                            normalised.hasNametag,
                            normalised.nametag,
                            normalised.scale,
                            normalised.yOffset)) {
                        scheduleTabRemoval(player.getRealName(), uniqueId);
                    } else {
                        HLIB_LOG_WARN(
                            "HologramLib: 换皮肤失败（皮肤没注册?）: entity={} skin={}",
                            uniqueId,
                            normalised.skinId);

                        respawnForPlayer(player, *actor, actor->getTypeName(), logic::metaPlanOf(false, {}, false));
                    }
                }
                return;
            }
            if (normalised.hidden) {
                sendRemoveActor(player, uniqueId);
            } else if (!normalised.identifier.empty()) {
                if (actor != nullptr) {
                    if (actor->isType(::ActorType::ItemEntity)) {

                        view::sendItemActorAsOther(player, *actor, normalised.identifier, planOf(normalised));
                        scheduleMetaPush(player.getRealName(), uniqueId);
                    } else {
                        respawnForPlayer(player, *actor, normalised.identifier, planOf(normalised));
                    }
                } else if (normalised.hasNametag && record.runtimeKnown) {
                    view::sendViewMeta(player, record.runtimeId, logic::metaPlanOf(true, normalised.nametag, normalised.nametagAlwaysShow));
                }
            } else if (normalised.hasNametag && record.runtimeKnown) {
                view::sendViewMeta(player, record.runtimeId, logic::metaPlanOf(true, normalised.nametag, normalised.nametagAlwaysShow));
            }
        });
    } else if (pushNametagNow) {
        forEachOverrideViewer(playerName, uniqueId, [&](::Player& player) {
            view::sendViewMeta(player, record.runtimeId, logic::metaPlanOf(true, normalised.nametag, normalised.nametagAlwaysShow));
        });
    }
    return true;
}

bool ViewOverrideManager::overrideBlock(
    std::string const&            playerName,
    int                           x,
    int                           y,
    int                           z,
    hologramlib::BlockView const& view) {
    if (hlibDomainDisabled("HLIB_NO_VIEW"))
        return false;
    auto const type = logic::normaliseType(view.type);
    if (type.empty())
        return clearBlock(playerName, x, y, z);

    auto const networkId = networkIdOfBlockType(type);
    if (!networkId) {
        HLIB_LOG_WARN("HologramLib: 方块覆盖失败, 类型名不在注册表: {}", type);
        return false;
    }

    auto const key = logic::packPos(x, y, z);
    auto const dim = dimensionOfTarget(playerName);
    {
        std::lock_guard lock(mMutex);
        auto&           table = ensureTableLocked(playerName);
        auto&           rec   = table.blocks[key];
        rec.view              = hologramlib::BlockView{type};
        rec.viewId            = *networkId;
        rec.dim               = dim;
        refreshActiveLocked();
    }

    forEachTargetPlayer(playerName, [&](::Player& player) {
        sendBlockUpdate(player, x, y, z, *networkId);
    });
    return true;
}

bool ViewOverrideManager::clearEntity(std::string const& playerName, std::int64_t uniqueId) {
    EntityRecord record;
    bool         existed = false;
    {
        std::lock_guard lock(mMutex);
        auto            erase = [&](Table& table) {
            auto const it = table.entities.find(uniqueId);
            if (it == table.entities.end())
                return;
            if (it->second.runtimeKnown)
                table.byRuntime.erase(it->second.runtimeId);
            record = it->second;
            table.entities.erase(it);
            existed = true;
        };
        if (!playerName.empty()) {
            if (auto const it = mPlayers.find(playerName); it != mPlayers.end())
                erase(it->second);
        }
        erase(mGlobal);
        refreshActiveLocked();
    }

    if (existed) {
        forEachRestoreViewer(playerName, uniqueId, !record.view.asPlayer.empty(), [&](::Player& player) {
            restoreEntityForPlayer(player, uniqueId, record);
        });
    }
    return existed;
}

bool ViewOverrideManager::clearBlock(std::string const& playerName, int x, int y, int z) {
    auto const key     = logic::packPos(x, y, z);
    bool       existed = false;
    {
        std::lock_guard lock(mMutex);
        auto            erase = [&](Table& table) {
            if (table.blocks.erase(key) > 0)
                existed = true;
        };
        if (!playerName.empty()) {
            if (auto const it = mPlayers.find(playerName); it != mPlayers.end())
                erase(it->second);
        }
        erase(mGlobal);
        refreshActiveLocked();
    }
    if (!existed)
        return false;

    forEachTargetPlayer(playerName, [&](::Player& player) {
        sendBlockUpdate(player, x, y, z, realBlockNetworkIdAt(player, x, y, z));
    });
    return true;
}

void ViewOverrideManager::clearAll(std::string const& playerName) {
    std::vector<std::pair<std::int64_t, EntityRecord>> entities;
    std::vector<std::int64_t>                          blockKeys;
    {
        std::lock_guard lock(mMutex);
        auto            collect = [&](Table& table) {
            for (auto const& [uniqueId, record] : table.entities)
                entities.emplace_back(uniqueId, record);
            for (auto const& [key, record] : table.blocks)
                blockKeys.push_back(key);
        };
        if (playerName.empty()) {
            collect(mGlobal);
            for (auto& [name, table] : mPlayers)
                collect(table);
            mPlayers.clear();
            mPulse.clear();
            mGlobal = Table{};
        } else if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            collect(it->second);
            mPlayers.erase(it);
            mPulse.erase(playerName);
            collect(mGlobal);
        }
        refreshActiveLocked();
    }
    restoreCleared(playerName, entities, blockKeys);
}

void ViewOverrideManager::restoreCleared(
    std::string const&                                        playerName,
    std::vector<std::pair<std::int64_t, EntityRecord>> const& entities,
    std::vector<std::int64_t> const&                          blockKeys) {
    if (entities.empty() && blockKeys.empty())
        return;
    forEachTargetPlayer(playerName, [&](::Player& player) {
        for (auto const key : blockKeys) {
            auto const x = logic::unpackX(key);
            auto const y = logic::unpackY(key);
            auto const z = logic::unpackZ(key);
            sendBlockUpdate(player, x, y, z, realBlockNetworkIdAt(player, x, y, z));
        }
    });
    for (auto const& [uniqueId, record] : entities) {
        forEachRestoreViewer(playerName, uniqueId, !record.view.asPlayer.empty(), [&](::Player& player) {
            restoreEntityForPlayer(player, uniqueId, record);
        });
    }
}

void ViewOverrideManager::respawnForPlayer(
    ::Player&              player,
    ::Actor&               actor,
    std::string const&     identifier,
    logic::MetaPlan const& plan) {
    if (hlibDomainDisabled("HLIB_NO_VIEW"))
        return;
    if (actor.isPlayer())
        return;

    auto const uniqueId = actor.getOrCreateUniqueID().rawID;
    if (actor.getTypeName() == identifier) {

        if (plan.setName) {
            view::sendViewMeta(player, static_cast<std::uint64_t>(actor.getRuntimeID()), logic::metaPlanOf(true, plan.name, plan.alwaysShow));
        } else if (!plan.empty()) {
            view::sendViewMeta(player, static_cast<std::uint64_t>(actor.getRuntimeID()), plan);
        }
        return;
    }
    sendRemoveActor(player, uniqueId);
    view::sendActorSpawn(player, actor, identifier, plan);
}

void ViewOverrideManager::restoreEntityForPlayer(
    ::Player&           player,
    std::int64_t        uniqueId,
    EntityRecord const& record) {
    if (hlibDomainDisabled("HLIB_NO_VIEW"))
        return;
    auto* actor = liveActorNear(player, uniqueId);
    if (actor == nullptr) {

        auto* asPlayer = playerByUniqueId(uniqueId);
        if (asPlayer != nullptr && player.distanceTo(*asPlayer) <= 64.0f)
            actor = asPlayer;
    }

    if (actor != nullptr && actor->isPlayer()) {
        auto& target = *static_cast<::Player*>(actor);

        if (!record.view.asPlayer.empty())
            sendPlayerSkin(player, target, target.getRealName());

        bool const bodyTouched = record.substituted || record.view.hidden || !record.view.identifier.empty();
        if (bodyTouched && &player != &target) {

            sculk::protocol::SerializedSkin ownSkin;
            if (captureOwnSkin(target, ownSkin)) {

                InputSnapshot snap{};
                int const     playMode = inputSnapshotOf(uniqueId, snap) ? snap.playMode : 0;
                sendRemoveActor(player, uniqueId);
                sendRealPlayerBody(player, target, ownSkin, playMode);
            }
            return;
        }

        if (record.view.hasNametag) {
            view::sendViewMeta(player, static_cast<std::uint64_t>(target.getRuntimeID()), logic::metaPlanOf(true, target.getNameTag(), false));
        }
        return;
    }

    if (record.substituted)
        sendRemoveActor(player, uniqueId);

    if (record.view.hidden || !record.view.identifier.empty() || !record.view.skinId.empty()) {
        if (actor == nullptr)
            return;
        respawnForPlayer(player, *actor, actor->getTypeName(), logic::metaPlanOf(true, actor->getNameTag(), false));
        return;
    }

    if (record.view.hasNametag && actor != nullptr) {
        view::sendViewMeta(
            player,
            static_cast<std::uint64_t>(actor->getRuntimeID()),
            logic::metaPlanOf(true, actor->getNameTag(), false));
    }
}

bool ViewOverrideManager::inputSnapshotOf(std::int64_t uniqueId, InputSnapshot& out) const {
    std::lock_guard lock(mMutex);
    return inputSnapshotOfLocked(uniqueId, out);
}

bool ViewOverrideManager::inputSnapshotOfLocked(std::int64_t uniqueId, InputSnapshot& out) const {
    for (auto const& [name, snap] : mInputs) {
        if (snap.uniqueId == uniqueId && mServerTick - snap.tick <= 20) {
            out = snap;
            return true;
        }
    }
    return false;
}

bool ViewOverrideManager::uniqueIdOfPlayer(std::string const& playerName, std::int64_t& uniqueId) const {
    bool found = false;
    forEachTargetPlayer(playerName, [&](::Player& p) {
        uniqueId = p.getOrCreateUniqueID().rawID;
        found    = true;
    });
    return found;
}

void ViewOverrideManager::noteAuthInput(std::string const& playerName, InputSnapshot const& snapshot) {
    std::lock_guard lock(mMutex);
    auto            stamped = snapshot;
    stamped.tick            = mServerTick;
    mInputs[playerName]     = stamped;
}

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

    float const feetY = feetAnchorYFromInput(player, snapshot.y);

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
        auto* actor = liveActorNear(*player, uniqueId);
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
        sculkSendFailureCount().load(std::memory_order_relaxed),
        mControlSteps.load(std::memory_order_relaxed),
        mSkinnedSpawnCounter.load(std::memory_order_relaxed));
}

void ViewOverrideManager::noteHookCall() {
    mHookCalls.fetch_add(1, std::memory_order_relaxed);
}

ViewOverrideManager::HookCounters ViewOverrideManager::hookCounters() const {
    HookCounters out;
    out.calls = mHookCalls.load(std::memory_order_relaxed);
    out.spawn = mSpawnSeen.load(std::memory_order_relaxed);
    out.meta  = mMetaSeen.load(std::memory_order_relaxed);
    out.block = mBlockSeen.load(std::memory_order_relaxed);
    out.chunk = mChunkSeen.load(std::memory_order_relaxed);
    return out;
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

std::size_t ViewOverrideManager::globalEntityCount() const {
    std::lock_guard lock(mMutex);
    return mGlobal.entities.size();
}

std::size_t ViewOverrideManager::globalBlockCount() const {
    std::lock_guard lock(mMutex);
    return mGlobal.blocks.size();
}

} // namespace debugshape_export::view

namespace debugshape_export {

namespace {

using view::ViewOverrideManager;

class ViewOverrideAdapter : public hologramlib::IViewOverride {
public:
    bool overrideEntity(
        std::string const&             playerName,
        std::int64_t                   entityUniqueId,
        hologramlib::EntityView const& view) override {
        return ViewOverrideManager::getInstance().overrideEntity(playerName, entityUniqueId, view);
    }

    bool overrideBlock(
        std::string const&            playerName,
        int                           x,
        int                           y,
        int                           z,
        hologramlib::BlockView const& view) override {
        return ViewOverrideManager::getInstance().overrideBlock(playerName, x, y, z, view);
    }

    bool clearEntity(std::string const& playerName, std::int64_t entityUniqueId) override {
        return ViewOverrideManager::getInstance().clearEntity(playerName, entityUniqueId);
    }

    bool clearBlock(std::string const& playerName, int x, int y, int z) override {
        return ViewOverrideManager::getInstance().clearBlock(playerName, x, y, z);
    }

    void clearAll(std::string const& playerName) override {
        ViewOverrideManager::getInstance().clearAll(playerName);
    }

    [[nodiscard]] std::string describeFor(std::string const& playerName) const override {
        return ViewOverrideManager::getInstance().describeFor(playerName);
    }
};

} // namespace

hologramlib::IViewOverride& viewOverrideAdapter() {
    static ViewOverrideAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
