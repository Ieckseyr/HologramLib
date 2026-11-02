# 实体表现事件（actor*）

给协议层显示的实体播原版动画事件：受伤、死亡，以及任意 `ActorEvent`。

目标是实体的 **runtimeId**（`entityGetInfo`、`playerNpc*` 一类接口返回的那个），不是插件层 id。

```js
const { ActorEffect } = require("plugins/HologramLib/lib/HologramLib.js");

ActorEffect.playHurt(runtimeId);     // 受伤闪红
ActorEffect.playDeath(runtimeId);    // 死亡动画
ActorEffect.playInstantDeath(runtimeId); // 立即消失（不掉落、不播完整死亡动画）
```

| 函数 | 返回 | 说明 |
|---|---|---|
| `actorPlayHurt(runtimeId)` | `Boolean` | 受伤动画（`ActorEvent::HURT = 2`） |
| `actorPlayDeath(runtimeId)` | `Boolean` | 死亡动画（`DEATH = 3`） |
| `actorPlayInstantDeath(runtimeId)` | `Boolean` | 立即死亡（`INSTANT_DEATH = 61`） |
| `actorPlayEvent(runtimeId, eventId, data)` | `Boolean` | 任意事件，事件值见协议 `ActorEvent` 枚举 |
| `actorPlayEventFor(playerName, runtimeId, eventId, data)` | `Boolean` | 只发给指定玩家 |
| `actorRuntimeIdOfPlayer(playerName)` | `Integer` | 取玩家实体的 runtimeId（不在线 `-1`） |
| `actorRuntimeIdOfEntity(libId)` | `Integer` | 取协议层实体的 runtimeId（查不到 `-1`） |

常用事件值：`2` 受伤 · `3` 死亡 · `10` 吃草 · `21` 爱心 · `22` 村民生气 · `39` 抖动 · `61` 立即死亡。

播完死亡动画后实体仍在客户端，需要消失再调对应的 destroy 接口。
