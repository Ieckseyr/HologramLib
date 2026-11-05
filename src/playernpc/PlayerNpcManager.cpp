#include "core/EventIds.h"

#include "DiagLog.h"
#include "core/TickDriver.h"

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
#include <sculk/protocol/codec/packet/EmotePacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorLinkPacket.hpp>

#include "SculkPacketSend.h"
#include "customentity/CustomEntityManager.h"

#include "../DiagFlags.h"
#include "PlayerNpcManager.h"

#include "playernpc/PlayerNpcRuntimeOps.h"

namespace debugshape_export {
using namespace npc_ops;

PlayerNpcManager& PlayerNpcManager::getInstance() {
    static PlayerNpcManager instance;
    return instance;
}

void PlayerNpcManager::init() {
    std::lock_guard lock(mMutex);
    if (mJoinListener)
        return;

    NpcSkinRegistry::getInstance().init();

    mJoinListener       = debugshape_export::core::listen<ll::event::PlayerJoinEvent>(core::kPlayerJoinId,
                                                                                      [this](ll::event::PlayerJoinEvent& ev) {
                                                                                    if (hlibDomainDisabled("HLIB_NO_NPC"))
                                                                                        return;
                                                                                    std::lock_guard lock(mMutex);
                                                                                    mInitializedPlayers.insert(ev.self().getUuid());
                                                                                    syncVisibilityLocked();
                                                                                      });
    mDisconnectListener = debugshape_export::core::listen<ll::event::PlayerDisconnectEvent>(core::kPlayerDisconnectId,
                                                                                            [this](ll::event::PlayerDisconnectEvent& ev) {
                                                                                                std::lock_guard lock(mMutex);
                                                                                                auto const&     uuid = ev.self().getUuid();
                                                                                                mInitializedPlayers.erase(uuid);
                                                                                                mEntries.forEachLocked([&](int64_t, Entry& entry) {
                                                                                                    entry.runtime.shownPlayers.erase(uuid);
                                                                                                    entry.runtime.playerRot.erase(uuid);
                                                                                                });
                                                                                                for (auto& [id, removals] : mTabRemovals) {
                                                                                                    std::erase_if(removals, [&uuid](TabRemoval const& r) {
                                                                                                        return r.playerUuid == uuid;
                                                                                                    });
                                                                                                }
                                                                                            });
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
        mEntries.clear();
        mDirtyIds.clear();
        mLightDirtyIds.clear();
        mTabRemovals.clear();
        mVisibleFilter.clear();
        mInitializedPlayers.clear();
    }
    NpcSkinRegistry::getInstance().shutdown();
}

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
    if (sizeBlocks <= 0.0f)
        sizeBlocks = 1.0f;
    float const w = 8.0f * sizeBlocks;
    float const h = 16.0f * sizeBlocks;

    std::string const geoId = std::format("geometry.hlib_image_{}", static_cast<int>(sizeBlocks * 10.0f + 0.5f));

    static constexpr char kTemplate[] =
        R"({"format_version":"1.14.0","minecraft:geometry":[{"description":{"identifier":"%ID%","texture_width":128,"texture_height":128},"bones":[{"name":"root"},{"name":"body","parent":"root","pivot":[0,0,0],"poly_mesh":{"normalized_uvs":true,"positions":[[-%W%,0,-0.01],[%W%,0,-0.01],[%W%,%H%,-0.01],[-%W%,%H%,-0.01],[%W%,0,0.01],[-%W%,0,0.01],[-%W%,%H%,0.01],[%W%,%H%,0.01]],"normals":[[0,0,-1],[0,0,-1],[0,0,-1],[0,0,-1],[0,0,1],[0,0,1],[0,0,1],[0,0,1]],"uvs":[[0,0],[1,0],[1,1],[0,1],[1,0],[0,0],[0,1],[1,1]],"polys":[[[0,0,0],[1,0,1],[2,0,2],[3,0,3]],[[4,1,4],[5,1,5],[6,1,6],[7,1,7]]]}}]}]})";

    auto const num = [](float v) {
        return std::format("{:.6g}", v);
    };
    std::string geo        = kTemplate;
    auto        replaceAll = [&](std::string_view key, std::string_view val) {
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
    bool const  ok = NpcSkinRegistry::getInstance().registerSkinFromPng(skin, error, false, transparent ? 1 : 2);
    if (!ok)
        HLIB_LOG_WARN("[PlayerNpc] registerImageSkin failed: {}", error);
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
    bool referenced = false;
    mEntries.forEachLocked([&](int64_t, Entry const& entry) {
        if (entry.config.skinId == skinId)
            referenced = true;
    });
    return referenced;
}

