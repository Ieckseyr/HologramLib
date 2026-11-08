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

## 空手挥动 / 攻击空气（swing*，1.37.0）

客户端"挥一下"会发 `AnimatePacket(Swing)`——**空手打空气、对着方块挥、打实体都算**，
服务端在 `ServerNetworkHandler::$handle(AnimatePacket)` 里计数。没有事件回调，查计数即可。

计数按玩家真实名（大小写敏感）记录，玩家退出不清零，直到 `swingReset`。

### 查某个玩家挥了多少次

`swingCount(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名（真实名，大小写敏感）
- 返回值：该玩家自上次重置以来的挥动次数；玩家从没挥过或不在线返回 `0`
- 返回值类型：`Integer`
- 示例：

    ```js
    const swingCount = ll.import("HologramLib", "swingCount");
    logger.info(`Steve 挥了 ${swingCount("Steve")} 次`);
    ```

### 查全服总次数

`swingTotal()`

- 参数：无
- 返回值：所有玩家累计挥动次数（含已退出的玩家，直到重置）
- 返回值类型：`Integer`
- 示例：

    ```js
    const swingTotal = ll.import("HologramLib", "swingTotal");
    logger.info(`全服累计 ${swingTotal()} 次`);
    ```

### 清零

`swingReset()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```js
    const swingReset = ll.import("HologramLib", "swingReset");
    swingReset();   // 清空全部玩家计数与总数
    ```

!!! note "能区分挥空气和挥方块吗"

    不能。协议里只有一个 `Swing` 动作，客户端不会告诉服务端"我对着什么挥"。
    要区分得由脚本自己结合视线/方块交互事件判断；这个接口只回答"挥了没有、挥了几次"。
---

!!! note "endstone 侧同名能力"

    endstone 的 `lib.sensing` 也提供 `count(player_name)` / `total()` / `reset()`，
    语义与这里完全一致（同样是入站 `AnimatePacket(Swing)` 计数，不区分目标）。
