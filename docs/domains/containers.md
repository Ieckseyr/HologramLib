# 容器与背包

虚拟容器（客户端箱子 UI）与背包虚容器；动态刷新只发给 UI 开着的玩家。


## 所有虚拟界面的共同边界

**服务端没有这个容器 / 这个交易表。** 所以：

| 玩家动作 | 会发生什么 |
|---|---|
| 点条目 / 点物品 | 库收到回调并回传槽位号；**物品不会被真的拿走** |
| 把物品放进虚拟槽 | 服务端真实槽位对不上 → BDS 判失败 → **客户端把预测撤回**（物品弹回原位） |
| 界面关闭 | 库回调 `closed = true`（此时 `slot = -1`） |

**库不代答失败应答。** 命中的动作只回传、不拦：放行后 BDS 自己会失败并让客户端撤回，回调照常收到。
若自己代答一条失败应答（`ItemStackNetResult` 3），客户端会**弹一个错误提示**；交给 BDS 走它自己的失败路径反而是**安静的**。

---

## 虚拟容器（列表界面）

**如果你只想要一个「点了有回调的列表界面」，这就是那个域。** 它的点击 = 一次物品拾取，**任何输入设备都会发包**（触屏也能点），这是它相对于交易菜单的核心优势。

```cpp
hologramlib::ContainerMenuSpec menu;
menu.title = "§8任务列表";
menu.rows  = 3;                    // 3 = 小容器 27 格；6 = 大容器 54 格
menu.items.resize(27);
menu.items[0].type = "minecraft:diamond_sword";
menu.items[0].name = "§b试炼";
menu.items[0].lore = {"§7点击领取"};

auto menuId = lib.containerMenus().open("Steve", menu);
lib.containerMenus().addClickListener([](hologramlib::ContainerClickEvent const& e) {
    if (e.closed) return;          // 界面关闭（slot = -1）
    // e.slot = 被点击的条目下标
});
```

### 实现方式

逐条复刻参考实现 **GMLIB 的 `ChestUI`**：

| 步骤 | 包 |
|---|---|
| 客户端侧摆箱子方块 | `UpdateBlock` |
| 写入方块实体数据 | `BlockActorData` —— 条目走 `Items` 列表，标题走 `CustomName` |
| 开界面 | `ContainerOpen`（类型 `Container(0)`、绑方块坐标、目标实体 `-1`） |

`rows = 3` → 单箱子 **27 格**；`rows = 6` → 大箱子 **54 格**（带 `pairx` / `pairz` / `pairlead` 配对，两半各自 `Slot` 0..26）。
点击回传的**槽位号 = 条目下标**。

**回传的 `menuId` 必须等于 `open()` 的返回值。** 调用方普遍按它路由（一个玩家同时只有一个菜单，但
一个插件可能同时开多个），所以这条是契约：26.51.6 曾漏回填 `Menu::menuId`，表现为所有点击回传
`menuId=0`，按 id 匹配的调用方全部失效（`fix(container): 回填 Menu::menuId` 已修）。回归用例见
`plugins/HoloBoxUITest`（`C9 点击回传的 menuId 与打开时一致`）。

> **方块只在客户端。** 服务端世界与存档里都没有这个箱子方块。

### 打开延迟：7 tick 的来历（实测）

打开一次容器的时序是「客户端侧摆方块 + 方块实体」→ 等 N tick → 发 `ContainerOpen`。那个等待**不是移植时写错的**：

| 实现 | 时序 | 打开耗时 |
|---|---|---|
| GMLIB `ChestUI::sendTo` | `updateBlock` + `updateBlockActor` → **`ticks(10)`** → `ContainerOpen` → **`ticks(4)`** → 逐格补玩家物品栏 / 光标 | **14 tick ≈ 700ms** |
| 本库 `ContainerMenuManager` | 摆方块 + 方块实体 → **`ticks(openDelayTicks)`（默认 7）** → `ContainerOpen` | **7 tick ≈ 350ms** |

