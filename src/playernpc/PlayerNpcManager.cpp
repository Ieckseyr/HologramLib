// PlayerNpcManager.cpp - 假玩家 NPC 管理器实现

#include "EventIdCompat.h"

#include "DiagLog.h"
#include "core/TickDriver.h" // 每 tick 驱动（全库唯一 Level::$tick hook）

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/service/Bedrock.h>

#include <ll/api/memory/Hook.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/world/level/Level.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/network/packet/MobArmorEquipmentPacket.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/ContainerID.h>
#include <mc/world/item/ItemStack.h>

#include <algorithm>
#include <format>
#include <random>

#include "NpcProtocol.h"
#include "NpcSkinRegistry.h"

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/level/MolangVersion.hpp>
#include <sculk/protocol/codec/packet/AnimateEntityPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorLinkPacket.hpp>

#include "SculkPacketSend.h"
#include "customentity/CustomEntityManager.h"

#include "PlayerNpcManager.h"
#include "../DiagFlags.h"

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

// ── 26.40.12: 姿势 / 装备 / 骑乘 ──

// 装备条目 → 原生 ItemStack（空 name = 空栈）
::ItemStack buildNpcEquip(PlayerNpcManager::Runtime::NpcEquip const& eq) {
    if (eq.name.empty()) return {};
    auto level = ll::service::getLevel();
    if (!level) return {};
    try {
        std::unique_ptr<::CompoundTag> tag;
        if (!eq.nbt.empty()) {
            if (auto parsed = ::CompoundTag::fromSnbt(eq.nbt)) {
                tag = std::make_unique<::CompoundTag>(std::move(*parsed));
            }
        }
        auto tryGet = [&](std::string const& name) -> ::ItemStack {
            auto weak = level->getItemRegistry().getItem(::HashedString(name));
            if (!weak) return {};
            ::ItemStack stack{};
            stack.reinit(*weak, 1, eq.aux);
            if (tag) stack.setUserData(std::make_unique<::CompoundTag>(*tag));
            return stack;
        };
        ::ItemStack stack = tryGet(eq.name);
        if (stack.isNull()) {
            auto normalized = eq.name;
            if (normalized.find(':') == std::string::npos) normalized = "minecraft:" + normalized;
            stack = tryGet(std::move(normalized));
        }
        return stack;
    } catch (...) {
        return {};
    }
}

// 主/副手 + 四甲整组下发（四甲必须一次发四槽, MobArmorEquipmentPacket 是整包）
void sendNpcEquipment(Player& player, PlayerNpcManager::Runtime const& rt) {
    for (int slot = 0; slot <= 1; ++slot) {
        auto stack = buildNpcEquip(rt.equip[slot]);
        if (stack.isNull()) continue;
        auto cid = (slot == 1) ? ::ContainerID::Offhand : ::ContainerID::Inventory;
        ::MobEquipmentPacket pkt{::ActorRuntimeID{rt.runtimeId}, stack, static_cast<unsigned char>(slot), 0, cid};
        pkt.sendTo(player);
    }
    auto head  = buildNpcEquip(rt.equip[2]);
    auto torso = buildNpcEquip(rt.equip[3]);
    auto legs  = buildNpcEquip(rt.equip[4]);
    auto feet  = buildNpcEquip(rt.equip[5]);
    if (!head.isNull() || !torso.isNull() || !legs.isNull() || !feet.isNull()) {
        ::MobArmorEquipmentPacket armorPkt;
        armorPkt.mRuntimeId = ::ActorRuntimeID{rt.runtimeId};
        std::destroy_at(&armorPkt.mHead);
        std::construct_at(&armorPkt.mHead, head);
        std::destroy_at(&armorPkt.mTorso);
        std::construct_at(&armorPkt.mTorso, torso);
        std::destroy_at(&armorPkt.mLegs);
        std::construct_at(&armorPkt.mLegs, legs);
        std::destroy_at(&armorPkt.mFeet);
        std::construct_at(&armorPkt.mFeet, feet);
        armorPkt.sendTo(player);
    }
}

