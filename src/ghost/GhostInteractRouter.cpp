// GhostInteractRouter.cpp - ghost 交互事件路由实现
//
// hook 1: ServerNetworkHandler::$handle(NetworkIdentifier const&, InteractPacket const&)
//         （协议 944 起 InteractPacket 仅承载 Invalid/StopRiding/InteractUpdate/NpcOpen/OpenInventory,
//          实体右键/攻击已移出该包; 保留兜底）
// hook 2: ItemUseOnActorInventoryTransaction::$handle(Player&, bool)
//         （协议 944 实体交互/攻击的新载体, 经 InventoryTransactionPacket 内嵌到达）
// 路由: 目标 runtimeId 命中 0x6D/0x6E/0x6F 段 → 反查库内 id → 派发监听器 + 入轮询队列
#include "GhostInteractRouter.h"

#include "customentity/CustomEntityManager.h"
#include "itemdisplay/ItemDisplayManager.h"
#include "lse/LseBridge.h"
#include "playernpc/PlayerNpcManager.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/network/NetworkIdentifier.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/InteractPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/inventory/transaction/InventoryTransactionError.h>
#include <mc/world/inventory/transaction/ItemUseOnActorInventoryTransaction.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {

// 库内 Actor runtimeId 段（与三 Manager 的分配段一致; 区间判别）
//   ItemDisplay: [0x6D000000, 0x6E000000)   CustomEntity: [0x6E000000, 0x6F000000)
//   PlayerNpc:   [0x6F000000, 0x70000000)
constexpr std::uint64_t kItemDisplayRuntimeMin  = 0x6D000000ULL;
constexpr std::uint64_t kItemDisplayRuntimeMax  = 0x6E000000ULL;
constexpr std::uint64_t kCustomEntityRuntimeMin = 0x6E000000ULL;
constexpr std::uint64_t kCustomEntityRuntimeMax = 0x6F000000ULL;
constexpr std::uint64_t kPlayerNpcRuntimeMin    = 0x6F000000ULL;
constexpr std::uint64_t kPlayerNpcRuntimeMax    = 0x70000000ULL;

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

std::vector<hologramlib::GhostInteractEvent> GhostInteractRouter::poll() {
    std::lock_guard lock(mMutex);
    std::vector<hologramlib::GhostInteractEvent> out(mQueue.begin(), mQueue.end());
    mQueue.clear();
    return out;
}

void GhostInteractRouter::clearQueue() {
    std::lock_guard lock(mMutex);
    mQueue.clear();
}

void GhostInteractRouter::dispatch(hologramlib::GhostInteractEvent const& ev) {
    std::lock_guard lock(mMutex);
    // 队列有界（防无人轮询时无限膨胀; 满时丢最旧）
    constexpr std::size_t kMaxQueue = 256;
    if (mQueue.size() >= kMaxQueue) mQueue.pop_front();
    mQueue.push_back(ev);
    if (mListener) mListener(ev);
}

std::string GhostInteractRouter::formatEvent(hologramlib::GhostInteractEvent const& ev) {
    auto line = std::format(
        "player={} action={} domain={} id={}",
        ev.playerName,
        ev.action,
        ev.domain,
        ev.id
    );
    if (ev.hasPos) {
        line += std::format(" pos=({:.2f},{:.2f},{:.2f})", ev.x, ev.y, ev.z);
    }
    return line;
}

void GhostInteractRouter::exportLse() {
    static constexpr const char* NAMESPACE = "HologramLib";

    // ghostPollInteractions() -> [s]（取走并清空待处理交互队列; 条目格式见 formatEvent）
    hologramlib::lse::exportAs(NAMESPACE, "ghostPollInteractions", []() -> std::vector<std::string> {
        auto events = GhostInteractRouter::getInstance().poll();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events) out.push_back(formatEvent(ev));
        return out;
    });

    // ghostClearInteractions() -> void（丢弃队列中全部待处理事件）
    hologramlib::lse::exportAs(
        NAMESPACE, "ghostClearInteractions", []() -> void { GhostInteractRouter::getInstance().clearQueue(); });
}