本库比参考实现少 7 tick：条目放在**方块实体 NBT** 里，省掉了 GMLIB 那一步逐格补格（4 tick）；剩下的等待本身也压到实测下限（3 tick）。

**那段等待是方案固有的**：客户端要先把这个方块与它的方块实体应用上去，`ContainerOpen` 才绑得住这个位置（背靠背发实测打不开界面）。

> **实测（26.40 本机客户端）：`openDelayTicks = 7` 能正常开界面，`6` 打不开。**
> 下限与客户端 / 机器 / 负载有关，换环境可能要回调大，所以保留为可调项（`ContainerMenuSpec::openDelayTicks`，默认 `7`）。

### 三个可调处（含一个「不要用 open 翻页」的坑）

| 你想做 | 用这个 | 代价 |
|---|---|---|
| 翻页 / 换整页内容 | `update(menuId, spec)` | **无延迟** —— 复用同一个载体方块，不拆界面、不重摆方块、不等待，只重发方块实体 NBT + `ContainerOpen` |
| 改一格（打勾 / 数量 / 价格） | `setItem(menuId, slot, item)` | **无延迟无闪烁** —— 只发一条 `InventorySlotPacket`，客户端就地换掉那一格；`item.type` 为空即清空该槽 |
| 只改标题 | `setTitle(menuId, title)` | 内部就是「改完 spec 再 `update`」 |

> ⚠ **翻页不要走 `open()`。** `open()` 会先关旧菜单、恢复真方块，**每次都要重付一次打开延迟**（≈350ms）。翻页属于同一界面的内容变更，用 `update()`。

### 可交互模式（默认关）

```cpp
menu.interactive = true;   // 或 containerSetInteractive(menuId, true)
```

打开后客户端在容器**内部**拖动 / 交换 / 拆分物品时，库会**接住这条请求并回成功**（自己回 `ItemStackResponse` Success），同时把改动记进条目表 —— 物品真的留在新格子上，不会再被 BDS 打回。

默认关时的行为就是「点击只回传、物品弹回原位」。

**只接「整条请求都落在本容器」的动作**；涉及玩家背包的动作照旧放行给 BDS —— 服务端没有这个容器，那是真实背包的固有边界（真要连背包一起改，配合 `IFakeInventory` 那条路）。

### 物品请求有两条包通道（实测，26.40）

客户端的 `ItemStackRequest` **不是只有一种送法**：

| 通道 | 形态 | 谁在用 |
|---|---|---|
| `ItemStackRequestPacket`(147) | 独立请求，动作为 cereal 形态 | 容器槽位的取放（实测**主通道**） |
| `PlayerAuthInputPacket`(144, AuthInput) | **内嵌** `mItemStackRequest`（输入标志 `PerformItemStackRequest = 36`） | 菜单界面的交互（实测：**触屏时 147 可能一条都没有**，只有这条） |

库**两条都挂**（`src/interaction/PlayerInteractionHooks.cpp`）：AuthInput 那条用 BDS 自己的 `ItemStackRequestCereal::toActionData()` 把解析态动作转成与 147 相同的 cereal 形态，再喂进同一个派发函数。

> **AuthInput 只观察、不拦** —— 它同时承载玩家移动，拦下会把移动一起吞掉；而内嵌请求由 BDS 自己处理。
>
> 这也是为什么「虚拟容器的点击」在触屏场景下曾经整个漏掉：`26.40.3` 之前两个钩子只在**交易**菜单打开时才进入。

### 点击怎么取

```cpp
lib.containerMenus().addClickListener(cb);   // C++：回调
```

```js
// LSE：轮询队列（取走并清空），每条是 "player=X menuId=N slot=S closed=0|1"
for (const line of containerPollClicks()) { /* 自行切分 */ }
```

队列与 C++ 监听器收到的是**同一份事件**。

> `useMinecart = true` 保留的是旧路径（只发一只合成箱子矿车）。**实测客户端不弹界面**，仅供对比排查 —— 别用。

---

## 背包虚容器

让客户端「以为自己背包里有这些物品」。

