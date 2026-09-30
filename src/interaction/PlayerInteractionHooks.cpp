// PlayerInteractionHooks.cpp - 物品请求 / 容器关闭 的协议层路由（虚拟容器 + 背包虚容器 + 真结算交易）
//
// 三个钩子集中在这里:
//   · ItemStackRequestPacket(147)          —— 点击的主通道（实测: 槽位操作都走这条）
//   · PlayerAuthInputPacket(144) 内嵌请求  —— 第二条通道（菜单交互、触屏常走这条）
//   · ContainerClosePacket                 —— 客户端关掉界面 → 容器域拆菜单 + 恢复真方块
//
// 派发目标（各域自己判命中, 互不抢）:
//   ContainerMenuManager  ← 容器枚举 LevelEntityContainer(7) 或动态 id 101..199（虚拟容器）
//   FakeInventoryManager  ← 容器枚举 12 / 28 / 29（玩家自己的背包 = 背包虚容器）
//   TradeMenuManager      ← 交易付费槽(31/32/47/48)上的转移 + 带自建配方 id 的 CraftRecipe
//                           （**只有开了 settleLocally 的菜单才认**; 否则交易域一个字都不读）
//
// **只观察、不拦**: 虚拟容器/伪造背包里的物品在服务端并不存在, 放行后 BDS 自己就会失败并让客户端
// 把预测撤回（物品在界面上闪一下回到原位）, 回调照常收到 —— 与参考实现 GMLIB 完全一致。实测教训:
// 若这里自己代答一条失败应答（ItemStackNetResult 3）, 客户端会**弹一个错误提示**; 交给 BDS 走它
// 自己的失败路径反而是安静的, 所以别"自作聪明"地代答。
// 例外是**真结算交易**: 那里的付费与产物是真要落地的（服务端没有交易容器可失败）, 所以由
// 交易域自己回成功应答并接管这条请求 —— 见 TradeMenuManager::handleItemRequest。
#include "container/ContainerMenuManager.h"
#include "container/ContainerResponse.h"
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
#include <mc/world/inventory/network/ItemStackNetIdVariant.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/containers/ContainerEnumName.h>
#include <mc/world/containers/FullContainerName.h>
#include <mc/world/level/Level.h>

#include <cstdint>
#include <variant>
#include <vector>

namespace debugshape_export {
namespace {

Player* findPlayerByNetworkId(NetworkIdentifier const& source) {
    auto level = ll::service::getLevel();
    if (!level) return nullptr;
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

// TypedStorage 在小尺寸/对齐时可能退化成透明别名（没有 .get()）, 两种形态统一取值
template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return value.get();
    } else {
        return (value);
    }
}

// 动作里的槽位信息（强类型直读: 包里的动作本来就是序列化数据结构, 不做运行期下转型）
struct SlotRef {
    int container{0};    // ContainerEnumName
    int containerId{-1}; // FullContainerName.mDynamicId（虚拟容器是 101..199）
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

// 槽位信息里的 netId（客户端为这次交互分配的物品网络 id; 回带槽位更正要用）
int netIdOf(::ItemStackRequestCereal::SlotInfoData const& slotInfo) {
    auto const& variant = unwrap(slotInfo.mNetIdVariant).mVariant.get();
    if (auto const* typed = std::get_if<::ItemStackNetId>(&variant)) {
        return static_cast<int>(typed->mRawId);
    }
    return 0;
}

// 把槽位交给两个域; 返回 true = 有域认下了（已回调）
bool dispatchSlot(Player& player, ::ItemStackRequestCereal::SlotInfoData const& slotInfo) {
    auto const ref = readSlot(slotInfo);
    if (ContainerMenuManager::getInstance().handleSlotAction(
            player.getRealName(),
            ref.container,
            ref.containerId,
            ref.slot
        )) {
        return true;
    }
    return FakeInventoryManager::getInstance().handleInventoryAction(player.getRealName(), ref.container, ref.slot);
}

template <typename ActionDataT>
void dispatchAction(Player& player, ActionDataT const& data) {
    // src 认下了就不再试 dst —— 免得同一个动作回传两次
    if constexpr (requires { data.mSource; }) {
        if (dispatchSlot(player, data.mSource.get())) return;
    }
    if constexpr (requires { data.mDestination; }) {
        (void)dispatchSlot(player, data.mDestination.get());
    }
}

// 一条请求里的槽位收集（src 优先, 无 src 取 dst）—— 可交互模式要"整条请求"才判断
struct RequestCollector {
    std::vector<ContainerMenuManager::RequestSlot> slots;
    int                                            amount{1};

    void add(::ItemStackRequestCereal::SlotInfoData const& slotInfo, bool isSource) {
        auto const ref = readSlot(slotInfo);
        slots.push_back(ContainerMenuManager::RequestSlot{ref.container, ref.containerId, ref.slot});
        (void)isSource;
    }

