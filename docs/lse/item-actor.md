# 掉落物生成（itemActor*）

在客户端显示一只掉落物实体。该实体只存在于客户端：服务端没有对应实体，不参与拾取、不进存档、没有服务端物理。

适用于奖励飞出、抽奖、过场道具等展示场景。需要玩家真正拾取的物品请用原版掉落物。

## 快速开始

```js
const { ItemActor } = require("mods/HologramLib/lib/HologramLib.js");

// 在 (100.5, 65, 100.5) 生成一颗钻石，初速度向上 0.4
const id = ItemActor.spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);

// 1 秒后改变方向
setTimeout(() => ItemActor.setMotion(id, -0.3, 0.2, 0), 1000);

// 播放飞向玩家的动画并删除
ItemActor.take(id, player.realName);
```

## 函数

### 生成一只掉落物

`itemActorSpawn(itemId, aux, count, x, y, z, dim, vx, vy, vz)`

- 参数：
  - `itemId` : `String`<br>物品 id，如 `minecraft:diamond`
  - `aux` : `Integer`<br>附加数据值，一般 `0`
  - `count` : `Integer`<br>数量，`< 1` 按 `1`
  - `x`、`y`、`z` : `Float`<br>世界坐标（`.5` 落在方块中心）
  - `dim` : `Integer`<br>维度 `0 / 1 / 2`
  - `vx`、`vy`、`vz` : `Float`<br>初速度
- 返回值：库内 id；该维度没有玩家或参数不合法返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const spawn = ll.import("HologramLib", "itemActorSpawn");
    const id = spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
    ```

### 设置速度与方向

`itemActorSetMotion(id, vx, vy, vz)`

- 参数：
  - `id` : `Integer`<br>`itemActorSpawn` 返回的 id
  - `vx`、`vy`、`vz` : `Float`<br>新速度；`0,0,0` 表示停止
- 返回值：`true` 已设置，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "itemActorSetMotion")(id, -0.3, 0.2, 0);
    ```

### 传送到指定坐标

`itemActorMoveTo(id, x, y, z)`

- 参数：
  - `id` : `Integer`<br>掉落物 id
  - `x`、`y`、`z` : `Float`<br>目标坐标
- 返回值：`true` 已传送，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "itemActorMoveTo")(id, 100, 70, 100);
    ```

### 播放拾取动画并删除

`itemActorTake(id, playerName)`

- 参数：
  - `id` : `Integer`<br>掉落物 id
  - `playerName` : `String`<br>拾取动画面向的玩家
- 返回值：`true` 已播放并删除，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "itemActorTake")(id, "Steve");
    ```

只播放动画并删除实体，**不发放物品**，实际发放由脚本处理。

### 删除一只

`itemActorDestroy(id)`

- 参数：
  - `id` : `Integer`<br>掉落物 id
- 返回值：`true` 已删除，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "itemActorDestroy")(id);
    ```

### 删除全部

`itemActorDestroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`Null`
- 示例：

    ```js
    ll.import("HologramLib", "itemActorDestroyAll")();
    ```

### 查询是否存在

`itemActorExists(id)`

- 参数：
  - `id` : `Integer`<br>掉落物 id
- 返回值：是否存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "itemActorExists")(id);
    ```

### 查询所有 id

`itemActorGetIds()`

- 参数：无
- 返回值：当前所有掉落物 id 的数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const ids = ll.import("HologramLib", "itemActorGetIds")();
    ```

## 注意

- 只发送给同维度的玩家；生成时该维度没有玩家返回 `-1`。
- 物品只包含物品 id、数量与 aux，**不携带 NBT**；需要自定义名称请用[物品显示](item-display.md)。
- 实体号段为 `0x70`，不参与交互事件路由。
