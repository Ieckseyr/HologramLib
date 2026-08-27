// GhostInteractRouter.h - ghost 交互事件路由（1.12.0）
//
// 客户端会对"协议上存在"的实体（CustomEntity / ItemDisplay 的协议层 ghost）
// 发出 InteractPacket; 本模块 hook ServerNetworkHandler::$handle(InteractPacket),
// 将目标 runtimeId 反查回库内 id 后派发:
//   - C++ 监听器（setGhostInteractListener, 每次点击回调）
//   - LSE 轮询队列（pollGhostInteractions / ghostPollInteractions, 取走并清空）
//
// BDS 对未知 runtimeId 的 InteractPacket 无动作, hook 恒调 origin, 零协议侵入。
// 事件仅在目标命中库内 Actor ID 段（0x6D 物品展示 / 0x6E 自定义实体）时产生,
// 真实实体交互不受影响。
#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "hologramlib/HologramLib.h" // hologramlib::GhostInteractEvent

namespace debugshape_export {

class GhostInteractRouter {
public:
    static GhostInteractRouter& getInstance();

    // 监听器（C++ 推送; 传空清除; 事件在主线程网络处理路径上回调）
    void setListener(std::function<void(hologramlib::GhostInteractEvent const&)> listener);
    void clearListener();

    // 轮询队列（LSE; 取走并清空）
    std::vector<hologramlib::GhostInteractEvent> poll();
    void                                          clearQueue();

    // 内部: hook 收包路径调用（已解析好的事件）
    void dispatch(hologramlib::GhostInteractEvent const& ev);

    // 事件格式化为可解析字符串（LSE 轮询条目格式）:
    // "player=X action=A domain=D id=I pos=(x,y,z)"（pos 仅存在时携带）
    [[nodiscard]] static std::string formatEvent(hologramlib::GhostInteractEvent const& ev);

    // LSE 导出（ModEntry::exportLseFunctions 调用; 统一命名空间 HologramLib）
    static void exportLse();

private:
    GhostInteractRouter()  = default;
    ~GhostInteractRouter() = default;

    std::mutex mMutex;
    std::function<void(hologramlib::GhostInteractEvent const&)> mListener;
    std::deque<hologramlib::GhostInteractEvent>                 mQueue;
};

} // namespace debugshape_export
