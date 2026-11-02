// BossBarManager.h - Boss 条（1.34.0）
//
// 协议: BossEventPacket(74)。字段: TargetActorID / EventType / Name / FilteredName /
//       Percentage(float 0~1) / Color(BossBarColor) / Overlay(BossBarOverlay)。
//
// 不需要实体: 目标 id 用库内虚拟号段（actorIds::bossBar）里分配的值，客户端拿它当"这条 Boss 条的键"，
// 服务端不存在对应 actor。所以创建/更新/删除全程只有包。
//
// 每条只属于一名玩家（协议是"发给某收件人"的），给所有人看要每人各建一条。
// 玩家下线时清理（客户端随下线消失，不需要再发包）。
#pragma once

#include "hologramlib/HologramLib.h" // BossBarColor / BossBarOverlay

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

class Player;

namespace debugshape_export {

class BossBarManager {
public:
    static BossBarManager& getInstance();

    int64_t add(
        std::string const&          playerName,
        std::string const&          title,
        float                       healthPercent,
        hologramlib::BossBarColor   color,
        hologramlib::BossBarOverlay overlay);
    bool setTitle(int64_t id, std::string const& title);
    bool setHealth(int64_t id, float healthPercent);
    bool setStyle(int64_t id, hologramlib::BossBarColor color, hologramlib::BossBarOverlay overlay);
    bool remove(int64_t id);
    void removeAll(std::string const& playerName);
    int  count(std::string const& playerName) const;

    void onPlayerLeave(std::string const& playerName);
    void shutdown();

private:
    BossBarManager() = default;

    struct Bar {
        int64_t                     id{-1};
        std::string                 playerName;
        std::int64_t                targetId{0}; // 虚拟 actor uniqueId
        std::string                 title;
        float                       percent{1.0f};
        hologramlib::BossBarColor   color{hologramlib::BossBarColor::Purple};
        hologramlib::BossBarOverlay overlay{hologramlib::BossBarOverlay::Progress};
    };

    // 组包并只发给这条 Boss 条的持有者；返回是否发出
    static bool         send(Player& player, Bar const& bar, int eventType);
    static std::int64_t nextTargetId();

    mutable std::mutex               mMutex;
    std::unordered_map<int64_t, Bar> mBars;
};

} // namespace debugshape_export
