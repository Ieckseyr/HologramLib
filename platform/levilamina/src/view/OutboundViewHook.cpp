#include "../effect/ActorEventFilter.h"
#include "../visibility/PlayerVisibilityManager.h"
#include "ViewOverrideManager.h"
#include "kernel/EventIds.h"
#include "core/HookRegistry.h"
#include "kernel/PlayerIndex.h"

#include "DiagLog.h"
#include "core/TickDriver.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/common/SubClientId.h>
#include <mc/network/LoopbackPacketSender.h>
#include <mc/network/NetworkIdentifier.h>
#include <mc/network/NetworkIdentifierWithSubId.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/Packet.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/ActorEventPacketPayload.h>
#include <mc/network/packet/AnimatePacketPayload.h>
#include <mc/network/packet/ContainerClosePacket.h>
#include <mc/network/packet/InteractPacket.h>
#include <mc/network/packet/InventoryTransactionPacket.h>
#include <mc/network/packet/ItemStackRequestPacket.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/network/packet/PlayerActionPacket.h>
#include <mc/network/packet/PlayerAuthInputPacket.h>
#include <mc/network/packet/PlayerAuthInputPacketPayload.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <cstdlib>
#include <exception>
#include <vector>

namespace debugshape_export::view {

namespace {

using Manager = ViewOverrideManager;

Manager& manager() {
    return Manager::getInstance();
}

template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return (value.get());
    } else {
        return (value);
    }
}

::Player* findPlayerByNetworkId(::NetworkIdentifier const& source) {
    return core::PlayerIndex::getInstance().byNetworkId(source);
}

bool anyRecipientHasOverrides(std::vector<::NetworkIdentifierWithSubId> const& ids) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return false;
    bool any = false;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (!manager().hasOverridesFor(player.getRealName()))
            return true;
        for (auto const& entry : ids) {
            if (entry.id == player.getNetworkIdentifier()) {
                any = true;
                return false;
            }
        }
        return true;
    });
    return any;
}

template <typename Fn>
bool runGuarded(char const* what, Fn&& fn) {
    try {
        fn();
        return true;
    } catch (std::exception const& e) {
        HLIB_LOG_ERROR("HologramLib: {} 抛出异常, 本次按无覆盖处理: {}", what, e.what());
    } catch (...) {
        HLIB_LOG_ERROR("HologramLib: {} 抛出未知异常, 本次按无覆盖处理", what);
    }
    return false;
}

} // namespace

static bool viewOverrideDisabled() {
    static bool const disabled = [] {
        char const* v = std::getenv("HLIB_NO_VIEW");
        return v != nullptr && v[0] == '1';
    }();
    return disabled;
}

LL_TYPE_INSTANCE_HOOK(
    ViewNetSendHook,
    ll::memory::HookPriority::Normal,
    NetworkSystem,
    &NetworkSystem::send,
    void,
    ::NetworkIdentifier const& id,
    ::Packet const&            packet,
    ::SubClientId              recipientSubId) {
    if (!debugshape_export::core::live()) {
        origin(id, packet, recipientSubId);
        return;
    }
    manager().noteHookCall();

    if (auto& filter = effect::ActorEventFilter::getInstance(); filter.active()) {
        std::uint64_t runtimeId = 0;
        int           eventId   = 0;
        if (effect::ActorEventFilter::readActorEvent(packet, runtimeId, eventId)) {
            if (auto* player = findPlayerByNetworkId(id);
                player != nullptr && filter.shouldDrop(*player, runtimeId, eventId)) {
                return;
            }
        }
    }

    if (viewOverrideDisabled() || !manager().active()) {
        origin(id, packet, recipientSubId);
        return;
    }
    auto* player = findPlayerByNetworkId(id);
    if (player == nullptr) {
        origin(id, packet, recipientSubId);
        return;
    }

    if (PlayerVisibilityManager::getInstance().shouldDrop(*player, packet)) {
        return;
    }

    bool       forwarded = false;
    bool const ran       = runGuarded("ViewNetSendHook", [&] {
        Manager::Applied applied;
        if (!manager().applyToOutbound(*player, packet, applied)) {
            origin(id, packet, recipientSubId);
            forwarded = true;
            return;
        }
        if (!applied.drop) {
            origin(id, packet, recipientSubId);
            forwarded = true;
        }
        manager().runPostActions(*player, applied);
    });
    if (!ran && !forwarded)
        origin(id, packet, recipientSubId);
}

