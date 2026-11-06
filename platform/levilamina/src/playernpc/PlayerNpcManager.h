#pragma once

#include "core/Registry.h"
#include "core/VirtualActorIds.h"
#include "kernel/VisibilitySync.h"

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <ll/api/event/Listener.h>

#include "hologramlib/HologramLib.h"

#include "mc/platform/UUID.h"

class Player;

namespace debugshape_export {

using PlayerNpcConfig = hologramlib::PlayerNpcConfig;

// 假玩家 NPC
class PlayerNpcManager {
public:
    static PlayerNpcManager& getInstance();

    void init();
    void shutdown();

    bool registerSkin(hologramlib::PlayerNpcSkin const& skin);

    bool registerSkinOpaque(hologramlib::PlayerNpcSkin const& skin);

    bool                     registerImageSkin(std::string const& pngPath, std::string const& skinId, float sizeBlocks, bool transparent);

    // 按方块尺寸注册图片皮肤（宽/高分开，不设上限）
    bool registerImageSkinSized(
        std::string const& pngPath,
        std::string const& skinId,
        float              widthBlocks,
        float              heightBlocks,
        bool               transparent
    );

    // 给已有图片 NPC 换尺寸（按方块宽高，不设上限）
    bool setImageSize(
        int64_t            id,
        std::string const& pngPath,
        float              widthBlocks,
        float              heightBlocks,
        bool               transparent
    );
    bool                     captureSkin(std::string const& skinId, std::string const& playerName);
    bool                     hasSkin(std::string const& skinId) const;
    bool                     unregisterSkin(std::string const& skinId);
    std::vector<std::string> getSkinIds() const;

    int importSkins(std::string const& dirPath);

    bool getSkinBlob(std::string const& skinId, std::string& out) const;
    bool registerSkinFromBlob(std::string const& blob);

    int64_t              create(PlayerNpcConfig const& config);
    int64_t              createRandom(PlayerNpcConfig const& config);
    int64_t              createWithId(PlayerNpcConfig const& config, int64_t desiredId);
    bool                 destroy(int64_t id);
    void                 destroyAll();
    bool                 exists(int64_t id) const;
    bool                 get(int64_t id, PlayerNpcConfig& out) const;
    bool                 isIdUsed(int64_t id) const;
    std::vector<int64_t> getAllIds() const;

    bool setPosition(int64_t id, float x, float y, float z, int dim);
    bool setRotation(int64_t id, float yaw);
    bool setNametag(int64_t id, std::string const& text);
    bool setSkin(int64_t id, std::string const& skinId);
    bool setViewDistance(int64_t id, double dist);
    bool setScale(int64_t id, float scale);
    bool setEnabled(int64_t id, bool enabled);

    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames);
    bool clearVisiblePlayers(int64_t id);
    bool setVisiblePlayer(int64_t id, std::string const& playerName);

    std::string getDebugInfo(int64_t id) const;

    std::uint64_t runtimeIdOf(int64_t id) const;

    bool findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const;

    bool setRotationLight(int64_t id, float yaw);

    bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw);
    bool clearPlayerRotation(int64_t id, std::string const& playerName);
    bool clearPlayerRotations(int64_t id);

    bool setPositionLight(int64_t id, float x, float y, float z, int dim);

    bool injectSkin(std::string const& viewerName, std::string const& targetName, std::string const& skinId);
    bool injectSkinAll(std::string const& targetName, std::string const& skinId);

    bool playAnimation(int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks);
    bool playAnimationTo(
        int64_t            id,
        std::string const& playerName,
        std::string const& animation,
        std::string const& stopExpression,
        int                durationTicks);

    bool stopAnimation(int64_t id);
    bool stopAnimationTo(int64_t id, std::string const& playerName);

    bool setPose(int64_t id, int pose);

    bool setEquipment(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt);

    bool setRideUniqueId(int64_t id, std::int64_t vehicleUniqueId);
    bool setRidePlayer(int64_t id, std::string const& playerName);
    bool setRideEntity(int64_t id, int64_t vehicleEntityId);
    bool clearRide(int64_t id);
    void setEntitySpawnCallback(std::function<void(int64_t, std::string const&)> callback);
    void drainSpawnCallbacks();
    void dropDisallowedLocked(int64_t id);

    bool playEmote(int64_t id, std::string const& pieceId, int lengthTicks);

    std::vector<std::string> pollSpawns();
    void                     clearSpawns();

    void notifySpawn(int64_t id, std::string const& playerName) const;