// 姿势: PoseIndex(79) + 对应的 ActorFlags 位（玩家模型的姿态由客户端按这两项选动画）
void sendNpcPose(Player& player, PlayerNpcManager::Runtime const& rt) {
    if (rt.pose == 0 && rt.flags == 0) return;
    std::int64_t flags = rt.flags;
    if (rt.pose == 1) flags |= (std::int64_t(1) << 1);   // ActorFlags::Sneaking
    if (rt.pose == 3) flags |= (std::int64_t(1) << 57);  // ActorFlags::Swimming
    // 注意: ActorFlags::Sleeping = 76 —— 超过 64 位掩码范围, Reserved0 放不下,
    // 只靠 PoseIndex=2 表达睡姿（不要写 1<<76, 那是 UB）。

    sculk::protocol::SetActorDataPacket pkt;
    pkt.mActorRuntimeId    = rt.runtimeId;
    pkt.mSynchedProperties = {};
    pkt.mTick              = currentTick();
    pkt.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0, flags},
        {sculk::protocol::ActorDataIDs::PoseIndex, static_cast<std::int32_t>(rt.pose)},
    };
    sendSculkPacketToPlayer(player, pkt);
}

// 骑乘链接: A=载具 uniqueId, B=本 NPC uniqueId（乘客）
void sendNpcRide(Player& player, PlayerNpcManager::Runtime const& rt) {
    if (rt.rideVehicleUniqueId == 0) return;
    sculk::protocol::SetActorLinkPacket pkt;
    pkt.mLink.mType                   = sculk::protocol::ActorLinkType::Riding;
    pkt.mLink.mTargetAUniqueId        = rt.rideVehicleUniqueId;
    pkt.mLink.mTargetBUniqueId        = static_cast<std::int64_t>(rt.uniqueId);
    pkt.mLink.mImmediate              = true;
    pkt.mLink.mPassengerInitiated     = false;
    pkt.mLink.mVehicleAngularVelocity = 0.0f;
    sendSculkPacketToPlayer(player, pkt);
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
            if (hlibDomainDisabled("HLIB_NO_NPC")) return;
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

bool PlayerNpcManager::registerSkinOpaque(hologramlib::PlayerNpcSkin const& skin) {
    std::string error;
    if (!NpcSkinRegistry::getInstance().registerSkinFromPng(skin, error, true)) {
        HLIB_LOG_WARN("[PlayerNpc] registerSkinOpaque failed: {}", error);
        return false;
    }
    return true;
}

bool PlayerNpcManager::registerImageSkin(std::string const& pngPath, std::string const& skinId, float sizeBlocks, bool transparent) {
    if (sizeBlocks <= 0.0f) sizeBlocks = 1.0f;
    float const w = 8.0f * sizeBlocks;
    float const h = 16.0f * sizeBlocks;
    // 几何 identifier 必须按尺寸唯一：客户端按 identifier 缓存几何，复用同一个会让 1x1/2x2 串形。
    std::string const geoId = std::format("geometry.hlib_image_{}", static_cast<int>(sizeBlocks * 10.0f + 0.5f));

    static constexpr char kTemplate[] =
        R"({"format_version":"1.14.0","minecraft:geometry":[{"description":{"identifier":"%ID%","texture_width":128,"texture_height":128},"bones":[{"name":"root"},{"name":"body","parent":"root","pivot":[0,0,0],"poly_mesh":{"normalized_uvs":true,"positions":[[-%W%,0,-0.01],[%W%,0,-0.01],[%W%,%H%,-0.01],[-%W%,%H%,-0.01],[%W%,0,0.01],[-%W%,0,0.01],[-%W%,%H%,0.01],[%W%,%H%,0.01]],"normals":[[0,0,-1],[0,0,-1],[0,0,-1],[0,0,-1],[0,0,1],[0,0,1],[0,0,1],[0,0,1]],"uvs":[[0,0],[1,0],[1,1],[0,1],[1,0],[0,0],[0,1],[1,1]],"polys":[[[0,0,0],[1,0,1],[2,0,2],[3,0,3]],[[4,1,4],[5,1,5],[6,1,6],[7,1,7]]]}}]}]})";

    auto const num = [](float v) { return std::format("{:.6g}", v); };
    std::string geo = kTemplate;
    auto replaceAll = [&](std::string_view key, std::string_view val) {
        for (auto pos = geo.find(key); pos != std::string::npos; pos = geo.find(key, pos + val.size())) {
            geo.replace(pos, key.size(), val);
        }
    };
    replaceAll("%ID%", geoId);
    replaceAll("%W%", num(w));
    replaceAll("%H%", num(h));

    hologramlib::PlayerNpcSkin skin;
    skin.pngPath      = pngPath;
    skin.skinId       = skinId;
    skin.armSize      = "wide";
    skin.geometryData = std::move(geo);
    std::string error;
    bool const ok = NpcSkinRegistry::getInstance().registerSkinFromPng(skin, error, false, transparent ? 1 : 2);
    if (!ok) HLIB_LOG_WARN("[PlayerNpc] registerImageSkin failed: {}", error);
    return ok;
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
    constexpr std::int64_t kBase = core::actorIds::randomIdBase, kSpan = core::actorIds::randomIdSpan;
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
    if (hlibDomainDisabled("HLIB_NO_NPC")) return -1;
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
    for (auto it = mAnimQueue.begin(); it != mAnimQueue.end();) {
        if (it->second.npcId == id) it = mAnimQueue.erase(it);
        else ++it;
    }
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

// ── 1.26.0: 轻量位置更新 + 玩家皮肤注入 ──

bool PlayerNpcManager::setPositionLight(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    // 跨维度不走轻量通道: 客户端那边是另一张实体表, 必须重建（由调用方用 setPosition）
    if (dim >= 0 && dim != it->second.dimension) return false;
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    mLightDirtyIds.insert(id); // 只发 MoveActorAbsolute, 不重建实体、不重发皮肤
    return true;
}

bool PlayerNpcManager::injectSkin(
    std::string const& viewerName,
    std::string const& targetName,
    std::string const& skinId
) {
    // 皮肤条目: 取自注册表（Id/FullId/trust 三态/OverridesPlayerAppearance 都已就绪）
    sculk::protocol::SerializedSkin skin;
    if (!NpcSkinRegistry::getInstance().getSkin(skinId, skin)) return false;

    Player* target = findPlayerByName(targetName);
    if (target == nullptr) return false; // 目标必须在线（uuid/uniqueId/名字取自本人）

    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
    packet.mPlayerEntryList = {npc_protocol::playerListEntryForPlayer(*target, skin)};

    // 单发（只给某个观看者; 常用于"我自己看我自己"）
    if (!viewerName.empty()) {
        Player* viewer = findPlayerByName(viewerName);
        if (viewer == nullptr) return false;
        return npc_protocol::sendToPlayer(*viewer, packet, NetworkPeer::Reliability::Reliable);
    }

    // 全体（含目标本人 —— 这正是"让玩家自己看到自己被换肤"的那一份）
    auto level = ll::service::getLevel();
    if (!level) return false;
    bool any = false;
    level->forEachPlayer([&](Player& p) {
        if (npc_protocol::sendToPlayer(p, packet, NetworkPeer::Reliability::Reliable)) any = true;
        return true;
    });
    return any;
}

bool PlayerNpcManager::injectSkinAll(std::string const& targetName, std::string const& skinId) {
    return injectSkin(std::string{}, targetName, skinId);
}

// ── 1.26.0: 动画（AnimateEntityPacket）──

static std::string npcAnimControllerName(int64_t id) { return "hololib.playernpc." + std::to_string(id); }

// ── 26.40.12: 动画取消配方（与 CustomEntityManager 同一套）──
// "query.any_animation" 不是合法 Molang 查询 → 当 0 处理, 旧停止包实际停不掉;
// 改为同 controller 换播空动作 + stopExpression=1。
constexpr char const* kStopAnimation = "animation.player.attack.positions";
constexpr char const* kStopExpr      = "1";

static bool isLegacyStopExpression(std::string const& e) {
    return e == "query.any_animation" || e == "query.any_animation()";
}

bool PlayerNpcManager::playAnimation(
    int64_t id,
    std::string const& animation,
    std::string const& stopExpression,
    int durationTicks
) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end() || animation.empty()) return false;
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end() || !it->second.enabled || rit->second.shownPlayers.empty()) return false;

    auto const now  = currentTick();
    auto const ctrl = npcAnimControllerName(id);
    auto const rt   = rit->second.runtimeId;
    if (isLegacyStopExpression(stopExpression)) {
        for (auto const& uuid : rit->second.shownPlayers) {
            mAnimQueue.emplace(now + 2, AnimEntry{uuid, rt, id, kStopAnimation, ctrl, kStopExpr});
        }
        return true;
    }
    for (auto const& uuid : rit->second.shownPlayers) {
        mAnimQueue.emplace(now + 2, AnimEntry{uuid, rt, id, animation, ctrl, stopExpression});
        if (durationTicks > 0) {
            mAnimQueue.emplace(
                now + 2 + static_cast<std::uint64_t>(durationTicks),
                AnimEntry{uuid, rt, id, kStopAnimation, ctrl, kStopExpr}
            );
        }
    }
    return true;
}

