#include "ItemDisplayManager.h"
#include "../DiagFlags.h"
#include "ItemDisplayRuntimeOps.h"
#include "core/EventIds.h"

#include "DiagLog.h"

#include "core/PacketSink.h"

#include "core/TickDriver.h"

#include <random>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/deps/nbt/ShortTag.h>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/enchanting/Enchant.h>
#include <mc/world/item/enchanting/EnchantmentInstance.h>
#include <mc/world/item/enchanting/ItemEnchants.h>
#include <mc/world/item/registry/ItemRegistryRef.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/level/MolangVersion.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/AnimateEntityPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <map>
#include <optional>

namespace debugshape_export {
using namespace ide_ops;

void ItemDisplayManager::enqueueFmbeAnimsLocked(
    int64_t                  id,
    ItemDisplayConfig const& data,
    Runtime const&           rt,
    mce::UUID const&         uuid,
    std::uint64_t            base,
    bool                     block) {
    auto const tag  = "." + std::to_string(id);
    auto       push = [&](std::uint64_t at, std::string anim, std::string ctrl, std::string stop) {
        AnimEntry e{uuid, rt.runtimeId, std::move(anim), std::move(ctrl), std::move(stop)};
        animQueue().emplace(at, std::move(e));
    };
    if (block) {
        push(base + 2, "animation.player.sleeping", "controller.animation.fox.move" + tag, buildBlockMatrixExpr(data));
        push(base + 3, "animation.creeper.swelling", "wiki.fmbe.3d_blocks.anim1" + tag, buildBlockSwellingExpr(rt.stretchXZ, rt.stretchY));
        push(base + 4, "animation.ender_dragon.neck_head_movement", "wiki.fmbe.3d_blocks.anim2" + tag, std::string{kBlockHeadPosExpr});
        push(base + 5, "animation.warden.move", "wiki.fmbe.3d_blocks.anim3" + tag, std::string{kBlockBodyRotExpr});
        push(base + 6, "animation.player.attack.rotations", "wiki.fmbe.3d_blocks.anim4" + tag, std::string{kBlockAttackRotExpr});
    } else {

        push(base + 2, "animation.player.sleeping", "controller.animation.fox.move" + tag, "");
        push(base + 3, "animation.creeper.swelling", "wiki.scale" + tag, buildItemScaleExpr(data, rt.stretchXZ, rt.stretchY));
        push(base + 4, "animation.ender_dragon.neck_head_movement", "wiki.posrot" + tag, std::string{kItemPosRotExpr});
    }
}

bool ItemDisplayManager::resendFmbeLocked(int64_t id, ItemDisplayConfig const& data, Runtime& rt) {
    if (rt.shownPlayers.empty())
        return false;

    auto& q = animQueue();
    for (auto it = q.begin(); it != q.end();) {

        if (it->second.runtimeId == rt.runtimeId && !it->second.equip) {
            it = q.erase(it);
        } else {
            ++it;
        }
    }
    bool block = false;
    if (data.mode == 2) {
        block = true;
    } else if (data.mode == 0) {
        if (!rt.cachedStack)
            return false;
        block = (effectiveMode(data, *rt.cachedStack) == 2);
    }
    auto const base = currentTick();
    for (auto const& uuid : rt.shownPlayers) {
        enqueueFmbeAnimsLocked(id, data, rt, uuid, base, block);
    }
    return true;
}

ItemDisplayManager& ItemDisplayManager::getInstance() {
    static ItemDisplayManager instance;
    return instance;
}

void ItemDisplayManager::init() {
    std::lock_guard lock(mMutex);
    if (mJoinListener)
        return;

    mJoinListener       = debugshape_export::core::listen<ll::event::PlayerJoinEvent>(core::kPlayerJoinId,
                                                                                      [this](ll::event::PlayerJoinEvent& ev) {
                                                                                    if (hlibDomainDisabled("HLIB_NO_DISPLAY"))
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
                                                                                                });
                                                                                            });
}

