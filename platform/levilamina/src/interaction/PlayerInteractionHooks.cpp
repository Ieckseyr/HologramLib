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
#include <mc/world/actor/player/Player.h>
#include <mc/world/containers/ContainerEnumName.h>
#include <mc/world/containers/FullContainerName.h>
#include <mc/world/inventory/network/ItemStackNetIdVariant.h>
#include <mc/world/inventory/network/ItemStackRequestActionBeaconPayment.h>
#include <mc/world/inventory/network/ItemStackRequestActionConsume.h>
#include <mc/world/inventory/network/ItemStackRequestActionCreate.h>
#include <mc/world/inventory/network/ItemStackRequestActionDestroy.h>
#include <mc/world/inventory/network/ItemStackRequestActionDrop.h>
#include <mc/world/inventory/network/ItemStackRequestActionMineBlock.h>
#include <mc/world/inventory/network/ItemStackRequestActionPlace.h>
#include <mc/world/inventory/network/ItemStackRequestActionSwap.h>
#include <mc/world/inventory/network/ItemStackRequestActionTake.h>
#include <mc/world/inventory/network/ItemStackRequestBatch.h>
#include <mc/world/inventory/network/ItemStackRequestData.h>
#include <mc/world/inventory/network/ItemStackRequestSlotInfo.h>

#include <mc/world/level/Level.h>

#include <cstdint>
#include <variant>
#include <vector>

// 1.9.7（BDS 1.21.132）的 ItemStackRequest action 类型名与 26.60 不同（没有 Cereal 命名空间），
// 这里做一层别名，下面 26.60 风格的代码不用逐行改。
namespace debugshape_export::interaction_compat {
using TakeActionData          = ::ItemStackRequestActionTake;
using PlaceActionData         = ::ItemStackRequestActionPlace;
using SwapActionData          = ::ItemStackRequestActionSwap;
using DropActionData          = ::ItemStackRequestActionDrop;
using DestroyActionData       = ::ItemStackRequestActionDestroy;
using ConsumeActionData       = ::ItemStackRequestActionConsume;
using CreateActionData        = ::ItemStackRequestActionCreate;
using BeaconPaymentActionData = ::ItemStackRequestActionBeaconPayment;
using MineBlockActionData     = ::ItemStackRequestActionMineBlock;
} // namespace debugshape_export::interaction_compat

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

[[nodiscard]] SlotRef readSlot(::ItemStackRequestSlotInfo const& slotInfo) {
    auto const& fcn = unwrap(slotInfo.mFullContainerName);
    auto const& dyn = unwrap(fcn.mDynamicId);
    return SlotRef{
        static_cast<int>(unwrap(fcn.mName)),
        dyn.has_value() ? static_cast<int>(*dyn) : -1,
        static_cast<int>(slotInfo.mSlot),
    };
}

int netIdOf(::ItemStackRequestSlotInfo const& slotInfo) {
    auto const& variant = unwrap(slotInfo.mNetIdVariant).mVariant.get();
    if (auto const* typed = std::get_if<::ItemStackNetId>(&variant)) {
        return static_cast<int>(typed->mRawId);
    }
    return 0;
}

// 点击主通道：先容器域，再伪造背包域
bool dispatchSlot(Player& player, ::ItemStackRequestSlotInfo const& slotInfo) {
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

    if constexpr (requires { data.mSrc; }) {
        if (dispatchSlot(player, data.mSrc.get()))
            return;
    }
    if constexpr (requires { data.mDst; }) {
        (void)dispatchSlot(player, data.mDst.get());
    }
}

struct RequestCollector {
    std::vector<ContainerMenuManager::RequestSlot> slots;
    int                                            amount{1};

    void add(::ItemStackRequestSlotInfo const& slotInfo) {
        auto const ref = readSlot(slotInfo);
        slots.push_back(ContainerMenuManager::RequestSlot{ref.container, ref.containerId, ref.slot});
    }

    template <typename ActionDataT>
    void operator()(ActionDataT const& data) {
        if constexpr (requires { data.mSrc; })
            add(data.mSrc.get());
        if constexpr (requires { data.mDst; })
            add(data.mDst.get());
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
    using namespace ::debugshape_export::interaction_compat;
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
    else if constexpr (std::is_same_v<T, BeaconPaymentActionData>)
        return 10;
    else if constexpr (std::is_same_v<T, MineBlockActionData>)
        return 11;
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
        if constexpr (requires { data.mSrc; }) {
            auto const ref        = readSlot(data.mSrc.get());
            action.hasSrc         = true;
            action.srcContainer   = ref.container;
            action.srcContainerId = ref.containerId;
            action.srcSlot        = ref.slot;
            action.srcNetId       = netIdOf(data.mSrc.get());
        }
        if constexpr (requires { data.mDst; }) {
            auto const ref        = readSlot(data.mDst.get());
            action.hasDst         = true;
            action.dstContainer   = ref.container;
            action.dstContainerId = ref.containerId;
            action.dstSlot        = ref.slot;
            action.dstNetId       = netIdOf(data.mDst.get());
        }
        if constexpr (requires { data.mAmount; }) {
            action.amount = static_cast<int>(unwrap(data.mAmount));
        }

        actions.push_back(action);
    }
};

// 1.9.7 的 action 是 ItemStackRequestAction 的多态基类（不是 variant），
// 所以按具体类型 dynamic_cast 分发；成功返回 true。
template <class Fn>
bool visitAction(::ItemStackRequestAction const& action, Fn&& fn) {
    using namespace ::debugshape_export::interaction_compat;
    if (auto const* p = dynamic_cast<TakeActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    if (auto const* p = dynamic_cast<PlaceActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    if (auto const* p = dynamic_cast<SwapActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    if (auto const* p = dynamic_cast<DropActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    if (auto const* p = dynamic_cast<DestroyActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    if (auto const* p = dynamic_cast<ConsumeActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    if (auto const* p = dynamic_cast<CreateActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    if (auto const* p = dynamic_cast<BeaconPaymentActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    if (auto const* p = dynamic_cast<MineBlockActionData const*>(&action)) {
        fn(*p);
        return true;
    }
    return false;
}

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
    if (packet.mRequestBatch == nullptr)
        return origin(source, packet);

    for (auto const& request : packet.mRequestBatch->mRequests.get()) {
        if (request == nullptr)
            continue;

        RequestCollector     collector;
        TradeActionCollector trade;
        for (auto const& action : request->mActions.get()) {
            if (action == nullptr)
                continue;
            visitAction(*action, collector);
            visitAction(*action, trade);
        }
        if (ContainerMenuManager::getInstance().handleInteractiveRequest(
                player->getRealName(),
                collector.slots,
                collector.amount)) {
            claimed.push_back(request->mClientRequestId.get().mRawId);
            continue;
        }

        if (TradeMenuManager::getInstance().handleItemRequest(
                *player,
                trade.actions,
                request->mClientRequestId.get().mRawId)) {
            continue;
        }
        for (auto const& action : request->mActions.get()) {
            if (action == nullptr)
                continue;
            visitAction(*action, [&](auto const& data) {
                dispatchAction(*player, data);
            });
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
            visitAction(*actionPtr, collector);
            visitAction(*actionPtr, trade);
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
            visitAction(*actionPtr, [&](auto const& data) {
                dispatchAction(*player, data);
            });
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
