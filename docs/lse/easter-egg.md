# 彩蛋：朋友的⑨

昨日一去不回，噢耶耶，开心比什么都贵........

## `easterEggCreeper(runtimeId)`

```js
const rid = actorRuntimeIdOfPlayer("Steve");   // 或自定义实体 / NPC 的 runtimeId
easterEggCreeper(rid);
```

- 参数：
  - `runtimeId` : `Int64`<br>要演的实体 runtimeId。用 `actorRuntimeIdOfPlayer` / `playerNpcRuntimeId` / `actorRuntimeIdOfEntity` / `itemDisplayRuntimeId` 取
- 返回值：`true` = 已下发；实体不存在返回 `false`
- 返回值类型：`Boolean`
- 说明：发一条 `AnimateEntity`（包 158），动画 `animation.creeper.swelling`，之后由客户端逐帧算，平滑。
  只有客户端在该实体的动画定义里找得到这条动画才会演（苦力怕族）；玩家等其它模型只有上半身跟着弹。

## `easterEggCreeperDisplay(displayId, durationTicks, rotate)`

```js
const id = itemDisplayCreate("minecraft:creeper_head", 100.5, 64, 100.5);
easterEggCreeperDisplay(id, 80, true);   // 80 tick ≈ 4 秒，同时旋转
```

- 参数：
  - `displayId` : `Int64`<br>物品展示 id（`itemDisplayCreate` 的返回值）
  - `durationTicks` : `Int`<br>持续 tick；`<= 0` 按 60 tick 处理
  - `rotate` : `Boolean`<br>`true` = 膨胀同时绕 Y 轴转，一次肿胀正好转一圈
- 返回值：`true` = 已生效；展示不存在返回 `false`
- 返回值类型：`Boolean`
- 说明：把 `stretchXZ` / `stretchY`（`rotate` 时连 `rotY`）临时换成带 `q.life_time` 的表达式，
  到期由库的 tick 自动还原；重复调用重置计时，不叠加。

## 注意

- 只影响客户端观感，不动服务端实体、方块、背包。
- 动画名来自原版资源包，换包可能就不演了。
