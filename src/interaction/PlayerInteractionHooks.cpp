#include "container/ContainerMenuManager.h"
#include "container/ContainerResponse.h"
#include "core/HookRegistry.h"
#include "fakeinv/FakeInventoryManager.h"

#include "DiagLog.h"
#include "trade/TradeMenuManager.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/network/NetworkIdentifier.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/ContainerClosePacket.h>
#include <mc/network/packet/ItemStackRequestPacket.h>
#include <mc/network/packet/PlayerAuthInputPacket.h>
#include <mc/network/packet/cerealize/types/item_stack_request_cereal/ItemStackRequestCereal.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/containers/ContainerEnumName.h>
#include <mc/world/containers/FullContainerName.h>
#include <mc/world/inventory/network/ItemStackNetIdVariant.h>
#include <mc/world/level/Level.h>

#include <cstdint>
#include <variant>
#include <vector>

namespace debugshape_export {
namespace {

Player* findPlayerByNetworkId(NetworkIdentifier const& source) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) -> bool {
        if (p.getNetworkIdentifier() == source) {
            found = &p;
            return false;
        }
        return true;
    });
    return found;
}

template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return value.get();
    } else {
        return (value);
    }
}

struct SlotRef {
    int container{0};
    int containerId{-1};
    int slot{-1};
};

[[nodiscard]] SlotRef readSlot(::ItemStackRequestCereal::SlotInfoData const& slotInfo) {
    auto const& fcn = unwrap(slotInfo.mFullContainerName);
    auto const& dyn = unwrap(fcn.mDynamicId);
    return SlotRef{
        static_cast<int>(unwrap(fcn.mName)),
        dyn.has_value() ? static_cast<int>(*dyn) : -1,
        static_cast<int>(slotInfo.mSlot),
    };
}

int netIdOf(::ItemStackRequestCereal::SlotInfoData const& slotInfo) {
    auto const& variant = unwrap(slotInfo.mNetIdVariant).mVariant.get();
    if (auto const* typed = std::get_if<::ItemStackNetId>(&variant)) {
        return static_cast<int>(typed->mRawId);
    }
    return 0;
}

// 点击主通道：先容器域，再伪造背包域
bool dispatchSlot(Player& player, ::ItemStackRequestCereal::SlotInfoData const& slotInfo) {
    auto const ref = readSlot(slotInfo);
    if (ContainerMenuManager::getInstance().handleSlotAction(
            player.getRealName(),
            ref.container,
            ref.containerId,
            ref.slot)) {
        return true;
    }
    return FakeInventoryManager::getInstance().handleInventoryAction(player.getRealName(), ref.container, ref.slot);
}

template <typename ActionDataT>
void dispatchAction(Player& player, ActionDataT const& data) {

    if constexpr (requires { data.mSource; }) {
        if (dispatchSlot(player, data.mSource.get()))
            return;
    }
    if constexpr (requires { data.mDestination; }) {
        (void)dispatchSlot(player, data.mDestination.get());
    }
}

struct RequestCollector {
    std::vector<ContainerMenuManager::RequestSlot> slots;
    int                                            amount{1};

    void add(::ItemStackRequestCereal::SlotInfoData const& slotInfo) {
        auto const ref = readSlot(slotInfo);
        slots.push_back(ContainerMenuManager::RequestSlot{ref.container, ref.containerId, ref.slot});
    }

    template <typename ActionDataT>
    void operator()(ActionDataT const& data) {
        if constexpr (requires { data.mSource; })
            add(data.mSource.get());
        if constexpr (requires { data.mDestination; })
            add(data.mDestination.get());
        if constexpr (requires { data.mAmount; })
            amount = static_cast<int>(data.mAmount);
    }
};

bool playerHasInteraction(Player& player) {
    auto const& name = player.getRealName();
    return ContainerMenuManager::getInstance().hasMenuFor(name) || FakeInventoryManager::getInstance().isActive(name) || TradeMenuManager::getInstance().hasSettlingMenuFor(name);
}

