// CustomEntityManager.cpp - 自定义实体协议层生成实现
//
// spawn 序列: AddActorPacket(identifier + metadata + attributes)
// despawn 序列: RemoveActorPacket
// 可见性: 玩家入服/每 20 tick 同步 + 视距/维度过滤（与 ItemDisplay 同模式）
#include "CustomEntityManager.h"

#include <random>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>

#include "SculkPacketSend.h"

#include <algorithm>
#include <string>

namespace debugshape_export {

// SculkPacketSend.h 的失败日志落点（模板内 extern 声明, 此处定义）
void logSculkPacketSendFailure(char const* name) {
    static auto log = ll::io::LoggerRegistry::getInstance().getOrCreate("HologramLib");
    log.warn("[CustomEntity] sculk packet validation failed for {}", name);
}

namespace {

auto& logger() {
    static auto log = ll::io::LoggerRegistry::getInstance().getOrCreate("HologramLib");
    return *log;
}

std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

Player* findPlayerByUuid(mce::UUID const& uuid) {
    auto level = ll::service::getLevel();
    if (!level) return nullptr;
    return level->getPlayer(uuid);
}

// identifier 规范化: 短名补 minecraft: 前缀; 空串返回空（创建时校验拒绝）
std::string normalizeIdentifier(std::string const& raw) {
    if (raw.empty()) return {};
    if (raw.find(':') != std::string::npos) return raw;
    return "minecraft:" + raw;
}

using Runtime = CustomEntityManager::Runtime;

// ── 发包原语 ──

void sendCustomActor(Player& player, CustomEntityConfig const& data, Runtime const& rt) {
    sculk::protocol::AddActorPacket pkt;
    pkt.mActorUniqueId  = static_cast<std::int64_t>(rt.uniqueId);
    pkt.mActorRuntimeId = rt.runtimeId;
    pkt.mIdentifier     = normalizeIdentifier(data.identifier);
    pkt.mPosition       = {data.x, data.y, data.z};
    pkt.mVelocity       = {0, 0, 0};
    pkt.mRotation       = {data.pitch, data.yaw}; // Vec2{pitch, yaw} 与 BDS ActorRotation 同序
    pkt.mYHeadRotation  = data.yaw;
    pkt.mYBodyRotation  = data.yaw;
    pkt.mActorLinks     = {};

    // flags 合成: 原始位掩码 | 隐身便捷位(0x20)
    std::int64_t const flags = data.flags | (data.invisible ? (std::int64_t(1) << 5) : 0);

    pkt.mMetaData.mDataItems.clear();
    pkt.mMetaData.mDataItems.push_back(
        {sculk::protocol::ActorDataIDs::Reserved0, flags}
    );
    if (!data.nametag.empty()) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::Name, data.nametag}
        );
        // 名字常显: 默认只在准星对准时显示, 全息/NPC 场景需要常显
        if (data.nametagAlwaysShow) {
            pkt.mMetaData.mDataItems.push_back(
                {sculk::protocol::ActorDataIDs::NametagAlwaysShow, static_cast<std::int32_t>(1)}
            );
        }
    }
    if (data.variant != 0) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::Variant, static_cast<std::int32_t>(data.variant)}
        );
    }
    if (data.markVariant != 0) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::MarkVariant, static_cast<std::int32_t>(data.markVariant)}
        );
    }
    if (data.colorIndex != 0) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::ColorIndex, static_cast<std::uint8_t>(data.colorIndex)}
        );
    }

    // attributes: health 基础 + minecraft:scale 缩放（客户端有效域 0.0625~10）
    float const scale = std::clamp(data.scale, 0.0625f, 10.0f);
    pkt.mAttributes = {
        {"minecraft:health", 0.f, 20.f, 20.f},
        {"minecraft:scale",  0.0625f, 10.f, scale},
    };
    pkt.mSynchedProperties = {};
    sendSculkPacketToPlayer(player, pkt);
}

void sendRemove(Player& player, Runtime const& rt) {
    sculk::protocol::RemoveActorPacket pkt;
    pkt.mActorUniqueId = static_cast<std::int64_t>(rt.uniqueId);
    sendSculkPacketToPlayer(player, pkt);
}

// ── 生成/销毁完整序列 ──