bool PlayerNpcManager::playAnimationTo(
    int64_t id,
    std::string const& playerName,
    std::string const& animation,
    std::string const& stopExpression,
    int durationTicks
) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end() || animation.empty()) return false;
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end() || !it->second.enabled) return false;

    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;
    auto const uuid = player->getUuid();
    if (!rit->second.shownPlayers.contains(uuid)) return false;

    auto const now  = currentTick();
    auto const ctrl = npcAnimControllerName(id);
    if (isLegacyStopExpression(stopExpression)) {
        mAnimQueue.emplace(now + 2, AnimEntry{uuid, rit->second.runtimeId, id, kStopAnimation, ctrl, kStopExpr});
        return true;
    }
    mAnimQueue.emplace(now + 2, AnimEntry{uuid, rit->second.runtimeId, id, animation, ctrl, stopExpression});
    if (durationTicks > 0) {
        mAnimQueue.emplace(
            now + 2 + static_cast<std::uint64_t>(durationTicks),
            AnimEntry{uuid, rit->second.runtimeId, id, kStopAnimation, ctrl, kStopExpr}
        );
    }
    return true;
}

bool PlayerNpcManager::stopAnimation(int64_t id) {
    std::lock_guard lock(mMutex);
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end() || rit->second.shownPlayers.empty()) return false;
    auto const now  = currentTick();
    auto const ctrl = npcAnimControllerName(id);
    for (auto const& uuid : rit->second.shownPlayers) {
        mAnimQueue.emplace(now + 2, AnimEntry{uuid, rit->second.runtimeId, id, kStopAnimation, ctrl, kStopExpr});
    }
    return true;
}

