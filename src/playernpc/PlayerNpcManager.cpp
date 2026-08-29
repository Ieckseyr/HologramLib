// PlayerNpcManager.cpp - 假玩家 NPC 管理器实现
//
// 架构对齐 ItemDisplayManager（自增/随机/指定 ID 三段隔离 + 脏刷新合并 respawn +
// 白名单过滤 + 事件驱动 spawn/despawn + tick hook 主线程统一发包）,
// 协议时序移植自 SCustomNpc: PlayerList(Add)→AddPlayer→[20t]PlayerList(Remove)
#include "PlayerNpcManager.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/service/Bedrock.h>

#include <ll/api/memory/Hook.h>
#include <mc/world/level/Level.h>

#include <format>
#include <random>

#include "NpcProtocol.h"
#include "NpcSkinRegistry.h"

namespace debugshape_export {

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

} // namespace

PlayerNpcManager& PlayerNpcManager::getInstance() {
    static PlayerNpcManager instance;
    return instance;
}

// ── 生命周期 ──

void PlayerNpcManager::init() {
    std::lock_guard lock(mMutex);
    if (mJoinListener) return; // 已初始化

    NpcSkinRegistry::getInstance().init();

    mJoinListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerJoinEvent>(
        [this](ll::event::PlayerJoinEvent& ev) {
            std::lock_guard lock(mMutex);
            mInitializedPlayers.insert(ev.self().getUuid());
            syncVisibilityLocked(); // 就绪立即补发（不等周期 sync）
        }
    );
    mDisconnectListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
        [this](ll::event::PlayerDisconnectEvent& ev) {
            std::lock_guard lock(mMutex);
            auto const& uuid = ev.self().getUuid();
            mInitializedPlayers.erase(uuid);
            for (auto& [id, rt] : mRuntimes) rt.shownPlayers.erase(uuid);
            for (auto& [id, removals] : mTabRemovals) {
                std::erase_if(removals, [&uuid](TabRemoval const& r) { return r.playerUuid == uuid; });
            }
        }
    );
}

void PlayerNpcManager::shutdown() {
    {
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
        mTabRemovals.clear();
        mVisibleFilter.clear();
        mInitializedPlayers.clear();
    }
    NpcSkinRegistry::getInstance().shutdown();
}

// ── 皮肤 ──

bool PlayerNpcManager::registerSkin(hologramlib::PlayerNpcSkin const& skin) {
    std::string error;
    if (!NpcSkinRegistry::getInstance().registerSkinFromPng(skin, error)) {
        logger().warn("[PlayerNpc] registerSkin failed: {}", error);
        return false;
    }
    return true;
}

bool PlayerNpcManager::captureSkin(std::string const& skinId, std::string const& playerName) {
    return NpcSkinRegistry::getInstance().captureSkin(skinId, playerName);
}

int PlayerNpcManager::importSkins(std::string const& dirPath) {
    std::string error;
    return NpcSkinRegistry::getInstance().importSkinsFromDir(dirPath, error);
}

bool PlayerNpcManager::getSkinBlob(std::string const& skinId, std::string& out) const {
    return NpcSkinRegistry::getInstance().getSkinBlob(skinId, out);
}

bool PlayerNpcManager::registerSkinFromBlob(std::string const& blob) {
    std::string error;
    return NpcSkinRegistry::getInstance().registerSkinFromBlob(blob, error);
}

bool PlayerNpcManager::hasSkin(std::string const& skinId) const {
    return NpcSkinRegistry::getInstance().hasSkin(skinId);
}

bool PlayerNpcManager::unregisterSkin(std::string const& skinId) {
    std::lock_guard lock(mMutex);
    if (skinReferencedLocked(skinId)) {
        logger().warn("[PlayerNpc] unregisterSkin '{}' rejected: still referenced by NPC", skinId);
        return false;
    }
    return NpcSkinRegistry::getInstance().unregisterSkin(skinId);
}

