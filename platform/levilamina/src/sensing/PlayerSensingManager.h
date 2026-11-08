#pragma once

#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

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

// 攻击 / 挥动监听
// 数据源：客户端 `AnimatePacket(action = Swing)`（空手挥、挥剑、打空气、点方块、打实体都会发）。
// 目标判定：先用包里的 Swing Source（ActorSwingSource），再用视线射线补方块位置。
// 协议本身不带"我对着什么挥"，所以这里明确分为 空气 / 方块 / 实体 三档。
enum class SwingTargetKind : std::uint8_t {
    Unknown = 0,
    Air     = 1,
    Block   = 2,
    Entity  = 3,
};

struct SwingEvent {
    std::string     playerName;
    std::uint8_t    source{0}; // ::ActorSwingSource 数值：0=None 1=Build 2=Mine 3=Interact 4=Attack 5=UseItem 6=ThrowItem 7=DropItem 8=Event
    bool            hasSource{false}; // 客户端没带 Swing Source 字段时为 false（source 无意义）
    SwingTargetKind target{SwingTargetKind::Unknown};
    int             blockX{0}; // target == Block 时有效
    int             blockY{0};
    int             blockZ{0};
    int             face{-1}; // 命中面 0..5；-1 = 无
    std::uint64_t   runtimeId{0};      // 包里 Target Actor Runtime ID（通常 0）
    std::uint64_t   entityUniqueId{0}; // target == Entity 且能按 runtimeId 找到实体时有效
    std::string     itemName;          // 手持物品 typeName；空手为空串
    int             itemCount{0};
    float           eyeX{0};
    float           eyeY{0};
    float           eyeZ{0};
    std::uint64_t   tick{0};
};

// 空手挥动（攻击空气）统计：服务端 AnimatePacket(Swing) 计数
std::uint64_t swingCount(std::string const& playerName);
std::uint64_t swingTotal();
std::uint64_t swingDropped(); // 队列满被丢弃的事件数（诊断用）
void          swingReset();
void          forgetSwings(std::string const& playerName);

// 事件队列（LSE 轮询模型：取走即清空）
std::vector<SwingEvent> swingPoll();
void                    swingClearEvents();
[[nodiscard]] bool      swingLastOf(std::string const& playerName, SwingEvent& out);

// 线上文本行，LSE 侧 swingPollEvents / swingLastEvent 返回这个格式：
//   player=Steve source=mine target=block pos=12,64,-3 face=1 item=minecraft:iron_sword count=1 runtimeId=0 uniqueId=0 tick=123
std::string              swingFormat(SwingEvent const& ev);
std::vector<std::string> swingFormatAll(std::vector<SwingEvent> const& events);

} // namespace debugshape_export