int64_t PlayerNpcManager::create(PlayerNpcConfig const& config) {
    std::lock_guard lock(mMutex);

    int64_t const id = mEntries.insert(std::make_unique<Entry>());

    if (int64_t const created = createLocked(config, id); created < 0) {
        mEntries.erase(id);
        return created;
    }
    return id;
}

int64_t PlayerNpcManager::createRandom(PlayerNpcConfig const& config) {

    static std::mt19937_64 rng{std::random_device{}()};
    std::lock_guard        lock(mMutex);
    constexpr std::int64_t kBase = core::actorIds::randomIdBase, kSpan = core::actorIds::randomIdSpan;
    for (int tries = 0; tries < 128; ++tries) {
        auto const id = kBase + static_cast<std::int64_t>(rng() % static_cast<std::uint64_t>(kSpan));
        if (configOf(id) != nullptr)
            continue;
        return createLocked(config, id);
    }
    return -1;
}

int64_t PlayerNpcManager::createWithId(PlayerNpcConfig const& config, int64_t desiredId) {
    if (desiredId <= 0)
        return -2;
    std::lock_guard lock(mMutex);
    if (configOf(desiredId) != nullptr)
        return -2;
    return createLocked(config, desiredId);
}

int64_t PlayerNpcManager::createLocked(PlayerNpcConfig const& config, int64_t id) {
    if (hlibDomainDisabled("HLIB_NO_NPC"))
        return -1;
    if (!NpcSkinRegistry::getInstance().hasSkin(config.skinId))
        return -3;

    Runtime rt{};
    rt.uniqueId  = mNextActorUniqueId++;
    rt.runtimeId = mNextRuntimeId++;

    if (auto* existing = mEntries.peek(id)) {
        existing->config  = config;
        existing->runtime = std::move(rt);
    } else {
        mEntries.insertWithId(id, std::make_unique<Entry>(Entry{config, std::move(rt)}));
    }

    mDirtyIds.insert(id);
    return id;
}

bool PlayerNpcManager::destroy(int64_t id) {
    std::uint64_t          uniqueId = 0;
    std::vector<::Player*> viewers;
    {
        std::lock_guard lock(mMutex);
        auto*           rit = runtimeOf(id);
        if (rit == nullptr)
            return false;
        uniqueId = rit->uniqueId;
        for (auto const& uuid : rit->shownPlayers) {
            if (auto* player = findPlayerByUuid(uuid))
                viewers.push_back(player);
        }
        mEntries.erase(id);
        mTabRemovals.erase(id);
        mVisibleFilter.erase(id);
        mDirtyIds.erase(id);
        for (auto it = mAnimQueue.begin(); it != mAnimQueue.end();) {
            if (it->second.npcId == id)
                it = mAnimQueue.erase(it);
            else
                ++it;
        }
    }
    // 移除包在锁外发：锁内发包会被网络线程拖住，也挡住本域重入
    for (auto* player : viewers)
        npc_protocol::remove(*player, id, uniqueId);
    return true;
}

void PlayerNpcManager::destroyAll() {
    std::vector<int64_t> ids;
    {
        std::lock_guard lock(mMutex);
        ids = mEntries.snapshotAll();
    }
    for (auto id : ids)
        destroy(id);
}

bool PlayerNpcManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return configOf(id) != nullptr;
}

bool PlayerNpcManager::get(int64_t id, PlayerNpcConfig& out) const {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    out = *it;
    return true;
}

bool PlayerNpcManager::isIdUsed(int64_t id) const {
    std::lock_guard lock(mMutex);
    return configOf(id) != nullptr;
}

std::vector<int64_t> PlayerNpcManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    return mEntries.snapshotAll();
}

