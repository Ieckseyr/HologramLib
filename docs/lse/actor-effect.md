# 实体表现事件（actor*）

给协议层显示的实体播原版动画事件：受伤、死亡，以及任意 `ActorEvent`。

目标是实体的 **runtimeId**（`entityGetInfo`、`playerNpc*` 一类接口返回的那个），不是插件层 id。

## 快速开始

```js
const { ActorEffect } = require("mods/HologramLib/lib/HologramLib.js");

ActorEffect.playHurt(runtimeId);         // 受伤闪红
ActorEffect.playDeath(runtimeId);        // 死亡动画
ActorEffect.playInstantDeath(runtimeId); // 立即消失
```

## 播放事件

### 受伤动画

`actorPlayHurt(runtimeId)`

- 参数：
  - `runtimeId` : `Integer`<br>目标实体的 runtimeId
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorPlayHurt")(rid);
    ```

对应 `ActorEvent::HURT = 2`。

### 死亡动画

`actorPlayDeath(runtimeId)`

- 参数：
  - `runtimeId` : `Integer`<br>目标实体 runtimeId
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorPlayDeath")(rid);
    ```

`DEATH = 3`。播完实体仍在客户端，要消失再调对应 destroy。

### 立即死亡

`actorPlayInstantDeath(runtimeId)`

- 参数：
  - `runtimeId` : `Integer`<br>目标实体 runtimeId
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorPlayInstantDeath")(rid);
    ```

`INSTANT_DEATH = 61`，不播完整死亡动画。

### 任意事件

`actorPlayEvent(runtimeId, eventId, data)`

- 参数：
  - `runtimeId` : `Integer`<br>目标实体 runtimeId
  - `eventId` : `Integer`<br>协议 `ActorEvent` 值：`2` 受伤 · `3` 死亡 · `10` 吃草 · `21` 爱心 · `22` 村民生气 · `39` 抖动 · `61` 立即死亡
  - `data` : `Integer`<br>事件附带数据，没有就传 `0`
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorPlayEvent")(rid, 21, 0);
    ```

### 只发给指定玩家

`actorPlayEventFor(playerName, runtimeId, eventId, data)`

