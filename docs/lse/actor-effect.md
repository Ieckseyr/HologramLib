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

## 图腾激活动画（`actorPlayTotem`）

```js
const rid = ActorEffect.runtimeIdOfPlayer("Steve");
ActorEffect.playTotem(rid, 20);   // 20 tick 后把副手还原
```

- 只发协议，不改服务端背包：先对**所有客户端**发一条 `MobEquipment`（副手 = 不死图腾），再发 `ActorEvent` 65（`TalismanActivate`），`holdTicks` 后再用原副手发一条 `MobEquipment` 还原。
- 原副手是从玩家身上现读的（`getAllHandEquipment`），所以不会把别人的副手清掉。
- 只支持真人玩家（要读原副手、要引擎实体）；`holdTicks <= 0` 按 20 tick 处理。

## 村庄英雄 / 灾厄之兆（只有动画）

```js
ActorEffect.showVillageHero("Steve", 0);   // 只播举手庆祝动画
ActorEffect.showBadOmen("Steve", 0);       // 只播喝不祥之瓶动画
```

- `showVillageHero`：`AnimateEntity` 播 `animation.humanoid.celebrating`（原版 humanoid 的举手庆祝，`q.anim_time` 到 3 秒自动停），**不挂状态效果**。
- `showBadOmen`：主手临时换成 `minecraft:ominous_bottle`（服务端背包会同步，40 tick 后还原）+ `ActorEvent` 29（`DrinkPotion` 喝瓶子动画），**不挂状态效果**。
- 第二个参数是旧的时长占位，现在不生效，传 0 即可。
- 就是要挂状态效果的话，用 `playerShowEffect(name, effectId, ticks, amplifier)`（灾厄之兆 28、村庄英雄 29、试炼之兆 31、袭击之兆 36）。

### 只对指定生物类型生效

```js
ActorEffect.aimAssistType("Steve", "minecraft:creeper", 90, 90, 32);
```

- 预设里 `entity_default = 0`、方块 `0`，只给目标类型 `10` 的优先级 —— 客户端只把这一个类型当候选。
- 仍然一次只标一个目标（客户端自己选最优），角度/距离是客户端锥算的，不能无视。
- 这条包只是设参数，客户端得先开辅助瞄准（`/aimassist @s set` 之类）才会显示。

## 姿势（`actorSetPose`）

```js
ActorEffect.setPose(rid, 4);   // 元数据 PoseIndex(79)，数字对应客户端姿势
```

- 只发 `SetActorData` 的 `PoseIndex`，0=站立、1=睡觉、2=潜行、3=游泳、4=爬行（不同版本可能有出入，直接试）。
- 和游泳一样，**本地玩家自己**的姿势由客户端输入驱动，元数据不一定拽得动；别人视角一般能变。

## 辅助瞄准高亮（`cameraAimAssist*`）

```js
ActorEffect.aimAssistSet("Steve", 35, 35, 8, 0);   // 水平角度 / 垂直角度 / 距离 / targetMode(0=Angle,1=Distance)
ActorEffect.aimAssistClear("Steve");
```

- 走 `CameraAimAssist`(316)：`Set` 时第一人称会出现辅助瞄准的调试高亮框（`showDebugRender=true`），`Clear` 关掉。
- 只发给指定玩家，不影响其他人。
- 这只是"高亮"视觉，不会真的吸附视角。

## 玩家死亡之后：要补 respawn 包

`actorPlayDeath(playerRuntimeId)` 对**真人玩家**使用时，客户端会进入死亡界面并向服务端请求复活，
但服务端认为该玩家一直活着，不会回 `RespawnPacket` —— 客户端就卡在死亡界面。
播放完动画后请用 [死亡后的复活](respawn.md) 补一个 `playerRespawn(playerName)`。

## 冰冻（`actorFreeze` / `actorUnfreeze`）

```js
const rid = ActorEffect.runtimeIdOfPlayer("Steve");
ActorEffect.freeze(rid, 0.6);   // 0..1，越大霜冻越重
ActorEffect.unfreeze(rid);      // 等价 freeze(rid, 0)
```

- 走实体元数据 **120 `FreezingEffectStrength`**（float，0..1），只改这个玩家客户端看到的霜冻暗角 / FOV 收缩。
- **不扣血、不加状态效果**；要"真冰冻"（减速、掉血）得再发 `MobEffect`（`effectId = 38` Freezing），那部分库没封装。
- 声音可以配 `LevelSoundEvent` 的 `FREEZE`(540) / `UNFREEZE`(541)，库也没封装声音。

## 播放客户端动画（等价 `/playanimation`）

```js
// 苦力怕膨胀（等价 /playanimation <实体> animation.creeper.swelling none 0 "<表达式>"）
ActorEffect.playAnimation(
  rid,
  "animation.creeper.swelling",
  "v.swelling_sin=math.sin(q.life_time*720);" +
    "v.swelling_scale1=1.0-0.1*math.max(v.swelling_sin,0)+0.15*math.max(-v.swelling_sin,0);" +
    "v.swelling_scale2=1.0+0.1*math.max(v.swelling_sin,0)-0.4*math.max(-v.swelling_sin,0);",
  0
);

// 完整版：next_state / controller / Molang 版本
ActorEffect.playAnimationFull(rid, "animation.creeper.swelling", "none", "<表达式>", 0, "", 13);
```

- 包体是 `AnimateEntity`(158)：动画名、下一个状态、停止表达式（Molang）、控制器、淡出时间、runtimeId 列表。
- **只有客户端在这个实体的动画定义里找得到该动画才会播**：`animation.creeper.swelling` 属于苦力怕族。
  给羊、僵尸发同一条包，客户端找不到动画就什么都不播。
- 想让别的生物也演这个动画，得让它"在客户端看起来是苦力怕"：视图覆盖换模型，或者自定义实体直接用苦力怕的客户端 type。
- 物品展示 / 展示方块没有实体动画，这套对它们无效。
