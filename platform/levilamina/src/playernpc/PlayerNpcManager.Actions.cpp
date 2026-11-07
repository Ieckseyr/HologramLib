#include "PlayerNpcInternal.h"
#include "kernel/EventIds.h"

#include "DiagLog.h"
#include "core/TickDriver.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/service/Bedrock.h>

#include <ll/api/memory/Hook.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/world/level/Level.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/network/packet/MobArmorEquipmentPacket.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/ContainerID.h>
#include <mc/world/item/ItemStack.h>

#include <algorithm>
#include <cctype>
#include <format>
#include <random>

#include "NpcProtocol.h"
#include "NpcSkinRegistry.h"

#include "core/proto/Version.h"

#include "ProtoPacketSend.h"
#include "customentity/CustomEntityManager.h"

#include "PlayerNpcManager.h"
#include "core/DiagFlags.h"

#include "playernpc/PlayerNpcRuntimeOps.h"

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

constexpr char const* kStopExpr = "1";

static bool isLegacyStopExpression(std::string const& e) {
    return e == "query.any_animation" || e == "query.any_animation()";
}

bool PlayerNpcManager::playAnimation(
    int64_t            id,
    std::string const& animation,
    std::string const& stopExpression,
    int                durationTicks) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr || animation.empty())
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr || !it->enabled || rit->shownPlayers.empty())
        return false;

    auto const now  = npc_ops::currentTick();
    auto const ctrl = npcAnimControllerName(id);
    auto const rt   = rit->runtimeId;
    if (isLegacyStopExpression(stopExpression)) {
        for (auto const& uuid : rit->shownPlayers) {
            mAnimQueue.emplace(now + 2, AnimEntry{uuid, rt, id, kStopAnimation, ctrl, kStopExpr});
        }
        return true;
    }
    for (auto const& uuid : rit->shownPlayers) {
        mAnimQueue.emplace(now + 2, AnimEntry{uuid, rt, id, animation, ctrl, stopExpression});
        if (durationTicks > 0) {
            mAnimQueue.emplace(
                now + 2 + static_cast<std::uint64_t>(durationTicks),
                AnimEntry{uuid, rt, id, kStopAnimation, ctrl, kStopExpr});
        }
    }
    return true;
}

bool PlayerNpcManager::playAnimationTo(
    int64_t            id,
    std::string const& playerName,
    std::string const& animation,
    std::string const& stopExpression,
    int                durationTicks) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr || animation.empty())
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr || !it->enabled)
        return false;

    auto* player = npc_ops::findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto const uuid = player->getUuid();
    if (!rit->shownPlayers.contains(uuid))
        return false;

    auto const now  = npc_ops::currentTick();
    auto const ctrl = npcAnimControllerName(id);
    if (isLegacyStopExpression(stopExpression)) {
        mAnimQueue.emplace(now + 2, AnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
        return true;
    }
    mAnimQueue.emplace(now + 2, AnimEntry{uuid, rit->runtimeId, id, animation, ctrl, stopExpression});
    if (durationTicks > 0) {
        mAnimQueue.emplace(
            now + 2 + static_cast<std::uint64_t>(durationTicks),
            AnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
    }
    return true;
}

void PlayerNpcManager::flushAnimsLocked() {
    if (mAnimQueue.empty())
        return;
    auto const now = npc_ops::currentTick();
    for (auto it = mAnimQueue.begin(); it != mAnimQueue.end() && it->first <= now;) {
        auto const& e    = it->second;
        bool        ok   = false;
        auto*       rt2  = runtimeOf(e.npcId);
        auto*       cfg2 = configOf(e.npcId);
        if (rt2 != nullptr && cfg2 != nullptr) {
            ok = rt2->runtimeId == e.runtimeId && rt2->shownPlayers.contains(e.playerUuid) && cfg2->enabled;
        }
        if (ok) {
            if (auto* player = npc_ops::findPlayerByUuid(e.playerUuid)) {
                proto::AnimateEntityPacket pkt;
                pkt.animation               = e.animation;
                pkt.next_state              = "none";
                pkt.stop_expression         = e.stopExpression;
                pkt.stop_expression_version = static_cast<std::int32_t>(proto::MolangVersion::Initial);
                pkt.controller              = e.controller;
                pkt.blend_out_time          = 0;
                pkt.runtime_ids             = {proto::bp::ActorRuntimeID{e.runtimeId}};
                npc_protocol::sendToPlayer(*player, pkt, wire::Reliability::Reliable);
            }
        }
        it = mAnimQueue.erase(it);
    }
}

bool PlayerNpcManager::playEmote(int64_t id, std::string const& pieceId, int lengthTicks) {
    proto::EmotePacket packet;
    std::vector<mce::UUID>       recipients;
    {
        std::lock_guard lock(mMutex);
        auto*           rt = runtimeOf(id);
        if (rt == nullptr || pieceId.empty() || rt->shownPlayers.empty())
            return false;

        packet.runtime_id  = proto::bp::ActorRuntimeID{rt->runtimeId};
        packet.piece_id    = pieceId;
        packet.emote_ticks = lengthTicks < 0 ? 0u : static_cast<std::uint32_t>(lengthTicks);
        packet.flags       = 0;
        recipients.assign(rt->shownPlayers.begin(), rt->shownPlayers.end());
    }

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    // 锁外发包：收件人是锁内快照（P04）
    for (auto const& uuid : recipients) {
        if (auto* player = npc_ops::findPlayerByUuid(uuid)) {
            wire::sendRaw(*player, bytes);
        }
    }
    return true;
}

std::string PlayerNpcManager::getDebugInfo(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto*           it  = configOf(id);
    auto*           rit = runtimeOf(id);
    if (it == nullptr || rit == nullptr)
        return "not_found";
    auto const& cfg = *it;
    auto const& rt  = *rit;
    return std::format(
        "id={} name='{}' skin={} pos=({:.1f},{:.1f},{:.1f}) dim={} yaw={:.0f} view={} enabled={} "
        "uniqueId={:#x} runtimeId={:#x} shown={} tabPending={} filter={}",
        id,
        cfg.name,
        cfg.skinId,
        cfg.x,
        cfg.y,
        cfg.z,
        cfg.dimension,
        cfg.yaw,
        cfg.viewDistance,
        cfg.enabled,
        rt.uniqueId,
        rt.runtimeId,
        rt.shownPlayers.size(),
        mTabRemovals.contains(id) ? mTabRemovals.at(id).size() : 0,
        mVisibleFilter.contains(id) ? "custom" : "all");
}

} // namespace debugshape_export
