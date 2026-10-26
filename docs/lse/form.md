# 表单控制（form*）

库**不生成表单** —— 表单还是 LSE / LeviLamina Form API / MeowMenu 那边的事。
这里只提供一件事：**把客户端当前打开的表单强制收掉**。

用在：玩家按 ESC 关不掉的界面、被别的插件顶掉的界面、过场需要先清屏、玩家下线前收尾。

```js
const formClose = ll.import("HologramLib", "formClose");

// 出一个全屏过场之前，先把玩家正在看的表单收掉
formClose(player.realName);

// 玩家掉线时顺手收一下（有些表单会残留在客户端）
mc.listen("onLeft", (p) => formClose(p.realName));
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `formClose(playerName)` | `Boolean` | 关掉该玩家当前打开的表单；玩家不在线 = `false` |
| `formCloseAll()` | `Integer` | 给所有在线玩家各发一条，返回发给了几个 |

## 边界

- **不检查表单是谁开的。** 客户端开着表单就关，不管那是 LSE 的、别的插件的，还是原版的。
- 玩家当时**没有**打开表单时也能调用，成功返回 `true`（客户端不会报错，只是没东西可关）。
- 这是纯客户端表现：服务端那边的界面状态（谁打开了什么）库不记录，也不改动。

## 还没实测

被关掉之后，打开表单的那方（比如 LSE 的 `onFormClose` 之类）会不会收到通知、表单的返回值走哪条路。
如果消费方依赖表单回调做状态清理，要自己确认一遍。
