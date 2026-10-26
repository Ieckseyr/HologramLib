# 掉落物生成（itemActor*）

客户端凭空出现一只**掉落物**。服务端没有这只实体 —— 不参与拾取判定、不进存档、没有物理；
运动由客户端自己算，所以给初速度、飞行途中改方向都只是发包。

这类东西适合做表演：奖励飞出来、抽奖转盘、剧情里飘出来的道具。要"玩家真能捡走"就不该用它。

```js
const itemActorSpawn     = ll.import("HologramLib", "itemActorSpawn");
const itemActorSetMotion = ll.import("HologramLib", "itemActorSetMotion");
const itemActorTake      = ll.import("HologramLib", "itemActorTake");

// 在 (100.5, 65, 100.5) 抛一颗钻石出去（初速度向上 0.4）
const id = itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);

// 1 秒后让它往回飞 —— 中途改速度/方向就是这一句
setTimeout(() => itemActorSetMotion(id, -0.3, 0.2, 0), 1000);

// 玩家点了就播"飞过去"的动画再删掉（物品不会真的进背包）
itemActorTake(id, player.realName);
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `itemActorSpawn(itemId, aux, count, x, y, z, dim, vx, vy, vz)` | `Integer` | 生成；返回库内 id，`-1` = 失败 |
| `itemActorSetMotion(id, vx, vy, vz)` | `Boolean` | 飞行途中改速度与方向；`0,0,0` = 停住 |
| `itemActorMoveTo(id, x, y, z)` | `Boolean` | 瞬移（不带速度） |
| `itemActorTake(id, playerName)` | `Boolean` | 只给这名玩家播"飞过去"，然后对所有人删掉 |
| `itemActorDestroy(id)` | `Boolean` | 删掉 |
| `itemActorDestroyAll()` | `Null` | 全删 |
| `itemActorExists(id)` | `Boolean` | 还在不在 |
| `itemActorGetIds()` | `Array<Integer>` | 当前所有 id |

## 边界

- **只发给同维度的玩家。** `spawn` 时该维度没人在线直接返回 `-1`，不占 id。
- **`itemActorSetMotion` 是"设定当前速度"**，不是一次性冲量。掉头、悬停、加速都用它，可以反复调。
- **物品不带 NBT。** 走的是数字物品 id + 数量 + aux。要"带自定义名字的物品"用
  [物品显示](item-display.md) 那条路。
- **`itemActorTake` 不会真的给物品。** 它只播动画再删除；发不发物品是脚本自己的事。
- 实体号段是 `0x70` 段，**不在交互事件（ghost）的路由范围内** —— 地面掉落物不该被当成"点了要回调"的对象。

## 还没实测

外观（是不是原版那个旋转的小方块）、抛物线是否自然、`itemActorTake` 的动画、
以及"两个人同时看同一只时的位置同步"。第一次验按这四条走。
