# 1.12 IContainerMenu · 虚拟容器

在玩家客户端摆一个箱子界面并逐格写入条目，做「只展示 + 点击回调」的列表；服务端并没有这个容器。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().containerMenus()`
- 槽位：小箱子 `0..26`、大箱子 `0..53`；`ContainerMenuSpec::items` 的下标就是槽位号
- 行数：`rows = 3` → 27 格；`rows >= 6` → 54 格；其它值一律按小箱子处理
- 容器 id：库内分配 `101..199`，避开 BDS 真实容器的 `1..100`

!!! info "服务端根本没有这个容器"

    库只做三件事：用 `UpdateBlock` 在玩家头上摆一个**只发给该玩家**的箱子方块；
    用 `BlockActorData` 写方块实体 NBT（`CustomName` 标题 + `Items` 条目，条目走这份 NBT，不是逐格 `InventorySlot`）；
    等 `openDelayTicks` 之后再发 `ContainerOpen`，把界面绑到这个坐标。

    所以物品只是「摆在那里」：玩家拿走 / 移动都不会真的改变任何东西（天然只读），
    命中容器的动作**只回传、不拦** —— 服务端判失败后客户端自己把预测撤回（物品闪一下回原位，无错误提示）。

!!! tip "刷新一个格子用 setItem，换一整页用 update"

    `setItem` 只发一条 `InventorySlotPacket`（真实箱子同步内容用的同一种包），客户端就地换格，无延迟、无闪烁。
    `update` 复用同一个载体方块，只重发方块实体 NBT + 逐格条目，不重摆方块、不等待。
    走 `open` 重开每次都要付一次打开延迟：它会先关掉旧菜单、恢复真方块，再重摆、再等 `openDelayTicks`。

## 数据结构

`ContainerMenuItem` —— 一格内容：

| 字段 | 类型 | 说明 |
|---|---|---|
| `type` | `std::string` | 物品 id，如 `minecraft:diamond_sword`；空串 = 这一格留空 |
| `count` | `int` | 数量，`1..127`（物品堆叠上限）；`setItem` 路径会钳到 `1..127`，别指望超上限 |
| `damage` | `int` | 附加数据值（耐久 / 变种），一般 `0` |
| `name` | `std::string` | 自定义名，支持 `§` 颜色码；空串 = 无 |
| `lore` | `std::vector<std::string>` | 描述行，一行一项；空数组 = 无 |

`ContainerMenuSpec` —— 一个界面：

| 字段 | 类型 | 默认 | 说明 |
|---|---|---|---|
| `rows` | `int` | `3` | `3` = 小箱子 27 格；`>= 6` = 大箱子 54 格 |
| `items` | `std::vector<ContainerMenuItem>` | 空 | 下标即槽位；长度决定 `setItem` 能改到的最大槽位 |
| `useMinecart` | `bool` | `false` | `false` = 方块方案（正路）；`true` = 旧矿车载体，客户端不认，仅对比用 |
| `title` | `std::string` | `"虚拟容器"` | 界面标题（方块实体的 `CustomName`），支持 `§` |
| `openDelayTicks` | `int` | `7` | 摆完方块后等多少 tick 再发 `ContainerOpen`；`< 1` 按 `1` |
| `interactive` | `bool` | `false` | 初始是否可交互，等价于开完再调 `setInteractive(id, true)` |

`ContainerClickEvent` —— 回调载荷：

| 字段 | 类型 | 说明 |
|---|---|---|
| `playerName` | `std::string` | 点击的玩家名 |
| `menuId` | `int64_t` | 菜单 id；`-1` = 无 |
| `slot` | `int` | 被点击的槽位（= `items` 下标）；界面关闭事件为 `-1` |
| `closed` | `bool` | `true` = 界面被关闭（此时 `slot = -1`） |

## 生命周期

### 打开一个虚拟容器界面

`int64_t open(std::string const& playerName, ContainerMenuSpec const& spec)`

- 参数：
  - `playerName` : `std::string const&`<br>目标玩家名（精确匹配，必须在线）
  - `spec` : `ContainerMenuSpec const&`<br>界面描述；同一玩家已有菜单会先自动关掉旧的
- 返回值：菜单 id（`>= 0`，后续所有调用都用它）；`-1` = 打开失败（玩家不在线、容器 id 用尽）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& menus = hologramlib::IHologramLib::getInstance().containerMenus();

    hologramlib::ContainerMenuSpec spec;
    spec.rows  = 3;                 // 27 格；填 6 就是 54 格大箱子
    spec.title = "§8任务列表";
    spec.items.resize(27);          // 先把槽位撑满，setItem 才能改到任意一格
    spec.items[0].type = "minecraft:diamond_sword";
    spec.items[0].name = "§b试炼 · 第 1 章";

    int64_t menuId = menus.open("Steve", spec);
    if (menuId < 0) {
        return; // 玩家不在线
    }
    ```

