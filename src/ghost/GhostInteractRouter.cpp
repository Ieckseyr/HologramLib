// GhostInteractRouter.cpp - ghost 交互事件路由实现
//
// hook: ServerNetworkHandler::$handle(NetworkIdentifier const&, InteractPacket const&)
// 路由: 目标 runtimeId 命中 0x6D/0x6E 段 → 反查库内 id → 派发监听器 + 入轮询队列
#include "GhostInteractRouter.h"

#include "customentity/CustomEntityManager.h"
#include "itemdisplay/ItemDisplayManager.h"
#include "lse/LseBridge.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/network/NetworkIdentifier.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/InteractPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {

// 库内 Actor runtimeId 段（与两 Manager 的分配段一致; 区间判别）
//   ItemDisplay: [0x6D000000, 0x6E000000)   CustomEntity: [0x6E000000, 0x6F000000)
constexpr std::uint64_t kItemDisplayRuntimeMin  = 0x6D000000ULL;
constexpr std::uint64_t kItemDisplayRuntimeMax  = 0x6E000000ULL;
constexpr std::uint64_t kCustomEntityRuntimeMin = 0x6E000000ULL;
constexpr std::uint64_t kCustomEntityRuntimeMax = 0x6F000000ULL;

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

// ── InteractPacket hook ──
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
    if (!isDisplay && !isEntity) return;

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

    // runtimeId -> 库内 id 反查（找不到 = 实体已销毁, 忽略迟到的交互）
    hologramlib::GhostInteractEvent ev;
    ev.playerName = actor->getRealName();
    ev.action     = static_cast<int>(packet.mAction);
    if (auto const& pos = packet.mPos.get()) {
        ev.hasPos = true;
        ev.x      = pos->x;
        ev.y      = pos->y;
        ev.z      = pos->z;
    }

    int64_t id = -1;
    if (isEntity && CustomEntityManager::getInstance().findByRuntimeId(runtimeId, id)) {
        ev.domain = "entity";
        ev.id     = id;
    } else if (isDisplay && ItemDisplayManager::getInstance().findByRuntimeId(runtimeId, id)) {
        ev.domain = "itemDisplay";
        ev.id     = id;
    } else {
        return; // 段内但已销毁
    }

    GhostInteractRouter::getInstance().dispatch(ev);
}

// hook 生命周期（静态注册; 无监听且无人轮询时仅做段前缀判别, 近零开销）
static ll::memory::HookRegistrar<GhostInteractHook> gGhostInteractHookRegistrar;

} // namespace debugshape_export
