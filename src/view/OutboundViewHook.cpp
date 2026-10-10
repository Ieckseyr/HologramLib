// OutboundViewHook.cpp - 客户端视图覆盖的挂钩点（1.25.0）

#include "ViewOverrideManager.h"

#include "DiagLog.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/common/SubClientId.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/ContainerClosePacket.h>
#include <mc/network/packet/InteractPacket.h>
#include <mc/network/packet/InventoryTransactionPacket.h>
#include <mc/network/packet/ItemStackRequestPacket.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/network/packet/PlayerActionPacket.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/network/packet/PlayerAuthInputPacket.h>
#include <mc/network/packet/PlayerAuthInputPacketPayload.h>
#include <mc/network/packet/ActorEventPacketPayload.h>
#include <mc/network/packet/AnimatePacketPayload.h>
#include <mc/network/NetworkIdentifier.h>
#include <mc/world/actor/Actor.h>
#include <mc/network/NetworkIdentifierWithSubId.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/Packet.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <vector>
#include <exception>

namespace debugshape_export::view {

namespace {

using Manager = ViewOverrideManager;

Manager& manager() { return Manager::getInstance(); }

// TypedStorage 在小尺寸/对齐时可能退化成透明别名（没有 .get()）, 两种形态统一取值
template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return (value.get());
    } else {
        return (value);
    }
}

// 与库内其它域同源: 按网络标识找玩家
::Player* findPlayerByNetworkId(::NetworkIdentifier const& source) {
    auto level = ::ll::service::getLevel();
    if (!level) return nullptr;
    ::Player* found = nullptr;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (player.getNetworkIdentifier() == source) {
            found = &player;
            return false;
        }
        return true;
    });
    return found;
}

// 按 runtimeId 找实体（动作包只带 runtimeId）
::Actor* actorByRuntimeId(std::uint64_t runtimeId) {
    auto level = ::ll::service::getLevel();
    if (!level) return nullptr;
    return level->getRuntimeEntity(::ActorRuntimeID{runtimeId}, false);
}