!!! note "打开有延迟，窗口期内改格子要等界面开出来"

    `open` 立刻返回 id，但界面要等 `openDelayTicks`（默认 7 tick ≈ 350ms）才出现：
    客户端得先把箱子方块与方块实体应用上去，`ContainerOpen` 才绑得住那个坐标。
    这段窗口里 `setItem` 只改库内记录、不会单独发包；方块实体 NBT 一旦发出去，
    后面改的格子在界面上不会自己刷新。
    稳妥写法是**先把 `items` 填好再 `open`**；已经 `open` 了就在界面出现之后再刷。
    实测 7 能开、6 开不了 —— 默认值别往下调。

### 就地刷新一个槽位

`bool setItem(int64_t menuId, int slot, ContainerMenuItem const& item)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
  - `slot` : `int`<br>槽位下标，取值 `0 .. items.size() - 1`
  - `item` : `ContainerMenuItem const&`<br>新条目；`type` 留空 = 清空该格
- 返回值：成功 `true`（界面开着时发一条 `InventorySlotPacket`，客户端就地换格）；`false` = 菜单已不在 / 槽位越界 / 玩家离线
- 返回值类型：`bool`
- 示例：

    ```cpp
    hologramlib::ContainerMenuItem item;
    item.type  = "minecraft:emerald";
    item.count = 3;
    item.name  = "§a奖励";
    item.lore  = {"§7今天刚发"};

    menus.setItem(menuId, 1, item);
    menus.setItem(menuId, 2, {});   // 清空 2 号格
    ```

!!! note "setItem 改不到列表外的槽位"

    槽位必须已经存在于 `spec.items` 里：`slot >= items.size()` 直接返回 `false`，
    不会自动扩建。要 54 格就先 `spec.items.resize(54)`，再 `open`。

### 整页重发（换页 / 换标题 / 进子菜单）

`bool update(int64_t menuId, ContainerMenuSpec const& spec)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
  - `spec` : `ContainerMenuSpec const&`<br>新的整页内容；`items` 下标仍是槽位号
- 返回值：成功 `true`；`false` = 菜单已不存在或玩家离线
- 返回值类型：`bool`
- 示例：

    ```cpp
    spec.title = "§8任务列表 · 第 2 页";
    spec.items[0].name = "§b试炼 · 第 2 章";
    menus.update(menuId, spec);
    ```

界面已经开着时，`update` 会重发方块实体 NBT 与逐格条目，客户端重读一遍就换页；
**不重摆方块、不重发 `ContainerOpen`、没有打开延迟**。

!!! note "行数在打开时就定了"

    `update` 不会改尺寸：`rows` 从 3 改成 6 仍然按 27 格走。
    要换大小只能 `close` 之后重新 `open`。

### 切换可交互模式

`bool setInteractive(int64_t menuId, bool on)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
  - `on` : `bool`<br>`true` = 接住容器内的搬运（库内镜像跟着改，物品不会被打回）；`false`（默认）= 只回传点击
- 返回值：成功 `true`；`false` = 菜单不存在
- 返回值类型：`bool`
- 示例：

    ```cpp
    menus.setInteractive(menuId, true);   // 变成「能真的搬东西」的容器
    ```

只有**整条请求的每个动作都落在本容器**（槽位都在 `items` 范围内）时才会被接住，
否则整条放行给服务端去判失败 —— 与默认模式的「只回传」表现一致。

### 播方块事件（假箱子的开合动画）

`bool blockEvent(int64_t menuId, int b0, int b1)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
  - `b0` : `int`<br>`BlockEventPacket` 的 `B0`，原样透传；箱子盖 `1` = 开、`0` = 合
  - `b1` : `int`<br>`BlockEventPacket` 的 `B1`，原样透传，箱子一般填 `0`
- 返回值：成功 `true`；`false` = 菜单不存在或玩家离线
- 返回值类型：`bool`
- 示例：

    ```cpp
    menus.blockEvent(menuId, 1, 0);   // 开盖（纯客户端表现，不改服务端）
    ```

### 关闭一个界面

