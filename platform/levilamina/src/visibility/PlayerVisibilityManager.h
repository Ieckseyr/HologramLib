#pragma once

#include "hologramlib/HologramLib.h"

#include <mutex>
#include <string>
#include <unordered_map>

class Packet;
class Player;

namespace debugshape_export {

class PlayerVisibilityManager {
public:
    static PlayerVisibilityManager& getInstance();

    bool setMask(std::string const& playerName, hologramlib::PlayerPacketMask const& mask, bool alsoSelf);
    bool setHidden(std::string const& playerName, bool hidden);
    bool restore(std::string const& playerName);

    [[nodiscard]] hologramlib::PlayerPacketMask maskOf(std::string const& playerName) const;
    [[nodiscard]] bool                          isHidden(std::string const& playerName) const;

    bool shouldDrop(::Player& recipient, ::Packet const& packet);

    void onPlayerLeave(std::string const& playerName);
    void shutdown();

private:
    PlayerVisibilityManager() = default;

    struct Entry {
        hologramlib::PlayerPacketMask mask;
        bool                          alsoSelf{false};
        bool                          hidden{false};
    };

    [[nodiscard]] bool anyActive() const;

    static void forceRespawn(std::string const& playerName);

    mutable std::mutex                     mMutex;
    std::unordered_map<std::string, Entry> mEntries;
};

} // namespace debugshape_export
