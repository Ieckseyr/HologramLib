# 侧边栏计分板（sidebar*）

给一名玩家显示只有他看得到的侧边栏。**只发包，不动服务端记分板** —— 不读也不改 BDS 自己的计分板。

## 快速开始

```js
const { Sidebar } = require("mods/HologramLib/lib/HologramLib.js");

// 显示（行自上而下）
Sidebar.set("Steve", "§6深渊竞技场", ["§7第一行", "§7第二行", "§e积分: §f" + score]);

// 只改标题 / 只改某一行
Sidebar.setTitle("Steve", "§c血战阶段");
Sidebar.setLine("Steve", 0, "§7新的第一行");
```

## 函数

### 显示或更新侧边栏

`sidebarSet(playerName, title, lines)`

- 参数：
  - `playerName` : `String`<br>玩家名（在线，大小写敏感）
  - `title` : `String`<br>顶栏标题，支持 `§` 颜色码
  - `lines` : `Array<String>`<br>行内容，自上而下；空数组等于移除
- 返回值：`true` 已下发；玩家不在线或参数不合法返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const set = ll.import("HologramLib", "sidebarSet");
    set("Steve", "§6竞技场", ["§7第一行", "§e积分: §f42"]);
    ```

### 只改标题

`sidebarSetTitle(playerName, title)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `title` : `String`<br>新标题
- 返回值：`true` 已改，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "sidebarSetTitle")("Steve", "§c血战阶段");
    ```

### 只改某一行

`sidebarSetLine(playerName, index, text)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `index` : `Integer`<br>行号，从 `0` 开始（`0` 在最上面）
  - `text` : `String`<br>新的行内容
- 返回值：`true` 已改；行号越界或玩家不在线返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "sidebarSetLine")("Steve", 0, "§7新的第一行");
    ```

### 移除该玩家的侧边栏

`sidebarRemove(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`true` 已移除，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "sidebarRemove")("Steve");
    ```

### 移除所有玩家的侧边栏

`sidebarRemoveAll()`

- 参数：无
- 返回值：无
- 返回值类型：`Null`
- 示例：

    ```js
    ll.import("HologramLib", "sidebarRemoveAll")();
    ```

### 查询是否在显示

`sidebarIsShown(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：当前是否显示
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "sidebarIsShown")("Steve");
    ```

### 查询当前行数

`sidebarLineCount(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：行数；没显示时返回 `0`
- 返回值类型：`Integer`
- 示例：

    ```js
    ll.import("HologramLib", "sidebarLineCount")("Steve");
    ```

## 注意

- 每条侧边栏的 objective 名按**玩家名**派生，不同玩家互不干扰，也不与服务器自己的记分板撞名。
- 行是"假玩家"条目，`score` 只当排序键用（第 0 行在最上面）。
- 协议走 `SetDisplayObjective`(107) / `SetScore`(108) / `RemoveObjective`(106)。
- 行数变化时先清旧行再写新行，不会留下残影。
- 玩家下线时库自动清记录，客户端随下线消失。
