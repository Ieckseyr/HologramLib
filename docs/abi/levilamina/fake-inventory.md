# 1.14 IFakeInventory · 假背包

完整签名（`#include "HologramLib/HologramLib.h"`）：

`IHologramLib::fakeInventories()`。**协议层改写客户端看到的玩家背包**：一条 `InventoryContentPacket(49)`
（`ContainerId = Inventory(0)`、0..35 号槽位）把整份内容换成调用方给的那一份；单格改动走 `InventorySlotPacket(50)`。
服务端背包一个字都不动 —— 物品是"看起来有"。

> **物品序列化交给 BDS**：物品描述符的 User Data 用的是 u16 名字长度 / 4 字节整数的那套 NBT 方言，
> 与 sculk `CompoundTag` 的 varint 方言不同；手写会写错（实测把客户端卡死）。所以本域用 BDS 的
> `InventoryContentPacket` / `InventorySlotPacket` 构造包体（容器名字节 0 + storage 空描述符, 与抓包一致）。

参考实现 GMLIB 的 `ChestUI` 填玩家物品栏用的就是同一条路（它逐格写 `InventorySlot`，容器 id 同样是
`Inventory(0)` + `InventoryContainer(29)`）；本域的整份下发用一条 `InventoryContent` 搞定。

**功能项与虚拟容器一致**：玩家点自己背包里的伪造物品 → 回调报槽位号，物品不会真的被拿走 ——
客户端按伪造内容发出请求，服务端真实槽位对不上 → BDS 判失败 → 客户端把预测撤回（与虚拟容器同一套
表现，库不需要代答）；库随后把那一格重发一次，让伪造内容不被这次回滚冲掉。

**与交易菜单 / 虚拟容器共存**：打开交易界面或箱子界面时 BDS 会重发玩家背包内容（伪造内容被覆盖），
所以本域默认按 `refreshIntervalTicks` 周期重发（默认 20 tick = 1s），也可随时 `refresh()`。
界面上看到的背包区域因此始终是伪造内容，点击照常回调。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 下发 | apply | `(std::string const& playerName, FakeInventorySpec const&) -> bool` | 整份伪造内容（覆盖该玩家此前的）; 玩家不在线 false |
| | setSlot | `(std::string const& playerName, int slot, ContainerMenuItem const&) -> bool` | 单格改动（一条 `InventorySlot`，无延迟）; 未 apply 过返回 false |
| | refresh | `(std::string const& playerName) -> bool` | 立即重发当前伪造内容 |
| | clear / clearAll | `(std::string const& playerName) -> bool` / `() -> void` | 撤销伪造 + 停周期重发，并把**真实**背包重发一遍 |
| 查询 | isActive / getActivePlayers | `(std::string const&) -> bool` / `() -> std::vector<std::string>` | |
| 回传 | addClickListener / removeClickListener | `(std::function<void(FakeInventoryClickEvent const&)>) -> uint64_t` | 多播；点伪造成非空的格子才回调 |

```cpp
inline constexpr int kFakeInventorySlots = 36; // 0..8 快捷栏, 9..35 主背包

struct FakeInventorySpec {
    std::vector<ContainerMenuItem> items;   // 下标即槽位; type 空 = 该格留空
    int refreshIntervalTicks{20};           // 0 = 不周期重发
};

struct FakeInventoryClickEvent {
    std::string playerName;
    int         slot{-1};                   // 被点击的伪造槽位
};
```

**边界（实测前先写清楚，免得误用）**：

- 伪造只影响客户端显示。玩家"真正能用/能吃"的仍是服务端真实物品。
- 若某格真实物品与伪造物品恰好一致，那一次操作会被服务端当真执行 —— 想让某格纯展示，别把它伪造成与真实物品相同的东西。
- 该格真实物品因为别的原因改变（捡东西 / 用物品 / 别的插件改背包）时，周期重发会把伪造内容重新盖回去。
- 护甲（`ArmorContainer` 6）/ 副手（`OffhandContainer` 34）不在本域范围内，它们各有独立的容器枚举。

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
