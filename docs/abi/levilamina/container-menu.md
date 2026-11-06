# 1.12 IContainerMenu · 虚拟容器

完整签名（`#include "HologramLib/HologramLib.h"`）：

`IHologramLib::containerMenus()`。方案逐条复刻参考实现 GMLIB 的 `ChestUI`：

1. `UpdateBlockPacket` 在玩家头上摆一个**客户端侧箱子方块**（只发给该玩家，服务端世界里没有它）
2. `BlockActorDataPacket` 摆该方块的方块实体 NBT：`id=Chest` / `CustomName=标题` / `x,y,z` / `pairx,pairz,pairlead`（大箱子）/ `Items=[条目物品]` —— 条目物品走这份 NBT，**不是**逐格 `InventorySlot`
3. 等 10 tick 发 `ContainerOpen`（`ContainerType::Container(0)` + 方块坐标 + 目标实体 `-1`）
**按槽动态刷新用 `setItem(menuId, slot, item)`**：只发一条 `InventorySlotPacket`（真实箱子同步内容用的同一种包），
客户端就地换掉那一格——不重发方块实体、不重发 `ContainerOpen`，**无延迟、无闪烁**；`item.type` 为空即清空该槽。
改整页内容才用 `update(menuId, spec)`。

**翻页/进子菜单/返回用 `update(menuId, spec)`**: 复用同一个载体方块, 不拆界面、不重摆方块、不等待,
只重发方块实体 NBT（新条目/标题）+ `ContainerOpen` 让客户端重读。走 `open()` 重开每次都要付一次打开延迟
（它会先关旧菜单、恢复真方块, 再重摆、再等 `openDelayTicks`）。

4. 玩家点击 → 客户端发 `ItemStackRequest(147)` → 槽位命中本容器 → 回调（槽位号 = 条目下标）。命中容器的动作**只回传、不拦**：服务端没有这个容器，放行后 BDS 自己就会失败、客户端把预测撤回（物品闪一下回原位，无错误提示），回调照常收到——与参考实现 GMLIB 一致。实测教训: 若库自己代答一条 `ItemStackNetResult` 3（请求不允许），客户端会**弹错误提示**
5. 关闭 → `ContainerClosePacket` → 把真方块改回去 → 回调 `closed = true`

**服务端根本没有这个容器**：物品只是"摆在那里"，玩家拿走/移动都不会真的改变任何东西（天然只读）。适合任务列表、成就列表、商店预览这类"只展示 + 点击回调"的界面。

大小容器：`rows = 3` → 单箱子 **27 格**（方块实体不带配对键）；`rows = 6` → 大箱子 **54 格**（相邻两个箱子方块 + `pairx`/`pairz`/`pairlead` 配对，前 27 格进 lead 半、后 27 格进副半，各半的 `Slot` 都是 0..26）。方块位置取玩家脚上方 5 格并优先挑空气位（免得盖掉真实方块），关闭时按该位置真方块的网络 id 改回。容器 id 用 101..199 显示区间，与 BDS 真实容器的动态 id（1..100）不冲突。

```cpp
struct ContainerMenuSpec {
    int rows{3};                       // 3 = 小箱子 27 格, 6 = 大箱子 54 格
    std::vector<ContainerMenuItem> items; // 下标即槽位号（大箱子 0..53）
    bool useMinecart{false};           // false(默认) = GMLIB 方块方案; true = 旧矿车路径（客户端不认, 仅对比）
    std::string title{"虚拟容器"};      // 箱子界面标题（方块实体的 CustomName）
    int openDelayTicks{7};             // 摆方块+方块实体后等多少 tick 再发 ContainerOpen
                                       // （GMLIB: 10 再等 4 补格 ≈700ms; 本库条目在方块实体 NBT 里
                                       // 只需 7 ≈350ms。实测 7 能开、6 开不了; 只影响"打开"那次）
};

struct ContainerClickEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         slot{-1};       // 被点击的槽位（= items 的下标）
    bool        closed{false};  // true = 界面被关闭（此时 slot = -1）
};
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