std::vector<std::string> PlayerNpcManager::getSkinIds() const {
    return NpcSkinRegistry::getInstance().getSkinIds();
}

bool PlayerNpcManager::skinReferencedLocked(std::string const& skinId) const {
    for (auto const& [id, cfg] : mConfigs) {
        if (cfg.skinId == skinId) return true;
    }
    return false;
}

// ── NPC 生命周期 ──

int64_t PlayerNpcManager::create(PlayerNpcConfig const& config) {
    std::lock_guard lock(mMutex);
    return createLocked(config, mNextId++);
}

int64_t PlayerNpcManager::createRandom(PlayerNpcConfig const& config) {
    // 随机 ID 段 [0x10000000,0x7FFFFFFF): 与自增段(从 1 起)长期隔离
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

int64_t PlayerNpcManager::createWithId(PlayerNpcConfig const& config, int64_t desiredId) {
    if (desiredId <= 0) return -2;
    std::lock_guard lock(mMutex);
    if (mConfigs.contains(desiredId)) return -2;
    if (desiredId >= mNextId && desiredId < 0x10000000) mNextId = desiredId + 1; // 防游标撞车
    return createLocked(config, desiredId);
}

int64_t PlayerNpcManager::createLocked(PlayerNpcConfig const& config, int64_t id) {
    if (!NpcSkinRegistry::getInstance().hasSkin(config.skinId)) return -3;

    Runtime rt{};
    rt.uniqueId  = mNextActorUniqueId++;
    rt.runtimeId = mNextRuntimeId++;

    mConfigs.emplace(id, config);
    mRuntimes.emplace(id, std::move(rt));

    syncVisibilityLocked();
    return id;
}

bool PlayerNpcManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;

    // 对所有已显示玩家发移除包（PlayerList Remove + RemoveActor）
    for (auto const& uuid : rit->second.shownPlayers) {
        if (auto* player = findPlayerByUuid(uuid)) {
            npc_protocol::remove(*player, id, rit->second.uniqueId);
        }
    }
    mRuntimes.erase(rit);
    mConfigs.erase(id);
    mTabRemovals.erase(id);
    mVisibleFilter.erase(id);
    mDirtyIds.erase(id);
    return true;
}

void PlayerNpcManager::destroyAll() {
    std::vector<int64_t> ids;
    {
        std::lock_guard lock(mMutex);
        ids.reserve(mConfigs.size());
        for (auto const& [id, cfg] : mConfigs) ids.push_back(id);
    }
    for (auto id : ids) destroy(id);
}

bool PlayerNpcManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mConfigs.contains(id);
}

bool PlayerNpcManager::get(int64_t id, PlayerNpcConfig& out) const {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    out = it->second;
    return true;
}

bool PlayerNpcManager::isIdUsed(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mConfigs.contains(id);
}

std::vector<int64_t> PlayerNpcManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    std::vector<int64_t> ids;
    ids.reserve(mConfigs.size());
    for (auto const& [id, cfg] : mConfigs) ids.push_back(id);
    return ids;
}

// ── 属性（setter 标脏, tick 内合并 respawn）──

bool PlayerNpcManager::setPosition(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    if (dim >= 0) it->second.dimension = dim;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setRotation(int64_t id, float yaw) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.yaw = yaw;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setNametag(int64_t id, std::string const& text) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.name = text;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setSkin(int64_t id, std::string const& skinId) {
    if (!NpcSkinRegistry::getInstance().hasSkin(skinId)) return false;
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.skinId = skinId;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setViewDistance(int64_t id, double dist) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.viewDistance = dist;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setEnabled(int64_t id, bool enabled) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.enabled = enabled;
    mDirtyIds.insert(id);
    return true;
}

// ── 可见玩家白名单 ──

bool PlayerNpcManager::setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) {
    std::lock_guard lock(mMutex);
    if (!mConfigs.contains(id)) return false;
    if (playerNames.empty()) {
        mVisibleFilter.erase(id);
    } else {
        mVisibleFilter[id] = std::unordered_set<std::string>(playerNames.begin(), playerNames.end());
    }
    syncVisibilityLocked();
    return true;
}

