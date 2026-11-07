#include "GhostInteractRouter.h"
#include "core/HookRegistry.h"

#include "customentity/CustomEntityManager.h"
#include "itemdisplay/ItemDisplayManager.h"
#include "lse/LseBridge.h"

#include "DiagLog.h"
#include "playernpc/PlayerNpcManager.h"

#include <ll/api/memory/Hook.h>
#include <exception>
#include <ll/api/service/Bedrock.h>

#include <mc/network/NetworkIdentifier.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/InteractPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/inventory/transaction/InventoryTransactionError.h>
#include <mc/world/inventory/transaction/ItemUseOnActorInventoryTransaction.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {

GhostInteractRouter& GhostInteractRouter::getInstance() {
    static GhostInteractRouter instance;
    return instance;
}

void GhostInteractRouter::setListener(std::function<void(hologramlib::GhostInteractEvent const&)> listener) {
    std::lock_guard lock(mMutex);
    mListener = std::move(listener);
}

void GhostInteractRouter::clearListener() {
    std::lock_guard lock(mMutex);
    mListener = nullptr;
}

std::uint64_t GhostInteractRouter::addListener(std::function<void(hologramlib::GhostInteractEvent const&)> listener) {
    if (!listener)
        return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextToken++;
    mListeners.push_back({token, std::move(listener)});
    return token;
}

bool GhostInteractRouter::removeListener(std::uint64_t token) {
    std::lock_guard lock(mMutex);
    for (auto it = mListeners.begin(); it != mListeners.end(); ++it) {
        if (it->token == token) {
            mListeners.erase(it);
            return true;
        }
    }
    return false;
}

std::vector<hologramlib::GhostInteractEvent> GhostInteractRouter::poll() {
    std::lock_guard                              lock(mMutex);
    std::vector<hologramlib::GhostInteractEvent> out(mQueue.begin(), mQueue.end());
    mQueue.clear();
    return out;
}

void GhostInteractRouter::clearQueue() {
    std::lock_guard lock(mMutex);
    mQueue.clear();
}

std::uint64_t GhostInteractRouter::addActorListener(std::function<void(hologramlib::ActorInteractEvent const&)> listener) {
    if (!listener)
        return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextToken++;
    mActorListeners.push_back({token, std::move(listener)});
    return token;
}

bool GhostInteractRouter::removeActorListener(std::uint64_t token) {
    std::lock_guard lock(mMutex);
    for (auto it = mActorListeners.begin(); it != mActorListeners.end(); ++it) {
        if (it->token == token) {
            mActorListeners.erase(it);
            return true;
        }
    }
    return false;
}

std::vector<hologramlib::ActorInteractEvent> GhostInteractRouter::pollActor() {
    std::lock_guard                              lock(mMutex);
    std::vector<hologramlib::ActorInteractEvent> out(mActorQueue.begin(), mActorQueue.end());
    mActorQueue.clear();
    return out;
}

void GhostInteractRouter::clearActorQueue() {
    std::lock_guard lock(mMutex);
    mActorQueue.clear();
}

void GhostInteractRouter::dispatchActor(hologramlib::ActorInteractEvent const& ev) {

    std::vector<std::function<void(hologramlib::ActorInteractEvent const&)>> multicast;
    {
        std::lock_guard lock(mMutex);

        constexpr std::size_t kMaxQueue = 256;
        if (mActorQueue.size() >= kMaxQueue)
            mActorQueue.pop_front();
        mActorQueue.push_back(ev);
        multicast.reserve(mActorListeners.size());
        for (auto const& l : mActorListeners)
            multicast.push_back(l.fn);
    }
    for (auto const& fn : multicast) {
        try {
            fn(ev);
        } catch (std::exception const&) {

            HLIB_LOG_ERROR("HologramLib: ActorInteract 监听器抛出异常（已忽略）");
        }
    }
}

void GhostInteractRouter::dispatch(hologramlib::GhostInteractEvent const& ev) {

    std::function<void(hologramlib::GhostInteractEvent const&)>              legacy;
    std::vector<std::function<void(hologramlib::GhostInteractEvent const&)>> multicast;
    {
        std::lock_guard lock(mMutex);

        constexpr std::size_t kMaxQueue = 256;
        if (mQueue.size() >= kMaxQueue)
            mQueue.pop_front();
        mQueue.push_back(ev);
        legacy = mListener;
        multicast.reserve(mListeners.size());
        for (auto const& l : mListeners)
            multicast.push_back(l.fn);
    }
    for (auto const& fn : multicast)
        fn(ev);
    if (legacy)
        legacy(ev);
}

std::string GhostInteractRouter::formatEvent(hologramlib::GhostInteractEvent const& ev) {
    auto line = std::format(
        "player={} action={} domain={} id={}",
        ev.playerName,
        ev.action,
        ev.domain,
        ev.id);
    if (ev.hasPos) {
        line += std::format(" pos=({:.2f},{:.2f},{:.2f})", ev.x, ev.y, ev.z);
    }
    return line;
}

std::string GhostInteractRouter::formatActorEvent(hologramlib::ActorInteractEvent const& ev) {
    auto line = std::format(
        "player={} action={} runtimeId={} uniqueId={} type={}",
        ev.playerName,
        ev.action,
        ev.runtimeId,
        ev.uniqueId,
        ev.targetType);
    if (ev.hasPos) {
        line += std::format(" pos=({:.2f},{:.2f},{:.2f})", ev.x, ev.y, ev.z);
    }
    return line;
}

