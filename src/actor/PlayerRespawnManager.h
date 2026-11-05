#pragma once

#include <string>

namespace debugshape_export {

// 复活包
class PlayerRespawnManager {
public:
    static PlayerRespawnManager& getInstance();

    bool respawn(std::string const& playerName, int state = 2);

    bool respawnAt(std::string const& playerName, float x, float y, float z, int state = 2);

private:
    PlayerRespawnManager() = default;
};

} // namespace debugshape_export
