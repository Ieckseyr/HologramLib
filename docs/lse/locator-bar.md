# 方位条（locatorBar*）

1.26 客户端在快捷栏下方有一条方位指示。库按玩家维护一组**航点**：`key` 自己起名，
同一个 `key` 再 `set` 就是更新位置 —— 不需要保管 UUID（库按 key 稳定派生）。

适合做：追踪某个 boss / 队友 / 任务点、"往这边走"的引导。

```js
const locatorBarSet   = ll.import("HologramLib", "locatorBarSet");
const locatorBarClear = ll.import("HologramLib", "locatorBarClear");

// 给 Steve 指一个点；每 tick 刷新一次就是"跟着某个东西走"
mc.listen("onTick", () => {
  locatorBarSet("Steve", "boss", bossX, bossY, bossZ, 0, 0xFF0000, true);
});

locatorBarClear("Steve");
```

## 函数

| 函数 | 返回 | 说明 |
|---|---|---|
| `locatorBarSet(playerName, key, x, y, z, dim, color, visible)` | `Boolean` | 新增或更新；`color` = ARGB，`-1` = 客户端默认色 |
| `locatorBarRemove(playerName, key)` | `Boolean` | 删掉一个航点 |
| `locatorBarClear(playerName)` | `Integer` | 清全部，返回清掉了几个 |
| `locatorBarCount(playerName)` | `Integer` | 当前航点数 |
| `locatorBarHideFromOthers(playerName, hidden)` | `Boolean` | 把玩家从**其他人**客户端上藏起来（[隐身](visibility.md) 的薄包装） |
| `locatorBarIsHiddenFromOthers(playerName)` | `Boolean` | 查询上面那个状态 |

`hideFromOthers` 那两条只是同一个实现换了个入口 —— 完整的控制（按类别挡包、让已出生的人消失、
恢复时补发出生包）在 [隐身与出站包控制](visibility.md)。

## 边界

- `key` 是调用方起的名字，同一个玩家下唯一；不同玩家的 key 互不影响。
- `visible=false` 只是暂时隐藏，记录还在，再 `set(true)` 就回来。
- 玩家下线时库清掉他的记录（客户端那边随下线消失，不需要发包）。
