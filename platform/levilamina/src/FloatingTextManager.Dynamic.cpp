#include "FloatingTextManager.h"
#include "FloatingTextInternal.h"
#include "PacketDebugRenderer.h"
#include "kernel/EventIds.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/Listener.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/event/world/ServerLevelTickEvent.h>
#include <ll/api/service/Bedrock.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace debugshape_export {

void FloatingTextManager::registerBuiltinVariables() {

    registerVariable("time", [](const std::string&) -> std::string {
        auto    now  = std::chrono::system_clock::now();
        auto    time = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        return oss.str();
    });

    registerVariable("online", [](const std::string&) -> std::string {
        auto level = ll::service::getLevel();
        if (level.has_value()) {
            return std::to_string(level->getActivePlayerCount());
        }
        return "0";
    });

    registerVariable("player", [](const std::string& playerName) -> std::string {
        return playerName.empty() ? "Unknown" : playerName;
    });

    registerVariable("tps", [this](const std::string&) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << mTps.load(std::memory_order_relaxed);
        return oss.str();
    });

    registerVariable("dimension", [](const std::string& playerName) -> std::string {
        if (playerName.empty())
            return "?";
        auto level = ll::service::getLevel();
        if (!level.has_value())
            return "?";
        auto* p = level->getPlayer(playerName);
        return p ? std::to_string(static_cast<int>(p->getDimensionId())) : "?";
    });
    for (auto const& axis : {"x", "y", "z"}) {
        std::string const name(axis);
        registerVariable(name, [name](const std::string& playerName) -> std::string {
            if (playerName.empty())
                return "?";
            auto level = ll::service::getLevel();
            if (!level.has_value())
                return "?";
            auto* p = level->getPlayer(playerName);
            if (!p)
                return "?";
            auto const   pos = p->getPosition();
            double const v   = name == "x" ? pos.x : name == "y" ? pos.y
                                                                 : pos.z;
            return std::to_string(static_cast<int>(std::floor(v)));
        });
    }
}

} // namespace debugshape_export
