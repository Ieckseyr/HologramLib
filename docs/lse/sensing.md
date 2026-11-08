# 客户端感知 · LSE

从 PlayerAuthInput 捕获输入设备类型。


## 感知（sensing*）

数据源是客户端每 tick 上报的输入方式，库记在内存里。**没有事件、不用轮询**，需要时直接查。

### 查玩家当前用什么设备

`sensingDeviceOf(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`keyboardMouse` / `touch` / `gamepad` / `motionController` / `unknown`（玩家不在线或还没上报过输入时是 `unknown`）
- 返回值类型：`String`
- 示例：

    ```js
    const sensingDeviceOf = ll.import("HologramLib", "sensingDeviceOf");
    logger.info(`Steve 的设备：${sensingDeviceOf("Steve")}`);
    ```

### 查玩家是不是触屏

`sensingIsTouch(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`true` = 触屏；`false` = 不是触屏，或玩家不在线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const sensingIsTouch = ll.import("HologramLib", "sensingIsTouch");
    const containerOpen  = ll.import("HologramLib", "containerOpen");
    const tradeOpen      = ll.import("HologramLib", "tradeOpen");

    // 触屏用容器列表（点击必然发包），键鼠用交易菜单
    if (sensingIsTouch("Steve")) containerOpen("Steve", "§8任务列表", 3);
    else tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
    ```

### 取设备编号的数字版本

`sensingDeviceCode(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`0` = Unknown `1` = 键鼠 `2` = 触屏 `3` = 手柄 `4` = 体感（方便存进计分板或配置）
- 返回值类型：`Integer`
- 示例：

    ```js
    const sensingDeviceCode = ll.import("HologramLib", "sensingDeviceCode");
    logger.info(`设备码：${sensingDeviceCode("Steve")}`);
    ```

## 攻击 / 挥动监听（swing*）

!!! note "endstone 侧同名能力"

    endstone 的 `lib.sensing` 也实现了同样的九个方法（`poll_events()` / `count()` / `last_target()` …），
    目标判定改用 `PlayerInteractEvent` 的 `LEFT_CLICK_BLOCK` / `LEFT_CLICK_AIR`，
    事件行格式与本文一致，见 [Endstone · 客户端感知](../endstone/sensing.md)。


客户端每「挥一下」都会发 `AnimatePacket(action = Swing)`：空手挥、挥剑、打空气、点方块、打实体全算。
库在 `ServerNetworkHandler` 的 `AnimatePacket` 入口记录，**不拦包、不改服务端行为**。

协议里只有「挥动」这一个动作，没有「我挥的是什么」。所以库把每条事件判定成三档之一：

| `target` | 什么时候是它 | 怎么判定 |
|---|---|---|
| `air` | 对着空气挥（空挥） | 来源不是攻击，且视线 5 格内没有方块 |
| `block` | 对着方块挥（挖 / 放 / 点） | 视线射线命中方块，带方块坐标与命中面 |
| `entity` | 打实体 | 客户端自报来源是 `attack`，或包里带了 Target Actor Runtime ID |

来源 `source` 取自 1.26 起客户端会带的 `Swing Source` 字段：
`unset`（客户端没带）/ `none` / `build` / `mine` / `interact` / `attack` / `useItem` / `throwItem` / `dropItem` / `event`。

!!! note "为什么还要自己射线"

    `Swing Source` 只回答「这次挥动是哪类操作触发的」，不回答「面前有没有方块」——
    对着空气左键也会报 `mine`。方块判定由服务端沿视线补一条 5 格射线（用引擎的 outline 形状，
    草丛、火把这类没有碰撞体的方块不会被误判成方块）。

### 事件行格式

每行 `key=value`，空格分隔：

```text
player=Steve source=mine target=block pos=12,64,-3 face=1 item=minecraft:iron_sword count=1 runtimeId=0 uniqueId=0 tick=123456
```

| 字段 | 说明 |
|---|---|
| `player` | 玩家真实名（大小写敏感） |
| `source` | 见上：`unset` / `none` / `build` / `mine` / `interact` / `attack` / `useItem` / `throwItem` / `dropItem` / `event` |
| `target` | `air` / `block` / `entity` / `unknown` |
| `pos` | `target=block` 时的方块坐标；其它情况 `0,0,0` |
| `face` | 命中面 0..5（down/up/north/south/west/east）；非方块为 `-1` |
| `item` | 手持物品 typeName；空手为 `none` |
| `count` | 手持物品数量；空手为 `0` |
| `runtimeId` | 包里的 Target Actor Runtime ID（通常为 0） |
| `uniqueId` | `target=entity` 且能按 runtimeId 找到实体时的实体 uniqueId，否则 0 |
| `tick` | 事件发生时的服务器 tick |

### 轮询挥动事件

`swingPollEvents()`

- 参数：无
- 返回值：自上次轮询以来积累的事件行数组（每行一个事件）；取走即清空
- 返回值类型：`Array<String>`
- 示例：

    ```js
    const swingPollEvents = ll.import("HologramLib", "swingPollEvents");

    mc.listen("onTick", () => {
        for (const line of swingPollEvents()) {
            const f = Object.fromEntries(line.split(" ").map(kv => kv.split("=")));
            if (f.target === "block") logger.info(`${f.player} 对着方块 ${f.pos} 挥动（面 ${f.face}）`);
            if (f.target === "air") logger.info(`${f.player} 空挥了一下`);
            if (f.target === "entity") logger.info(`${f.player} 打到了实体 ${f.uniqueId}`);
        }
    });
    ```

!!! tip "队列上限 1024 条"

    没人轮询时事件会一直累积：超过 1024 条后丢最早的。
    用 `swingDropped()` 看丢了多少条；查过之后可以用 `swingReset()` 一起清零。

### 清空事件队列

`swingClearEvents()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```js
    const swingClearEvents = ll.import("HologramLib", "swingClearEvents");
    swingClearEvents();   // 丢掉还没轮询的事件（计数不清）
    ```

### 查某玩家最近一次挥动

`swingLastEvent(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家真实名
- 返回值：最近一次事件行；该玩家还没挥过时返回空串
- 返回值类型：`String`
- 示例：

    ```js
    const swingLastEvent = ll.import("HologramLib", "swingLastEvent");
    logger.info(swingLastEvent("Steve"));
    ```

`swingLastTarget(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家真实名
- 返回值：`air` / `block` / `entity`；该玩家还没挥过时返回 `none`
- 返回值类型：`String`

`swingLastBlock(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家真实名
- 返回值：最近一次挥动的方块坐标 `x,y,z`；最近一次不是对着方块挥时返回空串
- 返回值类型：`String`

### 计数

`swingCount(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家真实名
- 返回值：该玩家自上次 `swingReset` 以来的挥动次数；从没挥过返回 `0`
- 返回值类型：`Integer`

`swingTotal()`

- 参数：无
- 返回值：全服累计挥动次数（含已退出玩家，直到 `swingReset`）
- 返回值类型：`Integer`

`swingDropped()`

- 参数：无
- 返回值：事件队列满被丢弃的条数（诊断用；`swingReset` 清零）
- 返回值类型：`Integer`

`swingReset()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 说明：清空全部玩家计数、总数、事件队列、最近一次事件与丢弃计数
- 示例：

    ```js
    const swingReset = ll.import("HologramLib", "swingReset");
    swingReset();
    ```

---