bool spawnForPlayer(int64_t id, CustomEntityConfig const& data, Runtime& rt, Player& player) {
    // Level 未就绪: 不缓存不告警, 下个 tick 重试
    if (!ll::service::getLevel()) return false;

    sendCustomActor(player, data, rt);
    logger().debug(
        "[CustomEntity] spawned #{} '{}' at ({:.1f},{:.1f},{:.1f}) dim={} scale={:.3f} tag='{}'",
        id,
        data.identifier,
        data.x,
        data.y,
        data.z,
        data.dimension,
        data.scale,
        data.nametag
    );
    return true;
}

void despawnForPlayer(Runtime const& rt, Player& player) { sendRemove(player, rt); }

} // namespace

// ── Manager 公开实现 ──

CustomEntityManager& CustomEntityManager::getInstance() {
    static CustomEntityManager instance;
    return instance;
}

void CustomEntityManager::init() {
    std::lock_guard lock(mMutex);
    if (mJoinListener) return; // 已初始化

    mJoinListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerJoinEvent>(
        [this](ll::event::PlayerJoinEvent& ev) {
            std::lock_guard lock(mMutex);
            mInitializedPlayers.insert(ev.self().getUuid());
        }
    );
    mDisconnectListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
        [this](ll::event::PlayerDisconnectEvent& ev) {
            std::lock_guard lock(mMutex);
            auto const& uuid = ev.self().getUuid();
            mInitializedPlayers.erase(uuid);
            for (auto& [id, rt] : mRuntimes) rt.shownPlayers.erase(uuid);
        }
    );
}

void CustomEntityManager::shutdown() {
    std::lock_guard lock(mMutex);
    if (mJoinListener) {
        ll::event::EventBus::getInstance().removeListener(mJoinListener);
        mJoinListener.reset();
    }
    if (mDisconnectListener) {
        ll::event::EventBus::getInstance().removeListener(mDisconnectListener);
        mDisconnectListener.reset();
    }
    mConfigs.clear();
    mRuntimes.clear();
    mDirtyIds.clear();
    mInitializedPlayers.clear();
}

int64_t CustomEntityManager::create(CustomEntityConfig const& config) {
    std::lock_guard lock(mMutex);
    return createLocked(config, mNextId++);
}

int64_t CustomEntityManager::createRandom(CustomEntityConfig const& config) {
    // 随机 ID 段 [0x10000000,0x7FFFFFFF): 与自增段(从 1 起)长期隔离, 不会撞车
    static std::mt19937_64 rng{std::random_device{}()};
    std::lock_guard       lock(mMutex);
    constexpr std::int64_t kBase = 0x10000000, kSpan = 0x70000000;
    for (int tries = 0; tries < 128; ++tries) {
        auto const id = kBase + static_cast<std::int64_t>(rng() % static_cast<std::uint64_t>(kSpan));
        if (mConfigs.contains(id)) continue;
        return createLocked(config, id);
    }
    return -1;
}

int64_t CustomEntityManager::createWithId(CustomEntityConfig const& config, int64_t desiredId) {
    if (desiredId <= 0) return -2;
    std::lock_guard lock(mMutex);
    if (mConfigs.contains(desiredId)) return -2;
    // 防自增游标未来撞上该 ID
    if (desiredId >= mNextId && desiredId < 0x10000000) mNextId = desiredId + 1;
    return createLocked(config, desiredId);
}

bool CustomEntityManager::isIdUsed(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mConfigs.contains(id);
}

int64_t CustomEntityManager::createLocked(CustomEntityConfig const& config, int64_t id) {
    // identifier 必须非空（客户端无类型可渲染的实体会被丢弃）
    if (config.identifier.empty()) {
        logger().warn("[CustomEntity] create rejected: empty identifier");
        return -1;
    }

    // 生成本次运行的唯一 Actor ID（0x6E 高位段, 与真实实体及 0x6D 物品域隔离）
    Runtime rt{};
    rt.uniqueId  = allocUniqueIdLocked();
    rt.runtimeId = allocRuntimeIdLocked();

    mConfigs.emplace(id, config);
    mRuntimes.emplace(id, std::move(rt));

    syncVisibilityLocked();
    return id;
}

std::uint64_t CustomEntityManager::allocUniqueIdLocked() { return mNextActorUniqueId++; }