void GhostInteractRouter::exportLse() {
    static constexpr const char* NAMESPACE = "HologramLib";

    hologramlib::lse::exportAs(NAMESPACE, "ghostPollInteractions", []() -> std::vector<std::string> {
        auto                     events = GhostInteractRouter::getInstance().poll();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events)
            out.push_back(formatEvent(ev));
        return out;
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "ghostClearInteractions", []() -> void {
            GhostInteractRouter::getInstance().clearQueue();
        });

    hologramlib::lse::exportAs(NAMESPACE, "actorPollInteractions", []() -> std::vector<std::string> {
        auto                     events = GhostInteractRouter::getInstance().pollActor();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events)
            out.push_back(formatActorEvent(ev));
        return out;
    });

    hologramlib::lse::exportAs(NAMESPACE, "actorClearInteractions", []() -> void {
        GhostInteractRouter::getInstance().clearActorQueue();
    });
}

static void routeGhostInteract(
    std::string const& playerName,
    int                action,
    std::uint64_t      runtimeId,
    bool               hasPos,
    float              x,
    float              y,
    float              z) {
    bool const isDisplay = core::actorIds::itemDisplay.contains(runtimeId);
    bool const isEntity  = core::actorIds::customEntity.contains(runtimeId);
    bool const isNpc     = core::actorIds::playerNpc.contains(runtimeId);
    if (!isDisplay && !isEntity && !isNpc) {

        hologramlib::ActorInteractEvent actor;
        actor.playerName = playerName;
        actor.action     = action;
        actor.runtimeId  = runtimeId;
        actor.hasPos     = hasPos;
        actor.x          = x;
        actor.y          = y;
        actor.z          = z;
        if (auto level = ::ll::service::getLevel(); level != nullptr) {
            if (auto* target = level->getRuntimeEntity(::ActorRuntimeID{runtimeId}, false); target != nullptr) {
                actor.uniqueId   = target->getOrCreateUniqueID().rawID;
                actor.targetType = target->getTypeName();
            }
        }
        GhostInteractRouter::getInstance().dispatchActor(actor);
        return;
    }

    int64_t                         id = -1;
    hologramlib::GhostInteractEvent ev;
    ev.playerName = playerName;
    ev.action     = action;
    ev.hasPos     = hasPos;
    ev.x          = x;
    ev.y          = y;
    ev.z          = z;

    if (isEntity && CustomEntityManager::getInstance().findByRuntimeId(runtimeId, id)) {
        ev.domain = "entity";
        ev.id     = id;
    } else if (isDisplay && ItemDisplayManager::getInstance().findByRuntimeId(runtimeId, id)) {
        ev.domain = "itemDisplay";
        ev.id     = id;
    } else if (isNpc && PlayerNpcManager::getInstance().findByRuntimeId(runtimeId, id)) {
        ev.domain = "npc";
        ev.id     = id;
    } else {
        return;
    }

    GhostInteractRouter::getInstance().dispatch(ev);
}

LL_TYPE_INSTANCE_HOOK(
    GhostInteractHook,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const& source,
    InteractPacket const&    packet) {
    if (!debugshape_export::core::live()) {
        origin(source, packet);
        return;
    }
    origin(source, packet);

    auto const runtimeId = static_cast<std::uint64_t>(packet.mTargetId.get().rawID);
    bool const isDisplay = core::actorIds::itemDisplay.contains(runtimeId);
    bool const isEntity  = core::actorIds::customEntity.contains(runtimeId);
    bool const isNpc     = core::actorIds::playerNpc.contains(runtimeId);
    if (!isDisplay && !isEntity && !isNpc)
        return;

    Player* actor = nullptr;
    if (auto level = ll::service::getLevel()) {
        level->forEachPlayer([&](Player& p) {
            if (p.getNetworkIdentifier() == source) {
                actor = &p;
                return false;
            }
            return true;
        });
    }
    if (!actor)
        return;

    if (auto const& pos = packet.mPos.get()) {
        routeGhostInteract(
            actor->getRealName(),
            static_cast<int>(packet.mAction),
            runtimeId,
            true,
            static_cast<float>(pos->x),
            static_cast<float>(pos->y),
            static_cast<float>(pos->z));
    } else {
        routeGhostInteract(actor->getRealName(), static_cast<int>(packet.mAction), runtimeId, false, 0, 0, 0);
    }
}

LL_TYPE_INSTANCE_HOOK(
    GhostItemUseOnActorHook,
    HookPriority::Normal,
    ItemUseOnActorInventoryTransaction,
    &ItemUseOnActorInventoryTransaction::$handle,
    ::InventoryTransactionError,
    ::Player& player,
    bool      isSenderAuthority) {
    if (!debugshape_export::core::live()) {
        return static_cast<::InventoryTransactionError>(origin(player, isSenderAuthority));
    }
    auto const result = origin(player, isSenderAuthority);

    int action = 1;
    switch (this->mActionType) {
    case ::ItemUseOnActorInventoryTransaction::ActionType::Attack:
        action = 2;
        break;
    case ::ItemUseOnActorInventoryTransaction::ActionType::Interact:
    case ::ItemUseOnActorInventoryTransaction::ActionType::ItemInteract:
    default:
        action = 1;
        break;
    }

    auto const& hitPos = this->mHitPos.get();
    routeGhostInteract(
        player.getRealName(),
        action,
        static_cast<std::uint64_t>(this->mRuntimeId.get().rawID),
        true,
        static_cast<float>(hitPos.x),
        static_cast<float>(hitPos.y),
        static_cast<float>(hitPos.z));
    return result;
}

static ll::memory::HookRegistrar<GhostInteractHook, GhostItemUseOnActorHook> gGhostInteractHookRegistrar;
static debugshape_export::core::HookRegistrar                                gHookReg_ghost_interact{
    "ghost-interact",
    [] {
        decltype(gGhostInteractHookRegistrar)::hook();
    },
    [] {
        decltype(gGhostInteractHookRegistrar)::unhook();
    },
};

} // namespace debugshape_export
