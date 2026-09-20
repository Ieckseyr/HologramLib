// PlayerSensingExporter.cpp - 感知域（客户端设备判断）LSE 导出实现
//
// 数据源是客户端每 tick 上报的 PlayerAuthInput（InputMode 字段）, 库逐包捕获后在内存里记一份,
// 脚本随时查询即可 —— 没有事件、不用轮询队列。
//
// 脚本侧用法:
//   if (sensingIsTouch("Steve")) containerOpen("Steve", "§8任务列表", 3);
//   else                        packetTradeOpen(...);
#include "PlayerSensingExporter.h"
#include "PlayerSensingManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void PlayerSensingExporter::exportAll() {
    auto& mgr = PlayerSensingManager::getInstance();

    // sensingDeviceOf(playerName) -> s
    //   "keyboardMouse" / "touch" / "gamepad" / "motionController" / "unknown"
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

    // sensingIsTouch(playerName) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "sensingIsTouch", [&mgr](std::string const& playerName) -> bool {
            return mgr.isTouch(playerName);
        });

    // sensingDeviceCode(playerName) -> i（0=Unknown 1=键鼠 2=触屏 3=手柄 4=体感, 与 C++ 枚举同值）
    hologramlib::lse::exportAs(
        NAMESPACE, "sensingDeviceCode", [&mgr](std::string const& playerName) -> int {
            return static_cast<int>(mgr.deviceOf(playerName));
        });
}

} // namespace debugshape_export
