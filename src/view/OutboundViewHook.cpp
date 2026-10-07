// OutboundViewHook.cpp - 客户端视图覆盖的挂钩点（1.25.0）
//
// 出站（改写发给观看者的包）: NetworkSystem::send / sendToMultiple + Level::tick 心跳
//
// 挂的是 BDS 的**按收件人发包汇合点**:
//   NetworkSystem::send(id, packet, subId)        单收件人
//   NetworkSystem::sendToMultiple(ids, packet)    多收件人
//
// 为什么挂这里而不是 LoopbackPacketSender —— 反编译 BDS 后确认:
//   · NetworkSystem::send **自己负责序列化**（拼包头 + Packet::writeWithSerializationMode）,
//     所以结构化包在这一层**还没变成字节** —— 改字段依然有效, 序列化仍由 BDS 做;
//   · 它是 BDS 内部各条发包路径的共同落点（Level / ServerPlayer / 区块 / 实体出生 …）,
//     而 LoopbackPacketSender 只覆盖"经它转发"的那一部分 —— 实测实体出生包就没走它
//     （这正是"换类型不生效"的根因: 那一刻只有库自己补发的那几条包起了作用）。
//
// 三条纪律:
//   · **零开销快路径**: 全库没有任何覆盖（active() == false）→ 一行分支直接 origin, 不碰包
//   · **命中才动**: 目标玩家没有覆盖时 applyToOutbound 立刻返回 false, 一个字段都不读
//   · **原值必须还原**: sendToMultiple 是同一条包发给多个人, 改完不还就会串味 ——
//     多收件人变体展开成逐个 send（重入自身, 改写/还原/补发都在那一层完成）
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

// 包的载荷（Packet& → Payload<T>&）: 先降到 PayloadPacket<T>, 再升到 T
template <typename PayloadT>
PayloadT const& payloadOfConst(::Packet const& packet) {
    return static_cast<PayloadT const&>(static_cast<::ll::PayloadPacket<PayloadT> const&>(packet));
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

    Manager::Applied applied;
    auto&            mutablePacket = const_cast<::Packet&>(packet); // BDS 随后自己把它交给序列化器
    if (!manager().applyToOutbound(*player, mutablePacket, applied)) {
        origin(id, packet, recipientSubId);
        return;
    }
    if (!applied.drop) origin(id, packet, recipientSubId);
    manager().runPostActions(*player, applied);        // 原包之后才补发（顺序才对）
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
    if (!manager().active() || !anyRecipientHasOverrides(ids)) {
        origin(ids, packet); // 没有任何收件人有覆盖 → 整条包原样走 BDS 自己的多播
        return;
    }
    auto& mutablePacket = const_cast<::Packet&>(packet);
    for (auto const& entry : ids) {
        this->send(entry.id, mutablePacket, entry.subClientId); // 重入 ViewNetSendHook（逐收件人套用）
    }
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
    manager().tickPulse();
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