    template <typename ActionDataT>
    void operator()(ActionDataT const& data) {
        if constexpr (requires { data.mSource; }) add(data.mSource.get(), true);
        if constexpr (requires { data.mDestination; }) add(data.mDestination.get(), false);
        if constexpr (requires { data.mAmount; }) amount = static_cast<int>(data.mAmount);
    }
};

// 玩家是否开着任一交互界面 / 有伪造背包 / 有真结算交易（钩子入口条件; 都没开就整包放行, 一个字节都不解析）
bool playerHasInteraction(Player& player) {
    auto const& name = player.getRealName();
    return ContainerMenuManager::getInstance().hasMenuFor(name)
        || FakeInventoryManager::getInstance().isActive(name)
        || TradeMenuManager::getInstance().hasSettlingMenuFor(name);
}

// 交易域: 把一条请求的动作明细收集成 TradeRequestAction（真结算要"动作类型 + 配方 id"）
// 动作类型码只为日志好看; 分类只认容器枚举与配方 id（见 TradeSettlementLogic.h）
template <typename T>
constexpr int tradeActionTypeCode() {
    using namespace ::ItemStackRequestCereal;
    if constexpr (std::is_same_v<T, TakeActionData>) return 0;
    else if constexpr (std::is_same_v<T, PlaceActionData>) return 1;
    else if constexpr (std::is_same_v<T, SwapActionData>) return 2;
    else if constexpr (std::is_same_v<T, DropActionData>) return 3;
    else if constexpr (std::is_same_v<T, DestroyActionData>) return 4;
    else if constexpr (std::is_same_v<T, ConsumeActionData>) return 5;
    else if constexpr (std::is_same_v<T, CreateActionData>) return 6;
    else if constexpr (std::is_same_v<T, LabTableCombineActionData>) return 9;
    else if constexpr (std::is_same_v<T, BeaconPaymentActionData>) return 10;
    else if constexpr (std::is_same_v<T, MineBlockActionData>) return 11;
    else if constexpr (std::is_same_v<T, CraftRecipeActionData>) return 12;
    else if constexpr (std::is_same_v<T, CraftRecipeAutoActionData>) return 13;
    else if constexpr (std::is_same_v<T, CraftCreativeActionData>) return 14;
    else if constexpr (std::is_same_v<T, CraftRecipeOptionalActionData>) return 15;
    else if constexpr (std::is_same_v<T, CraftRepairAndDisenchantActionData>) return 16;
    else if constexpr (std::is_same_v<T, CraftLoomActionData>) return 17;
    else if constexpr (std::is_same_v<T, CraftNonImplementedActionData>) return 18;
    else if constexpr (std::is_same_v<T, CraftResultsActionData>) return 19;
    else return -1;
}

struct TradeActionCollector {
    std::vector<TradeMenuManager::TradeRequestAction> actions;

