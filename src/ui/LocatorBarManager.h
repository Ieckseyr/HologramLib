// LocatorBarManager.h - 方位条航点（26.51.7）
//
// 每个玩家一组航点，key 由调用方起名；库按 key 稳定派生 UUID（同一 key 永远同一 UUID），
// 所以删除/更新不需要调用方保管 UUID。
#pragma once

#include "hologramlib/HologramLib.h" // ILocatorBar

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

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
    bool              removeWaypoint(std::string const& playerName, std::string const& key);
    int               clearWaypoints(std::string const& playerName);
    [[nodiscard]] int waypointCount(std::string const& playerName) const;

    // 玩家离开时清记录（客户端那边的航点随下线消失，不需要发包）
    void forget(std::string const& playerName);
    void shutdown();

private:
    LocatorBarManager() = default;

    struct Handle {
        std::uint64_t msb{0};
        std::uint64_t lsb{0};
    };

    // key → 句柄；一个玩家一张表
    std::unordered_map<std::string, std::unordered_map<std::string, Handle>> mWaypoints;
    mutable std::mutex                                                       mMutex;
};

} // namespace debugshape_export