bool PlayerNpcManager::setPosition(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->x = x;
    it->y = y;
    it->z = z;
    if (dim >= 0)
        it->dimension = dim;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setRotation(int64_t id, float yaw) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->yaw = yaw;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setRotationLight(int64_t id, float yaw) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->yaw = yaw;
    mLightDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setPlayerRotation(int64_t id, std::string const& playerName, float yaw) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr || runtimeOf(id) == nullptr)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto* rtPtr = runtimeOf(id);
    if (rtPtr == nullptr)
        return false;
    auto& rt = *rtPtr;
    if (!rt.shownPlayers.contains(player->getUuid()))
        return false;
    rt.playerRot[player->getUuid()] = hologramlib::PerPlayerRotation{yaw, 0.0f};
    mLightDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::clearPlayerRotation(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    if (rit->playerRot.erase(player->getUuid()) == 0)
        return false;
    mLightDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::clearPlayerRotations(int64_t id) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    if (rit->playerRot.empty())
        return true;
    rit->playerRot.clear();
    mLightDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setPositionLight(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    if (dim >= 0 && dim != it->dimension)
        return false;
    it->x = x;
    it->y = y;
    it->z = z;
    mLightDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::injectSkin(
    std::string const& viewerName,
    std::string const& targetName,
    std::string const& skinId) {

    sculk::protocol::SerializedSkin skin;
    if (!NpcSkinRegistry::getInstance().getSkin(skinId, skin))
        return false;

    Player* target = findPlayerByName(targetName);
    if (target == nullptr)
        return false;

    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
    packet.mPlayerEntryList = {npc_protocol::playerListEntryForPlayer(*target, skin)};

    if (!viewerName.empty()) {
        Player* viewer = findPlayerByName(viewerName);
        if (viewer == nullptr)
            return false;
        return npc_protocol::sendToPlayer(*viewer, packet, wire::Reliability::Reliable);
    }

    auto level = ll::service::getLevel();
    if (!level)
        return false;
    bool any = false;
    level->forEachPlayer([&](Player& p) {
        if (npc_protocol::sendToPlayer(p, packet, wire::Reliability::Reliable))
            any = true;
        return true;
    });
    return any;
}

bool PlayerNpcManager::injectSkinAll(std::string const& targetName, std::string const& skinId) {
    return injectSkin(std::string{}, targetName, skinId);
}

static std::string npcAnimControllerName(int64_t id) {
    return "hololib.playernpc." + std::to_string(id);
}

constexpr char const* kStopAnimation = "animation.player.attack.positions";
constexpr char const* kStopExpr      = "1";

static bool isLegacyStopExpression(std::string const& e) {
    return e == "query.any_animation" || e == "query.any_animation()";
}

bool PlayerNpcManager::playAnimation(
    int64_t            id,
    std::string const& animation,
    std::string const& stopExpression,
    int                durationTicks) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr || animation.empty())
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr || !it->enabled || rit->shownPlayers.empty())
        return false;

    auto const now  = currentTick();
    auto const ctrl = npcAnimControllerName(id);
    auto const rt   = rit->runtimeId;
    if (isLegacyStopExpression(stopExpression)) {
        for (auto const& uuid : rit->shownPlayers) {
            mAnimQueue.emplace(now + 2, AnimEntry{uuid, rt, id, kStopAnimation, ctrl, kStopExpr});
        }
        return true;
    }
    for (auto const& uuid : rit->shownPlayers) {
        mAnimQueue.emplace(now + 2, AnimEntry{uuid, rt, id, animation, ctrl, stopExpression});
        if (durationTicks > 0) {
            mAnimQueue.emplace(
                now + 2 + static_cast<std::uint64_t>(durationTicks),
                AnimEntry{uuid, rt, id, kStopAnimation, ctrl, kStopExpr});
        }
    }
    return true;
}

bool PlayerNpcManager::playAnimationTo(
    int64_t            id,
    std::string const& playerName,
    std::string const& animation,
    std::string const& stopExpression,
    int                durationTicks) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr || animation.empty())
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr || !it->enabled)
        return false;

    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto const uuid = player->getUuid();
    if (!rit->shownPlayers.contains(uuid))
        return false;

    auto const now  = currentTick();
    auto const ctrl = npcAnimControllerName(id);
    if (isLegacyStopExpression(stopExpression)) {
        mAnimQueue.emplace(now + 2, AnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
        return true;
    }
    mAnimQueue.emplace(now + 2, AnimEntry{uuid, rit->runtimeId, id, animation, ctrl, stopExpression});
    if (durationTicks > 0) {
        mAnimQueue.emplace(
            now + 2 + static_cast<std::uint64_t>(durationTicks),
            AnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
    }
    return true;
}

bool PlayerNpcManager::stopAnimation(int64_t id) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr || rit->shownPlayers.empty())
        return false;
    auto const now  = currentTick();
    auto const ctrl = npcAnimControllerName(id);
    for (auto const& uuid : rit->shownPlayers) {
        mAnimQueue.emplace(now + 2, AnimEntry{uuid, rit->runtimeId, id, kStopAnimation, ctrl, kStopExpr});
    }
    return true;
}