    template <typename ActionDataT>
    void operator()(ActionDataT const& data) {
        using T = std::decay_t<ActionDataT>;
        TradeMenuManager::TradeRequestAction action;
        action.type = tradeActionTypeCode<T>();
        if constexpr (requires { data.mSource; }) {
            auto const ref    = readSlot(data.mSource.get());
            action.hasSrc     = true;
            action.srcContainer   = ref.container;
            action.srcContainerId = ref.containerId;
            action.srcSlot        = ref.slot;
            action.srcNetId       = netIdOf(data.mSource.get());
        }
        if constexpr (requires { data.mDestination; }) {
            auto const ref    = readSlot(data.mDestination.get());
            action.hasDst     = true;
            action.dstContainer   = ref.container;
            action.dstContainerId = ref.containerId;
            action.dstSlot        = ref.slot;
            action.dstNetId       = netIdOf(data.mDestination.get());
        }
        if constexpr (requires { data.mAmount; }) {
            action.amount = static_cast<int>(unwrap(data.mAmount));
        }
        // CraftRecipe / CraftRecipeAuto / CraftRecipeOptional 三种动作都带 mRecipeNetId
        // —— "玩家点的是哪一条交易"的主信号。注意 CraftRepairAndDisenchantActionData 也有个同名
        // 成员, 但类型是 ItemStackNetIdVariant, 所以必须按具体类型分辨（不能用 requires）。
        if constexpr (
            std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeActionData>
            || std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeAutoActionData>
            || std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeOptionalActionData>
        ) {
            action.recipeNetId = static_cast<int>(unwrap(data.mRecipeNetId).mRawId);
        }
        actions.push_back(action);
    }
};

} // namespace

// ── ItemStackRequestPacket(147): 点击主通道 ──
LL_TYPE_INSTANCE_HOOK(
    PlayerInteractionItemStackRequestHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&      source,
    ItemStackRequestPacket const& packet
) {
    auto* player = findPlayerByNetworkId(source);
    if (player == nullptr) {
        origin(source, packet);
        return;
    }
    if (!playerHasInteraction(*player)) {
        origin(source, packet);
        return;
    }

    std::vector<std::int32_t> claimed; // 被可交互模式/真结算接住的请求 id
    for (auto const& request : packet.mRequests.get()) {
        // 先把整条请求的槽位收齐: 可交互模式要"整条请求都落在本容器"才敢接
        RequestCollector collector;
        TradeActionCollector trade;
        for (auto const& action : request.mActions.get()) {
            std::visit(collector, action);
            std::visit(trade, action);
        }
        if (ContainerMenuManager::getInstance().handleInteractiveRequest(
                player->getRealName(),
                collector.slots,
                collector.amount
            )) {
            claimed.push_back(request.mClientRequestId.get().mRawId);
            continue; // 这条我们接管了: 不回传(已回)、也不交给 BDS
        }
        // 真结算交易: 付费放置/取回/成交 —— 动了真背包, 必须自己结算并接管。应答由交易域自己发
        // （它要在应答里带上交易槽的**槽位更正** —— 交易界面吃这一套, 空 containers 会被撤回）。
        if (TradeMenuManager::getInstance().handleItemRequest(
                *player,
                trade.actions,
                request.mClientRequestId.get().mRawId
            )) {
            continue;
        }
        for (auto const& action : request.mActions.get()) {
            std::visit([&](auto const& data) { dispatchAction(*player, data); }, action);
        }
    }
    if (!claimed.empty()) {
        // 自己回成功应答（不回客户端会卡在预测态）; 这一包不再交给 BDS ——
        // 服务端没有这个容器, 交过去只会被它拒掉并把客户端预测撤回
        container::sendRequestSuccess(*player, claimed);
        return;
    }
    origin(source, packet); // 只回传、不拦, 理由见文件头
}

static ll::memory::HookRegistrar<PlayerInteractionItemStackRequestHook> gPlayerInteractionItemStackRequestHook;

// ── PlayerAuthInputPacket(144, AuthInput): 第二条通道 ──
// 内嵌的 mItemStackRequest（输入标志 PerformItemStackRequest = 36）; 用 BDS 自己的
// ItemStackRequestCereal::toActionData() 把解析态动作转成与 147 相同的 cereal 形态。
// **只观察、不拦**: 这个包同时承载玩家移动, 拦下会把移动一起吞掉; 内嵌请求由 BDS 自己处理。
LL_TYPE_INSTANCE_HOOK(
    PlayerInteractionAuthInputHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&     source,
    PlayerAuthInputPacket const& packet
) {
    origin(source, packet); // 移动照常处理

    // 绝大多数 AuthInput 不带物品请求 —— 先做这个指针判断, 免得每 tick 都去查状态
    if (packet.mItemStackRequest == nullptr) return;

    auto* player = findPlayerByNetworkId(source);
    if (player == nullptr) return;
    if (!playerHasInteraction(*player)) return;

    RequestCollector     collector;
    TradeActionCollector trade;
    for (auto const& actionPtr : packet.mItemStackRequest->mActions.get()) {
        if (actionPtr != nullptr) {
            std::visit(collector, ::ItemStackRequestCereal::toActionData(*actionPtr));
            std::visit(trade, ::ItemStackRequestCereal::toActionData(*actionPtr));
        }
    }
    if (ContainerMenuManager::getInstance().handleInteractiveRequest(
            player->getRealName(),
            collector.slots,
            collector.amount
        )) {
        // 认下: 回成功让客户端保留预测。**包本身照旧 origin** —— 它同时承载玩家移动, 拦下会把移动吞掉
        container::sendRequestSuccess(*player, {packet.mItemStackRequest->mClientRequestId.get().mRawId});
        return;
    }
    if (TradeMenuManager::getInstance().handleItemRequest(
            *player,
            trade.actions,
            packet.mItemStackRequest->mClientRequestId.get().mRawId
        )) {
        // 真结算交易同上: 包照旧放行（移动不能吞）; 应答由交易域自己发（带槽位更正）
        return;
    }
    for (auto const& actionPtr : packet.mItemStackRequest->mActions.get()) {
        if (actionPtr != nullptr) {
            std::visit(
                [&](auto const& data) { dispatchAction(*player, data); },
                ::ItemStackRequestCereal::toActionData(*actionPtr)
            );
        }
    }
}

static ll::memory::HookRegistrar<PlayerInteractionAuthInputHook> gPlayerInteractionAuthInputHook;

// ── ContainerClosePacket: 客户端关掉界面 ──
// 先问虚拟容器域（它要恢复真方块 + 回传 closed）; 不是它的容器再看交易域（交易界面同样由
// ContainerClose 结束, 那边只做"删载体 + 清记录"）。
LL_TYPE_INSTANCE_HOOK(
    PlayerInteractionContainerCloseHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&    source,
    ContainerClosePacket const& packet
) {
    origin(source, packet);

    auto* player = findPlayerByNetworkId(source);
    if (player == nullptr) return;
    auto const containerId = static_cast<int>(packet.mContainerId);
    if (ContainerMenuManager::getInstance().handleContainerClose(player->getRealName(), containerId)) {
        return;
    }
    auto& trade = TradeMenuManager::getInstance();
    if (!trade.hasMenuFor(player->getRealName())) return;
    trade.handleContainerClose(player->getRealName(), containerId);
}

static ll::memory::HookRegistrar<PlayerInteractionContainerCloseHook> gPlayerInteractionContainerCloseHook;

} // namespace debugshape_export
