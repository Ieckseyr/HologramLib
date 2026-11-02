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

## 对虚假实体用（自定义实体 / 假玩家 / 物品展示 / 掉落物）

可以。这一组包**只按 runtimeId 寻址**，库不会去服务端找实体——只要客户端那边有这个 runtimeId
的实体，动画就播得出来。库自己造的协议层实体正好满足这一点：

| 实体 | 取 runtimeId |
|---|---|
| 自定义实体 | `actorRuntimeIdOfEntity(libId)` |
| 假玩家 NPC | `playerNpcRuntimeId(id)` |
| 物品展示 | `itemDisplayRuntimeId(id)` |
| 掉落物 | `itemActorRuntimeId(id)` |

找不到时都返回 `-1`。

```js
const rid = PlayerNpc.runtimeId(npcId);
ActorEffect.playHurt(rid);        // 只影响"已经看到这只 NPC"的客户端
ActorEffect.playDeath(rid);
```

**逐玩家生效**：`actorPlayEventFor(playerName, runtimeId, eventId, data)` 只发给指定玩家，
其他玩家的客户端收不到这条指令，也就看不到动画。

```js
ActorEffect.playEventFor("Steve", rid, 3, 0);   // 只有 Steve 看到这只 NPC 死亡
```

注意两点：

- 非要发给"看不到这个实体"的玩家也没意义——客户端收到未知 runtimeId 的事件会直接忽略，不会报错。
- `playDeath` 之后实体在客户端进入"死亡"状态。**虚假实体没有 `RespawnPacket` 可发**
  （那个包只作用于玩家自己的死亡界面），要让它"复活"就是**重发它的出生包**：
  假玩家用 `setEnabled(id, false)` → `setEnabled(id, true)`（内部会换新 runtimeId 重新 spawn），
  自定义实体重新 `spawn`，物品展示重新 `setItem`/`draw` 即可。

## 玩家死亡之后：要补 respawn 包

`actorPlayDeath(playerRuntimeId)` 对**真人玩家**使用时，客户端会进入死亡界面并向服务端请求复活，
但服务端认为该玩家一直活着，不会回 `RespawnPacket` —— 客户端就卡在死亡界面。
播放完动画后请用 [死亡后的复活](respawn.md) 补一个 `playerRespawn(playerName)`。
