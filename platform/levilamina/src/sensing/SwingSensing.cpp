#include "PlayerSensingManager.h"

#include "core/HookRegistry.h"
#include "kernel/PlayerIndex.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/math/Vec3.h>
#include <mc/network/NetworkIdentifier.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/AnimatePacket.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/ActorSwingSource.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/BlockPos.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/ShapeType.h>
#include <mc/world/phys/HitResult.h>
#include <mc/world/phys/HitResultType.h>

#include <atomic>
#include <deque>
#include <format>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace debugshape_export {

namespace {

// 方块判定射线长度：与生存模式方块交互距离一致（客户端视角 5 格）
constexpr float       kReachBlocks   = 5.0f;
constexpr std::size_t kMaxSwingQueue = 1024;
constexpr std::size_t kMaxLastPlayer = 4096;

struct SwingStore {
    std::mutex                                     mutex;
    std::unordered_map<std::string, std::uint64_t> counts;
    std::unordered_map<std::string, SwingEvent>    last;
    std::deque<SwingEvent>                         queue;
    std::atomic<std::uint64_t>                     total{0};
    std::atomic<std::uint64_t>                     dropped{0};
};

SwingStore& store() {
    static SwingStore instance;
    return instance;
}

// 优先走索引；插件热重载时可能错过在线玩家的 Join 事件，回退扫描一次并补登记
::Player* findPlayer(::NetworkIdentifier const& source) {
    auto& index = core::PlayerIndex::getInstance();
    if (auto* player = index.byNetworkId(source); player != nullptr)
        return player;

    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;

    ::Player* found = nullptr;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (player.getNetworkIdentifier() == source) {
            found = &player;
            return false;
        }
        return true;
    });
    if (found != nullptr)
        index.track(*found);
    return found;
}

std::uint64_t tickNow() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

struct BlockAim {
    bool hit{false};
    int  x{0};
    int  y{0};
    int  z{0};
    int  face{-1};
};

// 沿视线做一次方块射线（用引擎自己的 outline 形状，草丛/火把这类无碰撞体的方块不会误判）
BlockAim aimBlock(::Player& player) {
    BlockAim out;

    auto const eye = player.getEyePos();
    auto const dir = player.getViewVector();
    ::Vec3 const end{
        eye.x + dir.x * kReachBlocks,
        eye.y + dir.y * kReachBlocks,
        eye.z + dir.z * kReachBlocks,
    };

    ::HitResult const hit = player.getDimensionBlockSource().clip(
        eye,
        end,
        /*checkAgainstLiquid*/ false,
        ::ShapeType::Outline,
        static_cast<int>(kReachBlocks) + 1,
        /*ignoreBorderBlocks*/ false,
        /*fullOnly*/ false,
        &player,
        [](::BlockSource const&, ::Block const&, bool) { return true; },
        /*stopOnFirstLiquidHit*/ false);

    if (hit.mType != ::HitResultType::Tile)
        return out;

    out.hit  = true;
    out.x    = hit.mBlock.x;
    out.y    = hit.mBlock.y;
    out.z    = hit.mBlock.z;
    out.face = static_cast<int>(hit.mFacing);
    return out;
}

// 包里的 runtimeId → 实体 uniqueId；找不到就返回 0（协议不保证一定带 target）
std::uint64_t resolveUniqueId(std::uint64_t runtimeId) {
    if (runtimeId == 0)
        return 0;

    auto level = ll::service::getLevel();
    if (!level)
        return 0;

    for (auto* actor : level->getRuntimeActorList()) {
        if (actor == nullptr)
            continue;
        if (static_cast<std::uint64_t>(actor->getRuntimeID().rawID) != runtimeId)
            continue;
        return static_cast<std::uint64_t>(actor->getOrCreateUniqueID().rawID);
    }
    return 0;
}

SwingEvent makeSwingEvent(::Player& player, ::AnimatePacket const& packet) {
    SwingEvent ev;
    ev.playerName = player.getRealName();
    ev.tick       = tickNow();

    auto const& source = packet.mSwingSource.get();
    if (source.has_value()) {
        ev.hasSource = true;
        ev.source    = static_cast<std::uint8_t>(*source);
    }

    ev.runtimeId = static_cast<std::uint64_t>(packet.mRuntimeId.get().rawID);

    auto const eye  = player.getEyePos();
    ev.eyeX         = eye.x;
    ev.eyeY         = eye.y;
    ev.eyeZ         = eye.z;

    ::ItemStack const& held = player.getCarriedItem();
    if (!held.isNull()) {
        ev.itemName  = held.getTypeName();
        ev.itemCount = static_cast<int>(held.mCount);
    }

    // 先看客户端自报的来源：Attack 一定是打实体；其余（Build/Mine/Interact/UseItem…）
    // 再叠加一次射线，判断到底是"对着方块"还是"对着空气"。
    bool const attacking = ev.hasSource && ev.source == static_cast<std::uint8_t>(::ActorSwingSource::Attack);

    if (attacking || ev.runtimeId != 0) {
        ev.entityUniqueId = resolveUniqueId(ev.runtimeId);
        ev.target         = SwingTargetKind::Entity;
        return ev;
    }

    auto const aim = aimBlock(player);
    if (aim.hit) {
        ev.target = SwingTargetKind::Block;
        ev.blockX = aim.x;
        ev.blockY = aim.y;
        ev.blockZ = aim.z;
        ev.face   = aim.face;
    } else {
        ev.target = SwingTargetKind::Air;
    }
    return ev;
}