// 收件人列表里有没有"有覆盖的玩家"（没有就整条包走原路, 一次展开都不做）
bool anyRecipientHasOverrides(std::vector<::NetworkIdentifierWithSubId> const& ids) {
    auto level = ::ll::service::getLevel();
    if (!level) return false;
    bool any = false;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (!manager().hasOverridesFor(player.getRealName())) return true;
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

// ── 钩子体的异常护栏 ──
// 这几个钩子都夹在 BDS 核心路径中间（发包汇合点 / 收包处理 / 世界 tick）。钩子体里抛出来的异常
// 本地没人接, 一路穿到 noexcept 边界就是 std::terminate → abort: 整服崩, 而它换来的只是
// "这一条包没改写对 / 这一拍心跳没做" —— 不划算, 所以全部兜底。
// 实测过一次: 同线程重入 mMutex 抛 std::system_error（见 ViewOverrideManager.cpp 的
// inputSnapshotOfLocked 注释）。返回 false = 改写没走完, 调用方按原路放行。
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

// ── 汇合点: 单收件人 ──
LL_TYPE_INSTANCE_HOOK(
    ViewNetSendHook,
    ll::memory::HookPriority::Normal,
    NetworkSystem,
    &NetworkSystem::send,
    void,
    ::NetworkIdentifier const& id,
    ::Packet const&            packet,
    ::SubClientId              recipientSubId
) {
    manager().noteHookCall(); // 诊断计数（不改变行为; 见 describeFor 的输出）
    if (!manager().active()) { // 快路径
        origin(id, packet, recipientSubId);
        return;
    }
    auto* player = findPlayerByNetworkId(id);
    if (player == nullptr) {
        origin(id, packet, recipientSubId);
        return;
    }

    bool       forwarded = false; // 原包是否已经放行（决定异常兜底时要不要补发）
    bool const ran       = runGuarded("ViewNetSendHook", [&] {
        Manager::Applied applied;
        if (!manager().applyToOutbound(*player, packet, applied)) { // 只读判定: 放行 / 丢弃
            origin(id, packet, recipientSubId);
            forwarded = true;
            return;
        }
        if (!applied.drop) {
            origin(id, packet, recipientSubId);
            forwarded = true;
        }
        manager().runPostActions(*player, applied);        // 原包之后才补发（顺序才对）
    });
    if (!ran && !forwarded) origin(id, packet, recipientSubId); // 改写中途抛了 → 原包照发, 不丢包
}

static ll::memory::HookRegistrar<ViewNetSendHook> gViewNetSendHook;

// ── 汇合点: 多收件人 —— 展开成逐个 send（重入上面的钩子, 逐收件人改写/还原）──
LL_TYPE_INSTANCE_HOOK(
    ViewNetSendToMultipleHook,
    ll::memory::HookPriority::Normal,
    NetworkSystem,
    &NetworkSystem::sendToMultiple,
    void,
    ::std::vector<::NetworkIdentifierWithSubId> const& ids,
    ::Packet const&                                    packet
) {
    manager().noteHookCall();
    bool dispatched = false; // 已经展开成逐收件人发（决定了中途异常时不能再走整包多播: 会重复）
    bool const ran  = runGuarded("ViewNetSendToMultipleHook", [&] {
        if (!manager().active() || !anyRecipientHasOverrides(ids)) {
            origin(ids, packet); // 没有任何收件人有覆盖 → 整条包原样走 BDS 自己的多播
            return;
        }
        dispatched = true;
        for (auto const& entry : ids) {
            this->send(entry.id, packet, entry.subClientId); // 重入 ViewNetSendHook（逐收件人判定）
        }
    });
    if (!ran && !dispatched) origin(ids, packet); // 展开前就抛了 → 按原样多播, 不丢包
}

static ll::memory::HookRegistrar<ViewNetSendToMultipleHook> gViewNetSendToMultipleHook;

// ── 心跳: 服务器每 tick 一次 ──
// 出生包不走按玩家发包函数（实测), 所以"实体重新进入视野 / 玩家换了区块"这两件事只能靠心跳兜:
// 心跳里发现"刚才看不见、现在看得见"就把覆盖重新套用一遍; 换了区块就把方块覆盖重推一遍。
// 这层与上面的钩子互为保险 —— 钩子命中时它是空转, 钩子没命中时它保证功能仍然成立。
LL_TYPE_INSTANCE_HOOK(
    ViewOverrideTickHook,
    ll::memory::HookPriority::Normal,
    Level,
    &Level::$tick,
    void
) {
    origin();
    runGuarded("ViewOverrideTickHook", [&] { manager().tickPulse(); });
}

static ll::memory::HookRegistrar<ViewOverrideTickHook> gViewOverrideTickHook;

// ── 入站: A 的输入包（PlayerAuthInput）—— "输入操作"的协议层真相 ──
// 位置/朝向/头顶朝向/输入位（跳跃、潜行、疾跑、游泳、爬行…）/界面状态（ClientPlayMode）全在这一包里。
// 代理复现 A 的动作以它为准: 客户端自己上报的东西, 比等服务端同步状态更贴"输入"本意, 也快一拍。
LL_TYPE_INSTANCE_HOOK(
    ViewAuthInputHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    ::NetworkIdentifier const&      source,
    ::PlayerAuthInputPacket const&  packet
) {
    origin(source, packet);
    runGuarded("ViewAuthInputHook", [&] {
        if (!manager().active()) return;
        auto* player = findPlayerByNetworkId(source);
        if (player == nullptr) return;

        auto const& payload = static_cast<::PlayerAuthInputPacketPayload const&>(packet);
        auto const& rawPos  = unwrap(payload.mPos);
        auto const& rawRot  = unwrap(payload.mRot);

        // bitset<66> → 低 64 位。不用 to_ullong(): 高位(64/65)置位时它会抛。
        std::uint64_t bits = 0;
        {
            auto const& bs = unwrap(payload.mInputData);
            for (int i = 0; i < 64; ++i) {
                if (bs[static_cast<std::size_t>(i)]) bits |= (std::uint64_t(1) << i);
            }
        }

        Manager::InputSnapshot snapshot;
        snapshot.uniqueId  = player->getOrCreateUniqueID().rawID; // 身份识别（不是读实体状态）
        snapshot.runtimeId = static_cast<std::uint64_t>(player->getRuntimeID());
        snapshot.x       = rawPos.x;
        snapshot.y       = rawPos.y;
        snapshot.z       = rawPos.z;
        snapshot.pitch   = rawRot.x; // BDS: Vec2{pitch, yaw}
        snapshot.yaw     = rawRot.y;
        snapshot.headYaw = unwrap(payload.mYHeadRot);
        snapshot.bits    = bits;
        snapshot.playMode = static_cast<int>(unwrap(payload.mPlayMode));
        manager().noteAuthInput(player->getRealName(), snapshot);
        manager().pushSubstitutedMovement(*player, snapshot); // 被替换玩家的位移推送（跳过本人）
    });
}

static ll::memory::HookRegistrar<ViewAuthInputHook> gViewAuthInputHook;


// ── 玩家下线: 清掉运行时 id 索引（运行时 id 换会话会撞上别的实体）──
void initViewOverrideEvents() {
    static ll::event::ListenerPtr listener = nullptr;
    if (listener != nullptr) return;
    listener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
        [](ll::event::PlayerDisconnectEvent& event) {
            auto& player = event.self();
            ViewOverrideManager::getInstance().onPlayerLeave(player.getRealName());
        }
    );
}

} // namespace debugshape_export::view
