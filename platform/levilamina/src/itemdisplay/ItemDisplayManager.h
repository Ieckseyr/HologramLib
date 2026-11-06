#pragma once

#include "core/Registry.h"
#include "core/VirtualActorIds.h"
#include "kernel/VisibilitySync.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ll/api/event/Listener.h>

#include "hologramlib/HologramLib.h"

#include "mc/platform/UUID.h"
#include "mc/world/item/ItemStack.h"

class Player;

namespace debugshape_export {

using ItemDisplayConfig = hologramlib::ItemDisplayConfig;
using ItemDisplayAnim   = hologramlib::ItemDisplayAnim;

// 物品展示实体
class ItemDisplayManager {
public:
    static ItemDisplayManager& getInstance();

    void init();
    void shutdown();

    int64_t create(ItemDisplayConfig const& config);

    int64_t createRandom(ItemDisplayConfig const& config);

    int64_t createWithId(ItemDisplayConfig const& config, int64_t desiredId);

    int64_t createSeamless(
        ItemDisplayConfig const& config,
        int                      mode,
        double                   viewDistance,
        std::string const&       visiblePlayer);

    bool isIdUsed(int64_t id) const;
    bool destroy(int64_t id);
    void destroyAll();
    bool exists(int64_t id) const;
    bool get(int64_t id, ItemDisplayConfig& out) const;

    bool setItem(int64_t id, std::string const& item, int aux);
    bool setItemWithNbt(int64_t id, std::string const& item, int aux, std::string const& nbt);
    bool setPosition(int64_t id, float x, float y, float z, int dim);
    bool setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz);
    bool setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz);
    bool setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz);
    bool setScale(int64_t id, std::string const& scale);
    bool setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry);
    bool setMode(int64_t id, int mode);
    bool setEnabled(int64_t id, bool enabled);
    bool setViewDistance(int64_t id, double dist);
    bool rotateY(int64_t id, float delta);
    bool scaleBy(int64_t id, double factor);
    bool setGlint(int64_t id, bool on);

    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames);
    bool clearVisiblePlayers(int64_t id);

    bool setVisiblePlayer(int64_t id, std::string const& playerName);

    std::string getDebugInfo(int64_t id) const;

    bool scaleTo(int64_t id, double targetScale);

    bool follow(int64_t id, std::string const& playerName, float offX, float offY, float offZ);
    bool unfollow(int64_t id);

    bool setHitbox(int64_t id, float width, float height);

    bool setStretch(int64_t id, std::string const& xz, std::string const& y);
    bool setStretchLive(int64_t id, std::string const& xz, std::string const& y);

    // 彩蛋：让展示方块/物品也做苦力怕膨胀（rotate = 一边膨胀一边转）
    bool playCreeperSwelling(int64_t id, int durationTicks, bool rotate);

    // 彩蛋到期还原拉伸（由 tick 驱动调用）
    void tickEasterEggs();

    bool setRotationLive(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz);
    bool setOffsetLive(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz);
    bool setScaleLive(int64_t id, std::string const& scale);
    bool setTransformLive(
        int64_t            id,
        std::string const& ox, std::string const& oy, std::string const& oz,
        std::string const& rx, std::string const& ry, std::string const& rz,
        std::string const& scale);

    bool setAnim(int64_t id, ItemDisplayAnim const& anim);

    bool setAnimPreset(int64_t id, std::string const& preset, float a, float b, int c);
    bool clearAnim(int64_t id);

    int emitForEntity(std::string const& playerName, std::int64_t uniqueId, ItemDisplayConfig const& cfg);

    std::vector<int64_t> getAllIds() const;

    int64_t findNearest(float x, float y, float z, int dim, double maxDist) const;

    bool findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const;

    std::uint64_t runtimeIdOf(int64_t id) const;

private:
    ItemDisplayManager()  = default;
    ~ItemDisplayManager() = default;

public:
    struct Runtime {
        std::uint64_t                 uniqueId{};
        std::uint64_t                 runtimeId{};
        std::unordered_set<mce::UUID> shownPlayers;

        std::optional<::ItemStack> cachedStack;
        std::string                cachedItemName{};
        int                        cachedItemAux{0};
        std::string                cachedItemNbt{};
        bool                       cachedGlint{false};
        bool                       itemWarned{false};

        std::string stretchXZ{"1"};
        std::string stretchY{"1"};

        bool            animActive{false};
        ItemDisplayAnim anim{};
        struct AnimBase {
            std::string ox{"0"}, oy{"-4"}, oz{"0"};
            std::string rx{"180"}, ry{"0"}, rz{"180"};
            std::string scale{"0.375"};
        } animBase;
    };

private:
    void refreshLocked(int64_t id);
    void syncVisibilityLocked();
    void syncOneLocked(int64_t id);
    bool spawnOneLocked(int64_t id, ItemDisplayConfig const& data, Runtime& rt, Player& player);

    [[nodiscard]] std::unordered_set<std::string> const* filterOf(int64_t id) const;

    void processDirtyLocked();

    void followTickLocked();

    int64_t createLocked(ItemDisplayConfig const& config, int64_t id);

    bool resendFmbeLocked(int64_t id, ItemDisplayConfig const& data, Runtime& rt);

    void enqueueFmbeAnimsLocked(
        int64_t id, ItemDisplayConfig const& data, Runtime const& rt,
        mce::UUID const& uuid, std::uint64_t base, bool block);

    [[nodiscard]] std::uint64_t allocUniqueIdLocked();
    [[nodiscard]] std::uint64_t allocRuntimeIdLocked();

    friend struct ItemDisplayTickHookAccess;

    struct FollowState {
        std::string playerName;
        float       offX{0}, offY{0}, offZ{0};
        float       lastX{0}, lastY{0}, lastZ{0};
        bool        synced{false};
    };

    mutable std::recursive_mutex mMutex;
    core::PlayerGrid             mGrid;
    std::vector<mce::UUID>       mSeenScratch;

    struct EggState {
        int64_t       id{};
        std::uint64_t endTick{};
        std::string   oldXZ;
        std::string   oldY;
        std::string   oldRotY;
        bool          rotate{};
    };

    std::vector<EggState> mEggs;

    struct Entry {
        ItemDisplayConfig config;
        Runtime           runtime;
    };

    [[nodiscard]] ItemDisplayConfig* configOf(int64_t id) noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->config;
    }
    [[nodiscard]] Runtime* runtimeOf(int64_t id) noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->runtime;
    }
    [[nodiscard]] ItemDisplayConfig const* configOf(int64_t id) const noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->config;
    }
    [[nodiscard]] Runtime const* runtimeOf(int64_t id) const noexcept {
        auto* entry = mEntries.peek(id);
        return entry == nullptr ? nullptr : &entry->runtime;
    }

    core::Registry<Entry, core::NoLock> mEntries;
    std::unordered_set<int64_t>         mDirtyIds;

    std::unordered_map<int64_t, std::unordered_set<std::string>> mVisibleFilter;

    std::unordered_map<int64_t, FollowState> mFollowers;

    std::uint64_t                 mNextActorUniqueId{0x6D00000000000001ULL};
    std::uint64_t                 mNextRuntimeId{core::actorIds::itemDisplay.first};
    std::unordered_set<mce::UUID> mInitializedPlayers;
    ll::event::ListenerPtr        mJoinListener;
    ll::event::ListenerPtr        mDisconnectListener;
};

} // namespace debugshape_export
