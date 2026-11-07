#include "ViewOverrideRuntimeOps.h"
#include "core/DiagFlags.h"

#include "ViewOverrideManager.h"

#include "ViewPackets.h"

#include "customentity/CustomEntityManager.h"

#include "DiagLog.h"

#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/network/MinecraftPacketIds.h>
#include <mc/network/Packet.h>
#include <mc/network/packet/AddActorPacketPayload.h>
#include <mc/network/packet/AddItemActorPacketPayload.h>
#include <mc/network/packet/AddPlayerPacketPayload.h>
#include <mc/network/packet/LevelChunkPacketPayload.h>
#include <mc/network/packet/MoveActorAbsoluteData.h>
#include <mc/network/packet/MoveActorAbsolutePacketPayload.h>
#include <mc/network/packet/MoveActorDeltaData.h>
#include <mc/network/packet/MoveActorDeltaPacketPayload.h>
#include <mc/network/packet/MovePlayerPacketPayload.h>
#include <mc/network/packet/SetActorDataPacketPayload.h>
#include <mc/network/packet/SetActorMotionPacketPayload.h>
#include <mc/network/packet/UpdateBlockPacketPayload.h>
#include <mc/network/packet/UpdateBlockSyncedPacketPayload.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/ActorDataIDs.h>
#include <mc/world/actor/ActorFlags.h>
#include <mc/world/actor/ActorType.h>
#include <mc/world/actor/player/Inventory.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/block/Block.h>

#include <cmath>
#include <format>
#include <optional>
#include <utility>

namespace debugshape_export::view {
using namespace vo_ops;

ViewOverrideManager& ViewOverrideManager::getInstance() {
    static ViewOverrideManager instance;
    return instance;
}

ViewOverrideManager::Table& ViewOverrideManager::ensureTableLocked(std::string const& playerName) {
    if (playerName.empty())
        return mGlobal;
    return mPlayers[playerName];
}

ViewOverrideManager::Table const* ViewOverrideManager::tableOfLocked(std::string const& playerName) const {
    if (playerName.empty())
        return &mGlobal;
    auto const it = mPlayers.find(playerName);
    return it == mPlayers.end() ? nullptr : &it->second;
}

ViewOverrideManager::EntityRecord*
ViewOverrideManager::findEntityLocked(std::string const& playerName, std::int64_t uniqueId) {
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto const e = it->second.entities.find(uniqueId); e != it->second.entities.end())
                return &e->second;
        }
    }
    if (auto const e = mGlobal.entities.find(uniqueId); e != mGlobal.entities.end())
        return &e->second;
    return nullptr;
}

ViewOverrideManager::EntityRecord*
ViewOverrideManager::findEntityByRuntimeLocked(std::string const& playerName, std::uint64_t runtimeId) {
    // 只查表：引擎侧确认（runtimeId 是否仍属于这个 uniqueId）由调用方解锁后做，见 vo_ops::runtimeBelongsTo
    auto lookup = [&](Table& table) -> EntityRecord* {
        auto const idx = table.byRuntime.find(runtimeId);
        if (idx == table.byRuntime.end())
            return nullptr;
        auto const e = table.entities.find(idx->second);
        if (e == table.entities.end())
            return nullptr;
        return &e->second;
    };
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto* rec = lookup(it->second))
                return rec;
        }
    }
    return lookup(mGlobal);
}

ViewOverrideManager::BlockRecord* ViewOverrideManager::findBlockLocked(std::string const& playerName, std::int64_t posKey) {
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto const b = it->second.blocks.find(posKey); b != it->second.blocks.end())
                return &b->second;
        }
    }
    if (auto const b = mGlobal.blocks.find(posKey); b != mGlobal.blocks.end())
        return &b->second;
    return nullptr;
}

void ViewOverrideManager::refreshActiveLocked() {

    bool any = !mGlobal.empty();
    if (!any) {
        for (auto const& [name, table] : mPlayers) {
            if (!table.empty()) {
                any = true;
                break;
            }
        }
    }
    mActive.store(any, std::memory_order_relaxed);
}

bool ViewOverrideManager::hasOverridesFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    if (!mGlobal.empty())
        return true;
    auto const it = mPlayers.find(playerName);
    return it != mPlayers.end() && !it->second.empty();
}