void ItemDisplayManager::shutdown() {
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
    mVisibleFilter.clear();
    mFollowers.clear();
    mInitializedPlayers.clear();
    animQueue().clear();
}

int64_t ItemDisplayManager::create(ItemDisplayConfig const& config) {
    std::lock_guard lock(mMutex);

    int64_t const id = mEntries.insert(std::make_unique<Entry>());
    if (int64_t const created = createLocked(config, id); created < 0) {
        mEntries.erase(id);
        return created;
    }
    return id;
}

int64_t ItemDisplayManager::createRandom(ItemDisplayConfig const& config) {

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

int64_t ItemDisplayManager::createWithId(ItemDisplayConfig const& config, int64_t desiredId) {
    if (desiredId <= 0)
        return -2;
    std::lock_guard lock(mMutex);
    if (configOf(desiredId) != nullptr)
        return -2;
    return createLocked(config, desiredId);
}

int64_t ItemDisplayManager::createSeamless(
    ItemDisplayConfig const& config,
    int                      mode,
    double                   viewDistance,
    std::string const&       visiblePlayer) {
    std::lock_guard lock(mMutex);
    if (hlibDomainDisabled("HLIB_NO_DISPLAY"))
        return -1;
    ItemDisplayConfig cfg = config;
    if (mode == 1 || mode == 2)
        cfg.mode = mode;
    if (viewDistance >= 0)
        cfg.viewDistance = viewDistance;

    int64_t const id = mEntries.insert(std::make_unique<Entry>());

    if (!visiblePlayer.empty()) {
        mVisibleFilter.emplace(id, std::unordered_set<std::string>{visiblePlayer});
    }

    if (int64_t const created = createLocked(cfg, id); created < 0) {
        mEntries.erase(id);
        mVisibleFilter.erase(id);
        return created;
    }
    return id;
}

bool ItemDisplayManager::isIdUsed(int64_t id) const {
    std::lock_guard lock(mMutex);
    return (configOf(id) != nullptr);
}

int64_t ItemDisplayManager::createLocked(ItemDisplayConfig const& config, int64_t id) {
    if (hlibDomainDisabled("HLIB_NO_DISPLAY"))
        return -1;

    Runtime rt{};
    rt.uniqueId  = allocUniqueIdLocked();
    rt.runtimeId = allocRuntimeIdLocked();

    if (auto* existing = mEntries.peek(id)) {
        existing->config  = config;
        existing->runtime = std::move(rt);
    } else {
        mEntries.insertWithId(id, std::make_unique<Entry>(Entry{config, std::move(rt)}));
    }

    syncOneLocked(id);
    return id;
}

std::uint64_t ItemDisplayManager::allocUniqueIdLocked() {
    return mNextActorUniqueId++;
}

std::uint64_t ItemDisplayManager::allocRuntimeIdLocked() {
    return mNextRuntimeId++;
}

bool ItemDisplayManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);

    mDirtyIds.erase(id);
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;

    auto uuids = std::vector<mce::UUID>{rit->shownPlayers.begin(), rit->shownPlayers.end()};
    for (auto const& uuid : uuids) {
        if (auto* player = findPlayerByUuid(uuid)) {
            despawnForPlayer(*rit, *player);
        }
    }
    mEntries.erase(id);
    mVisibleFilter.erase(id);
    mFollowers.erase(id);
    return true;
}

void ItemDisplayManager::destroyAll() {
    std::lock_guard lock(mMutex);
    mDirtyIds.clear();
    mEntries.forEachLocked([&](int64_t, Entry& entry) {
        auto& rt    = entry.runtime;
        auto  uuids = std::vector<mce::UUID>{rt.shownPlayers.begin(), rt.shownPlayers.end()};
        for (auto const& uuid : uuids) {
            if (auto* player = findPlayerByUuid(uuid)) {
                despawnForPlayer(rt, *player);
            }
        }
    });
    mEntries.clear();
    mVisibleFilter.clear();
    mFollowers.clear();
}