std::uint64_t CustomEntityManager::allocRuntimeIdLocked() { return mNextRuntimeId++; }

bool CustomEntityManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);

    mDirtyIds.erase(id); // 已删除的实体不再参与 tick 脏刷新
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;

    auto uuids = std::vector<mce::UUID>{rit->second.shownPlayers.begin(), rit->second.shownPlayers.end()};
    for (auto const& uuid : uuids) {
        if (auto* player = findPlayerByUuid(uuid)) {
            despawnForPlayer(rit->second, *player);
        }
    }
    mRuntimes.erase(rit);
    mConfigs.erase(id);
    return true;
}

void CustomEntityManager::destroyAll() {
    std::lock_guard lock(mMutex);
    mDirtyIds.clear();
    for (auto& [id, rt] : mRuntimes) {
        auto uuids = std::vector<mce::UUID>{rt.shownPlayers.begin(), rt.shownPlayers.end()};
        for (auto const& uuid : uuids) {
            if (auto* player = findPlayerByUuid(uuid)) {
                despawnForPlayer(rt, *player);
            }
        }
    }
    mConfigs.clear();
    mRuntimes.clear();
}

bool CustomEntityManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mConfigs.contains(id);
}

bool CustomEntityManager::get(int64_t id, CustomEntityConfig& out) const {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    out = it->second;
    return true;
}

bool CustomEntityManager::setIdentifier(int64_t id, std::string const& identifier) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (identifier.empty()) return false; // 空 identifier 拒绝（客户端无法渲染）
    it->second.identifier = identifier;
    mDirtyIds.insert(id); // 类型变更需整体 respawn
    return true;
}

bool CustomEntityManager::setPosition(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    if (dim >= 0) it->second.dimension = dim;
    mDirtyIds.insert(id); // 位置/维度变化影响可见性, tick 脏刷新时统一重算
    return true;
}

bool CustomEntityManager::setRotation(int64_t id, float yaw, float pitch) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.yaw   = yaw;
    it->second.pitch = pitch;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setNametag(int64_t id, std::string const& text) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.nametag = text;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setScale(int64_t id, float scale) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (scale <= 0) return false; // 非法缩放拒绝（0 不可见）
    it->second.scale = scale;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setVariant(int64_t id, int variant) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.variant = variant;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setMarkVariant(int64_t id, int markVariant) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.markVariant = markVariant;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setColorIndex(int64_t id, int colorIndex) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.colorIndex = colorIndex;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setFlags(int64_t id, int64_t flags) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.flags = flags;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setInvisible(int64_t id, bool on) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (it->second.invisible == on) return true; // 幂等
    it->second.invisible = on;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setEnabled(int64_t id, bool enabled) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.enabled = enabled;
    mDirtyIds.insert(id); // 开关影响可见性, tick 脏刷新时统一重算
    return true;
}

bool CustomEntityManager::setViewDistance(int64_t id, double dist) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.viewDistance = dist;
    mDirtyIds.insert(id); // 视距影响可见性, tick 脏刷新时统一重算
    return true;
}

std::vector<int64_t> CustomEntityManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    std::vector<int64_t> ids;
    ids.reserve(mConfigs.size());
    for (auto const& [id, cfg] : mConfigs) ids.push_back(id);
    return ids;
}

int64_t CustomEntityManager::findNearest(float x, float y, float z, int dim, double maxDist) const {
    std::lock_guard lock(mMutex);
    int64_t best      = -1;
    double  bestDist2 = (maxDist > 0) ? maxDist * maxDist : 1e300;
    for (auto const& [id, cfg] : mConfigs) {
        if (cfg.dimension != dim) continue;
        double const dx    = cfg.x - x;
        double const dy    = cfg.y - y;
        double const dz    = cfg.z - z;
        double const dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            best      = id;
        }
    }
    return best;
}

