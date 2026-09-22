// PlayerInteractionHooks.cpp - 物品请求 / 容器关闭 的协议层路由（虚拟容器 + 背包虚容器）
//
// 三个钩子集中在这里:
//   · ItemStackRequestPacket(147)          —— 点击的主通道（实测: 槽位操作都走这条）
//   · PlayerAuthInputPacket(144) 内嵌请求  —— 第二条通道（菜单交互、触屏常走这条）
//   · ContainerClosePacket                 —— 客户端关掉界面 → 容器域拆菜单 + 恢复真方块
//
// 派发目标（各域自己判命中, 互不抢）:
//   ContainerMenuManager  ← 容器枚举 LevelEntityContainer(7) 或动态 id 101..199（虚拟容器）
//   FakeInventoryManager  ← 容器枚举 12 / 28 / 29（玩家自己的背包 = 背包虚容器）
//   交易菜单（TradeMenuManager）是纯展示, 不读任何请求, 不在这里。
//
// **只观察、不拦**: 虚拟容器/伪造背包里的物品在服务端并不存在, 放行后 BDS 自己就会失败并让客户端
// 把预测撤回（物品在界面上闪一下回到原位）, 回调照常收到 —— 与参考实现 GMLIB 完全一致。实测教训:
// 若这里自己代答一条失败应答（ItemStackNetResult 3）, 客户端会**弹一个错误提示**; 交给 BDS 走它
// 自己的失败路径反而是安静的, 所以别"自作聪明"地代答。
#include "container/ContainerMenuManager.h"
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

// 玩家是否开着任一交互界面 / 有伪造背包（钩子入口条件; 都没开就整包放行, 一个字节都不解析）
bool playerHasInteraction(Player& player) {
    auto const& name = player.getRealName();
    return ContainerMenuManager::getInstance().hasMenuFor(name)
        || FakeInventoryManager::getInstance().isActive(name);
}

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

    for (auto const& request : packet.mRequests.get()) {
        for (auto const& action : request.mActions.get()) {
            std::visit([&](auto const& data) { dispatchAction(*player, data); }, action);
        }
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