bool PlayerNpcManager::stopAnimationTo(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;
    auto const uuid = player->getUuid();
    if (!rit->second.shownPlayers.contains(uuid)) return false;
    mAnimQueue.emplace(
        currentTick() + 2,
        AnimEntry{uuid, rit->second.runtimeId, id, kStopAnimation, npcAnimControllerName(id), kStopExpr}
    );
    return true;
}

void PlayerNpcManager::setEntitySpawnCallback(std::function<void(int64_t, std::string const&)> callback) {
    std::lock_guard lock(mMutex);
    mSpawnCallback = std::move(callback);
}

// ── 26.40.12: 姿势 / 装备 / 骑乘 ──

bool PlayerNpcManager::setPose(int64_t id, int pose) {
    std::lock_guard lock(mMutex);
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;
    auto& rt = rit->second;
    rt.pose  = std::clamp(pose, 0, 13);
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* p = findPlayerByUuid(uuid)) sendNpcPose(*p, rt);
    }
    return true;
}

bool PlayerNpcManager::setEquipment(
    int64_t id, int slot, std::string const& name, int aux, std::string const& nbt
) {
    if (slot < 0 || slot > 5) return false;
    std::lock_guard lock(mMutex);
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;
    auto& rt = rit->second;
    rt.equip[slot] = Runtime::NpcEquip{name, aux, nbt};
    if (name.empty()) {
        // 清空槽位: 空栈的 MobEquipment 不可靠 → 走一次 respawn（新实体只发非空槽）
        mDirtyIds.insert(id);
        return true;
    }
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* p = findPlayerByUuid(uuid)) sendNpcEquipment(*p, rt);
    }
    return true;
}