bool PlayerNpcManager::clearVisiblePlayers(int64_t id) {
    std::lock_guard lock(mMutex);
    if (!mConfigs.contains(id)) return false;
    mVisibleFilter.erase(id);
    syncVisibilityLocked();
    return true;
}

bool PlayerNpcManager::setVisiblePlayer(int64_t id, std::string const& playerName) {
    return setVisiblePlayers(id, std::vector<std::string>{playerName});
}

// ── 诊断 ──

std::string PlayerNpcManager::getDebugInfo(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto it  = mConfigs.find(id);
    auto rit = mRuntimes.find(id);
    if (it == mConfigs.end() || rit == mRuntimes.end()) return "not_found";
    auto const& cfg = it->second;
    auto const& rt  = rit->second;
    return std::format(
        "id={} name='{}' skin={} pos=({:.1f},{:.1f},{:.1f}) dim={} yaw={:.0f} view={} enabled={} "
        "uniqueId={:#x} runtimeId={:#x} shown={} tabPending={} filter={}",
        id,
        cfg.name,
        cfg.skinId,
        cfg.x,
        cfg.y,
        cfg.z,
        cfg.dimension,
        cfg.yaw,
        cfg.viewDistance,
        cfg.enabled,
        rt.uniqueId,
        rt.runtimeId,
        rt.shownPlayers.size(),
        mTabRemovals.contains(id) ? mTabRemovals.at(id).size() : 0,
        mVisibleFilter.contains(id) ? "custom" : "all"
    );
}

bool PlayerNpcManager::findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const {
    std::lock_guard lock(mMutex);
    for (auto const& [id, rt] : mRuntimes) {
        if (rt.runtimeId == runtimeId) {
            outId = id;
            return true;
        }
    }
    return false;
}

// ── 内部: respawn / 可见性 / 脏刷新 ──

void PlayerNpcManager::refreshLocked(int64_t id) {
    auto it  = mConfigs.find(id);
    auto rit = mRuntimes.find(id);
    if (it == mConfigs.end() || rit == mRuntimes.end()) return;

    auto& cfg = it->second;
    auto& rt  = rit->second;

    // despawn 全部已显示玩家 → 换新实体 ID → respawn（防客户端 ID 重映射串台, 与 ItemDisplay 一致）
    std::vector<Player*> toSpawn;
    bool                 replaced = false;
    for (auto const& uuid : rt.shownPlayers) {
        auto* player = findPlayerByUuid(uuid);
        if (player == nullptr) continue;
        npc_protocol::remove(*player, id, rt.uniqueId);
        replaced = true;
        if (cfg.enabled) toSpawn.push_back(player);
    }
    rt.shownPlayers.clear();

    if (replaced) {
        rt.uniqueId  = mNextActorUniqueId++;
        rt.runtimeId = mNextRuntimeId++;
    }

    for (auto* player : toSpawn) {
        sculk::protocol::SerializedSkin skin;
        if (!NpcSkinRegistry::getInstance().getSkin(cfg.skinId, skin)) continue;
        Vec3 pos{cfg.x, cfg.y, cfg.z};
        if (npc_protocol::spawnPlayerList(*player, id, rt.uniqueId, cfg.name, skin)
            && npc_protocol::spawnPlayerBody(*player, id, rt.runtimeId, rt.uniqueId, pos, cfg.yaw, cfg.name)) {
            rt.shownPlayers.insert(player->getUuid());
            mTabRemovals[id].push_back({player->getUuid(), currentTick() + 20});
        }
    }
}

