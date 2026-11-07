# 1.14 IFakeInventory · 假背包

在协议层改写客户端看到的玩家背包 —— 物品只是「看起来有」，服务端背包一个字都不动。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().fakeInventories()`
- 槽位：`0..8` 快捷栏、`9..35` 主背包（`hologramlib::kFakeInventorySlots == 36`）
- 协议：整份下发用 `InventoryContentPacket`（`ContainerId = Inventory(0)`，36 格）；单格改动走 `InventorySlotPacket`
- 作用域：只影响客户端显示，玩家真正能用 / 能吃 / 能掉的仍是服务端真实物品

!!! warning "物品序列化交给 BDS，别自己拼 NBT"

    物品描述符的 User Data 用的是 u16 名字长度 / 4 字节整数的那套 NBT 方言，
    与 sculk `CompoundTag` 的 varint 方言不同；手写会写错（实测把客户端卡死）。
    所以本域用 BDS 的 `InventoryContentPacket` / `InventorySlotPacket` 构造包体
    （容器名字节 `0` + 空的 storage 描述符，与抓包一致）。消费方只需要填 `ContainerMenuItem`。

!!! tip "和交易菜单 / 虚拟容器共存"

    打开交易界面或箱子界面时 BDS 会重发玩家背包内容（伪造内容被覆盖），所以本域默认按
    `refreshIntervalTicks` 周期重发（默认 20 tick = 1s），也可以随时手动 `refresh()`。
    界面上看到的背包区域因此始终是伪造内容，点击照常回调。

## 数据结构

`FakeInventorySpec` —— 一份伪造背包：

| 字段 | 类型 | 默认 | 说明 |
|---|---|---|---|
| `items` | `std::vector<ContainerMenuItem>` | 空 | 下标即槽位；短于 36 的补空气，长于 36 的忽略 |
| `refreshIntervalTicks` | `int` | `20` | 周期重发间隔；`0` = 不周期重发，`1..4` 按 `5` 执行（最快 5 tick） |

`FakeInventoryClickEvent` —— 回调载荷：

| 字段 | 类型 | 说明 |
|---|---|---|
| `playerName` | `std::string` | 点击的玩家名 |
| `slot` | `int` | 被点击的伪造槽位（`0..35`） |

`ContainerMenuItem` 的字段（`type` / `count` / `damage` / `name` / `lore`）与虚拟容器共用，
详见 [虚拟容器](container-menu.md#数据结构)：`type` 空 = 该格留空，`count` 有效 `1..127`。

## 下发与撤销

### 下发整份伪造背包

`bool apply(std::string const& playerName, FakeInventorySpec const& spec)`

- 参数：
  - `playerName` : `std::string const&`<br>目标玩家名（必须在线）
  - `spec` : `FakeInventorySpec const&`<br>整份内容；覆盖该玩家此前的伪造内容
- 返回值：已下发 `true`；`false` = 玩家不在线
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& fakes = hologramlib::IHologramLib::getInstance().fakeInventories();

    hologramlib::FakeInventorySpec spec;
    spec.items.resize(36);
    spec.items[0].type  = "minecraft:diamond_sword";
    spec.items[0].name  = "§c虚容之剑";
    spec.items[1].type  = "minecraft:diamond";
    spec.items[1].count = 64;
    spec.refreshIntervalTicks = 20;      // 每 20 tick 重发一次

    fakes.apply("Steve", spec);
    ```

### 改一个格子

`bool setSlot(std::string const& playerName, int slot, ContainerMenuItem const& item)`

- 参数：
  - `playerName` : `std::string const&`<br>目标玩家名
  - `slot` : `int`<br>槽位 `0..35`（`0..8` 快捷栏，`9..35` 主背包）
  - `item` : `ContainerMenuItem const&`<br>新条目；`type` 空 = 清空该格
- 返回值：已改 `true`（一条 `InventorySlotPacket`，无延迟）；`false` = 槽位越界 / 该玩家还没 `apply` 过 / 玩家不在线
- 返回值类型：`bool`
- 示例：

    ```cpp
    hologramlib::ContainerMenuItem item;
    item.type  = "minecraft:emerald";
    item.count = 3;
    item.name  = "§a奖励";
    item.lore  = {"§7今天刚发", "§8别乱花"};

    fakes.setSlot("Steve", 1, item);
    fakes.setSlot("Steve", 2, {});   // 清空 2 号格
    ```

`setSlot` 要求这名玩家已经 `apply` 过一次；没 `apply` 过直接返回 `false`（不会自动建会话）。

### 立刻重发当前伪造内容

`bool refresh(std::string const& playerName)`

- 参数：
  - `playerName` : `std::string const&`<br>目标玩家名
- 返回值：已重发 `true`；`false` = 该玩家没有生效中的伪造，或玩家不在线
- 返回值类型：`bool`
- 示例：

    ```cpp
    fakes.refresh("Steve");   // 刚打开箱子 / 交易界面时补一发
    ```