static ll::memory::HookRegistrar<ViewNetSendHook> gViewNetSendHook;
static debugshape_export::core::HookRegistrar     gHookReg_view_send{
    "view-send",
    [] {
        decltype(gViewNetSendHook)::hook();
    },
    [] {
        decltype(gViewNetSendHook)::unhook();
    },
};

LL_TYPE_INSTANCE_HOOK(
    ViewNetSendToMultipleHook,
    ll::memory::HookPriority::Normal,
    LoopbackPacketSender,
    &LoopbackPacketSender::$sendToClients,
    void,
    ::std::vector<::NetworkIdentifierWithSubId> const& ids,
    ::Packet const&                                    packet) {
    if (!debugshape_export::core::live()) {
        origin(ids, packet);
        return;
    }
    manager().noteHookCall();
    if (viewOverrideDisabled()) {
        origin(ids, packet);
        return;
    }
    bool       dispatched = false;
    bool const ran        = runGuarded("ViewNetSendToMultipleHook", [&] {
        if (!manager().active() || !anyRecipientHasOverrides(ids)) {
            origin(ids, packet);
            return;
        }
        dispatched = true;
        for (auto const& entry : ids) {
            this->sendToClient(entry.id, packet, entry.subClientId);
        }
    });
    if (!ran && !dispatched)
        origin(ids, packet);
}

static ll::memory::HookRegistrar<ViewNetSendToMultipleHook> gViewNetSendToMultipleHook;
static debugshape_export::core::HookRegistrar               gHookReg_view_send_multi{
    "view-send-multi",
    [] {
        decltype(gViewNetSendToMultipleHook)::hook();
    },
    [] {
        decltype(gViewNetSendToMultipleHook)::unhook();
    },
};

void viewOverrideTick() {
    if (viewOverrideDisabled())
        return;
    runGuarded("ViewOverrideTickHook", [&] {
        manager().tickPulse();
    });
}

tick::AutoRegister gViewOverrideTickHook{"viewOverride", &viewOverrideTick, tick::Order::ViewOverride};

LL_TYPE_INSTANCE_HOOK(
    ViewAuthInputHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    ::NetworkIdentifier const&     source,
    ::PlayerAuthInputPacket const& packet) {
    if (!debugshape_export::core::live()) {
        origin(source, packet);
        return;
    }
    origin(source, packet);
    runGuarded("ViewAuthInputHook", [&] {
        if (!manager().active())
            return;
        auto* player = findPlayerByNetworkId(source);
        if (player == nullptr)
            return;

        auto const& payload = static_cast<::PlayerAuthInputPacketPayload const&>(packet);
        auto const& rawPos  = unwrap(payload.mPos);
        auto const& rawRot  = unwrap(payload.mRot);

        std::uint64_t bits = 0;
        {
            auto const& bs = unwrap(payload.mInputData);
            for (int i = 0; i < 64; ++i) {
                if (bs.contains(static_cast<::PlayerAuthInputPacketPayload::InputData>(i))) {
                    bits |= (std::uint64_t(1) << i);
                }
            }
        }

        Manager::InputSnapshot snapshot;
        snapshot.uniqueId  = player->getOrCreateUniqueID().rawID;
        snapshot.runtimeId = static_cast<std::uint64_t>(player->getRuntimeID());
        snapshot.x         = rawPos.x;
        snapshot.y         = rawPos.y;
        snapshot.z         = rawPos.z;
        snapshot.pitch     = rawRot.x;
        snapshot.yaw       = rawRot.y;
        snapshot.headYaw   = unwrap(payload.mYHeadRot);
        snapshot.bits      = bits;
        snapshot.playMode  = static_cast<int>(unwrap(payload.mPlayMode));
        manager().noteAuthInput(player->getRealName(), snapshot);
        manager().pushSubstitutedMovement(*player, snapshot);
    });
}

static ll::memory::HookRegistrar<ViewAuthInputHook> gViewAuthInputHook;
static debugshape_export::core::HookRegistrar       gHookReg_view_authinput{
    "view-authinput",
    [] {
        decltype(gViewAuthInputHook)::hook();
    },
    [] {
        decltype(gViewAuthInputHook)::unhook();
    },
};

void initViewOverrideEvents() {
    static ll::event::ListenerPtr listener = nullptr;
    if (listener != nullptr)
        return;
    listener = debugshape_export::core::listen<ll::event::PlayerDisconnectEvent>(core::kPlayerDisconnectId, [](ll::event::PlayerDisconnectEvent& event) {
        auto& player = event.self();
        ViewOverrideManager::getInstance().onPlayerLeave(player.getRealName());
    },
                                                                                 debugshape_export::core::ListenerScope::Process);
}

} // namespace debugshape_export::view
