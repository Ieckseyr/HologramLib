# 掉落物生成（itemActor*）

在客户端显示一只掉落物实体。该实体只存在于客户端：服务端没有对应实体，不参与拾取、不进存档、没有服务端物理。

适用于奖励飞出、抽奖、过场道具等展示场景。需要玩家真正拾取的物品请使用原版掉落物。

```js
const { ItemActor } = require("plugins/HologramLib/HologramLib.js");

// 在 (100.5, 65, 100.5) 生成一颗钻石，初速度向上 0.4
const id = ItemActor.spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);

// 1 秒后改变方向
setTimeout(() => ItemActor.setMotion(id, -0.3, 0.2, 0), 1000);

// 播放飞向玩家的动画并删除
ItemActor.take(id, player.realName);
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `itemActorSpawn(itemId, aux, count, x, y, z, dim, vx, vy, vz)` | `Integer` | 生成掉落物，返回 id；`-1` 表示失败 |
| `itemActorSetMotion(id, vx, vy, vz)` | `Boolean` | 设置当前速度与方向，可重复调用 |
| `itemActorMoveTo(id, x, y, z)` | `Boolean` | 传送到指定坐标 |
| `itemActorTake(id, playerName)` | `Boolean` | 向该玩家播放拾取动画，然后对所有人删除 |
| `itemActorDestroy(id)` | `Boolean` | 删除 |
| `itemActorDestroyAll()` | `Null` | 删除全部 |
| `itemActorExists(id)` | `Boolean` | 是否存在 |
| `itemActorGetIds()` | `Array<Integer>` | 当前所有 id |

## 说明

- 只发送给同维度的玩家。生成时该维度没有玩家则返回 `-1`。
- `itemActorSetMotion` 为设置当前速度，可随时调整，`0,0,0` 表示停止。
- 物品只包含物品 id、数量与 aux，不携带 NBT。需要自定义名称的物品请使用[物品显示](item-display.md)。
- `itemActorTake` 只播放动画并删除实体，不发放物品，实际发放由脚本处理。
- 实体号段为 `0x70`，不参与交互事件路由。
