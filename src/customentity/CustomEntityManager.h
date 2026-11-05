#pragma once

#include "core/Registry.h"
#include "core/VirtualActorIds.h"
#include "core/VisibilitySync.h"

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
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

using CustomEntityConfig = hologramlib::CustomEntityConfig;

// 自定义实体
class CustomEntityManager {
public:
    static CustomEntityManager& getInstance();

    void init();
    void shutdown();

    int64_t create(CustomEntityConfig const& config);

    int64_t createRandom(CustomEntityConfig const& config);

    int64_t createWithId(CustomEntityConfig const& config, int64_t desiredId);

    bool isIdUsed(int64_t id) const;
    bool destroy(int64_t id);
    void destroyAll();
    bool exists(int64_t id) const;
    bool get(int64_t id, CustomEntityConfig& out) const;

    bool setIdentifier(int64_t id, std::string const& identifier);
    bool setPosition(int64_t id, float x, float y, float z, int dim);
    bool setRotation(int64_t id, float yaw, float pitch);
    bool setNametag(int64_t id, std::string const& text);
    bool setScale(int64_t id, float scale);
    bool setVariant(int64_t id, int variant);
    bool setMarkVariant(int64_t id, int markVariant);
    bool setColorIndex(int64_t id, int colorIndex);
    bool setFlags(int64_t id, int64_t flags);
    bool setInvisible(int64_t id, bool on);
    bool setEnabled(int64_t id, bool enabled);
    bool setViewDistance(int64_t id, double dist);
    bool setPose(int64_t id, int pose);

    bool setEquipmentSlot(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt);

    bool setMobProperty(int64_t id, std::string const& name, std::string const& value);
    bool clearMobProperties(int64_t id);

    bool scaleBy(int64_t id, double factor);

    bool        setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames);
    bool        clearVisiblePlayers(int64_t id);
    bool        setVisiblePlayer(int64_t id, std::string const& playerName);
    std::string getDebugInfo(int64_t id) const;

    bool setRidePlayer(int64_t id, std::string const& playerName);
    bool setRideEntity(int64_t id, int64_t vehicleEntityId);
    bool clearRide(int64_t id);

    bool playAnimation(
        int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks);

    bool playAnimationTo(
        int64_t id, std::string const& playerName,
        std::string const& animation, std::string const& stopExpression, int durationTicks);

    bool stopAnimation(int64_t id);
    bool stopAnimationTo(int64_t id, std::string const& playerName);

    using EntitySpawnCallback = std::function<void(int64_t id, std::string const& playerName)>;
    void setEntitySpawnCallback(EntitySpawnCallback callback);
    void drainSpawnCallbacks();

    std::vector<std::string> pollSpawns();
    void                     clearSpawns();

    std::vector<int64_t> getAllIds() const;

    int64_t findNearest(float x, float y, float z, int dim, double maxDist) const;

    bool findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const;

    bool getIdPair(int64_t id, std::uint64_t& outUniqueId, std::uint64_t& outRuntimeId) const;

    bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw, float pitch);
    bool clearPlayerRotation(int64_t id, std::string const& playerName);
    bool clearPlayerRotations(int64_t id);

    bool setPlayerNametag(int64_t id, std::string const& playerName, std::string const& text);

    bool setPlayerScale(int64_t id, std::string const& playerName, float scale);

    bool setPlayerEquipmentSlot(
        int64_t id, std::string const& playerName, int slot, std::string const& name, int aux, std::string const& nbt);

    bool clearPlayerAppearance(int64_t id, std::string const& playerName);

private:
    CustomEntityManager()  = default;
    ~CustomEntityManager() = default;

public:
    struct Runtime {
        std::uint64_t                 uniqueId{};
        std::uint64_t                 runtimeId{};
        std::unordered_set<mce::UUID> shownPlayers;

        std::unordered_map<mce::UUID, hologramlib::PerPlayerRotation> playerRot;

        std::unordered_map<mce::UUID, std::string> playerNameTag;
        std::unordered_map<mce::UUID, float>       playerScale;

        std::unordered_map<mce::UUID, std::array<hologramlib::CustomEntityEquipment, 6>> playerEquip;
    };

private:
    void refreshLocked(int64_t id);

    void notifySpawnLocked(int64_t id, Player& player);
    void refreshLightLocked(int64_t id);
    void syncVisibilityLocked();
    void syncOneLocked(int64_t id);
    bool spawnOneLocked(int64_t id, CustomEntityConfig const& data, Runtime& rt, Player& player);

    [[nodiscard]] std::unordered_set<std::string> const* filterOf(int64_t id) const;

    void processDirtyLocked();

    int64_t createLocked(CustomEntityConfig const& config, int64_t id);

    [[nodiscard]] std::uint64_t allocUniqueIdLocked();
    [[nodiscard]] std::uint64_t allocRuntimeIdLocked();

    [[nodiscard]] std::optional<std::uint64_t> resolveVehicleUniqueIdLocked(CustomEntityConfig const& data);

    void flushAnimsLocked();

    friend struct CustomEntityTickHookAccess;

    struct EntityAnimEntry {
        mce::UUID     playerUuid;
        std::uint64_t runtimeId;
        int64_t       entityId;
        std::string   animation;
        std::string   controller;
        std::string   stopExpression;
    };

    mutable std::recursive_mutex mMutex;
    core::PlayerGrid             mGrid;
    std::vector<mce::UUID>       mSeenScratch;

    struct Entry {
        CustomEntityConfig config;
        Runtime            runtime;
    };

    [[nodiscard]] CustomEntityConfig* configOf(int64_t id) noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->config;
    }
    [[nodiscard]] Runtime* runtimeOf(int64_t id) noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->runtime;
    }
    [[nodiscard]] CustomEntityConfig const* configOf(int64_t id) const noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->config;
    }
    [[nodiscard]] Runtime const* runtimeOf(int64_t id) const noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->runtime;
    }

    core::Registry<Entry, core::NoLock> mEntries;

    std::unordered_set<int64_t> mDirtyIds;

    std::unordered_set<int64_t> mLightDirtyIds;

    std::unordered_map<int64_t, std::unordered_set<std::string>> mVisibleFilter;
    std::multimap<std::uint64_t, EntityAnimEntry>                mAnimQueue;
    EntitySpawnCallback                                          mSpawnCallback;

    std::deque<std::pair<int64_t, std::string>>  mSpawnQueue;
    std::vector<std::pair<int64_t, std::string>> mPendingSpawnCallbacks;

    std::uint64_t                 mNextActorUniqueId{0x6E00000000000001ULL};
    std::uint64_t                 mNextRuntimeId{core::actorIds::customEntity.first};
    std::unordered_set<mce::UUID> mInitializedPlayers;
    ll::event::ListenerPtr        mJoinListener;
    ll::event::ListenerPtr        mDisconnectListener;
};

} // namespace debugshape_export