### 撤销伪造并重发真实背包

`bool clear(std::string const& playerName)`

- 参数：
  - `playerName` : `std::string const&`<br>目标玩家名
- 返回值：已撤销 `true`（玩家在线时同时把**真实**背包重发一遍）；`false` = 该玩家本来就没有伪造
- 返回值类型：`bool`
- 示例：

    ```cpp
    fakes.clear("Steve");   // 玩家看到的立刻变回真实背包
    ```

### 撤销所有玩家

`void clearAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    fakes.clearAll();
    ```

## 查询

### 查询伪造是否生效

`bool isActive(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：生效中 `true`；没有 `false`（玩家下线后自动清掉，也会变 `false`）
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (!fakes.isActive("Steve")) {
        fakes.apply("Steve", spec);
    }
    ```

### 列出生效中的玩家

`std::vector<std::string> getActivePlayers() const`

- 参数：无
- 返回值：玩家名数组；一个都没有时是空数组
- 返回值类型：`std::vector<std::string>`
- 示例：

    ```cpp
    for (auto const& name : fakes.getActivePlayers()) {
        fakes.refresh(name);
    }
    ```

## 点击回调

### 注册点击监听

`uint64_t addClickListener(std::function<void(FakeInventoryClickEvent const&)> listener)`

- 参数：
  - `listener` : `std::function<void(FakeInventoryClickEvent const&)>`<br>回调；只有点到「伪造且非空」的格子才会触发
- 返回值：监听 token（`>= 1`）；传空函数返回 `0`，表示没有注册
- 返回值类型：`uint64_t`
- 示例：

    ```cpp
    uint64_t token = fakes.addClickListener(
        [](hologramlib::FakeInventoryClickEvent const& ev) {
            // ev.playerName / ev.slot
            if (ev.slot == 0) {
                // 快捷栏第 0 格被点了
            }
        });
    ```

点击的表现与虚拟容器一致：客户端按伪造内容发请求，服务端真实槽位对不上 → BDS 判失败 →
客户端把预测撤回（物品闪一下回原位，无错误提示）。库随后把那一格重发一次，让伪造内容不被这次回滚冲掉。

### 注销监听

`bool removeClickListener(uint64_t token)`

- 参数：
  - `token` : `uint64_t`<br>`addClickListener` 返回的 token
- 返回值：注销成功 `true`；token 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    fakes.removeClickListener(token);
    ```

## 边界（用之前先看清楚）

- 伪造只影响客户端显示；玩家「真正能用 / 能吃」的仍是服务端真实物品。
- 若某格真实物品与伪造物品**恰好一致**，那一次操作会被服务端当真执行 —— 想让某格纯展示，
  别把它伪造成与真实物品相同的东西。
- 该格真实物品因为别的原因改变（捡东西 / 用物品 / 别的插件改背包）时，周期重发会把伪造内容盖回去。
- 护甲（`ArmorContainer` 6）与副手（`OffhandContainer` 34）不在本域范围内，它们各有独立的容器枚举。
- 玩家下线时伪造记录会被清掉，重连后看到的是真实背包，不需要手动撤销。

## 版本注记

| API · 插件版本 | 变化 |
|---|---|
| 1.23.0 · 26.40.5 | `IFakeInventory` 首次发布：整份下发（`InventoryContent`）+ 单格 `InventorySlot` + 周期重发 + 点击回传槽位号 |

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <string>

namespace {

uint64_t g_fakeInvClickToken = 0;

} // namespace

void show_virtual_backpack(std::string const& playerName) {
    auto& fakes = hologramlib::IHologramLib::getInstance().fakeInventories();

    // 1) 整份伪造：快捷栏 0 号一把剑、1 号 64 个钻石，其余留空
    hologramlib::FakeInventorySpec spec;
    spec.items.resize(hologramlib::kFakeInventorySlots);   // 36 格
    spec.items[0].type  = "minecraft:diamond_sword";
    spec.items[0].name  = "§c虚容之剑";
    spec.items[0].lore  = {"§7攻击力 +7"};
    spec.items[1].type  = "minecraft:diamond";
    spec.items[1].count = 64;
    spec.items[1].name  = "§b虚容钻石";
    spec.refreshIntervalTicks = 20;                        // 每 1s 重发，压住 BDS 的背包刷新
    if (!fakes.apply(playerName, spec)) {
        return;                                            // 玩家不在线
    }

    // 2) 回调只注册一次（比如在 onEnable 里）
    if (g_fakeInvClickToken == 0) {
        g_fakeInvClickToken = fakes.addClickListener(
            [](hologramlib::FakeInventoryClickEvent const& ev) {
                // 点到伪造且非空的格子才会进来：ev.playerName / ev.slot
            });
    }
}

void hide_virtual_backpack(std::string const& playerName) {
    auto& fakes = hologramlib::IHologramLib::getInstance().fakeInventories();
    fakes.clear(playerName);   // 同时把真实背包重发一遍
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
