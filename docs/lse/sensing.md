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

---
