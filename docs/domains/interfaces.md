# 能力域指南 · 界面与容器

覆盖 4 个域：**村民交易菜单**、**虚拟容器**、**背包虚容器**、**NPC 对话框**。

这四个域都在做同一件反直觉的事：**在服务端并不存在这个界面的情况下，让客户端打开一个真实的原版界面**。理解这一点，后面所有的「为什么会撤回」「为什么要点两次」都有答案。

| 域 | C++ | LSE | 一句话 |
|---|---|---|---|
| 村民交易菜单 | `ITradeMenu` | `trade*` · 10 | 协议层 `UpdateTrade` 开交易界面 |
| 虚拟容器 | `IContainerMenu` | `container*` · 10 | 客户端侧箱子方块 + 方块实体 NBT，27 / 54 格 |
| 背包虚容器 | `IFakeInventory` | `fakeInv*` · 9 | 改写客户端看到的玩家背包内容 |
| NPC 对话框 | `INpcDialogue` | `npcDialog*` · 9 | `NpcDialoguePacket` + 合成 NPC 载体 |

---

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

## 村民交易菜单

协议层自建 `UpdateTrade` 开界面：包级字段与 Offers NBT 都与 BDS 抓包**逐字节对拍**。隐身载体实体驱动经验条。

### 三条路径

```cpp
hologramlib::TradeMenuSpec trade;
trade.tradeType = "entity.villager.butcher";
// ...
auto tradeId = lib.tradeMenus().open("Steve", trade);
```

| `usePacketOffers` | `settleLocally` | 行为 |
|---|---|---|
| `true`（默认） | `false`（默认） | **纯展示** —— 只发自建 `UpdateTrade`，服务端不放交易表，付费放不进槽、点了不成交（天然只读） |
| `true` | `true` | **纯协议真结算** —— 库接住客户端的付费放置 / 取回 / 成交请求，真的扣背包、真的发产物（见下方限制） |
| `false` | 无效 | 给载体装**真实交易表**并走 BDS `openTrading`，**成交由 BDS 执行**（物品真的消耗；**触屏也能走完整流程**） |

> **要稳定成交，用 `usePacketOffers = false` 或容器 UI。** 理由见下。

两条路径下 `UpdateTrade` 都会在**几 tick 后**发送：载体实体是异步送达客户端的，背靠背发时客户端还不认识那个实体，界面绑不上去。

### 纯协议真结算（`settleLocally = true`）

界面仍是自建 `UpdateTrade` 开的（服务端**不放**交易表），但库把客户端的付费放置 / 取回 / 成交请求接住并自己落地：真的从玩家背包扣付费、真的把产物写进背包，**关界面时把暂存但没花掉的付费退还**。

判定逻辑独立在 `src/trade/TradeSettlementLogic.h`（不碰 BDS 类型，离线自检 `tests/check-trade-settlement.bat`，43 项），动背包那层在 `TradeMenuManager`。
结果通过 `TradeSettlementEvent` 回传（C++ 监听器 / LSE `tradePollSettlements`）。

> ### ⚠ 已知限制（实测，未打通）
> 客户端要求服务端在物品应答里回带交易槽的**槽位更正**（按客户端分配的物品网络 id）。该段在**本协议版本与协议库 `sculk` 的线格式不一致**（协议库多写一个字符串字段、`DurabilityCorrection` 按 varint 写，而客户端要定长 `short`）—— 照着协议库手写会**直接把客户端打崩**（已踩过）。
>
> 所以：放料虽然被服务端受理（扣款、暂存、关界面退还是真的），客户端界面仍可能把这次放料撤回（表现为「放进交易槽又弹回背包」、成交按钮点不动）。
> **要稳定成交请用 `usePacketOffers = false`（BDS 自己结算）或容器 UI。**

其他实测边界：

- **触屏到不了成交** —— 纯协议层下付费拖不进交易槽（客户端反复弹回）。触屏请用真实交易表路径或容器 UI；键鼠 / 手柄正常。
- 客户端**买不起就不发包**（没有付费物品时客户端一个请求都不发）—— 这是客户端的判定，不是库拦的。
- 账本按「**类型 + 数量**」扣，**不按交易槽位细分**；同一付费类型的多条交易共用料槽时，扣的是先放进去的那些。
- 包里带的 `numCrafts`（一次点几下）**不展开**，一次成交发一份产物。

### 显示栏值用 1 基

`TradeMenuOffer` 的显示栏值用 **1 基**（`1` = 新手 … `5` = 大师）；wire 上是 `0..4`，由库换算并夹紧。`locked = true` 可强制某条显示为未解锁。

经验条由载体实体的 `TradeTier` / `MaxTradeTier` / `TradeExperience` 元数据驱动 —— 三项都是 `Int`，与真实村民生成包（`minecraft:villager_v2`）实测一致，其中 **`MaxTradeTier` 恒为 4**（大师）。

`TradeMenuSpec::carrierIdentifier` 可选村民或流浪商人。

### 动态刷内容

