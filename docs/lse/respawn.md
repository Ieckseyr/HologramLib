# 死亡后的复活（playerRespawn*，1.34.0）

给玩家播死亡动画（[实体表现事件](actor-effect.md)）之后，客户端会进入死亡界面，并向服务端发
`PlayerAction(Respawn)` 请求复活。

问题在于：HologramLib 的死亡动画只是**发给客户端的一个包**（`ActorEvent::DEATH`），
服务端那边这个玩家从头到尾都活着。于是服务端收到复活请求后认为玩家不需要复活，
**不会发出 `RespawnPacket`** —— 客户端就一直卡在死亡界面里。

这一组接口补的就是那一个包：直接给客户端发 `RespawnPacket`，把它从死亡界面拉回来。
**服务端实体完全不动**：血量、位置、维度、背包都不变，也不会触发任何死亡 / 重生事件。

```js
const { ActorEffect, PlayerRespawn } = require("mods/HologramLib/lib/HologramLib.js");

const rid = ActorEffect.runtimeIdOfPlayer("Steve");
ActorEffect.playDeath(rid);          // 客户端开始播死亡动画

setTimeout(() => {
    PlayerRespawn.respawn("Steve");  // 约 1 秒后把客户端拉回正常状态
}, 1000);
```

| 函数 | 返回 | 说明 |
|---|---|---|
| `playerRespawn(playerName)` | `Boolean` | 原地复活（用玩家当前坐标），内部 `state = 2` |
| `playerRespawnState(playerName, state)` | `Boolean` | 指定状态：`0` 搜索出生点 / `1` ReadyToSpawn / `2` ClientReadyToSpawn |
| `playerRespawnAt(playerName, x, y, z)` | `Boolean` | 指定坐标复活（只改包里的坐标，不动服务端位置） |

## 什么时候用哪个

- **只想"死亡动画演完再站起来"**：`playerRespawn(name)` 就够（`state = 2` = 服务端已同意复活）。
- 个别客户端版本仍停在死亡界面：先 `playerRespawnState(name, 1)`，随后再 `playerRespawnState(name, 2)`。
  原版流程就是 `ReadyToSpawn → 客户端确认 → ClientReadyToSpawn`，有的版本要看到第一包才继续。
- `playerRespawnAt` 只是换 `RespawnPacket` 里的坐标；要同时把服务端的人挪过去，需要自己再调传送。

## 注意事项

- 只对**在线玩家**有效：名字找不到或玩家不在线 → `false`。
- 这是客户端单向的"复活"：服务端没有死亡状态需要清，所以**重复调用是安全的**（多调只是重复纠正客户端状态）。
- 不要拿它替代原版重生：血量真的归零的玩家仍然要由服务端走正常的 `Player::respawn()` 流程。
- 死亡动画之后如果不补这个包，客户端会一直停在死亡界面，直到玩家自己断开重连。

## 逐玩家与虚假实体

- `playerRespawn(playerName)` 本来就是**只对那名玩家**发 `RespawnPacket`，其他玩家完全不受影响。
- `RespawnPacket` 作用对象是**玩家自己的死亡界面**，对虚假实体（假玩家 NPC / 自定义实体 / 物品展示）
  **没有对应包**。要让虚假实体"复活"，重发它的出生包即可：
  假玩家 `setEnabled(id, false)` → `setEnabled(id, true)`；自定义实体重新 `spawn`；物品展示重新 `draw`。
- 想让"某玩家看到假玩家死亡"：`actorPlayEventFor(playerName, playerNpcRuntimeId(id), 3, 0)`；
  想让他看到"复活"：对该玩家重发这只假玩家的出生包（`setEnabled` 会按可见玩家重发）。

## 协议

`RespawnPacket`（id 45）：

| 字段 | 类型 | 说明 |
|---|---|---|
| `Position` | `Vec3`（3×float） | 复活点坐标 |
| `State` | `uint8` | `0` SearchingForSpawn · `1` ReadyToSpawn · `2` ClientReadyToSpawn |
| `PlayerRuntimeId` | `uvarint64` | 玩家实体的 runtimeId |

包体走库的统一发包出口（26.60 编码 + BDS 回读校验），所以不会因为协议版本差异发错字节。
