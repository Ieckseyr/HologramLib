// PlayerRespawnManager.h - 死亡动画后的复活（只发包，1.34.0）
//
// 背景：给玩家放死亡动画（ActorEvent DEATH）后，客户端会进入死亡界面并向服务端发
// PlayerAction(Respawn) 请求复活；但服务端认为玩家一直活着，不会发 RespawnPacket，
// 客户端于是卡在死亡界面。这里由库直接补一个 RespawnPacket 给客户端：
//   服务端实体不动（血量/位置/维度都不变），只是把客户端那侧的"死亡状态"清掉。
//
// 线格式（26.60 / 协议 2225）:
//   Position(Vec3: 3×float) + State(uint8: 0=SearchingForSpawn 1=ReadyToSpawn
//   2=ClientReadyToSpawn) + PlayerRuntimeId(uvarint64)
#pragma once

#include <string>

namespace debugshape_export {

class PlayerRespawnManager {
public:
    static PlayerRespawnManager& getInstance();

    // 原地复活：用玩家当前位置发 RespawnPacket
    bool respawn(std::string const& playerName, int state = 2);

    // 指定坐标复活
    bool respawnAt(std::string const& playerName, float x, float y, float z, int state = 2);

private:
    PlayerRespawnManager() = default;
};

} // namespace debugshape_export
