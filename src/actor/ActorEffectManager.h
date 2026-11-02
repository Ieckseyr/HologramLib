// ActorEffectManager.h - 实体表现事件（受伤 / 死亡动画，1.33.0）
//
// 协议: ActorEventPacket(27)。字段: RuntimeId / EventId(ActorEvent) / Data / FireAtPosition。
// 事件值见协议枚举: HURT=2 / DEATH=3 / STARVE=… / SHAKE=39 / INSTANT_DEATH=61 等。
//
// 目标是我们用协议层显示的实体（自定义实体 / 假玩家 / 掉落物）的 runtimeId。
// 服务端没有对应实体也没关系 —— 这只是发给客户端的一条表现指令。
#pragma once

#include <cstdint>
#include <string>

namespace debugshape_export {

class ActorEffectManager {
public:
    static ActorEffectManager& getInstance();

    bool playHurt(int64_t entityRuntimeId);
    bool playDeath(int64_t entityRuntimeId);
    bool playInstantDeath(int64_t entityRuntimeId);
    // 通用：eventId 见协议 ActorEvent
    bool play(int64_t entityRuntimeId, int eventId, int data);

    // 只发给某一名玩家（不传 = 发给该维度所有玩家）
    bool playFor(std::string const& playerName, int64_t entityRuntimeId, int eventId, int data);

private:
    ActorEffectManager() = default;

    // playerName 为空 = 发给所有在线玩家；否则只发给这一名
    bool dispatch(std::string const& playerName, int64_t entityRuntimeId, int eventId, int data);
};

} // namespace debugshape_export
