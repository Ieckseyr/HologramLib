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
        vo_ops::forEachOverrideViewer(playerName, uniqueId, [&](::Player& player) {
            sendRemoveActor(player, uniqueId);
        });

    } else if (touchedShape && (!normalised.asPlayer.empty() || !normalised.identifier.empty() || !normalised.skinId.empty())) {
        vo_ops::forEachTargetPlayer(playerName, [&](::Player& player) {
            auto* actor = vo_ops::liveActorNear(player, uniqueId);

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
        vo_ops::forEachOverrideViewer(playerName, uniqueId, [&](::Player& player) {
            view::sendViewMeta(player, record.runtimeId, logic::metaPlanOf(true, normalised.nametag, normalised.nametagAlwaysShow));
        });
    }
    return true;
}

} // namespace debugshape_export::view
