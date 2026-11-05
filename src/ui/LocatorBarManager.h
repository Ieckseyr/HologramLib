#pragma once

#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace debugshape_export {

class LocatorBarManager {
public:
    static LocatorBarManager& getInstance();

    bool setWaypoint(
        std::string const& playerName,
        std::string const& key,
        float              x,
        float              y,
        float              z,
        int                dimension,
        int                color,
        bool               visible);

    bool setWaypointEx(
        std::string const& playerName,
        std::string const& key,
        float              x,
        float              y,
        float              z,
        int                dimension,
        int                color,
        bool               visible,
        std::string const& texturePath,
        float              iconWidth,
        float              iconHeight,
        bool               clientAuthority);

    bool setClientAuthority(std::string const& playerName, std::string const& key, bool enabled);
    bool setIcon(std::string const& playerName, std::string const& key, std::string const& texturePath);
    bool setIconSize(std::string const& playerName, std::string const& key, float width, float height);
    bool attachEntity(std::string const& playerName, std::string const& key, int64_t entityUniqueId);
    bool clearAttach(std::string const& playerName, std::string const& key);

    bool setGroup(std::string const& playerName, std::string const& key, std::string const& groupKey);

    int syncGroup(std::string const& playerName, std::string const& groupKey);

    bool               hideFromOthers(std::string const& playerName, bool hidden);
    [[nodiscard]] bool isHiddenFromOthers(std::string const& playerName) const;

    bool              removeWaypoint(std::string const& playerName, std::string const& key);
    int               clearWaypoints(std::string const& playerName);
    [[nodiscard]] int waypointCount(std::string const& playerName) const;

    void forget(std::string const& playerName);
    void shutdown();

private:
    LocatorBarManager() = default;

    struct Handle {
        std::uint64_t msb{0};
        std::uint64_t lsb{0};
    };

    struct Waypoint {
        Handle       handle;
        float        x{0}, y{0}, z{0};
        int          dimension{0};
        int          color{-1};
        bool         visible{true};
        bool         clientAuthority{false};
        std::string  texturePath;
        bool         hasIconSize{false};
        float        iconWidth{0}, iconHeight{0};
        std::int64_t actorUniqueId{0};
        std::string  groupKey;
    };

    std::unordered_map<std::string, std::unordered_map<std::string, Waypoint>> mWaypoints;

    static bool sendOne(::Player& player, Waypoint const& waypoint, bool isUpdate);

    static bool sendMany(::Player& player, std::vector<Waypoint> const& waypoints);

    mutable std::mutex mMutex;
};

} // namespace debugshape_export
