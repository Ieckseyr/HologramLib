#pragma once

#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

class Player;

namespace debugshape_export {

// Boss 条
class BossBarManager {
public:
    static BossBarManager& getInstance();

    int64_t add(
        std::string const&          playerName,
        std::string const&          title,
        float                       healthPercent,
        hologramlib::BossBarColor   color,
        hologramlib::BossBarOverlay overlay);
    bool        setTitle(int64_t id, std::string const& title);
    bool        setHealth(int64_t id, float healthPercent);
    bool        setStyle(int64_t id, hologramlib::BossBarColor color, hologramlib::BossBarOverlay overlay);
    bool        remove(int64_t id);
    void        removeAll(std::string const& playerName);
    int         count(std::string const& playerName) const;
    std::string getDebugInfo(int64_t id) const;

    int64_t addOn(
        std::string const&          playerName,
        std::int64_t                targetId,
        std::string const&          title,
        float                       healthPercent,
        hologramlib::BossBarColor   color,
        hologramlib::BossBarOverlay overlay);

    std::string probeWire() const;

    void onPlayerLeave(std::string const& playerName);
    void shutdown();

private:
    BossBarManager() = default;

    struct Bar {
        int64_t                     id{-1};
        std::string                 playerName;
        std::int64_t                targetId{0};
        std::string                 title;
        float                       percent{1.0f};
        hologramlib::BossBarColor   color{hologramlib::BossBarColor::Purple};
        hologramlib::BossBarOverlay overlay{hologramlib::BossBarOverlay::Progress};
    };

    static bool         send(Player& player, Bar const& bar, int eventType, std::int64_t playerId);
    static std::int64_t nextTargetId();

    mutable std::mutex               mMutex;
    std::unordered_map<int64_t, Bar> mBars;
};

} // namespace debugshape_export
