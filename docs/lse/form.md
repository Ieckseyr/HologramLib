# 表单控制（form*）

关闭客户端当前打开的表单。表单的创建仍由 LSE、LeviLamina Form API 或 MeowMenu 负责，这一组只管"关掉"。

适用于：玩家无法用 ESC 关闭的界面、被其他插件覆盖的界面、过场前清屏、玩家下线前收尾。

## 快速开始

```js
const { Form } = require("mods/HologramLib/lib/HologramLib.js");

// 过场前关闭玩家当前打开的表单
Form.close(player.realName);

// 玩家下线时关闭
mc.listen("onLeft", (p) => Form.close(p.realName));
```

## 函数

### 关闭一名玩家的表单

`formClose(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名（在线，大小写敏感）
- 返回值：`true` 已发送关闭包；玩家不在线返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const close = ll.import("HologramLib", "formClose");
    close("Steve");
    ```

### 关闭所有在线玩家的表单

`formCloseAll()`

- 参数：无
- 返回值：实际发出的关闭包数量
- 返回值类型：`Integer`
- 示例：

    ```js
    const n = ll.import("HologramLib", "formCloseAll")();
    ```

## 注意

- 不区分表单来源，客户端当前打开的表单都会被关闭。
- 玩家没开表单时调用也返回 `true`，客户端不会报错。
- 只影响客户端显示，服务端不记录表单状态。