bool ViewOverrideManager::overrideBlock(
    std::string const&            playerName,
    int                           x,
    int                           y,
    int                           z,
    hologramlib::BlockView const& view) {
    if (hlibDomainDisabled("HLIB_NO_VIEW"))
        return false;
    auto const type = logic::normaliseType(view.type);
    if (type.empty())
        return clearBlock(playerName, x, y, z);

    auto const networkId = networkIdOfBlockType(type);
    if (!networkId) {
        HLIB_LOG_WARN("HologramLib: 方块覆盖失败, 类型名不在注册表: {}", type);
        return false;
    }

    auto const key = logic::packPos(x, y, z);
    auto const dim = dimensionOfTarget(playerName);
    {
        std::lock_guard lock(mMutex);
        auto&           table = ensureTableLocked(playerName);
        auto&           rec   = table.blocks[key];
        rec.view              = hologramlib::BlockView{type};
        rec.viewId            = *networkId;
        rec.dim               = dim;
        refreshActiveLocked();
    }

    forEachTargetPlayer(playerName, [&](::Player& player) {
        sendBlockUpdate(player, x, y, z, *networkId);
    });
    return true;
}

hologramlib::EntityInfo ViewOverrideManager::queryEntity(std::int64_t uniqueId) const {
    hologramlib::EntityInfo info{};
    auto                    level = ::ll::service::getLevel();
    if (!level || uniqueId <= 0)
        return info;

    auto* actor = level->fetchEntity(::ActorUniqueID(uniqueId), false);
    if (actor == nullptr)
        return info;

    info.identifier = actor->getTypeName();
    info.nameTag    = actor->getNameTag();
    info.hasNameTag = !info.nameTag.empty();
    info.isPlayer   = actor->isPlayer();
    info.health     = actor->getHealth();
    return info;
}

bool ViewOverrideManager::alignEntity(std::string const& playerName, std::int64_t uniqueId) {
    auto const info = queryEntity(uniqueId);
    if (info.identifier.empty())
        return false;

    hologramlib::EntityView view;
    view.identifier = info.identifier;
    view.hasNametag = info.hasNameTag;
    view.nametag    = info.nameTag;
    return overrideEntity(playerName, uniqueId, view);
}

bool ViewOverrideManager::clearEntity(std::string const& playerName, std::int64_t uniqueId) {
    EntityRecord record;
    bool         existed = false;
    {
        std::lock_guard lock(mMutex);
        auto            erase = [&](Table& table) {
            auto const it = table.entities.find(uniqueId);
            if (it == table.entities.end())
                return;
            if (it->second.runtimeKnown)
                table.byRuntime.erase(it->second.runtimeId);
            record = it->second;
            table.entities.erase(it);
            existed = true;
        };
        if (!playerName.empty()) {
            if (auto const it = mPlayers.find(playerName); it != mPlayers.end())
                erase(it->second);
        }
        erase(mGlobal);
        refreshActiveLocked();
    }

    if (existed) {
        forEachRestoreViewer(playerName, uniqueId, !record.view.asPlayer.empty(), [&](::Player& player) {
            restoreEntityForPlayer(player, uniqueId, record);
        });
    }
    return existed;
}

bool ViewOverrideManager::clearBlock(std::string const& playerName, int x, int y, int z) {
    auto const key     = logic::packPos(x, y, z);
    bool       existed = false;
    {
        std::lock_guard lock(mMutex);
        auto            erase = [&](Table& table) {
            if (table.blocks.erase(key) > 0)
                existed = true;
        };
        if (!playerName.empty()) {
            if (auto const it = mPlayers.find(playerName); it != mPlayers.end())
                erase(it->second);
        }
        erase(mGlobal);
        refreshActiveLocked();
    }
    if (!existed)
        return false;

    forEachTargetPlayer(playerName, [&](::Player& player) {
        sendBlockUpdate(player, x, y, z, realBlockNetworkIdAt(player, x, y, z));
    });
    return true;
}

