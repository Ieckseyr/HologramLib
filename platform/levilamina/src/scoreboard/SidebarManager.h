#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace debugshape_export {

// 侧边栏计分板
class SidebarManager {
public:
    static SidebarManager& getInstance();

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

    static std::string objectiveOf(std::string const& playerName);

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