bool ItemDisplayManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return (configOf(id) != nullptr);
}

bool ItemDisplayManager::get(int64_t id, ItemDisplayConfig& out) const {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    out = *it;
    return true;
}

bool ItemDisplayManager::setItem(int64_t id, std::string const& item, int aux) {
    return setItemWithNbt(id, item, aux, "");
}

bool ItemDisplayManager::setItemWithNbt(
    int64_t            id,
    std::string const& item,
    int                aux,
    std::string const& nbt) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->item    = item;
    it->itemAux = aux;
    it->itemNbt = nbt;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setGlint(int64_t id, bool on) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->itemGlint == on)
        return true;
    it->itemGlint = on;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setPosition(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    mFollowers.erase(id);
    it->x = x;
    it->y = y;
    it->z = z;
    if (dim >= 0)
        it->dimension = dim;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->offsetX = ox;
    it->offsetY = oy;
    it->offsetZ = oz;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->baseOffsetX = ox;
    it->baseOffsetY = oy;
    it->baseOffsetZ = oz;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->rotX = rx;
    it->rotY = ry;
    it->rotZ = rz;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setScale(int64_t id, std::string const& scale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->scale = scale;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->extendScale = scale;
    it->extendRotX  = rx;
    it->extendRotY  = ry;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setMode(int64_t id, int mode) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->mode == mode)
        return true;
    it->mode = mode;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setEnabled(int64_t id, bool enabled) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    it->enabled = enabled;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setViewDistance(int64_t id, double dist) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (it->viewDistance == dist)
        return true;
    it->viewDistance = dist;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;

    if (playerNames.empty()) {
        mVisibleFilter.erase(id);
    } else {
        std::unordered_set<std::string> names(playerNames.begin(), playerNames.end());
        mVisibleFilter.insert_or_assign(id, std::move(names));
    }

    syncOneLocked(id);
    return true;
}

bool ItemDisplayManager::clearVisiblePlayers(int64_t id) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;
    if (mVisibleFilter.erase(id) == 0)
        return true;
    syncOneLocked(id);
    return true;
}

bool ItemDisplayManager::setVisiblePlayer(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;

    mVisibleFilter.insert_or_assign(id, std::unordered_set<std::string>{playerName});

    syncOneLocked(id);
    return true;
}

