# 隐身与出站包控制（playerHide*）

按"**被看的人**（主体）"屏蔽出站包：其他玩家的客户端收不到关于他的某些包，或者干脆收不到他。

两套机制，用途不同，不要混：

| 机制 | 管什么 | 已经站在别人屏幕上的人会怎样 |
|---|---|---|
| **掩码**（`playerHideSetMask`） | 之后某几类包不再发给别人 | **不会消失**（它只挡后续的包） |
| **隐身**（`playerHideSetHidden`） | 出生、移动、状态、皮肤、Tab 一起挡 | **立刻消失**（库会把它从别人客户端上撤掉） |

```js
const playerHideSetMask   = ll.import("HologramLib", "playerHideSetMask");
const playerHideSetHidden = ll.import("HologramLib", "playerHideSetHidden");
const playerHideGetMask   = ll.import("HologramLib", "playerHideGetMask");
const playerHideRestore   = ll.import("HologramLib", "playerHideRestore");

// 只挡出生 + Tab，移动照发（别人在 Tab 里看不到他，但他动起来仍然看得到）
playerHideSetMask("Steve", true, true, false, false, false, false);

// 整个人从别人屏幕上消失（本人照常看得到自己）
playerHideSetHidden("Steve", true);

// 恢复：清掩码 + 清隐身，并立刻把出生包重新发给所有观看者
playerHideRestore("Steve");

log(playerHideGetMask("Steve"));   // "spawn=0 tab=0 motion=0 state=0 skin=0 hidden=0"
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `playerHideSetMask(playerName, spawn, tab, motion, state, skin, alsoSelf)` | `Boolean` | 设置掩码；全 `false` = 撤销 |
| `playerHideGetMask(playerName)` | `String` | 当前状态，形如 `"spawn=0 tab=1 motion=0 state=0 skin=1 hidden=0"` |
| `playerHideSetHidden(playerName, hidden)` | `Boolean` | 整体隐身 / 取消隐身 |
| `playerHideIsHidden(playerName)` | `Boolean` | 查询隐身状态 |
| `playerHideRestore(playerName)` | `Boolean` | 清掩码 + 清隐身 + **立刻补发出生包**（不用重进服） |

`alsoSelf=true` 表示"连他自己收到的这些包也丢" —— 默认只影响其他人。

## 掩码里每一位挡的是什么

| 位 | 挡掉的包 | 别人看到的效果 |
|---|---|---|
| `spawn` | `AddPlayer` / `PlayerList(Add)` | 看不到他出生（已经出生的不受影响） |
| `tab` | `PlayerList` | 从 Tab 列表消失 |
| `motion` | `MovePlayer` / `MoveActorAbsolute` / `MoveActorDelta` / `SetActorMotion` | 他钉在原地 |
| `state` | `SetActorData` / `UpdateAttributes` / `MobEffect` / `Animate` / `ActorEvent` | 动作与状态不更新 |
| `skin` | `PlayerSkin` | 不发皮肤 |