void ViewOverrideManager::clearAll(std::string const& playerName) {
    std::vector<std::pair<std::int64_t, EntityRecord>> entities;
    std::vector<std::int64_t>                          blockKeys;
    {
        std::lock_guard lock(mMutex);
        auto            collect = [&](Table& table) {
            for (auto const& [uniqueId, record] : table.entities)
                entities.emplace_back(uniqueId, record);
            for (auto const& [key, record] : table.blocks)
                blockKeys.push_back(key);
        };
        if (playerName.empty()) {
            collect(mGlobal);
            for (auto& [name, table] : mPlayers)
                collect(table);
            mPlayers.clear();
            mPulse.clear();
            mGlobal = Table{};
        } else if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            collect(it->second);
            mPlayers.erase(it);
            mPulse.erase(playerName);
            collect(mGlobal);
        }
        refreshActiveLocked();
    }
    restoreCleared(playerName, entities, blockKeys);
}

void ViewOverrideManager::restoreCleared(
    std::string const&                                        playerName,
    std::vector<std::pair<std::int64_t, EntityRecord>> const& entities,
    std::vector<std::int64_t> const&                          blockKeys) {
    if (entities.empty() && blockKeys.empty())
        return;
    forEachTargetPlayer(playerName, [&](::Player& player) {
        for (auto const key : blockKeys) {
            auto const x = logic::unpackX(key);
            auto const y = logic::unpackY(key);
            auto const z = logic::unpackZ(key);
            sendBlockUpdate(player, x, y, z, realBlockNetworkIdAt(player, x, y, z));
        }
    });
    for (auto const& [uniqueId, record] : entities) {
        forEachRestoreViewer(playerName, uniqueId, !record.view.asPlayer.empty(), [&](::Player& player) {
            restoreEntityForPlayer(player, uniqueId, record);
        });
    }
}

void ViewOverrideManager::respawnForPlayer(
    ::Player&              player,
    ::Actor&               actor,
    std::string const&     identifier,
    logic::MetaPlan const& plan) {
    if (hlibDomainDisabled("HLIB_NO_VIEW"))
        return;
    if (actor.isPlayer())
        return;

    auto const uniqueId = actor.getOrCreateUniqueID().rawID;
    if (actor.getTypeName() == identifier) {

        if (plan.setName) {
            view::sendViewMeta(player, static_cast<std::uint64_t>(actor.getRuntimeID()), logic::metaPlanOf(true, plan.name, plan.alwaysShow));
        } else if (!plan.empty()) {
            view::sendViewMeta(player, static_cast<std::uint64_t>(actor.getRuntimeID()), plan);
        }
        return;
    }
    sendRemoveActor(player, uniqueId);
    view::sendActorSpawn(player, actor, identifier, plan);
}

void ViewOverrideManager::restoreEntityForPlayer(
    ::Player&           player,
    std::int64_t        uniqueId,
    EntityRecord const& record) {
    if (hlibDomainDisabled("HLIB_NO_VIEW"))
        return;
    auto* actor = liveActorNear(player, uniqueId);
    if (actor == nullptr) {

        auto* asPlayer = playerByUniqueId(uniqueId);
        if (asPlayer != nullptr && player.distanceTo(*asPlayer) <= 64.0f)
            actor = asPlayer;
    }

    if (actor != nullptr && actor->isPlayer()) {
        auto& target = *static_cast<::Player*>(actor);

        if (!record.view.asPlayer.empty())
            sendPlayerSkin(player, target, target.getRealName());

        bool const bodyTouched = record.substituted || record.view.hidden || !record.view.identifier.empty();
        if (bodyTouched && &player != &target) {

            proto::SerializedSkinRef ownSkin;
            if (captureOwnSkin(target, ownSkin)) {

                InputSnapshot snap{};
                int const     playMode = inputSnapshotOf(uniqueId, snap) ? snap.playMode : 0;
                sendRemoveActor(player, uniqueId);
                sendRealPlayerBody(player, target, ownSkin, playMode);
            }
            return;
        }

        if (record.view.hasNametag) {
            view::sendViewMeta(player, static_cast<std::uint64_t>(target.getRuntimeID()), logic::metaPlanOf(true, target.getNameTag(), false));
        }
        return;
    }

    if (record.substituted)
        sendRemoveActor(player, uniqueId);

    if (record.view.hidden || !record.view.identifier.empty() || !record.view.skinId.empty()) {
        if (actor == nullptr)
            return;
        respawnForPlayer(player, *actor, actor->getTypeName(), logic::metaPlanOf(true, actor->getNameTag(), false));
        return;
    }

    if (record.view.hasNametag && actor != nullptr) {
        view::sendViewMeta(
            player,
            static_cast<std::uint64_t>(actor->getRuntimeID()),
            logic::metaPlanOf(true, actor->getNameTag(), false));
    }
}