```cpp
hologramlib::FakeInventorySpec bag;
bag.items.resize(hologramlib::kFakeInventorySlots);   // 36 格：0..8 快捷栏，9..35 主背包
bag.items[0].type = "minecraft:diamond_sword";
bag.refreshIntervalTicks = 20;                        // 周期重发（默认 20 tick = 1s）
lib.fakeInventories().apply("Steve", bag);
lib.fakeInventories().clear("Steve");                 // 撤销并恢复真实背包
```

一条 `InventoryContentPacket(49)`（`ContainerId = Inventory(0)`、`FullContainerId = InventoryContainer(29)`、0..35 号描述符）把整份内容换掉；单格改动走 `InventorySlotPacket(50)`。**服务端背包一个字都不动。**

方案取自参考实现 GMLIB 的 `ChestUI`（它填玩家物品栏时逐格写 `InventorySlot`，容器 id 同样是 `Inventory(0)`）。

### 血的教训：物品序列化必须交给 BDS

> 物品描述符里的 User Data 用的是**另一套 NBT 方言**（名字长度 u16 小端 / 整数与计数 4 字节小端），而 sculk 的 `CompoundTag` 写的是 varint 那套（`UpdateTrade` 的 Offers 与方块实体 NBT 才是那套，两者都逐字节对拍过）。
>
> **第一版手写描述符因此写错了方言 → 客户端解析越界直接卡死**，并且会话没随掉线清理，重连时 tick 驱动又重发一遍 → 表现为「**进不去服务器**」。

现在本域的包体用 **BDS 自己的** `InventoryContentPacket` / `InventorySlotPacket` 构造（`src/fakeinv/FakeInventoryPackets.h`），容器名与 storage item 的取值也以抓包为准（BDS 写的是容器名字节 `0`、storage 空描述符）。
其余域（交易菜单 / 虚拟容器 / NPC 对话）仍然全部手写包体。

配套：注册 `PlayerDisconnectEvent` 清理会话（`FakeInventoryManager::onPlayerLeave`），周期重发间隔钳了下限（最快 5 tick）。

### 与交易菜单 / 虚拟容器共存

打开交易界面或箱子界面时 **BDS 会重发背包内容**（伪造内容被覆盖），所以本域默认每 `refreshIntervalTicks` 重发一次，也可以用 `refresh()` 手动补 —— 界面上看到的背包区域因此始终是伪造内容，点击照常回调。

### 边界（先写清楚，免得误用）

- 伪造**只影响客户端显示**；玩家真正能用 / 能吃的是服务端真实物品。
- 某格**真实物品与伪造物品恰好一致**时，那一次操作会被服务端**当真执行** —— 想让某格纯展示，**别把它伪造成与真实物品相同的东西**。
- 该格真实物品因别的原因改变（捡东西 / 用物品 / 别的插件改背包）时，周期重发会把伪造内容**盖回去**。
- **护甲（6）/ 副手（34）不在本域范围内**，它们各有独立的容器枚举。

### 钩子怎么分流

物品请求钩子由容器域与本域**共用**（`src/interaction/PlayerInteractionHooks.cpp`），靠容器 id 区分、互不抢：

| 域 | 认的容器 id |
|---|---|
| 容器域 | `LevelEntityContainer(7)` 与动态 id `101..199` |
| 背包域 | `CombinedHotbarAndInventory(12)` / `Hotbar(28)` / `Inventory(29)` |

---
## 新 API / 行为（1.31.0）

- **只向 UI 开着的玩家刷新**：`Menu.uiOpen` 为 false 时，`update()` 只更新服务端 spec，`setItem()` 直接丢弃。
- **会话隔离**：`ownerEpoch` 在玩家退出/重进后 +1，旧的延迟 `ContainerOpen` / `BlockActorData` / `InventorySlot` 一律作废。
- **容器 id 防复用**：关闭后 200 tick 内不复用同一个 `containerId`，避免客户端缓存撞旧界面。
- `update()` 对已打开界面不再重发 `ContainerOpen`，改为逐槽 `InventorySlot`。
- 诊断开关：`HLIB_NO_CONTAINER=1` 完全停发容器/背包包。