void recordSwing(::Player& player, ::AnimatePacket const& packet) {
    SwingEvent ev = makeSwingEvent(player, packet);

    auto& s = store();
    {
        std::lock_guard lock(s.mutex);
        ++s.counts[ev.playerName];
        if (s.last.size() >= kMaxLastPlayer)
            s.last.clear();
        s.last[ev.playerName] = ev;
        if (s.queue.size() >= kMaxSwingQueue) {
            s.queue.pop_front();
            s.dropped.fetch_add(1, std::memory_order_relaxed);
        }
        s.queue.push_back(std::move(ev));
    }
    s.total.fetch_add(1, std::memory_order_relaxed);
}

constexpr char const* sourceName(SwingEvent const& ev) {
    if (!ev.hasSource)
        return "unset";
    switch (static_cast<::ActorSwingSource>(ev.source)) {
    case ::ActorSwingSource::None:
        return "none";
    case ::ActorSwingSource::Build:
        return "build";
    case ::ActorSwingSource::Mine:
        return "mine";
    case ::ActorSwingSource::Interact:
        return "interact";
    case ::ActorSwingSource::Attack:
        return "attack";
    case ::ActorSwingSource::UseItem:
        return "useItem";
    case ::ActorSwingSource::ThrowItem:
        return "throwItem";
    case ::ActorSwingSource::DropItem:
        return "dropItem";
    case ::ActorSwingSource::Event:
        return "event";
    }
    return "unknown";
}

constexpr char const* targetName(SwingTargetKind kind) {
    switch (kind) {
    case SwingTargetKind::Air:
        return "air";
    case SwingTargetKind::Block:
        return "block";
    case SwingTargetKind::Entity:
        return "entity";
    default:
        return "unknown";
    }
}

} // namespace

// 客户端「挥一下」走服务端 AnimatePacket(action = Swing)——空手、挥剑、打空气、点方块、打实体都算。
// 这里只记录，不改变任何服务端行为，也不拦截包。
LL_TYPE_INSTANCE_HOOK(
    SwingPacketHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    ::NetworkIdentifier const& source,
    ::AnimatePacket const&     packet) {
    if (debugshape_export::core::live() && packet.mAction == ::AnimatePacket::Action::Swing) {
        if (auto* player = debugshape_export::findPlayer(source); player != nullptr) {
            // 监听是旁路功能：异常一律吞掉，绝不能把异常抛进 BDS 的收包路径
            try {
                debugshape_export::recordSwing(*player, packet);
            } catch (...) {
            }
        }
    }
    origin(source, packet);
}

static ll::memory::HookRegistrar<SwingPacketHook> gSwingPacketHook;
static debugshape_export::core::HookRegistrar     gHookReg_swing{
    "sensing-swing",
    [] {
        decltype(gSwingPacketHook)::hook();
    },
    [] {
        decltype(gSwingPacketHook)::unhook();
    },
};

std::uint64_t swingCount(std::string const& playerName) {
    auto&           s  = store();
    std::lock_guard lock(s.mutex);
    auto const      it = s.counts.find(playerName);
    return it != s.counts.end() ? it->second : 0;
}

std::uint64_t swingTotal() {
    return store().total.load(std::memory_order_relaxed);
}

std::uint64_t swingDropped() {
    return store().dropped.load(std::memory_order_relaxed);
}

void swingReset() {
    auto&           s = store();
    std::lock_guard lock(s.mutex);
    s.counts.clear();
    s.last.clear();
    s.queue.clear();
    s.dropped.store(0, std::memory_order_relaxed);
    s.total.store(0, std::memory_order_relaxed);
}

void forgetSwings(std::string const& playerName) {
    auto&           s = store();
    std::lock_guard lock(s.mutex);
    s.counts.erase(playerName);
    s.last.erase(playerName);
}

std::vector<SwingEvent> swingPoll() {
    auto& s = store();
    std::lock_guard lock(s.mutex);
    std::vector<SwingEvent> out;
    out.reserve(s.queue.size());
    while (!s.queue.empty()) {
        out.push_back(std::move(s.queue.front()));
        s.queue.pop_front();
    }
    return out;
}

void swingClearEvents() {
    auto&           s = store();
    std::lock_guard lock(s.mutex);
    s.queue.clear();
}

bool swingLastOf(std::string const& playerName, SwingEvent& out) {
    auto&           s = store();
    std::lock_guard lock(s.mutex);
    auto const      it = s.last.find(playerName);
    if (it == s.last.end())
        return false;
    out = it->second;
    return true;
}

std::string swingFormat(SwingEvent const& ev) {
    return std::format(
        "player={} source={} target={} pos={},{},{} face={} item={} count={} runtimeId={} uniqueId={} tick={}",
        ev.playerName,
        sourceName(ev),
        targetName(ev.target),
        ev.blockX,
        ev.blockY,
        ev.blockZ,
        ev.face,
        ev.itemName.empty() ? "none" : ev.itemName,
        ev.itemCount,
        ev.runtimeId,
        ev.entityUniqueId,
        ev.tick);
}

std::vector<std::string> swingFormatAll(std::vector<SwingEvent> const& events) {
    std::vector<std::string> out;
    out.reserve(events.size());
    for (auto const& ev : events)
        out.push_back(swingFormat(ev));
    return out;
}

} // namespace debugshape_export