bool ViewOverrideManager::inputSnapshotOf(std::int64_t uniqueId, InputSnapshot& out) const {
    std::lock_guard lock(mMutex);
    return inputSnapshotOfLocked(uniqueId, out);
}

bool ViewOverrideManager::inputSnapshotOfLocked(std::int64_t uniqueId, InputSnapshot& out) const {
    for (auto const& [name, snap] : mInputs) {
        if (snap.uniqueId == uniqueId && mServerTick - snap.tick <= 20) {
            out = snap;
            return true;
        }
    }
    return false;
}

bool ViewOverrideManager::uniqueIdOfPlayer(std::string const& playerName, std::int64_t& uniqueId) const {
    bool found = false;
    forEachTargetPlayer(playerName, [&](::Player& p) {
        uniqueId = p.getOrCreateUniqueID().rawID;
        found    = true;
    });
    return found;
}

void ViewOverrideManager::noteAuthInput(std::string const& playerName, InputSnapshot const& snapshot) {
    std::lock_guard lock(mMutex);
    auto            stamped = snapshot;
    stamped.tick            = mServerTick;
    mInputs[playerName]     = stamped;
}

ViewOverrideManager::HookCounters ViewOverrideManager::hookCounters() const {
    HookCounters out;
    out.calls = mHookCalls.load(std::memory_order_relaxed);
    out.spawn = mSpawnSeen.load(std::memory_order_relaxed);
    out.meta  = mMetaSeen.load(std::memory_order_relaxed);
    out.block = mBlockSeen.load(std::memory_order_relaxed);
    out.chunk = mChunkSeen.load(std::memory_order_relaxed);
    return out;
}

std::size_t ViewOverrideManager::globalEntityCount() const {
    std::lock_guard lock(mMutex);
    return mGlobal.entities.size();
}

std::size_t ViewOverrideManager::globalBlockCount() const {
    std::lock_guard lock(mMutex);
    return mGlobal.blocks.size();
}

} // namespace debugshape_export::view

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

namespace {

using view::ViewOverrideManager;

class ViewOverrideAdapter : public hologramlib::IViewOverride {
public:
    bool overrideEntity(
        std::string const&             playerName,
        std::int64_t                   entityUniqueId,
        hologramlib::EntityView const& view) override {
        return ViewOverrideManager::getInstance().overrideEntity(playerName, entityUniqueId, view);
    }

    bool overrideBlock(
        std::string const&            playerName,
        int                           x,
        int                           y,
        int                           z,
        hologramlib::BlockView const& view) override {
        return ViewOverrideManager::getInstance().overrideBlock(playerName, x, y, z, view);
    }

    [[nodiscard]] hologramlib::EntityInfo queryEntity(std::int64_t entityUniqueId) const override {
        return ViewOverrideManager::getInstance().queryEntity(entityUniqueId);
    }

    bool alignEntity(std::string const& playerName, std::int64_t entityUniqueId) override {
        return ViewOverrideManager::getInstance().alignEntity(playerName, entityUniqueId);
    }

    bool clearEntity(std::string const& playerName, std::int64_t entityUniqueId) override {
        return ViewOverrideManager::getInstance().clearEntity(playerName, entityUniqueId);
    }

    bool clearBlock(std::string const& playerName, int x, int y, int z) override {
        return ViewOverrideManager::getInstance().clearBlock(playerName, x, y, z);
    }

    void clearAll(std::string const& playerName) override {
        ViewOverrideManager::getInstance().clearAll(playerName);
    }

    [[nodiscard]] std::string describeFor(std::string const& playerName) const override {
        return ViewOverrideManager::getInstance().describeFor(playerName);
    }
};

} // namespace

hologramlib::IViewOverride& viewOverrideAdapter() {
    static ViewOverrideAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
