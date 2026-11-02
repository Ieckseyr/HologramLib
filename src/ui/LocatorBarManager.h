// LocatorBarManager.h - 方位条航点（26.51.6 起；1.33.0 加图标 / 大小 / 绑定实体 / 分组）
//
// 每个玩家一组航点，key 由调用方起名；库按 key 稳定派生 UUID（同一 key 永远同一 UUID），
// 所以删除/更新不需要调用方保管 UUID。
//
// 1.33.0 追加的四个属性都只是 LocatorBarServerWaypointPayload 里的可选字段：
//   TexturePath(图标) / IconSize(大小) / ActorUniqueId(绑定实体) / ClientPositionAuthority。
// 「分组」在协议里没有独立字段（一条 LocatorBarPacket 的 mWaypoints 是数组），
// 所以分组 = 把同组航点合进同一条包发送 —— 见 syncGroup()。
#pragma once

#include "hologramlib/HologramLib.h" // ILocatorBar / ILocatorBarExt

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

    // ── 1.33.0：四个扩展属性（改完立刻重发该航点）──
    // 一次性写全字段再发（首个包就带 texture/iconSize/color），避免"先 Add 再补属性"
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
    // 分组 = 同组航点合并进一条包；groupKey 空 = 从组里移出
    bool setGroup(std::string const& playerName, std::string const& key, std::string const& groupKey);
    // 把某组的所有航点用一条包发出去；返回发送的航点数
    int syncGroup(std::string const& playerName, std::string const& groupKey);

    // 对其他人隐藏该玩家（见 ILocatorBar 的说明）: 转给视图隐藏通道
    bool               hideFromOthers(std::string const& playerName, bool hidden);
    [[nodiscard]] bool isHiddenFromOthers(std::string const& playerName) const;

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

    // playerName → key → 航点
    std::unordered_map<std::string, std::unordered_map<std::string, Waypoint>> mWaypoints;

    // 单条发送（isUpdate=false 时 action=Add）
    static bool sendOne(::Player& player, Waypoint const& waypoint, bool isUpdate);
    // 多条合成一条包（分组用）
    static bool sendMany(::Player& player, std::vector<Waypoint> const& waypoints);

    mutable std::mutex mMutex;
};

} // namespace debugshape_export