private:
    PlayerNpcManager()  = default;
    ~PlayerNpcManager() = default;

public:
    struct Runtime {
        std::uint64_t                 uniqueId{};
        std::uint64_t                 runtimeId{};
        std::unordered_set<mce::UUID> shownPlayers;

        std::unordered_map<mce::UUID, hologramlib::PerPlayerRotation> playerRot;

        int          pose{0};
        std::int64_t flags{0};
        struct NpcEquip {
            std::string name{};
            int         aux{0};
            std::string nbt{};
        };
        std::array<NpcEquip, 6> equip{};
        std::int64_t            rideVehicleUniqueId{0};
    };

    struct TabRemoval {
        mce::UUID     playerUuid;
        std::uint64_t dueTick{};
    };

private:
    void    refreshLocked(int64_t id);
    void    refreshLightLocked(int64_t id);
    void    syncVisibilityLocked();
    void    syncOneLocked(int64_t id, core::SpawnBudget& budget);
    bool    spawnForViewerLocked(int64_t id, PlayerNpcConfig const& config, Runtime& rt, Player& player, core::SpawnBudget& budget);
    void    processDirtyLocked();
    void    flushAnimsLocked();
    int64_t createLocked(PlayerNpcConfig const& config, int64_t id);
    bool    skinReferencedLocked(std::string const& skinId) const;

    [[nodiscard]] std::unordered_set<std::string> const* filterOf(int64_t id) const;

    void afterSpawnLocked(Player& player, int64_t id, Runtime const& rt);

    friend struct PlayerNpcTickHookAccess;

    struct Entry {
        PlayerNpcConfig config;
        Runtime         runtime;
    };

    [[nodiscard]] PlayerNpcConfig* configOf(int64_t id) noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->config;
    }
    [[nodiscard]] Runtime* runtimeOf(int64_t id) noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->runtime;
    }
    [[nodiscard]] PlayerNpcConfig const* configOf(int64_t id) const noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->config;
    }
    [[nodiscard]] Runtime const* runtimeOf(int64_t id) const noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->runtime;
    }

    mutable std::recursive_mutex                                 mMutex;
    core::Registry<Entry, core::NoLock>                          mEntries;
    core::PlayerGrid                                             mGrid;
    std::vector<mce::UUID>                                       mSeenScratch;
    std::unordered_set<int64_t>                                  mDirtyIds;
    std::unordered_set<int64_t>                                  mLightDirtyIds;
    std::unordered_map<int64_t, std::vector<TabRemoval>>         mTabRemovals;
    std::unordered_map<int64_t, std::unordered_set<std::string>> mVisibleFilter;
    std::unordered_set<std::string>                              mWarnedMissingSkins;

    struct AnimEntry {
        mce::UUID     playerUuid;
        std::uint64_t runtimeId{0};
        int64_t       npcId{0};
        std::string   animation;
        std::string   controller;
        std::string   stopExpression;
    };
    std::multimap<std::uint64_t, AnimEntry>          mAnimQueue;
    std::function<void(int64_t, std::string const&)> mSpawnCallback;

    mutable std::deque<std::pair<int64_t, std::string>>  mSpawnQueue;
    mutable std::vector<std::pair<int64_t, std::string>> mPendingSpawnCallbacks;

    std::uint64_t mNextActorUniqueId{0x1F600000ULL};
    std::uint64_t mNextRuntimeId{core::actorIds::playerNpc.first};

    std::unordered_set<mce::UUID> mInitializedPlayers;
    ll::event::ListenerPtr        mJoinListener;
    ll::event::ListenerPtr        mDisconnectListener;
};

} // namespace debugshape_export
