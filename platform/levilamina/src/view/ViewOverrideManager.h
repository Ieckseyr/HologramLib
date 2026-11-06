#pragma once

#include "ViewOverrideLogic.h"

#include "hologramlib/HologramLib.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mc/network/NetworkIdentifier.h>
#include <mc/world/actor/DataItem.h>

class NetworkIdentifier;
class Player;
class Packet;
class Actor;

namespace debugshape_export::view {

void                     pushSpawnEvent(std::string event);
std::vector<std::string> drainSpawnEvents();

class ViewOverrideManager {
public:
    static ViewOverrideManager& getInstance();

    bool                      overrideEntity(std::string const& playerName, std::int64_t uniqueId, hologramlib::EntityView const& view);
    bool                      overrideBlock(std::string const& playerName, int x, int y, int z, hologramlib::BlockView const& view);
    bool                      clearEntity(std::string const& playerName, std::int64_t uniqueId);
    bool                      clearBlock(std::string const& playerName, int x, int y, int z);
    void                      clearAll(std::string const& playerName);
    [[nodiscard]] std::string describeFor(std::string const& playerName) const;

    // 1.36.0：读真实实体信息 / 把覆盖参数对齐到真实实体
    [[nodiscard]] hologramlib::EntityInfo queryEntity(std::int64_t uniqueId) const;
    bool                                 alignEntity(std::string const& playerName, std::int64_t uniqueId);

    [[nodiscard]] bool active() const { return mActive.load(std::memory_order_relaxed); }

    [[nodiscard]] bool hasOverridesFor(std::string const& playerName) const;

    struct Applied {
        bool hit{false};
        bool drop{false};

        std::int64_t  respawnUniqueId{-1};
        std::uint64_t respawnRuntimeId{0};
        std::string   respawnIdentifier;
        std::string   respawnPlayerName;

        std::vector<std::pair<std::int64_t, std::uint32_t>> repush;

        std::vector<std::pair<std::int64_t, std::uint64_t>> nametagPush;

        std::vector<std::int64_t> metaPush;
    };

    bool applyToOutbound(::Player& target, ::Packet const& packet, Applied& applied);

    void runPostActions(::Player& target, Applied const& applied);

    void onPlayerLeave(std::string const& playerName);
    void shutdown();

    struct InputSnapshot {
        std::int64_t  uniqueId{0};
        std::uint64_t runtimeId{0};
        float         x{0}, y{0}, z{0};
        float         pitch{0}, yaw{0}, headYaw{0};
        std::uint64_t bits{0};
        int           playMode{0};
        std::uint64_t tick{0};
    };

    void noteAuthInput(std::string const& playerName, InputSnapshot const& snapshot);

    void pushSubstitutedMovement(::Player& player, InputSnapshot const& snapshot);

    [[nodiscard]] bool inputSnapshotOf(std::int64_t uniqueId, InputSnapshot& out) const;

    void scheduleTabRemoval(std::string const& playerName, std::int64_t uniqueId);
    void flushTabRemovals();

    void scheduleMetaPush(std::string const& playerName, std::int64_t uniqueId);
    void flushMetaPushes();

    void tickPulse();

    [[nodiscard]] bool uniqueIdOfPlayer(std::string const& playerName, std::int64_t& uniqueId) const;

    [[nodiscard]] std::size_t globalEntityCount() const;
    [[nodiscard]] std::size_t globalBlockCount() const;

    struct HookCounters {
        std::uint64_t calls{0};
        std::uint64_t spawn{0};
        std::uint64_t meta{0};
        std::uint64_t block{0};
        std::uint64_t chunk{0};
    };
    void                       noteHookCall();
    [[nodiscard]] HookCounters hookCounters() const;

private:
    struct EntityRecord {
        std::int64_t            uniqueId{0};
        hologramlib::EntityView view;
        bool                    runtimeKnown{false};
        std::uint64_t           runtimeId{0};
        bool                    substituted{false};
        std::string             playerName;
        float                   proxyX{0}, proxyY{0}, proxyZ{0}, proxyYaw{0}, proxyPitch{0};
        std::string             proxySig;
    };

    struct BlockRecord {
        hologramlib::BlockView view;
        std::uint32_t          viewId{0};
        std::uint32_t          realId{0};
        bool                   hasReal{false};
        int                    dim{logic::kAnyDimension};
    };

    struct Table {
        std::unordered_map<std::int64_t, EntityRecord>  entities;
        std::unordered_map<std::int64_t, BlockRecord>   blocks;
        std::unordered_map<std::uint64_t, std::int64_t> byRuntime;

        [[nodiscard]] bool empty() const { return entities.empty() && blocks.empty(); }
    };

    struct PulseState {

        struct SkinnedPush {
            float x{0}, y{0}, z{0}, yaw{0};
            bool  valid{false};
        };
        std::uint32_t                                 ticks{0};
        int                                           chunkX{0};
        int                                           chunkZ{0};
        bool                                          hasChunk{false};
        std::unordered_map<std::int64_t, bool>        visible;
        std::unordered_map<std::int64_t, SkinnedPush> pushed;
    };

    Table&       ensureTableLocked(std::string const& playerName);
    Table const* tableOfLocked(std::string const& playerName) const;

    EntityRecord* findEntityLocked(std::string const& playerName, std::int64_t uniqueId);
    EntityRecord* findEntityByRuntimeLocked(std::string const& playerName, std::uint64_t runtimeId);
    BlockRecord*  findBlockLocked(std::string const& playerName, std::int64_t posKey);
    void          refreshActiveLocked();

    [[nodiscard]] bool inputSnapshotOfLocked(std::int64_t uniqueId, InputSnapshot& out) const;

    void onEntityOverrideChanged(std::string const& playerName, std::int64_t uniqueId, EntityRecord const& record);
    void onBlockOverrideCleared(std::string const& playerName, std::int64_t posKey, BlockRecord const& record);

    void respawnForPlayer(
        ::Player&              player,
        ::Actor&               actor,
        std::string const&     identifier,
        logic::MetaPlan const& plan);

    void restoreEntityForPlayer(::Player& player, std::int64_t uniqueId, EntityRecord const& record);

    void pulseForPlayer(::Player& player);

    void restoreCleared(
        std::string const&                                        playerName,
        std::vector<std::pair<std::int64_t, EntityRecord>> const& entities,
        std::vector<std::int64_t> const&                          blockKeys);

    mutable std::mutex mMutex;

    std::vector<std::pair<std::uint64_t, std::pair<std::string, std::int64_t>>> mPendingTabRemoval;

    std::vector<std::tuple<std::uint64_t, std::string, std::int64_t>> mPendingMetaPush;
    Table                                                             mGlobal;
    std::unordered_map<std::string, Table>                            mPlayers;
    std::unordered_map<std::string, PulseState>                       mPulse;
    std::unordered_map<std::string, InputSnapshot>                    mInputs;

    std::uint64_t     mServerTick{0};
    std::atomic<bool> mActive{false};

    std::atomic<std::uint64_t> mHookCalls{0};
    std::atomic<std::uint64_t> mSpawnSeen{0};
    std::atomic<std::uint64_t> mMetaSeen{0};
    std::atomic<std::uint64_t> mBlockSeen{0};
    std::atomic<std::uint64_t> mChunkSeen{0};
    std::atomic<std::uint64_t> mControlSteps{0};
    std::atomic<std::uint64_t> mSkinnedSpawn{0};
};

void initViewOverrideEvents();

} // namespace debugshape_export::view

namespace debugshape_export {

hologramlib::IViewOverride& viewOverrideAdapter();

}
