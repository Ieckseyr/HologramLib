// AnchorPool.h - 锚点实体池（1.33.0）
//
// 有些客户端界面要求"对象是一个真实存在的实体"：
//   · Boss 条（BossEventPacket 的 targetActorId）
//   · 方位条航点（ServerWaypoint::Payload 的 ActorUniqueID）
// 但我们不想为这种事在玩家眼前摆真东西。
//
// 做法和 NPC 对话菜单的载体一样：把一只"丢弃物形态的协议实体"放到虚空里
// （默认 y = -120，落在世界底部之下，任何玩家都看不到），只把它当作锚点 id 用。
// 这就是 `minecraft:structure_void` 掉落物 —— 一块没有模型的物品。
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace debugshape_export {

class AnchorPool {
public:
    static AnchorPool& getInstance();

    // 在指定维度开一个锚点（丢在虚空里），返回它的 ActorUniqueID；<0 = 失败
    std::int64_t acquire(int dimension);
    // 释放：销毁锚点实体。传 0 或未知 id 直接返回 false
    bool release(std::int64_t uniqueId);
    void releaseAll();

    [[nodiscard]] int  count() const;
    [[nodiscard]] bool exists(std::int64_t uniqueId) const;

    void shutdown();

private:
    AnchorPool() = default;

    mutable std::mutex                             mMutex;
    std::unordered_map<std::int64_t, std::int64_t> mAnchors; // uniqueId → 掉落物库内 id
};

} // namespace debugshape_export
