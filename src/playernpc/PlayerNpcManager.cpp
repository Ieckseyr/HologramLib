// PlayerNpcManager.cpp - 假玩家 NPC 管理器实现

#include "EventIdCompat.h"

#include "DiagLog.h"

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

#include "PlayerNpcManager.h"

namespace debugshape_export {

namespace {

std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

Player* findPlayerByUuid(mce::UUID const& uuid) {
    auto level = ll::service::getLevel();
    if (!level) return nullptr;
    return level->getPlayer(uuid);
}

// realName → 在线玩家（与可见白名单同一匹配口径; 未找到返回 nullptr）
Player* findPlayerByName(std::string const& realName) {
    if (realName.empty()) return nullptr;
    auto level = ll::service::getLevel();
    if (!level) return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) {
        if (found) return true;
        if (p.getRealName() == realName) {
            found = &p;
            return false;
        }
        return true;
    });
    return found;
}

// 取该玩家应看到的朝向: 有 per-player 覆盖用覆盖值, 否则用 config 值
float effectiveYaw(PlayerNpcManager::Runtime const& rt, mce::UUID const& uuid, PlayerNpcConfig const& cfg) {
    auto it = rt.playerRot.find(uuid);
    return it == rt.playerRot.end() ? cfg.yaw : it->second.yaw;
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
            for (auto& [id, rt] : mRuntimes) {
                rt.shownPlayers.erase(uuid);
                rt.playerRot.erase(uuid); // 逐客户端朝向覆盖随玩家离线清理
            }
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
        mLightDirtyIds.clear();
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
        HLIB_LOG_WARN("[PlayerNpc] registerSkin failed: {}", error);
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
        HLIB_LOG_WARN("[PlayerNpc] unregisterSkin '{}' rejected: still referenced by NPC", skinId);
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

    // 不在这里立刻发包，而是标脏等 tick 末尾统一处理：
    // 消费方常常在同一 tick 里创建又销毁（例如先建再改），立刻发包会让客户端
    // 在几毫秒内收到多个"新玩家"，直接报错断线。合并后只会发出最终留下的那个。
    // 代价是可见性最多晚一个 tick（50ms）。
    mDirtyIds.insert(id);
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

// ── 1.20.0: 轻量朝向 / 逐客户端朝向 ──

bool PlayerNpcManager::setRotationLight(int64_t id, float yaw) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.yaw = yaw;
    mLightDirtyIds.insert(id); // 只发 MoveActorAbsolute, 不重建实体
    return true;
}

bool PlayerNpcManager::setPlayerRotation(int64_t id, std::string const& playerName, float yaw) {
    std::lock_guard lock(mMutex);
    if (!mConfigs.contains(id) || !mRuntimes.contains(id)) return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;
    auto& rt = mRuntimes[id];
    if (!rt.shownPlayers.contains(player->getUuid())) return false; // 该玩家还没见过这个 NPC
    rt.playerRot[player->getUuid()] = hologramlib::PerPlayerRotation{yaw, 0.0f};
    mLightDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::clearPlayerRotation(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;
    if (rit->second.playerRot.erase(player->getUuid()) == 0) return false;
    mLightDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::clearPlayerRotations(int64_t id) {
    std::lock_guard lock(mMutex);
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;
    if (rit->second.playerRot.empty()) return true;
    rit->second.playerRot.clear();
    mLightDirtyIds.insert(id);
    return true;
}

// 轻脏刷新: 只发朝向增量（MoveActorAbsolute; 逐玩家用各自覆盖朝向）, 不重建实体/不重发皮肤
void PlayerNpcManager::refreshLightLocked(int64_t id) {
    auto it  = mConfigs.find(id);
    auto rit = mRuntimes.find(id);
    if (it == mConfigs.end() || rit == mRuntimes.end()) return;
    auto const& cfg = it->second;
    auto&       rt  = rit->second;
    if (!cfg.enabled || rt.shownPlayers.empty()) return;

    Vec3 const pos{cfg.x, cfg.y, cfg.z};
    for (auto const& uuid : rt.shownPlayers) {
        auto* player = findPlayerByUuid(uuid);
        if (player == nullptr) continue;
        npc_protocol::move(*player, rt.runtimeId, pos, effectiveYaw(rt, uuid, cfg));
    }
}

bool PlayerNpcManager::setNametag(int64_t id, std::string const& text) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (it->second.name == text) return true; // PAPI 每秒重译相同文本: 不标脏, 避免无谓重建
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

bool PlayerNpcManager::setScale(int64_t id, float scale) {
    if (scale < 0.0625f || scale > 10.0f) return false;
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.scale = scale;
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

    // 刷新属性时不要发 PlayerList Remove 再重新 Add：短时间内对同一份皮肤做
    // Remove→Add，客户端会在皮肤还没落地时丢弃条目，导致断线。
    // 改为原地更新：先 RemoveActor 清掉旧实体，再用同 UUID 同皮肤覆盖 PlayerList，
    // 最后用新的 runtimeId 重新 AddPlayer。
    std::vector<Player*> toRefresh;
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* player = findPlayerByUuid(uuid)) toRefresh.push_back(player);
    }
    if (toRefresh.empty()) return;

    rt.runtimeId = mNextRuntimeId++;

    for (auto* player : toRefresh) {
        sculk::protocol::RemoveActorPacket rm;
        rm.mActorUniqueId = static_cast<std::int64_t>(rt.uniqueId);
        npc_protocol::sendToPlayer(*player, rm, NetworkPeer::Reliability::Reliable);
    }
    rt.shownPlayers.clear();

    for (auto* player : toRefresh) {
        sculk::protocol::SerializedSkin skin;
        if (!NpcSkinRegistry::getInstance().getSkin(cfg.skinId, skin)) continue;
        Vec3 pos{cfg.x, cfg.y, cfg.z};
        if (npc_protocol::spawnPlayerList(*player, id, rt.uniqueId, cfg.name, skin)
            && npc_protocol::spawnPlayerBody(
                *player,
                id,
                rt.runtimeId,
                rt.uniqueId,
                pos,
                effectiveYaw(rt, player->getUuid(), cfg), // 逐客户端朝向覆盖
                cfg.name,
                cfg.scale
            )) {
            rt.shownPlayers.insert(player->getUuid());
            mTabRemovals[id].push_back({player->getUuid(), currentTick() + 20});
        }
    }
}

