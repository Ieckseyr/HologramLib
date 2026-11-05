# 实体表现事件（actor*）

给协议层显示的实体播原版动画事件：受伤、死亡，以及任意 `ActorEvent`。

目标是实体的 **runtimeId**（`entityGetInfo`、`playerNpc*` 一类接口返回的那个），不是插件层 id。

```js
const { ActorEffect } = require("mods/HologramLib/lib/HologramLib.js");

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

## 阻断表现事件（受伤 / 死亡不播）

在出站口拦 `ActorEventPacket`：被拦的包不会到达客户端，动画就不播。
**只拦表现包**，服务端状态完全不变（血量、掉落、死亡事件照常）。

| 函数 | 说明 |
|---|---|
| `actorBlockEvent(eventId)` | 全局拦截；`2` 受伤 / `3` 死亡 / `61` 立即死亡 / `-1` 全部 |
| `actorUnblockEvent(eventId)` | 解除全局拦截（`-1` = 全部解除） |
| `actorBlockEventFor(playerName, eventId)` | 只让该玩家看不到 |
| `actorUnblockEventFor(playerName, eventId)` | 解除该玩家的拦截 |
| `actorBlockEventForEntity(runtimeId, eventId)` | 只拦这个实体 |
| `actorUnblockEventForEntity(runtimeId, eventId)` | 解除该实体的拦截 |
| `actorBlockClear()` | 清空全部规则 |
| `actorBlockStats()` | `"blocked=N passed=N rules=gX/pY/eZ"` |

```js
ActorEffect.blockEvent(2);                 // 全服都看不到受伤闪红
ActorEffect.blockEventFor("Steve", 3);     // 只有 Steve 看不到死亡动画
ActorEffect.blockEventForEntity(rid, 2);   // 只有这只实体不闪红
ActorEffect.blockStats();                  // 看命中数
```

三档可叠加，命中任一即拦。用来做"无伤感的 BOSS 演出"、"死亡不播但要正常掉落"这类效果。

> 注意：`actorBlockEvent(-1)` 会把**所有** ActorEvent 都拦掉（吃东西、爱心、抖动、表情全没），
> 演出结束后记得 `actorBlockClear()`。

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
