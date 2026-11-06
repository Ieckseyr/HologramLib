#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <mc/world/item/ItemStack.h>

namespace debugshape_export {

// 实体表现事件发包
class ActorEffectManager {
public:
    static ActorEffectManager& getInstance();

    bool playHurt(int64_t entityRuntimeId);
    bool playDeath(int64_t entityRuntimeId);
    bool playInstantDeath(int64_t entityRuntimeId);

    bool play(int64_t entityRuntimeId, int eventId, int data);

    bool playFor(std::string const& playerName, int64_t entityRuntimeId, int eventId, int data);

    // 冰冻强度（元数据 120 FreezingEffectStrength，0..1；0 = 解除）
    bool setFreeze(int64_t entityRuntimeId, float strength);

    // 彩蛋：苦力怕膨胀（客户端动画，服务端只发一次）
    bool playCreeperSwelling(int64_t entityRuntimeId);

    // 图腾激活：临时把副手换成不死图腾 + ActorEvent 65，holdTicks 后还原（只改客户端观感）
    bool playTotem(int64_t entityRuntimeId, int holdTicks);

    // 客户端状态效果：给玩家挂 MobEffect（只影响观感，不改服务端状态）
    bool showEffect(std::string const& playerName, int effectId, int durationTicks, int amplifier);

    // 村庄英雄：MobEffect 29 + 举手庆祝动画（animation.humanoid.celebrating）
    bool playCelebrate(std::string const& playerName, float seconds);

    // 灾厄之兆：MobEffect 28 + 主手换不祥之瓶喝一口（ActorEvent 29），holdTicks 后还原
    bool playOminousDrink(std::string const& playerName, int holdTicks);

    // 姿势（元数据 PoseIndex 79）：0=站立 1=睡觉 2=潜行 3=游泳 4=爬行 等，具体看客户端
    bool setPose(int64_t entityRuntimeId, int poseIndex);

    // 每 tick 推进：到时还原
    void tickFx();

    // 播放客户端动画（等价 /playanimation）
    bool playAnimation(
        int64_t            entityRuntimeId,
        std::string const& animation,
        std::string const& nextState,
        std::string const& stopExpression,
        float              blendOutTime,
        std::string const& controller,
        int                molangVersion
    );

private:
    ActorEffectManager() = default;

    struct SpinState {
        std::uint64_t runtimeId{};
        std::uint64_t endTick{};
        float         yaw{};
    };

    struct ItemRestore {
        std::uint64_t runtimeId{};
        std::uint64_t endTick{};
        ::ItemStack   original{};
        int           slot{};
        int           containerId{};
        bool          engine{};
    };

    std::mutex               mFxMutex;
    std::vector<ItemRestore> mFxPending;
    std::vector<SpinState>   mSpins;

    bool dispatch(std::string const& playerName, int64_t entityRuntimeId, int eventId, int data);
};

} // namespace debugshape_export
