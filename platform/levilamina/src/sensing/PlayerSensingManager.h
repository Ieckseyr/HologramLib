#pragma once

#include "hologramlib/HologramLib.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace debugshape_export {

// 设备类型
class PlayerSensingManager {
public:
    static PlayerSensingManager& getInstance();

    void record(std::string const& playerName, int inputMode);

    [[nodiscard]] hologramlib::ClientInputDevice deviceOf(std::string const& playerName) const;
    [[nodiscard]] bool                           isTouch(std::string const& playerName) const;

    void forget(std::string const& playerName);

    void shutdown();

private:
    PlayerSensingManager() = default;

    mutable std::mutex                                              mMutex;
    std::unordered_map<std::string, hologramlib::ClientInputDevice> mDevices;
};

hologramlib::IPlayerSensing& playerSensingAdapter();

} // namespace debugshape_export
