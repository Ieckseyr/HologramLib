# 表单控制（form*）

关闭客户端当前打开的表单。表单的创建仍由 LSE、LeviLamina Form API 或 MeowMenu 负责。

适用于：玩家无法用 ESC 关闭的界面、被其他插件覆盖的界面、过场前清屏、玩家下线前收尾。

```js
const { Form } = require("plugins/HologramLib/lib/HologramLib.js");

// 过场前关闭玩家当前打开的表单
Form.close(player.realName);

// 玩家下线时关闭
mc.listen("onLeft", (p) => Form.close(p.realName));
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `formClose(playerName)` | `Boolean` | 关闭该玩家当前打开的表单；玩家不在线返回 `false` |
| `formCloseAll()` | `Integer` | 向所有在线玩家发送，返回发送数量 |

## 说明

- 不区分表单来源，客户端当前打开的表单都会被关闭。
- 玩家未打开表单时调用也会返回 `true`，客户端不会报错。
- 只影响客户端显示，服务端不记录表单状态。