void PlayerNpcManager::syncVisibilityLocked() {
    auto level = ll::service::getLevel();
    if (!level) return;

    // 每 tick 最多生成 2 个：多个 NPC 同时生成会让客户端一次收到数 MB 数据而断开
    int spawnedThisTick = 0;

    level->forEachPlayer([this, &spawnedThisTick](Player& player) {
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
                if (spawnedThisTick >= 2) return true; // 本 tick 名额已用完, 下个 tick 继续
                sculk::protocol::SerializedSkin skin;
                if (!NpcSkinRegistry::getInstance().getSkin(data.skinId, skin)) {
                    // 皮肤缺失会导致 NPC 永远不生成且无任何提示, 首次命中打 warn
                    if (!mWarnedMissingSkins.contains(data.skinId)) {
                        mWarnedMissingSkins.insert(data.skinId);
                        HLIB_LOG_WARN(
                            "[PlayerNpc] npc #{} 的皮肤 '{}' 未注册, 跳过生成 (请 registerSkin/采集后再试)",
                            id,
                            data.skinId
                        );
                    }
                    continue;
                }
                Vec3 pos{data.x, data.y, data.z};
                        // 旧版顺序：PlayerList → AddPlayer
                if (npc_protocol::spawnPlayerList(player, id, rt.uniqueId, data.name, skin)
                    && npc_protocol::spawnPlayerBody(
                        player,
                        id,
                        rt.runtimeId,
                        rt.uniqueId,
                        pos,
                        effectiveYaw(rt, uuid, data), // 逐客户端朝向覆盖
                        data.name,
                        data.scale
                    )) {
                    rt.shownPlayers.insert(uuid);
                    ++spawnedThisTick;
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
    // 轻脏: 只发朝向增量包（逐客户端朝向 / 跟踪式改朝向; 不重建实体）
    static void processLightDirty(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        if (mgr.mLightDirtyIds.empty()) return;
        auto light = std::move(mgr.mLightDirtyIds);
        mgr.mLightDirtyIds.clear();
        for (auto id : light) {
            if (!mgr.mConfigs.contains(id)) continue;
            mgr.refreshLightLocked(id);
        }
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
                // 不再按计划移除 Tab 条目：条目刚加入就移除会让客户端异常断线。
                // 假人就常驻在 Tab 列表里（多一个名字而已），实体销毁时自然一起移除。
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
    PlayerNpcTickHookAccess::processLightDirty(PlayerNpcManager::getInstance());
    PlayerNpcTickHookAccess::processTabRemovals(PlayerNpcManager::getInstance());

    static std::uint64_t lastSyncTick = 0;
    auto const           now          = currentTick();
    if (now - lastSyncTick >= 20) {
        lastSyncTick = now;
        PlayerNpcTickHookAccess::sync(PlayerNpcManager::getInstance());
    }
}

// ── BUGFIX: 缺失的 hook 注册器 ──
// 该 hook 此前从未注册（对比 CustomEntity/ItemDisplay/ParticleShape 均有 Registrar）,
// 导致: 视距动态裁剪/周期可见性同步/脏刷新(setViewDistance/setSkin 等)/Tab 移除队列
// 全部只在玩家进服或 NPC 创建时执行一次 —— 表现为"视距不生效"。
static ll::memory::HookRegistrar<PlayerNpcTickHook> gPlayerNpcTickHookRegistrar;

} // namespace debugshape_export
