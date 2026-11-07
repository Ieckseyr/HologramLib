#include "PlayerSensingExporter.h"
#include "PlayerSensingManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void PlayerSensingExporter::exportAll() {
    hologramlib::lse::exportAs(NAMESPACE, "swingCount", [](std::string const& playerName) -> int64_t {
        return static_cast<int64_t>(swingCount(playerName));
    });

    hologramlib::lse::exportAs(NAMESPACE, "swingTotal", []() -> int64_t {
        return static_cast<int64_t>(swingTotal());
    });

    hologramlib::lse::exportAs(NAMESPACE, "swingReset", []() -> void {
        swingReset();
    });

    auto& mgr = PlayerSensingManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "sensingDeviceOf", [&mgr](std::string const& playerName) -> std::string {
        using hologramlib::ClientInputDevice;
        switch (mgr.deviceOf(playerName)) {
        case ClientInputDevice::KeyboardMouse:
            return "keyboardMouse";
        case ClientInputDevice::Touch:
            return "touch";
        case ClientInputDevice::Gamepad:
            return "gamepad";
        case ClientInputDevice::MotionController:
            return "motionController";
        default:
            return "unknown";
        }
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "sensingIsTouch", [&mgr](std::string const& playerName) -> bool {
            return mgr.isTouch(playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "sensingDeviceCode", [&mgr](std::string const& playerName) -> int {
            return static_cast<int>(mgr.deviceOf(playerName));
        });
}

} // namespace debugshape_export