template <typename T>
constexpr int tradeActionTypeCode() {
    using namespace ::ItemStackRequestCereal;
    if constexpr (std::is_same_v<T, TakeActionData>)
        return 0;
    else if constexpr (std::is_same_v<T, PlaceActionData>)
        return 1;
    else if constexpr (std::is_same_v<T, SwapActionData>)
        return 2;
    else if constexpr (std::is_same_v<T, DropActionData>)
        return 3;
    else if constexpr (std::is_same_v<T, DestroyActionData>)
        return 4;
    else if constexpr (std::is_same_v<T, ConsumeActionData>)
        return 5;
    else if constexpr (std::is_same_v<T, CreateActionData>)
        return 6;
    else if constexpr (std::is_same_v<T, LabTableCombineActionData>)
        return 9;
    else if constexpr (std::is_same_v<T, BeaconPaymentActionData>)
        return 10;
    else if constexpr (std::is_same_v<T, MineBlockActionData>)
        return 11;
    else if constexpr (std::is_same_v<T, CraftRecipeActionData>)
        return 12;
    else if constexpr (std::is_same_v<T, CraftRecipeAutoActionData>)
        return 13;
    else if constexpr (std::is_same_v<T, CraftCreativeActionData>)
        return 14;
    else if constexpr (std::is_same_v<T, CraftRecipeOptionalActionData>)
        return 15;
    else if constexpr (std::is_same_v<T, CraftRepairAndDisenchantActionData>)
        return 16;
    else if constexpr (std::is_same_v<T, CraftLoomActionData>)
        return 17;
    else if constexpr (std::is_same_v<T, CraftNonImplementedActionData>)
        return 18;
    else if constexpr (std::is_same_v<T, CraftResultsActionData>)
        return 19;
    else
        return -1;
}

struct TradeActionCollector {
    std::vector<TradeMenuManager::TradeRequestAction> actions;

    template <typename ActionDataT>
    void operator()(ActionDataT const& data) {
        using T = std::decay_t<ActionDataT>;
        TradeMenuManager::TradeRequestAction action;
        action.type = tradeActionTypeCode<T>();
        if constexpr (requires { data.mSource; }) {
            auto const ref        = readSlot(data.mSource.get());
            action.hasSrc         = true;
            action.srcContainer   = ref.container;
            action.srcContainerId = ref.containerId;
            action.srcSlot        = ref.slot;
            action.srcNetId       = netIdOf(data.mSource.get());
        }
        if constexpr (requires { data.mDestination; }) {
            auto const ref        = readSlot(data.mDestination.get());
            action.hasDst         = true;
            action.dstContainer   = ref.container;
            action.dstContainerId = ref.containerId;
            action.dstSlot        = ref.slot;
            action.dstNetId       = netIdOf(data.mDestination.get());
        }
        if constexpr (requires { data.mAmount; }) {
            action.amount = static_cast<int>(unwrap(data.mAmount));
        }

        if constexpr (
            std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeActionData> || std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeAutoActionData> || std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeOptionalActionData>) {
            action.recipeNetId = static_cast<int>(unwrap(data.mRecipeNetId).mRawId);
        }
        actions.push_back(action);
    }
};

} // namespace

LL_TYPE_INSTANCE_HOOK(
    PlayerInteractionItemStackRequestHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&      source,
    ItemStackRequestPacket const& packet) {
    if (!debugshape_export::core::live()) {
        origin(source, packet);
        return;
    }
    auto* player = findPlayerByNetworkId(source);
    if (player == nullptr) {
        origin(source, packet);
        return;
    }
    if (!playerHasInteraction(*player)) {
        origin(source, packet);
        return;
    }

    std::vector<std::int32_t> claimed;
    for (auto const& request : packet.mRequests.get()) {

        RequestCollector     collector;
        TradeActionCollector trade;
        for (auto const& action : request.mActions.get()) {
            std::visit(collector, action);
            std::visit(trade, action);
        }
        if (ContainerMenuManager::getInstance().handleInteractiveRequest(
                player->getRealName(),
                collector.slots,
                collector.amount)) {
            claimed.push_back(request.mClientRequestId.get().mRawId);
            continue;
        }

        if (TradeMenuManager::getInstance().handleItemRequest(
                *player,
                trade.actions,
                request.mClientRequestId.get().mRawId)) {
            continue;
        }
        for (auto const& action : request.mActions.get()) {
            std::visit([&](auto const& data) {
                dispatchAction(*player, data);
            },
                       action);
        }
    }
    if (!claimed.empty()) {

        container::sendRequestSuccess(*player, claimed);
        return;
    }
    origin(source, packet);
}