bool PlayerNpcManager::stopAnimationTo(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    auto const uuid = player->getUuid();
    if (!rit->shownPlayers.contains(uuid))
        return false;
    mAnimQueue.emplace(
        currentTick() + 2,
        AnimEntry{uuid, rit->runtimeId, id, kStopAnimation, npcAnimControllerName(id), kStopExpr});
    return true;
}

void PlayerNpcManager::setEntitySpawnCallback(std::function<void(int64_t, std::string const&)> callback) {
    std::lock_guard lock(mMutex);
    mSpawnCallback = std::move(callback);
}

bool PlayerNpcManager::setPose(int64_t id, int pose) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto& rt = *rit;
    rt.pose  = std::clamp(pose, 0, 13);
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* p = findPlayerByUuid(uuid))
            sendNpcPose(*p, rt);
    }
    return true;
}

bool PlayerNpcManager::setEquipment(
    int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) {
    if (slot < 0 || slot > 5)
        return false;
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto& rt       = *rit;
    rt.equip[slot] = Runtime::NpcEquip{name, aux, nbt};
    if (name.empty()) {

        mDirtyIds.insert(id);
        return true;
    }
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* p = findPlayerByUuid(uuid))
            sendNpcEquipment(*p, rt);
    }
    return true;
}

bool PlayerNpcManager::setRideUniqueId(int64_t id, std::int64_t vehicleUniqueId) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto& rt = *rit;
    if (vehicleUniqueId == static_cast<std::int64_t>(rt.uniqueId))
        return false;

    if (vehicleUniqueId == 0) {
        if (rt.rideVehicleUniqueId == 0)
            return true;
        auto const old         = rt.rideVehicleUniqueId;
        rt.rideVehicleUniqueId = 0;
        sculk::protocol::SetActorLinkPacket pkt;
        pkt.mLink.mType                   = sculk::protocol::ActorLinkType::None;
        pkt.mLink.mTargetAUniqueId        = old;
        pkt.mLink.mTargetBUniqueId        = static_cast<std::int64_t>(rt.uniqueId);
        pkt.mLink.mImmediate              = true;
        pkt.mLink.mPassengerInitiated     = false;
        pkt.mLink.mVehicleAngularVelocity = 0.0f;
        for (auto const& uuid : rt.shownPlayers) {
            if (auto* p = findPlayerByUuid(uuid))
                sendSculkPacketToPlayer(*p, pkt);
        }
        return true;
    }

    auto level = ll::service::getLevel();
    if (!level)
        return false;
    if (!level->fetchEntity(::ActorUniqueID{vehicleUniqueId}, false))
        return false;

    rt.rideVehicleUniqueId = vehicleUniqueId;
    for (auto const& uuid : rt.shownPlayers) {
        if (auto* p = findPlayerByUuid(uuid))
            sendNpcRide(*p, rt);
    }
    return true;
}

bool PlayerNpcManager::setRidePlayer(int64_t id, std::string const& playerName) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    return setRideUniqueId(id, static_cast<std::int64_t>(player->getOrCreateUniqueID().rawID));
}

bool PlayerNpcManager::setRideEntity(int64_t id, int64_t vehicleEntityId) {

    std::uint64_t uid = 0;
    std::uint64_t rid = 0;
    if (!CustomEntityManager::getInstance().getIdPair(vehicleEntityId, uid, rid))
        return false;
    return setRideUniqueId(id, static_cast<std::int64_t>(uid));
}

bool PlayerNpcManager::clearRide(int64_t id) {
    return setRideUniqueId(id, 0);
}

void PlayerNpcManager::afterSpawnLocked(Player& player, int64_t, Runtime const& rt) {
    sendNpcPose(player, rt);
    sendNpcEquipment(player, rt);
    sendNpcRide(player, rt);
}

void PlayerNpcManager::drainSpawnCallbacks() {
    std::vector<std::pair<int64_t, std::string>>     pending;
    std::function<void(int64_t, std::string const&)> cb;
    {
        std::lock_guard lock(mMutex);
        pending.swap(mPendingSpawnCallbacks);
        cb = mSpawnCallback;
    }
    if (!cb) {
        return;
    }
    for (auto const& [id, name] : pending) {
        cb(id, name);
    }
}

void PlayerNpcManager::notifySpawn(int64_t id, std::string const& playerName) const {
    // 只入队：这里可能处在 forEachLocked 迭代中，同步回调会让 destroy() 破坏迭代器
    std::lock_guard       lock(mMutex);
    constexpr std::size_t kSpawnQueueCap = 512;
    if (mSpawnQueue.size() >= kSpawnQueueCap)
        mSpawnQueue.pop_front();
    mSpawnQueue.emplace_back(id, playerName);
    if (mSpawnCallback) {
        mPendingSpawnCallbacks.emplace_back(id, playerName);
    }
}

