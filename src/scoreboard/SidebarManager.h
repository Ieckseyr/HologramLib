// SidebarManager.h - 侧边栏计分板（1.33.0）
//
// 只发包，不动服务端记分板：给某名玩家显示一个"只有他看得到"的侧边栏。
// 协议: SetDisplayObjective(107) + SetScore(108) + RemoveObjective(106)。
//
// 每条侧边栏用**按玩家派生**的 objective 名（不同玩家互不干扰，也不与 BDS 自己的记分板撞名）；
// 行是 FakePlayer 条目，score 用作排序键（第 0 行在最上面）。
//
// 与 BDS Scoreboard 的关系：这里显示的内容完全独立于服务端记分板，不读也不改它。
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace debugshape_export {

class SidebarManager {
public:
    static SidebarManager& getInstance();

    // 显示或更新：title 是标题，lines 自上而下。空 lines 等同 remove。
    bool set(std::string const& playerName, std::string const& title, std::vector<std::string> const& lines);
    bool setTitle(std::string const& playerName, std::string const& title);
    bool setLine(std::string const& playerName, int index, std::string const& text);
    bool remove(std::string const& playerName);
    void removeAll();

    [[nodiscard]] bool isShown(std::string const& playerName) const;
    [[nodiscard]] int  lineCount(std::string const& playerName) const;

    void onPlayerLeave(std::string const& playerName);
    void shutdown();

private:
    SidebarManager() = default;

    struct Bar {
        std::string              title;
        std::vector<std::string> lines;
    };

    // objective 名：按玩家名派生（同一玩家恒定）
    static std::string objectiveOf(std::string const& playerName);
    // scoreboard id 基址：不同玩家错开，避免与 BDS 内部小 id 相撞
    static std::int64_t idBaseOf(std::string const& playerName);

    static bool sendDisplay(::Player& player, std::string const& objective, std::string const& title);
    static bool sendScores(::Player& player, std::string const& objective, std::vector<std::string> const& lines,
                           std::int64_t idBase);
    static bool sendRemoveScores(::Player& player, std::string const& objective, int count, std::int64_t idBase);
    static bool sendRemoveObjective(::Player& player, std::string const& objective);

    mutable std::mutex                   mMutex;
    std::unordered_map<std::string, Bar> mBars;
};

} // namespace debugshape_export
