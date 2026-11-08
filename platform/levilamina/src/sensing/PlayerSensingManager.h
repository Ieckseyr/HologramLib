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

// 空手挥动（攻击空气）统计：服务端 AnimatePacket(Swing) 计数
std::uint64_t swingCount(std::string const& playerName);
std::uint64_t swingTotal();
void          swingReset();
void          forgetSwings(std::string const& playerName);

} // namespace debugshape_export