bool PlayerNpcManager::setRideUniqueId(int64_t id, std::int64_t vehicleUniqueId) {
    std::lock_guard lock(mMutex);
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;
    auto& rt = rit->second;
    if (vehicleUniqueId == static_cast<std::int64_t>(rt.uniqueId)) return false; // 不能骑自己

    // 清除: 发一条 type=None 的链接包（不 respawn —— respawn 重发 PlayerList/AddPlayer
    // 在"正骑着东西"时最容易把客户端打崩, 这里只解链接）
    if (vehicleUniqueId == 0) {
        if (rt.rideVehicleUniqueId == 0) return true;
        auto const old = rt.rideVehicleUniqueId;
        rt.rideVehicleUniqueId = 0;
        sculk::protocol::SetActorLinkPacket pkt;
        pkt.mLink.mType                   = sculk::protocol::ActorLinkType::None;
        pkt.mLink.mTargetAUniqueId        = old;
        pkt.mLink.mTargetBUniqueId        = static_cast<std::int64_t>(rt.uniqueId);
        pkt.mLink.mImmediate              = true;
        pkt.mLink.mPassengerInitiated     = false;
        pkt.mLink.mVehicleAngularVelocity = 0.0f;
        for (auto const& uuid : rt.shownPlayers) {
            if (auto* p = findPlayerByUuid(uuid)) sendSculkPacketToPlayer(*p, pkt);
        }
        return true;
    }

    // 只接受**真实服务端实体**当载具: 客户端实体（库内 CustomEntity）在客户端不存在
    // 服务端实体记录, 拿它当载具会让客户端在挂接/物理阶段崩。真实猪/马/船/矿车都可以。
    auto level = ll::service::getLevel();
    if (!level) return false;
    if (!level->fetchEntity(::ActorUniqueID{vehicleUniqueId}, false)) return false;

    rt.rideVehicleUniqueId = vehicleUniqueId;
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* p = findPlayerByUuid(uuid)) sendNpcRide(*p, rt);
    }
    return true;
}

bool PlayerNpcManager::setRidePlayer(int64_t id, std::string const& playerName) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;
    return setRideUniqueId(id, static_cast<std::int64_t>(player->getOrCreateUniqueID().rawID));
}

bool PlayerNpcManager::setRideEntity(int64_t id, int64_t vehicleEntityId) {
    // 库内自定义实体是纯协议实体, 客户端没有它的服务端记录 —— 不作为载具（会崩客户端）。
    // 这里保留接口只为排查: 解析到 uniqueId 后仍会被 setRideUniqueId 的真实实体校验拒绝。
    std::uint64_t uid = 0;
    std::uint64_t rid = 0;
    if (!CustomEntityManager::getInstance().getIdPair(vehicleEntityId, uid, rid)) return false;
    return setRideUniqueId(id, static_cast<std::int64_t>(uid));
}

