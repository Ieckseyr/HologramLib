# 侧边栏计分板（sidebar*）

给一名玩家显示只有他看得到的侧边栏。**只发包，不动服务端记分板** —— 不读也不改 BDS 自己的计分板。

```js
const { Sidebar } = require("plugins/HologramLib/lib/HologramLib.js");

// 显示（行自上而下）
Sidebar.set("Steve", "§6深渊竞技场", [
    "§7第一行",
    "§7第二行",
    "§e积分: §f" + score
]);

// 只改标题 / 只改某一行（内部按整条重发，调用方不用管）
Sidebar.setTitle("Steve", "§c血战阶段");
Sidebar.setLine("Steve", 0, "§7新的第一行");

// 查询
Sidebar.isShown("Steve");     // true
Sidebar.lineCount("Steve");   // 3

// 移除（关掉该玩家的侧边栏）
Sidebar.remove("Steve");

// 移除所有人的
Sidebar.removeAll();
```

| 函数 | 返回 | 说明 |
|---|---|---|
| `sidebarSet(playerName, title, [lines])` | `Boolean` | 显示或更新；`lines` 空数组等于移除 |
| `sidebarSetTitle(playerName, title)` | `Boolean` | 改标题（行不变） |
| `sidebarSetLine(playerName, index, text)` | `Boolean` | 改第 `index` 行（0 起） |
| `sidebarRemove(playerName)` | `Boolean` | 移除该玩家的侧边栏 |
| `sidebarRemoveAll()` | `Null` | 移除所有玩家的侧边栏 |
| `sidebarIsShown(playerName)` | `Boolean` | 当前是否显示 |
| `sidebarLineCount(playerName)` | `Integer` | 当前行数 |

## 说明

- 每条侧边栏的 objective 名按**玩家名**派生，不同玩家互不干扰，也不与服务器自己的记分板撞名。
- 行是"假玩家"条目，`score` 只当排序键用（第 0 行在最上面）。
- 协议是 `SetDisplayObjective`(107) / `SetScore`(108) / `RemoveObjective`(106)。
- 行数变化时会先清旧行再写新行，不会留下残影。
- 玩家下线时库自动清掉记录（客户端那边随下线消失，不需要再发包）。