`bool close(int64_t menuId)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
- 返回值：成功 `true`（发 `ContainerClose`，并把那只假箱子改回真实方块）；`false` = 该 id 不存在
- 返回值类型：`bool`
- 示例：

    ```cpp
    menus.close(menuId);
    ```

### 关闭全部界面

`void closeAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    menus.closeAll();
    ```

## 查询

### 查询界面是否还开着

`bool isOpen(int64_t menuId) const`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
- 返回值：菜单还在 `true`；已关闭或不存在 `false`。打开延迟期间也算「在」
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (menus.isOpen(menuId)) {
        menus.setItem(menuId, 0, item);
    }
    ```

### 列出所有打开的菜单

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：当前所有菜单 id；一个都没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    for (int64_t id : menus.getAllIds()) {
        menus.close(id);
    }
    ```

## 点击与关闭回调

### 注册点击 / 关闭监听

`uint64_t addClickListener(std::function<void(ContainerClickEvent const&)> listener)`

- 参数：
  - `listener` : `std::function<void(ContainerClickEvent const&)>`<br>回调；`closed = true` 表示界面被关闭
- 返回值：监听 token（`>= 1`）；传空函数返回 `0`，表示没有注册
- 返回值类型：`uint64_t`
- 示例：

    ```cpp
    uint64_t token = menus.addClickListener(
        [](hologramlib::ContainerClickEvent const& ev) {
            if (ev.closed) {
                return;                      // 界面被关掉：ev.slot == -1
            }
            // ev.playerName / ev.menuId / ev.slot 就是这次点击
            if (ev.slot == 0) {
                // 第 0 格被点了
            }
        });
    ```

回调是**多播**：可以注册多个，每个都会收到。点击只回传、不拦，服务端那边该失败还是失败。

### 注销监听

`bool removeClickListener(uint64_t token)`

- 参数：
  - `token` : `uint64_t`<br>`addClickListener` 返回的 token
- 返回值：注销成功 `true`；token 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    menus.removeClickListener(token);
    ```

## 版本注记

| API · 插件版本 | 变化 |
|---|---|
| 1.21.0 · 26.40.3 | `IContainerMenu` 首次发布：客户端侧箱子方块 + 方块实体 NBT + 绑坐标的 `ContainerOpen`；小箱子 27 格 / 大箱子 54 格；点击回传槽位号 |
| 1.23.0 · 26.40.5 | 新增可交互模式（`ContainerMenuSpec::interactive` / `setInteractive`）：容器内拖动、交换会被库接住并保留 |
| 1.32.0 · 26.51.6 | 尾部追加 `blockEvent(menuId, b0, b1)`；用旧头文件编译的消费方需要重新编译 |

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <string>

namespace {

uint64_t g_menuClickToken = 0;

void register_menu_clicks() {
    // 回调只注册一次（比如在 onEnable 里）
    g_menuClickToken = hologramlib::IHologramLib::getInstance()
                           .containerMenus()
                           .addClickListener([](hologramlib::ContainerClickEvent const& ev) {
                               if (ev.closed) {
                                   return;   // 界面被关掉：ev.slot == -1
                               }
                               // 这里按 ev.playerName / ev.menuId / ev.slot 处理
                           });
}

void unregister_menu_clicks() {
    if (g_menuClickToken != 0) {
        hologramlib::IHologramLib::getInstance().containerMenus().removeClickListener(g_menuClickToken);
        g_menuClickToken = 0;
    }
}

} // namespace

void open_task_list(std::string const& playerName) {
    auto& menus = hologramlib::IHologramLib::getInstance().containerMenus();

    hologramlib::ContainerMenuSpec spec;
    spec.rows  = 3;                 // 27 格
    spec.title = "§8任务列表";
    spec.items.resize(27);          // 先把槽位撑满

    spec.items[0].type  = "minecraft:diamond_sword";
    spec.items[0].count = 1;
    spec.items[0].name  = "§b试炼 · 第 1 章";
    spec.items[0].lore  = {"§7点击接取", "§8奖励：64 钻石"};

    int64_t menuId = menus.open(playerName, spec);
    if (menuId < 0) {
        return;                     // 玩家不在线
    }

    // 界面开出来之后按槽刷新（这里只是示意，实际请挂在自己的定时器 / 事件里）
    hologramlib::ContainerMenuItem reward;
    reward.type  = "minecraft:diamond";
    reward.count = 64;
    reward.name  = "§b奖励";
    menus.setItem(menuId, 1, reward);
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