bool ItemDisplayManager::scaleTo(int64_t id, double targetScale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    if (!std::isfinite(targetScale) || targetScale < 0.01)
        targetScale = 0.01;

    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;

    it->scale = std::format("{:.4f}", targetScale);

    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setStretch(int64_t id, std::string const& xz, std::string const& y) {
    std::lock_guard lock(mMutex);
    if (configOf(id) == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    rit->stretchXZ = xz;
    rit->stretchY  = y;
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::setStretchLive(int64_t id, std::string const& xz, std::string const& y) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    rit->stretchXZ = xz;
    rit->stretchY  = y;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setRotationLive(
    int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    it->rotX = rx;
    it->rotY = ry;
    it->rotZ = rz;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setOffsetLive(
    int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    it->offsetX = ox;
    it->offsetY = oy;
    it->offsetZ = oz;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setScaleLive(int64_t id, std::string const& scale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    it->scale = scale;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setTransformLive(
    int64_t            id,
    std::string const& ox, std::string const& oy, std::string const& oz,
    std::string const& rx, std::string const& ry, std::string const& rz,
    std::string const& scale) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    it->offsetX = ox;
    it->offsetY = oy;
    it->offsetZ = oz;
    it->rotX    = rx;
    it->rotY    = ry;
    it->rotZ    = rz;
    it->scale   = scale;
    return resendFmbeLocked(id, (*it), (*rit));
}

bool ItemDisplayManager::setAnim(int64_t id, ItemDisplayAnim const& anim) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto& rt = *rit;

    if (!rt.animActive) {
        rt.animBase.ox    = it->offsetX;
        rt.animBase.oy    = it->offsetY;
        rt.animBase.oz    = it->offsetZ;
        rt.animBase.rx    = it->rotX;
        rt.animBase.ry    = it->rotY;
        rt.animBase.rz    = it->rotZ;
        rt.animBase.scale = it->scale;
        rt.animActive     = true;
    }
    rt.anim = anim;

    bool itemMode = true;
    if (it->mode == 2) {
        itemMode = false;
    } else if (it->mode == 0 && rt.cachedStack) {
        itemMode = (effectiveMode((*it), *rt.cachedStack) == 1);
    }

    applyAnimToConfig((*it), anim, rt.animBase, itemMode);
    return resendFmbeLocked(id, (*it), rt);
}

bool ItemDisplayManager::setAnimPreset(int64_t id, std::string const& preset, float a, float b, int c) {
    std::lock_guard lock(mMutex);
    auto*           rit = runtimeOf(id);
    if (rit == nullptr || configOf(id) == nullptr)
        return false;
    auto anim = rit->anim;
    if (preset == "spin") {
        anim.spinSpeed = a;
        anim.spinAxis  = (c >= 0 && c <= 2) ? c : 1;
    } else if (preset == "bob") {
        anim.bobAmplitude = a;
        anim.bobPeriod    = b > 0.01f ? b : 2.0f;
    } else if (preset == "pulse") {
        anim.pulseAmplitude = a;
        anim.pulsePeriod    = b > 0.01f ? b : 2.0f;
    } else if (preset == "sway") {
        anim.swayAmplitude = a;
        anim.swayPeriod    = b > 0.01f ? b : 3.0f;
    } else if (preset == "orbit") {
        anim.orbitRadius = a;
        anim.orbitPeriod = b > 0.01f ? b : 4.0f;
        anim.orbitPlane  = (c >= 0 && c <= 2) ? c : 2;
    } else if (preset == "tumble") {
        anim.tumbleSpeed = a;
    } else if (preset == "wobble") {
        anim.wobbleAmplitude = a;
        anim.wobbleSpeed     = b > 0.01f ? b : 1.0f;
    } else if (preset == "heartbeat") {
        anim.heartbeatAmplitude = a;
        anim.heartbeatPeriod    = b > 0.01f ? b : 1.2f;
    } else if (preset == "pendulum") {
        anim.pendulumAmplitude = a;
        anim.pendulumPeriod    = b > 0.01f ? b : 3.0f;
    } else if (preset == "billboard") {
        anim.billboard      = (a != 0.0f);
        anim.billboardPitch = (c != 0);
    } else if (preset == "lod") {
        anim.lodNear     = a;
        anim.lodFar      = b;
        anim.lodMinScale = c > 0 ? static_cast<float>(c) / 100.0f : 0.5f;
    } else if (preset == "firstperson") {
        anim.hideInFirstPerson = (a != 0.0f);
    } else if (preset == "clear") {
        return clearAnim(id);
    } else {
        return false;
    }
    return setAnim(id, anim);
}

bool ItemDisplayManager::clearAnim(int64_t id) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;
    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return false;
    auto& rt = *rit;
    if (!rt.animActive)
        return true;

    it->offsetX   = rt.animBase.ox;
    it->offsetY   = rt.animBase.oy;
    it->offsetZ   = rt.animBase.oz;
    it->rotX      = rt.animBase.rx;
    it->rotY      = rt.animBase.ry;
    it->rotZ      = rt.animBase.rz;
    it->scale     = rt.animBase.scale;
    rt.animActive = false;
    rt.anim       = {};
    return resendFmbeLocked(id, (*it), rt);
}

std::string ItemDisplayManager::getDebugInfo(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return "not_found";

    auto* rit = runtimeOf(id);
    if (rit == nullptr)
        return "no_runtime";

    auto        fit = mVisibleFilter.find(id);
    std::string filterList;
    if (fit != mVisibleFilter.end()) {
        for (auto const& n : fit->second) {
            if (!filterList.empty())
                filterList += ",";
            filterList += n;
        }
    }
    auto& d  = *it;
    auto& rt = *rit;
    return std::format(
        "id={} dim={} pos=({:.1f},{:.1f},{:.1f}) mode={} enabled={} item='{}' view={} filter=[{}] shown={}",
        id,
        d.dimension,
        d.x,
        d.y,
        d.z,
        d.mode,
        d.enabled,
        d.item,
        d.viewDistance,
        filterList,
        rt.shownPlayers.size());
}

bool ItemDisplayManager::rotateY(int64_t id, float delta) {
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    auto& ry = it->rotY;
    float current{};
    auto  parsed = std::from_chars(ry.data(), ry.data() + ry.size(), current);
    if (parsed.ec == std::errc{} && parsed.ptr == ry.data() + ry.size()) {
        ry = std::format("{}", current + delta);
    } else {
        ry = std::format("({})+{}", ry, delta);
    }
    mDirtyIds.insert(id);
    return true;
}

bool ItemDisplayManager::scaleBy(int64_t id, double factor) {
    if (factor <= 0)
        return false;
    std::lock_guard lock(mMutex);
    auto*           it = configOf(id);
    if (it == nullptr)
        return false;

    auto& s = it->scale;
    float current{};
    auto  parsed = std::from_chars(s.data(), s.data() + s.size(), current);
    if (parsed.ec == std::errc{} && parsed.ptr == s.data() + s.size()) {
        if (current * factor <= 0)
            return false;
        s = std::format("{}", current * factor);
    } else {
        s = std::format("({})*{}", s, factor);
    }
    mDirtyIds.insert(id);
    return true;
}

std::vector<int64_t> ItemDisplayManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    return mEntries.snapshotAll();
}

std::uint64_t ItemDisplayManager::runtimeIdOf(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto*           rt = runtimeOf(id);
    return rt == nullptr ? 0 : rt->runtimeId;
}

bool ItemDisplayManager::findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const {
    std::lock_guard lock(mMutex);
    bool            found = false;
    mEntries.forEachLocked([&](int64_t id, Entry const& entry) {
        if (!found && entry.runtime.runtimeId == runtimeId) {
            outId = id;
            found = true;
        }
    });
    return found;
}

int64_t ItemDisplayManager::findNearest(float x, float y, float z, int dim, double maxDist) const {
    std::lock_guard lock(mMutex);
    int64_t         best      = -1;
    double          bestDist2 = (maxDist > 0) ? maxDist * maxDist : 1e300;
    mEntries.forEachLocked([&](int64_t id, Entry const& entry) {
        auto const& cfg = entry.config;
        if (cfg.dimension != dim)
            return;
        double const dx    = cfg.x - x;
        double const dy    = cfg.y - y;
        double const dz    = cfg.z - z;
        double const dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            best      = id;
        }
    });
    return best;
}

void ItemDisplayManager::refreshLocked(int64_t id) {

    auto* it  = configOf(id);
    auto* rit = runtimeOf(id);
    if (it == nullptr || rit == nullptr)
        return;

    auto& rt    = *rit;
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
        if (it->enabled)
            toSpawn.push_back(player);
    }

    if (replaced) {
        rt.uniqueId  = allocUniqueIdLocked();
        rt.runtimeId = allocRuntimeIdLocked();
    }

    for (auto* player : toSpawn) {
        if (spawnForPlayer(id, (*it), rt, *player)) {
            rt.shownPlayers.insert(player->getUuid());
        }
    }
}

void ItemDisplayManager::processDirtyLocked() {

    if (mDirtyIds.empty())
        return;
    auto dirty = std::move(mDirtyIds);
    mDirtyIds.clear();

    for (auto id : dirty) {
        if (configOf(id) == nullptr)
            continue;
        refreshLocked(id);
        syncOneLocked(id);
    }
}

} // namespace debugshape_export