// ── 公共路由: 段过滤 + runtimeId 反查 + 派发 ──
// action 语义与旧 InteractPacket 对齐（1=右键交互, 2=左键攻击）, 保证消费端契约不变
static void routeGhostInteract(
    std::string const& playerName,
    int                action,
    std::uint64_t      runtimeId,
    bool               hasPos,
    float              x,
    float              y,
    float              z
) {
    bool const isDisplay = runtimeId >= kItemDisplayRuntimeMin && runtimeId < kItemDisplayRuntimeMax;
    bool const isEntity  = runtimeId >= kCustomEntityRuntimeMin && runtimeId < kCustomEntityRuntimeMax;
    bool const isNpc     = runtimeId >= kPlayerNpcRuntimeMin && runtimeId < kPlayerNpcRuntimeMax;
    if (!isDisplay && !isEntity && !isNpc) return;

    // runtimeId -> 库内 id 反查（找不到 = 实体已销毁, 忽略迟到的交互）
    int64_t id = -1;
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
        return; // 段内但已销毁
    }

    GhostInteractRouter::getInstance().dispatch(ev);
}

// ── hook 1: InteractPacket（兜底; 协议 944 起该包不再承载实体点击/攻击）──
// BDS 对未知 runtimeId 的交互包无动作 → 恒调 origin, 不改变原版行为
LL_TYPE_INSTANCE_HOOK(
    GhostInteractHook,
    HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const& source,
    InteractPacket const&    packet
) {
    origin(source, packet);

    // 快速过滤: 目标不在库内 Actor ID 段（真实实体）→ 不产生事件
    auto const runtimeId = static_cast<std::uint64_t>(packet.mTargetId.get().rawID);
    bool const isDisplay = runtimeId >= kItemDisplayRuntimeMin && runtimeId < kItemDisplayRuntimeMax;
    bool const isEntity  = runtimeId >= kCustomEntityRuntimeMin && runtimeId < kCustomEntityRuntimeMax;
    bool const isNpc     = runtimeId >= kPlayerNpcRuntimeMin && runtimeId < kPlayerNpcRuntimeMax;
    if (!isDisplay && !isEntity && !isNpc) return;

    // 点击者解析（按 NetworkIdentifier 匹配在线玩家）
    Player* actor = nullptr;
    if (auto level = ll::service::getLevel()) {
        level->forEachPlayer([&](Player& p) {
            if (p.getNetworkIdentifier() == source) {
                actor = &p;
                return false; // 已找到, 终止遍历
            }
            return true;
        });
    }
    if (!actor) return;

    if (auto const& pos = packet.mPos.get()) {
        routeGhostInteract(
            actor->getRealName(),
            static_cast<int>(packet.mAction),
            runtimeId,
            true,
            static_cast<float>(pos->x),
            static_cast<float>(pos->y),
            static_cast<float>(pos->z)
        );
    } else {
        routeGhostInteract(actor->getRealName(), static_cast<int>(packet.mAction), runtimeId, false, 0, 0, 0);
    }
}

// ── hook 2: ItemUseOnActorInventoryTransaction（协议 944 实体交互/攻击主路径）──
// 客户端点击/攻击实体改经 InventoryTransactionPacket 内嵌本事务到达;
// BDS 对未知 runtimeId 的事务返回错误（不影响我们派发事件）→ 恒调 origin
LL_TYPE_INSTANCE_HOOK(
    GhostItemUseOnActorHook,
    HookPriority::Normal,
    ItemUseOnActorInventoryTransaction,
    &ItemUseOnActorInventoryTransaction::$handle,
    ::InventoryTransactionError,
    ::Player& player,
    bool      isSenderAuthority
) {
    auto const result = origin(player, isSenderAuthority);

    // action 映射（保持旧 InteractPacket 语义: 1=右键交互, 2=左键攻击）
    // ItemUseOnActor: Interact=0 / Attack=1 / ItemInteract=2（手持物品右键, 视作交互）
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
        static_cast<float>(hitPos.z)
    );
    return result;
}

// hook 生命周期（静态注册; 无监听且无人轮询时仅做段前缀判别, 近零开销）
static ll::memory::HookRegistrar<GhostInteractHook, GhostItemUseOnActorHook> gGhostInteractHookRegistrar;

} // namespace debugshape_export