void CustomEntityManager::refreshLocked(int64_t id) {
    // 参数变化后刷新所有已见玩家（despawn → respawn 原子替换）
    //
    // respawn 必须更换全新 uniqueId/runtimeId（与 ItemDisplay 同源结论）:
    // 客户端实体删除是帧末延迟销毁, 同帧/临近帧 RemoveActor(旧ID)+AddActor(同ID)
    // 会触发客户端 runtimeId 重映射未定义行为。换新 ID 后是纯粹的
    // "删旧实体 + 建全新实体", 无 ID 复用。
    auto it  = mConfigs.find(id);
    auto rit = mRuntimes.find(id);
    if (it == mConfigs.end() || rit == mRuntimes.end()) return;

    auto& rt    = rit->second;
    auto  uuids = std::vector<mce::UUID>{rt.shownPlayers.begin(), rt.shownPlayers.end()};

    std::vector<Player*> toSpawn;
    bool                 replaced = false;
    for (auto const& uuid : uuids) {
        auto* player = findPlayerByUuid(uuid);
        if (!player) {
            rt.shownPlayers.erase(uuid);
            continue;
        }
        despawnForPlayer(rt, *player);
        rt.shownPlayers.erase(uuid);
        replaced = true;
        if (it->second.enabled) toSpawn.push_back(player);
    }

    if (replaced) {
        rt.uniqueId  = allocUniqueIdLocked();
        rt.runtimeId = allocRuntimeIdLocked();
    }

    for (auto* player : toSpawn) {
        if (spawnForPlayer(id, it->second, rt, *player)) {
            rt.shownPlayers.insert(player->getUuid());
        }
    }
}

void CustomEntityManager::processDirtyLocked() {
    // tick 内合并处理脏实体: setter 只标脏, 本方法在 tick hook
    // (主线程) 统一执行, 同一 tick 无论几次 setter 调用都只 respawn 一次。
    if (mDirtyIds.empty()) return;
    auto dirty = std::move(mDirtyIds);
    mDirtyIds.clear();

    bool needVisibility = false;
    for (auto id : dirty) {
        if (!mConfigs.contains(id)) continue; // 中途被 destroy
        needVisibility = true;
        refreshLocked(id);
    }
    if (needVisibility) syncVisibilityLocked();
}

void CustomEntityManager::syncVisibilityLocked() {
    auto level = ll::service::getLevel();
    if (!level) return;

    level->forEachPlayer([this](Player& player) {
        auto const uuid = player.getUuid();
        if (!mInitializedPlayers.contains(uuid)) return true;

        auto const dimId = player.getDimensionId();
        auto const& ppos = player.getPosition();

        for (auto& [id, data] : mConfigs) {
            auto rit = mRuntimes.find(id);
            if (rit == mRuntimes.end()) continue;
            auto& rt = rit->second;

            bool const visible =
                data.enabled && dimId == DimensionType(data.dimension)
                && (data.viewDistance <= 0
                    || ((ppos.x - data.x) * (ppos.x - data.x) + (ppos.y - data.y) * (ppos.y - data.y)
                        + (ppos.z - data.z) * (ppos.z - data.z))
                           <= data.viewDistance * data.viewDistance);

            if (visible && !rt.shownPlayers.contains(uuid)) {
                if (spawnForPlayer(id, data, rt, player)) rt.shownPlayers.insert(uuid);
            } else if (!visible && rt.shownPlayers.contains(uuid)) {
                despawnForPlayer(rt, player);
                rt.shownPlayers.erase(uuid);
            }
        }
        return true;
    });
}

// ── Tick Hook：脏刷新 + 周期同步 ──

// Manager 私有访问桥（friend 声明于 Manager 头文件）
struct CustomEntityTickHookAccess {
    static void sync(CustomEntityManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.syncVisibilityLocked();
    }
    static void processDirty(CustomEntityManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.processDirtyLocked();
    }
};

LL_TYPE_INSTANCE_HOOK(CustomEntityTickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();

    // 脏刷新: 本 tick 内累积的 setter 变更合并为单次 respawn
    CustomEntityTickHookAccess::processDirty(CustomEntityManager::getInstance());

    auto const now = currentTick();
    static std::uint64_t lastSyncTick = 0;
    if (now - lastSyncTick >= 20) {
        lastSyncTick = now;
        CustomEntityTickHookAccess::sync(CustomEntityManager::getInstance());
    }
}

// hook 生命周期（静态注册即可; 无实体时为空转 no-op）
static ll::memory::HookRegistrar<CustomEntityTickHook> gCustomEntityTickHookRegistrar;

} // namespace debugshape_export