# 隐身（playerHide*）

控制其他玩家的客户端是否接收某个玩家的数据包。提供两种方式：

| 方式 | 作用范围 | 对已出生的玩家 |
|---|---|---|
| 掩码（`playerHideSetMask`） | 屏蔽后续指定类型的数据包 | 不会消失，仅停止更新 |
| 隐身（`playerHideSetHidden`） | 出生、移动、状态、皮肤、Tab 一并屏蔽 | 立即从其他客户端消失 |

两种方式互不替代，按需求选择。

```js
const { Visibility } = require("plugins/HologramLib/lib/HologramLib.js");

// 只屏蔽出生包与 Tab 条目，移动仍然同步
Visibility.setMask("Steve", true, true, false, false, false, false);

// 完全隐身，本人仍可看到自己
Visibility.setHidden("Steve", true);

// 恢复：清除掩码与隐身，并立即重新发送出生包
Visibility.restore("Steve");

// 查询当前状态
logger.info(Visibility.getMask("Steve"));
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `playerHideSetMask(playerName, spawn, tab, motion, state, skin, alsoSelf)` | `Boolean` | 设置掩码；全部为 `false` 表示清除 |
| `playerHideGetMask(playerName)` | `String` | 当前状态，如 `"spawn=0 tab=1 motion=0 state=0 skin=1 hidden=0"` |
| `playerHideSetHidden(playerName, hidden)` | `Boolean` | 设置或取消隐身 |
| `playerHideIsHidden(playerName)` | `Boolean` | 查询隐身状态 |
| `playerHideRestore(playerName)` | `Boolean` | 清除掩码与隐身，并重新发送出生包 |

`alsoSelf=true` 时，该玩家自己也收不到这些数据包；默认为 `false`，仅影响其他玩家。

## 掩码说明

| 参数 | 屏蔽的数据包 | 效果 |
|---|---|---|
| `spawn` | `AddPlayer`、`PlayerList(Add)` | 看不到出生（已出生的不受影响） |
| `tab` | `PlayerList` | 从 Tab 列表移除 |
| `motion` | `MovePlayer`、`MoveActorAbsolute`、`MoveActorDelta`、`SetActorMotion` | 位置停止更新 |
| `state` | `SetActorData`、`UpdateAttributes`、`MobEffect`、`Animate`、`ActorEvent` | 状态与动作停止更新 |
| `skin` | `PlayerSkin` | 不发送皮肤 |

## 说明

- 无法识别类型的数据包不会被拦截。
- 一条 `PlayerList` 中包含多个玩家时，整条数据包放行。
- 玩家下线时库清除其记录，客户端随下线同步消失。
- `playerHideRestore` 重新发送的出生包包含当前皮肤、装备与姿态。