void PlayerNpcManager::flushAnimsLocked() {
    if (mAnimQueue.empty())
        return;
    auto const now = currentTick();
    for (auto it = mAnimQueue.begin(); it != mAnimQueue.end() && it->first <= now;) {
        auto const& e    = it->second;
        bool        ok   = false;
        auto*       rt2  = runtimeOf(e.npcId);
        auto*       cfg2 = configOf(e.npcId);
        if (rt2 != nullptr && cfg2 != nullptr) {
            ok = rt2->runtimeId == e.runtimeId && rt2->shownPlayers.contains(e.playerUuid) && cfg2->enabled;
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
                npc_protocol::sendToPlayer(*player, pkt, wire::Reliability::Reliable);
            }
        }
        it = mAnimQueue.erase(it);
    }
}

void PlayerNpcManager::refreshLightLocked(int64_t id) {
    auto* it  = configOf(id);
    auto* rit = runtimeOf(id);
    if (it == nullptr || rit == nullptr)
        return;
    auto const& cfg = *it;
    auto&       rt  = *rit;
    if (!cfg.enabled || rt.shownPlayers.empty())
        return;

    Vec3 const pos{cfg.x, cfg.y, cfg.z};
    for (auto const& uuid : rt.shownPlayers) {
        auto* player = findPlayerByUuid(uuid);
        if (player == nullptr)
            continue;
        npc_protocol::move(*player, rt.runtimeId, pos, effectiveYaw(rt, uuid, cfg));
    }
}

bool PlayerNpcManager::setNametag(int64_t id, std::string const& text) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->name == text)
        return true;
    it->name = text;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setSkin(int64_t id, std::string const& skinId) {
    if (!NpcSkinRegistry::getInstance().hasSkin(skinId))
        return false;
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->skinId = skinId;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setViewDistance(int64_t id, double dist) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->viewDistance = dist;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setScale(int64_t id, float scale) {
    if (scale < 0.0625f || scale > 10.0f)
        return false;
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->scale = scale;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setEnabled(int64_t id, bool enabled) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->enabled = enabled;
    mDirtyIds.insert(id);
    return true;
}

bool PlayerNpcManager::setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;
    if (playerNames.empty()) {
        mVisibleFilter.erase(id);
    } else {
        mVisibleFilter[id] = std::unordered_set<std::string>(playerNames.begin(), playerNames.end());
    }
    dropDisallowedLocked(id);
    return true;
}

bool PlayerNpcManager::clearVisiblePlayers(int64_t id) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;
    mVisibleFilter.erase(id);
    dropDisallowedLocked(id);
    return true;
}

bool PlayerNpcManager::setVisiblePlayer(int64_t id, std::string const& playerName) {
    return setVisiblePlayers(id, std::vector<std::string>{playerName});
}

std::uint64_t PlayerNpcManager::runtimeIdOf(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto*           rt = runtimeOf(id);
    return rt == nullptr ? 0 : rt->runtimeId;
}

std::string PlayerNpcManager::getDebugInfo(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto*           it  = configOf(id);
    auto*           rit = runtimeOf(id);
    if (it == nullptr || rit == nullptr)
        return "not_found";
    auto const& cfg = *it;
    auto const& rt  = *rit;
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
        mVisibleFilter.contains(id) ? "custom" : "all");
}

bool PlayerNpcManager::playEmote(int64_t id, std::string const& pieceId, int lengthTicks) {
    sculk::protocol::EmotePacket packet;
    std::vector<mce::UUID>       recipients;
    {
        std::lock_guard lock(mMutex);
        auto*           rt = runtimeOf(id);
        if (rt == nullptr || pieceId.empty() || rt->shownPlayers.empty())
            return false;

        packet.mActorRuntimeId   = rt->runtimeId;
        packet.mEmoteId          = pieceId;
        packet.mEmoteLengthTicks = lengthTicks < 0 ? 0u : static_cast<std::uint32_t>(lengthTicks);
        packet.mFlags            = 0;
        recipients.assign(rt->shownPlayers.begin(), rt->shownPlayers.end());
    }

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    // 锁外发包：收件人是锁内快照（P04）
    for (auto const& uuid : recipients) {
        if (auto* player = findPlayerByUuid(uuid)) {
            wire::sendRaw(*player, bytes);
        }
    }
    return true;
}

} // namespace debugshape_export