bool PlayerNpcManager::clearRide(int64_t id) { return setRideUniqueId(id, 0); }

// spawn 之后补发姿势/装备/骑乘（玩家已在 shownPlayers 里）
void PlayerNpcManager::afterSpawnLocked(Player& player, int64_t /*id*/, Runtime const& rt) {
    sendNpcPose(player, rt);
    sendNpcEquipment(player, rt);
    sendNpcRide(player, rt);
}

void PlayerNpcManager::notifySpawn(int64_t id, std::string const& playerName) const {
    std::function<void(int64_t, std::string const&)> cb;
    {
        std::lock_guard lock(mMutex);
        cb = mSpawnCallback;
    }
    if (cb) cb(id, playerName);
}

// 到期的动画包发出（与 customentity 域同款: 发包前校验实体仍对这名玩家可见 + runtimeId 未变）
void PlayerNpcManager::flushAnimsLocked() {
    if (mAnimQueue.empty()) return;
    auto const now = currentTick();
    for (auto it = mAnimQueue.begin(); it != mAnimQueue.end() && it->first <= now;) {
        auto const& e  = it->second;
        bool        ok = false;
        auto        rit = mRuntimes.find(e.npcId);
        if (rit != mRuntimes.end()) {
            auto const cfgIt = mConfigs.find(e.npcId);
            ok = rit->second.runtimeId == e.runtimeId && rit->second.shownPlayers.contains(e.playerUuid)
              && cfgIt != mConfigs.end() && cfgIt->second.enabled;
        }
        if (ok) {
            if (auto* player = findPlayerByUuid(e.playerUuid)) {
                sculk::protocol::AnimateEntityPacket pkt;
                pkt.mAnimation                   = e.animation;
                pkt.mNextState                   = "none";
                pkt.mStopExpression              = e.stopExpression;
                pkt.mStopExpressionMolangVersion = sculk::protocol::MolangVersion::Initial;
                pkt.mController                  = e.controller;
                pkt.mBlendOutTime                = 0;
                pkt.mRuntimeIds                  = {e.runtimeId};
                npc_protocol::sendToPlayer(*player, pkt, NetworkPeer::Reliability::Reliable);
            }
        }
        it = mAnimQueue.erase(it);
    }
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
            )
            && npc_protocol::sendPlayerSkin(*player, id, skin)) {
            rt.shownPlayers.insert(player->getUuid());
            afterSpawnLocked(*player, id, rt);
            mTabRemovals[id].push_back({player->getUuid(), currentTick() + 20});
            notifySpawn(id, player->getRealName());
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
                    )
                    && npc_protocol::sendPlayerSkin(player, id, skin)) {
                    rt.shownPlayers.insert(uuid);
                    ++spawnedThisTick;
                    afterSpawnLocked(player, id, rt);
                    mTabRemovals[id].push_back({uuid, currentTick() + 20});
                    notifySpawn(id, player.getRealName());
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
    static void flushAnims(PlayerNpcManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.flushAnimsLocked();
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

// 由 core/TickDriver.h 的统一 Level::$tick hook 驱动（order=PlayerNpc, 最先跑）; 主线程统一发包
void playerNpcTick() {
    if (hlibDomainDisabled("HLIB_NO_NPC")) return;

    PlayerNpcTickHookAccess::processDirty(PlayerNpcManager::getInstance());
    PlayerNpcTickHookAccess::processLightDirty(PlayerNpcManager::getInstance());
    PlayerNpcTickHookAccess::flushAnims(PlayerNpcManager::getInstance());
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
tick::AutoRegister gPlayerNpcTickHookRegistrar{"playernpc", &playerNpcTick, tick::Order::PlayerNpc};

} // namespace debugshape_export