void PlayerNpcManager::syncVisibilityLocked() {
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

            auto fit = mVisibleFilter.find(id);
            bool const allowedByFilter =
                fit == mVisibleFilter.end() || fit->second.contains(player.getRealName());

            auto const dx     = ppos.x - data.x;
            auto const dy     = ppos.y - data.y;
            auto const dz     = ppos.z - data.z;
            auto const distSq = dx * dx + dy * dy + dz * dz;

            bool const inView = data.viewDistance <= 0 || distSq <= data.viewDistance * data.viewDistance;
            // 滞回: 进入视距立即 spawn; 退出需超出 viewDistance+4（防边界抖动, 皮肤重发开销大）
            auto const hysteresis = (data.viewDistance > 0 ? data.viewDistance + 4.0 : 0.0);
            bool const outOfHysteresis =
                data.viewDistance <= 0 || distSq > hysteresis * hysteresis;

            bool const visible =
                data.enabled && allowedByFilter && dimId == DimensionType(data.dimension) && inView;

            if (visible && !rt.shownPlayers.contains(uuid)) {
                sculk::protocol::SerializedSkin skin;
                if (!NpcSkinRegistry::getInstance().getSkin(data.skinId, skin)) continue;
                Vec3 pos{data.x, data.y, data.z};
                if (npc_protocol::spawnPlayerList(player, id, rt.uniqueId, data.name, skin)
                    && npc_protocol::spawnPlayerBody(player, id, rt.runtimeId, rt.uniqueId, pos, data.yaw, data.name)) {
                    rt.shownPlayers.insert(uuid);
                    mTabRemovals[id].push_back({uuid, currentTick() + 20});
                }
            } else if (!visible && rt.shownPlayers.contains(uuid) && outOfHysteresis) {
                npc_protocol::remove(player, id, rt.uniqueId);
                rt.shownPlayers.erase(uuid);
            }
        }
        return true;
    });
}

void PlayerNpcManager::processDirtyLocked() {
    if (mDirtyIds.empty()) return;
    auto dirty = std::move(mDirtyIds);
    mDirtyIds.clear();

    for (auto id : dirty) {
        if (!mConfigs.contains(id)) continue;
        refreshLocked(id);
    }
    syncVisibilityLocked();
}

// ── Tick Hook：Tab 移除队列 + 脏刷新 + 周期同步 ──

struct PlayerNpcTickHookAccess {
    static void processDirty(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.processDirtyLocked();
    }
    static void sync(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.syncVisibilityLocked();
    }
    static void processTabRemovals(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        auto const now = currentTick();
        for (auto it = mgr.mTabRemovals.begin(); it != mgr.mTabRemovals.end();) {
            auto& removals = it->second;
            std::erase_if(removals, [&](PlayerNpcManager::TabRemoval const& r) {
                if (r.dueTick > now) return false;
                // 假玩家仍对该玩家可见: 移除 Tab 条目（皮肤已缓存, 实体持续渲染）
                auto rit = mgr.mRuntimes.find(it->first);
                if (rit != mgr.mRuntimes.end() && rit->second.shownPlayers.contains(r.playerUuid)) {
                    if (auto* player = findPlayerByUuid(r.playerUuid)) {
                        npc_protocol::removePlayerList(*player, it->first);
                    }
                }
                return true;
            });
            if (removals.empty()) it = mgr.mTabRemovals.erase(it);
            else ++it;
        }
    }
};

// 与 ItemDisplayTickHook 相同的挂点: Level::$tick 尾部, 主线程统一发包
LL_TYPE_INSTANCE_HOOK(PlayerNpcTickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();

    PlayerNpcTickHookAccess::processDirty(PlayerNpcManager::getInstance());
    PlayerNpcTickHookAccess::processTabRemovals(PlayerNpcManager::getInstance());

    static std::uint64_t lastSyncTick = 0;
    auto const           now          = currentTick();
    if (now - lastSyncTick >= 20) {
        lastSyncTick = now;
        PlayerNpcTickHookAccess::sync(PlayerNpcManager::getInstance());
    }
}

} // namespace debugshape_export
