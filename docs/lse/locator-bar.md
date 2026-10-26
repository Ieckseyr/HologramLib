# 方位条（locatorBar*）

在客户端快捷栏下方的方位条上标出航点。库按玩家维护航点列表，`key` 由调用方命名，同一 `key` 再次设置即为更新位置，无需保存 UUID。

适用于追踪目标、队友位置、任务点引导。

```js
const { LocatorBar } = require("plugins/HologramLib/lib/HologramLib.js");

// 每 tick 刷新一次，让航点跟随目标
mc.listen("onTick", () => {
  LocatorBar.set("Steve", "boss", bossX, bossY, bossZ, 0, 0xFF0000, true);
});

LocatorBar.clear("Steve");
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `locatorBarSet(playerName, key, x, y, z, dim, color, visible)` | `Boolean` | 新增或更新航点；`color` 为 ARGB，`-1` 表示客户端默认色 |
| `locatorBarRemove(playerName, key)` | `Boolean` | 删除一个航点 |
| `locatorBarClear(playerName)` | `Integer` | 清空该玩家全部航点，返回清除数量 |
| `locatorBarCount(playerName)` | `Integer` | 当前航点数量 |
| `locatorBarHideFromOthers(playerName, hidden)` | `Boolean` | 将玩家从其他客户端隐藏，见[隐身](visibility.md) |
| `locatorBarIsHiddenFromOthers(playerName)` | `Boolean` | 查询隐藏状态 |

## 说明

- `key` 由调用方命名，同一玩家下唯一，不同玩家之间互不影响。
- `visible=false` 仅隐藏显示，记录保留，重新设置为 `true` 即恢复。
- 玩家下线时库清除其航点记录，客户端随下线同步消失。
- 航点的显示效果取决于客户端版本与资源包。