static ll::memory::HookRegistrar<PlayerInteractionItemStackRequestHook> gPlayerInteractionItemStackRequestHook;
static debugshape_export::core::HookRegistrar                           gHookReg_interaction_itemstack{
    "interaction-itemstack",
    [] {
        decltype(gPlayerInteractionItemStackRequestHook)::hook();
    },
    [] {
        decltype(gPlayerInteractionItemStackRequestHook)::unhook();
    },
};

LL_TYPE_INSTANCE_HOOK(
    PlayerInteractionAuthInputHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&     source,
    PlayerAuthInputPacket const& packet) {
    if (!debugshape_export::core::live()) {
        origin(source, packet);
        return;
    }
    origin(source, packet);

    if (packet.mItemStackRequest == nullptr)
        return;

    auto* player = findPlayerByNetworkId(source);
    if (player == nullptr)
        return;
    if (!playerHasInteraction(*player))
        return;

    RequestCollector     collector;
    TradeActionCollector trade;
    for (auto const& actionPtr : packet.mItemStackRequest->mActions.get()) {
        if (actionPtr != nullptr) {
            auto const data = ::ItemStackRequestCereal::toActionData(*actionPtr);
            std::visit(collector, data);
            std::visit(trade, data);
        }
    }
    if (ContainerMenuManager::getInstance().handleInteractiveRequest(
            player->getRealName(),
            collector.slots,
            collector.amount)) {

        container::sendRequestSuccess(*player, {packet.mItemStackRequest->mClientRequestId.get().mRawId});
        return;
    }
    if (TradeMenuManager::getInstance().handleItemRequest(
            *player,
            trade.actions,
            packet.mItemStackRequest->mClientRequestId.get().mRawId)) {

        return;
    }
    for (auto const& actionPtr : packet.mItemStackRequest->mActions.get()) {
        if (actionPtr != nullptr) {
            std::visit(
                [&](auto const& data) {
                    dispatchAction(*player, data);
                },
                ::ItemStackRequestCereal::toActionData(*actionPtr));
        }
    }
}

static ll::memory::HookRegistrar<PlayerInteractionAuthInputHook> gPlayerInteractionAuthInputHook;
static debugshape_export::core::HookRegistrar                    gHookReg_interaction_authinput{
    "interaction-authinput",
    [] {
        decltype(gPlayerInteractionAuthInputHook)::hook();
    },
    [] {
        decltype(gPlayerInteractionAuthInputHook)::unhook();
    },
};

LL_TYPE_INSTANCE_HOOK(
    PlayerInteractionContainerCloseHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&    source,
    ContainerClosePacket const& packet) {
    if (!debugshape_export::core::live()) {
        origin(source, packet);
        return;
    }
    origin(source, packet);

    auto* player = findPlayerByNetworkId(source);
    if (player == nullptr)
        return;
    auto const containerId = static_cast<int>(packet.mContainerId);
    if (ContainerMenuManager::getInstance().handleContainerClose(player->getRealName(), containerId)) {
        return;
    }
    auto& trade = TradeMenuManager::getInstance();
    if (!trade.hasMenuFor(player->getRealName()))
        return;
    trade.handleContainerClose(player->getRealName(), containerId);
}

static ll::memory::HookRegistrar<PlayerInteractionContainerCloseHook> gPlayerInteractionContainerCloseHook;
static debugshape_export::core::HookRegistrar                         gHookReg_interaction_containerclose{
    "interaction-containerclose",
    [] {
        decltype(gPlayerInteractionContainerCloseHook)::hook();
    },
    [] {
        decltype(gPlayerInteractionContainerCloseHook)::unhook();
    },
};

} // namespace debugshape_export