- 参数：
  - `playerName` : `String`<br>只把事件发给这名玩家
  - 其余参数同 `actorPlayEvent`
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorPlayEventFor")("Steve", rid, 3, 0);
    ```

### 取玩家的 runtimeId

`actorRuntimeIdOfPlayer(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：该玩家实体的 runtimeId；不在线返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const rid = ll.import("HologramLib", "actorRuntimeIdOfPlayer")("Steve");
    ```

### 取协议层实体的 runtimeId

`actorRuntimeIdOfEntity(libId)`

- 参数：
  - `libId` : `Integer`<br>库内实体 id（自定义实体等）
- 返回值：runtimeId；查不到返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const rid = ll.import("HologramLib", "actorRuntimeIdOfEntity")(entityId);
    ```

## 阻断表现事件（受伤 / 死亡不播）

在出站口拦 `ActorEventPacket`：被拦的包不会到达客户端，动画就不播。
**只拦表现包**，服务端状态完全不变（血量、掉落、死亡事件照常）。

### 全局拦截

`actorBlockEvent(eventId)`

- 参数：
  - `eventId` : `Integer`<br>`2` 受伤 / `3` 死亡 / `61` 立即死亡 / `-1` 全部
- 返回值：`true` 已添加规则，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorBlockEvent")(2);
    ```

### 解除全局拦截

`actorUnblockEvent(eventId)`

- 参数：
  - `eventId` : `Integer`<br>`-1` = 全部解除
- 返回值：`true` 已解除，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorUnblockEvent")(-1);
    ```

### 只让某玩家看不到

`actorBlockEventFor(playerName, eventId)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `eventId` : `Integer`<br>事件值
- 返回值：`true` 已添加规则，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorBlockEventFor")("Steve", 3);
    ```

### 解除某玩家的拦截

`actorUnblockEventFor(playerName, eventId)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `eventId` : `Integer`<br>事件值
- 返回值：`true` 已解除，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorUnblockEventFor")("Steve", 3);
    ```

### 只拦这只实体

`actorBlockEventForEntity(runtimeId, eventId)`

- 参数：
  - `runtimeId` : `Integer`<br>实体 runtimeId
  - `eventId` : `Integer`<br>事件值
- 返回值：`true` 已添加规则，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorBlockEventForEntity")(rid, 2);
    ```

### 解除该实体的拦截

`actorUnblockEventForEntity(runtimeId, eventId)`

- 参数：
  - `runtimeId` : `Integer`<br>实体 runtimeId
  - `eventId` : `Integer`<br>事件值
- 返回值：`true` 已解除，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorUnblockEventForEntity")(rid, 2);
    ```

### 清空全部规则

`actorBlockClear()`

- 参数：无
- 返回值：无
- 返回值类型：`Null`
- 示例：

    ```js
    ll.import("HologramLib", "actorBlockClear")();
    ```

### 查看命中统计

`actorBlockStats()`

- 参数：无
- 返回值：形如 `"blocked=N passed=N rules=gX/pY/eZ"` 的统计串
- 返回值类型：`String`
- 示例：

    ```js
    ll.import("HologramLib", "actorBlockStats")();
    ```

三档规则可叠加，命中任一即拦。`actorBlockEvent(-1)` 会拦掉**所有** ActorEvent（吃东西、爱心、抖动、表情全没），演出结束后记得 `actorBlockClear()`。

## 对虚假实体用

这一组包**只按 runtimeId 寻址**，库不会去服务端找实体 —— 只要客户端那边有这个 runtimeId 的实体，动画就播得出来。

| 实体 | 取 runtimeId |
|---|---|
| 自定义实体 | `actorRuntimeIdOfEntity(libId)` |
| 假玩家 NPC | `playerNpcRuntimeId(id)` |
| 物品展示 | `itemDisplayRuntimeId(id)` |
| 掉落物 | `itemActorRuntimeId(id)` |

```js
const rid = PlayerNpc.runtimeId(npcId);
ActorEffect.playHurt(rid);
ActorEffect.playDeath(rid);
```

`playDeath` 之后实体在客户端进入"死亡"状态。**虚假实体没有 `RespawnPacket` 可发**，要"复活"就重发出生包：假玩家 `setEnabled(id, false)` → `setEnabled(id, true)`；自定义实体重新 `spawn`；物品展示重新 `draw`。

## 图腾激活动画

`actorPlayTotem(runtimeId, holdTicks)`

- 参数：
  - `runtimeId` : `Integer`<br>必须是真人玩家的 runtimeId
  - `holdTicks` : `Integer`<br>副手保留多少 tick；`<= 0` 按 `20`
- 返回值：`true` 已播放，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorPlayTotem")(rid, 20);
    ```

先对所有客户端发 `MobEquipment`（副手 = 不死图腾），再发 `ActorEvent` 65（`TalismanActivate`），`holdTicks` 后还原原副手。只支持真人玩家。

## 村庄英雄 / 灾厄之兆（只有动画）

### 村庄英雄动画

`actorShowVillageHero(playerName, unused)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `unused` : `Integer`<br>旧的时长占位，现在不生效，传 `0`
- 返回值：`true` 已播放，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorShowVillageHero")("Steve", 0);
    ```

`AnimateEntity` 播 `animation.humanoid.celebrating`，**不挂状态效果**。

### 灾厄之兆动画

`actorShowBadOmen(playerName, unused)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `unused` : `Integer`<br>传 `0`
- 返回值：`true` 已播放，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorShowBadOmen")("Steve", 0);
    ```

主手临时换成 `minecraft:ominous_bottle` + `ActorEvent` 29，40 tick 后还原，**不挂状态效果**。

要真的挂效果，用 `playerShowEffect(name, effectId, ticks, amplifier)`（灾厄之兆 28、村庄英雄 29、试炼之兆 31、袭击之兆 36）。

## 姿势

`actorSetPose(runtimeId, pose)`

- 参数：
  - `runtimeId` : `Integer`<br>实体 runtimeId
  - `pose` : `Integer`<br>`0` 站立 · `1` 睡觉 · `2` 潜行 · `3` 游泳 · `4` 爬行（不同版本可能有出入）
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorSetPose")(rid, 4);
    ```

只发 `SetActorData` 的 `PoseIndex(79)`；本地玩家自己的姿势由客户端输入驱动，不一定拽得动。

## 冰冻

### 设置霜冻强度

`actorFreeze(runtimeId, strength)`

- 参数：
  - `runtimeId` : `Integer`<br>实体 runtimeId
  - `strength` : `Float`<br>`0 ~ 1`，越大霜冻越重
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorFreeze")(rid, 0.6);
    ```

### 解除冰冻

`actorUnfreeze(runtimeId)`

- 参数：
  - `runtimeId` : `Integer`<br>实体 runtimeId
- 返回值：`true` 已解除，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorUnfreeze")(rid);
    ```

等价 `actorFreeze(rid, 0)`。走元数据 **120 `FreezingEffectStrength`**，只改客户端画面，不扣血、不加效果。

## 播放客户端动画

### 播放动画

`actorPlayAnimation(runtimeId, animation, stopExpression, blendOut)`

- 参数：
  - `runtimeId` : `Integer`<br>实体 runtimeId
  - `animation` : `String`<br>动画名，如 `animation.creeper.swelling`
  - `stopExpression` : `String`<br>Molang 停止表达式
  - `blendOut` : `Float`<br>淡出时间（秒）
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorPlayAnimation")(
        rid, "animation.creeper.swelling",
        "v.swelling_sin=math.sin(q.life_time*720);", 0);
    ```

### 完整参数版

`actorPlayAnimationFull(runtimeId, animation, nextState, stopExpression, blendOut, controller, molangVersion)`

- 参数：
  - 前 5 个参数同 `actorPlayAnimation`
  - `nextState` : `String`<br>下一个状态，一般 `"none"`
  - `controller` : `String`<br>动画控制器名，空串 = 不用
  - `molangVersion` : `Integer`<br>Molang 版本，如 `13`
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "actorPlayAnimationFull")(
        rid, "animation.creeper.swelling", "none", "<表达式>", 0, "", 13);
    ```

包体是 `AnimateEntity`(158)。**只有客户端在该实体的动画定义里找得到这个动画才会播**；物品展示 / 展示方块没有实体动画。

## 玩家死亡之后

`actorPlayDeath(playerRuntimeId)` 对**真人玩家**使用时，客户端会进入死亡界面并向服务端请求复活，但服务端认为该玩家一直活着，不会回 `RespawnPacket` —— 客户端会卡在死亡界面。播放完动画后请用[死亡后的复活](respawn.md)补一个 `playerRespawn(playerName)`。