```cpp
lib.tradeMenus().addOffer(tradeId, anotherOffer);   // 追加一条
lib.tradeMenus().setTier(tradeId, 2, 40);           // 改档位 / 经验条
```

两者都**就地重发交易表** —— 复用同一个界面与载体，不重开、不等延迟。

> **本域没有点击回调。** `26.40.3` 里短暂存在过的三套点击监听（`addClickListener` / `addActionListener` / `addRawActionListener` 与 `TradeClickEvent`、`TradeRawAction`）已在 `26.40.4` **整体删除**（发布后按需求撤回）。要「点条目就有回调」请用**虚拟容器**。

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

## NPC 对话框

```cpp
hologramlib::NpcDialogSpec dlg;
dlg.npcName = "§e村长";
dlg.dialogue = "要来点任务吗？";
dlg.buttons.push_back({"§a接受", {}, "main#0", 0});
auto dialogId = lib.npcDialogs().open("Steve", dlg);
```

下发 `NpcDialoguePacket`（场景名 + 正文 + NPC 名 + 按钮 JSON）并合成一个 `minecraft:npc` **载体实体**。
客户端点按钮 / 关闭时回传（`NpcRequestPacket`），回调给出 `sceneName` + `buttonIndex` + `actionId` + `commands` + `closed`。
**多层级对话就是「点击回传带回 `sceneName`，调用方据此发下一层」**。

### 三个实测要点

**1. 载体必须放在世界下方（`y = -66`）**
客户端看不到实体，但对话框里的头像**照常渲染**。换成「隐身标志位」会让头像**一起消失**（实测）。

**2. `update()` 换不了页 —— 点按钮后必须用 `open()`**

> **点任意按钮时客户端就会自行收起界面**，而 `INpcDialogue::update()` 只重发一次 `NpcDialoguePacket(Open)` —— 客户端**不会**因此重新弹出。
> 所以「点按钮就地换页」对 NPC 对话**不可行**：要让客户端重新显示，必须走 `open()`（删旧载体 + 建新载体 + 再发一次 Open）。
> `update()` 只适合**界面仍开着时**的内容微调。

**3. 合成 NPC 不代跑命令**
服务端侧没有 BDS 实体替它执行：`NpcDialogButton::commands` 只是**展示在按钮上**。要执行得由调用方自己跑（例如 `player.runCommand`）。

### 头像自定义（`26.40.8`）

`NpcDialogSpec` 尾部追加两个参数：

| 参数 | 作用 |
|---|---|
| `avatarSkinVariant` | NPC **内置皮肤变体** `0..59` → 写进载体 ActorData 的 `SkinId(104)`；`-1` = 不加该项（与旧行为**逐字节一致**） |
| `avatarViewSpec` | 在这只载体上**叠一层 `IViewOverride`**，spec 语法同 `viewEntity`：`type=` 换载体类型（测试矩阵：僵尸 / 鸡）、`name=` 换界面标题 |

LSE 侧 `npcDialogSetAvatar(id, skinVariant, viewSpec)` 就地更新 + 重开（与翻页同一条 0 闪烁路径）。

> **`avatarViewSpec` 的 `skin=` 分支已移除**（`26.40.9` 批次内的收缩，明记）。
> 它会把载体换成「玩家模型」（`PlayerList(Add)` + `AddPlayer`），实测客户端在该载体上点进菜单会**关掉界面**（同一次测试里 `type=` 载体：僵尸 / 鸡一切正常）。
> 按「库只做机制」的定位，「让客户端认它是玩家」的路线留给消费方自建：用 `playerNpcs()` 造真·假玩家 + `npcUniqueIdOverride` 把对话挂到它 id 上。
> 也可以走 `EntityView::skinId` 的**同 id 换模型**路线。
> **`avatarViewSpec` / `avatarSkinVariant` 字段本身不动 —— ABI 不变，消费方无需重编译。**

### 脚本侧

```js
// 按钮串 = label|actionId|mode|命令1,命令2（按钮之间用 ; 分隔）
npcDialogOpen("Steve", "§e村长", "main", "要来点任务吗？", "§a接受|main#0|0|;;§c离开|main#1|1|", "");

// 点击走轮询队列：每条是 "player=X dialogId=N scene=S button=I actionId=A closed=0|1 commands=..."
for (const line of npcDialogPollClicks()) { /* 自行切分 */ }
```

---

## 相关

- 完整签名：[C++ 接口参考](../reference/cpp-api.md)（§1.10 交易菜单 / §1.11 NPC 对话 / §1.12 虚拟容器 / §1.14 背包虚容器）
- LSE 函数清单：[界面与容器](../lse/interfaces.md) · [背包与硫磺立方体](../lse/inventory.md)
- 想知道这些包为什么这么写：[协议层实现笔记](https://github.com/Ieckseyr/HologramLib/blob/main/docs/architecture/protocol-notes.md)
- 按玩家设备分流界面（触屏 → 容器，键鼠 → 交易）：[感知域](view-sensing.md#感知域)
