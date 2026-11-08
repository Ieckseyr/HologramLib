#include "PlayerSensingExporter.h"
#include "PlayerSensingManager.h"

#include "lse/LseBridge.h"

#include <format>
#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void PlayerSensingExporter::exportAll() {
    // 输入设备：无事件，直接查
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

    // 攻击 / 挥动监听：数据源 AnimatePacket(Swing)
    hologramlib::lse::exportAs(NAMESPACE, "swingCount", [](std::string const& playerName) -> int64_t {
        return static_cast<int64_t>(swingCount(playerName));
    });

    hologramlib::lse::exportAs(NAMESPACE, "swingTotal", []() -> int64_t {
        return static_cast<int64_t>(swingTotal());
    });

    hologramlib::lse::exportAs(NAMESPACE, "swingDropped", []() -> int64_t {
        return static_cast<int64_t>(swingDropped());
    });

    hologramlib::lse::exportAs(NAMESPACE, "swingReset", []() -> void {
        swingReset();
    });

    // 轮询：取走即清空；每行一个事件（格式见 docs/lse/sensing.md）
    hologramlib::lse::exportAs(NAMESPACE, "swingPollEvents", []() -> std::vector<std::string> {
        return swingFormatAll(swingPoll());
    });

    hologramlib::lse::exportAs(NAMESPACE, "swingClearEvents", []() -> void {
        swingClearEvents();
    });

    // 最近一次：不清队列，适合"每 tick 只关心最新一次"的脚本
    hologramlib::lse::exportAs(NAMESPACE, "swingLastEvent", [](std::string const& playerName) -> std::string {
        SwingEvent ev;
        return swingLastOf(playerName, ev) ? swingFormat(ev) : std::string{};
    });

    hologramlib::lse::exportAs(NAMESPACE, "swingLastTarget", [](std::string const& playerName) -> std::string {
        SwingEvent ev;
        if (!swingLastOf(playerName, ev))
            return "none";
        switch (ev.target) {
        case SwingTargetKind::Air:
            return "air";
        case SwingTargetKind::Block:
            return "block";
        case SwingTargetKind::Entity:
            return "entity";
        default:
            return "unknown";
        }
    });

    hologramlib::lse::exportAs(NAMESPACE, "swingLastBlock", [](std::string const& playerName) -> std::string {
        SwingEvent ev;
        if (!swingLastOf(playerName, ev) || ev.target != SwingTargetKind::Block)
            return std::string{};
        return std::format("{},{},{}", ev.blockX, ev.blockY, ev.blockZ);
    });
}

} // namespace debugshape_export
