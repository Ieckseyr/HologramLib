// PlayerSensingManager.h - 感知域: 客户端设备判断(1.21.0 未发布线)
//
// 数据源: PlayerAuthInputPacket(AuthInput) 每个玩家每 tick 持续上报, 其中 InputMode
// 字段就是客户端当前输入设备(0=Undefined 1=Mouse 2=Touch 3=GamePad 4=MotionController)。
// 库在协议层挂钩逐包捕获, 消费方经 IPlayerSensing 随时查询。
//
// 典型用途: 按设备路由 UI —— 触屏玩家给容器列表(实证触屏走不通纯协议层交易),
// 键鼠/手柄给交易界面。
#pragma once

#include "hologramlib/HologramLib.h" // ClientInputDevice / IPlayerSensing

#include <mutex>
#include <string>
#include <unordered_map>

namespace debugshape_export {

class PlayerSensingManager {
public:
    static PlayerSensingManager& getInstance();

    // AuthInput 钩子调用: 记录该玩家最新的输入设备
    void record(std::string const& playerName, int inputMode);

    // 查询(不在线/未上报 = Unknown)
    [[nodiscard]] hologramlib::ClientInputDevice deviceOf(std::string const& playerName) const;
    [[nodiscard]] bool isTouch(std::string const& playerName) const;

    // 玩家离线时清记录(由库的玩家离开路径调用; 不调也不致错, 查询对离线玩家返回 Unknown)
    void forget(std::string const& playerName);

    // 库卸载
    void shutdown();

private:
    PlayerSensingManager() = default;

    mutable std::mutex                                              mMutex;
    std::unordered_map<std::string, hologramlib::ClientInputDevice> mDevices;
};

// 公开接口适配(IHologramLib::playerSensing())
hologramlib::IPlayerSensing& playerSensingAdapter();

} // namespace debugshape_export
