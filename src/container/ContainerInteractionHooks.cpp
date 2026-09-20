// ContainerInteractionHooks.cpp - 物品请求 / 容器关闭 的协议层路由（虚拟容器域）
//
// 三个钩子集中在这里, 全部服务于**虚拟容器（列表）**的点击与生命周期:
//   · ItemStackRequestPacket(147)          —— 点击的主通道（实测: 容器点击走这条）
//   · PlayerAuthInputPacket(144) 内嵌请求  —— 菜单交互的第二条通道（触屏常走这条）
//   · ContainerClosePacket                 —— 客户端关掉界面 → 拆菜单 + 恢复真方块
//
// **只观察、不拦**: 虚拟容器在服务端并不存在, 放行后 BDS 自己就会失败并让客户端把预测撤回
// （物品在界面上闪一下回到原位）, 回调照常收到 —— 与参考实现 GMLIB 完全一致。实测教训: 若这里
// 自己代答一条失败应答（ItemStackNetResult 3）, 客户端会**弹一个错误提示**; 交给 BDS 走它自己的
// 失败路径反而是安静的, 所以别"自作聪明"地代答。
//
// 交易菜单（TradeMenuManager）是**纯展示**, 不参与本文件的任何解析: 它不读客户端的点击/物品请求。
// 容器关闭那一条会顺带结束交易菜单（交易界面同样由 ContainerClose 收起）, 故两个域都出现在那里。
#include "container/ContainerMenuManager.h"

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

// 把一个槽位交给虚拟容器域。取物时槽位在 src, 往容器里放物时在 dst —— 都算"点了这个格子"。
// 命中与否由容器域自己按容器枚举 + 动态 id 区间判定; 返回 true = 这个动作属于本域的容器。
bool dispatchSlot(Player& player, ::ItemStackRequestCereal::SlotInfoData const& slotInfo) {
    auto const ref = readSlot(slotInfo);
    if (ref.container != static_cast<int>(ContainerEnumName::LevelEntityContainer)) return false;
    return ContainerMenuManager::getInstance().handleSlotAction(
        player.getRealName(),
        ref.container,
        ref.containerId,
        ref.slot
    );
}

template <typename ActionDataT>
void dispatchContainerAction(Player& player, ActionDataT const& data) {
    // src 认下了就不再试 dst —— 免得同一个动作回传两次
    if constexpr (requires { data.mSource; }) {
        if (dispatchSlot(player, data.mSource.get())) return;
    }
    if constexpr (requires { data.mDestination; }) {
        (void)dispatchSlot(player, data.mDestination.get());
    }
}

} // namespace

// ── ItemStackRequestPacket(147): 虚拟容器的点击主通道 ──
// 实测: 客户端对容器槽位的操作全部走这条包（从不经过 ComplexInventoryTransaction(30)）。
LL_TYPE_INSTANCE_HOOK(
    ContainerItemStackRequestHook,
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
    // 入口条件: 该玩家开着容器菜单。没开就整包放行, 一个字节都不解析（147 很吵）。
    if (!ContainerMenuManager::getInstance().hasMenuFor(player->getRealName())) {
        origin(source, packet);
        return;
    }

    for (auto const& request : packet.mRequests.get()) {
        for (auto const& action : request.mActions.get()) {
            std::visit([&](auto const& data) { dispatchContainerAction(*player, data); }, action);
        }
    }
    origin(source, packet); // 只回传、不拦, 理由见文件头
}

static ll::memory::HookRegistrar<ContainerItemStackRequestHook> gContainerItemStackRequestHookRegistrar;

// ── PlayerAuthInputPacket(144, AuthInput): 菜单交互的第二条通道 ──
// 客户端把物品请求发上来有两条路: 独立的 147, 以及搭在 AuthInput 里内嵌的 mItemStackRequest
// （PlayerAuthInputPacketPayload::mItemStackRequest, 输入标志 PerformItemStackRequest = 36）。
// BDS 的 ItemStackRequestCereal::toActionData() 能把它的解析态动作转成与 147 相同的 cereal 形态。
// **只观察、不拦**: 这个包同时承载玩家移动, 拦下它会把移动一起吞掉; 内嵌请求由 BDS 自己处理。
LL_TYPE_INSTANCE_HOOK(
    ContainerAuthInputHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&     source,
    PlayerAuthInputPacket const& packet
) {
    origin(source, packet); // 移动照常处理

    // 绝大多数 AuthInput 不带物品请求 —— 先做这个指针判断, 免得每 tick 都去查菜单
    if (packet.mItemStackRequest == nullptr) return;

    auto* player = findPlayerByNetworkId(source);
    if (player == nullptr) return;
    if (!ContainerMenuManager::getInstance().hasMenuFor(player->getRealName())) return;

    for (auto const& actionPtr : packet.mItemStackRequest->mActions.get()) {
        if (actionPtr != nullptr) {
            std::visit(
                [&](auto const& data) { dispatchContainerAction(*player, data); },
                ::ItemStackRequestCereal::toActionData(*actionPtr)
            );
        }
    }
}

static ll::memory::HookRegistrar<ContainerAuthInputHook> gContainerAuthInputHookRegistrar;

// ── ContainerClosePacket: 客户端关掉界面 ──
// 先问虚拟容器域（它要恢复真方块 + 回传 closed）; 不是它的容器再看交易域（交易界面同样由
// ContainerClose 结束, 那边只做"删载体 + 清记录"）。
LL_TYPE_INSTANCE_HOOK(
    ContainerCloseRouterHook,
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

static ll::memory::HookRegistrar<ContainerCloseRouterHook> gContainerCloseRouterHookRegistrar;

} // namespace debugshape_export
