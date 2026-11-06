# 隐身（playerHide*）

控制其他玩家的客户端是否接收某个玩家的数据包。两种方式互不替代：

| 方式 | 作用范围 | 对已出生的玩家 |
|---|---|---|
| 掩码（`playerHideSetMask`） | 屏蔽后续指定类型的数据包 | 不会消失，仅停止更新 |
| 隐身（`playerHideSetHidden`） | 出生、移动、状态、皮肤、Tab 一并屏蔽 | 立即从其他客户端消失 |

## 快速开始

```js
const { Visibility } = require("mods/HologramLib/lib/HologramLib.js");

// 只屏蔽出生包与 Tab 条目，移动仍然同步
Visibility.setMask("Steve", true, true, false, false, false, false);

// 完全隐身，本人仍可看到自己
Visibility.setHidden("Steve", true);

// 恢复：清除掩码与隐身，并立即重新发送出生包
Visibility.restore("Steve");
```

## 函数

### 设置掩码

`playerHideSetMask(playerName, spawn, tab, motion, state, skin, alsoSelf)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `spawn` : `Boolean`<br>屏蔽 `AddPlayer` / `PlayerList(Add)`，看不到出生（已出生的不受影响）
  - `tab` : `Boolean`<br>屏蔽 `PlayerList`，从 Tab 列表移除
  - `motion` : `Boolean`<br>屏蔽移动包，位置停止更新
  - `state` : `Boolean`<br>屏蔽状态与动作包（`SetActorData` / `UpdateAttributes` / `MobEffect` / `Animate` / `ActorEvent`）
  - `skin` : `Boolean`<br>屏蔽 `PlayerSkin`
  - `alsoSelf` : `Boolean`<br>`true` 时这名玩家自己也收不到这些包；默认 `false`，只影响别人
- 返回值：`true` 已设置；全 `false` 表示清除，玩家不在线返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setMask = ll.import("HologramLib", "playerHideSetMask");
    setMask("Steve", true, true, false, false, false, false);
    ```

### 查询掩码

`playerHideGetMask(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：形如 `"spawn=0 tab=1 motion=0 state=0 skin=1 hidden=0"` 的状态串
- 返回值类型：`String`
- 示例：

    ```js
    const state = ll.import("HologramLib", "playerHideGetMask")("Steve");
    ```

### 设置或取消隐身

`playerHideSetHidden(playerName, hidden)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `hidden` : `Boolean`<br>`true` 隐身，`false` 恢复
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "playerHideSetHidden")("Steve", true);
    ```

### 查询隐身状态

`playerHideIsHidden(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：当前是否隐身
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "playerHideIsHidden")("Steve");
    ```

### 恢复（清掩码 + 取消隐身 + 重发出生包）

`playerHideRestore(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`true` 已恢复，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "playerHideRestore")("Steve");
    ```

下发的出生包包含当前皮肤、装备与姿态。

## 注意

- 无法识别类型的数据包不会被拦截。
- 一条 `PlayerList` 包含多个玩家时，整条数据包放行。
- 玩家下线时库清除记录，客户端随下线同步消失。
- 与[方位条](locator-bar.md#隐身)的 `locatorBarHideFromOthers` 是两套独立机制。
