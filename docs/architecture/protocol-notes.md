# 协议层实现笔记

这一页记的是**跨域共用的协议层事实**：发包原语、线格式陷阱、tick 语义，以及把它们钉死的离线检查。

> 排一个「包发不出去 / 客户端把更新丢掉」的问题时，先看这里的 **线格式陷阱** 与 **tick 语义** 两节。

---

## 发送原语：SculkPacketSend

除 ItemDisplay 有一份独立副本（已知技术债）外，全库共用一条发送路径：

```
sculk 序列化构造包体
      ▼
BDS createPacket 同型 → 回读校验（把刚构造的字节解回来比对）
      ▼
手动补 VarUInt 包头
      ▼
NetworkPeer::sendPacket 原始字节发送
```

**为什么要回读校验**：协议库的写路径与 BDS 的实际线格式可能不一致（下面「线格式陷阱」里就有两个真实案例）。回读校验让「写错了」在**发送前**变成一次失败，而不是变成一个客户端崩溃。

**失败如何可见**：`sculkSendFailureCount()` 是**回读校验失败的全库共享计数** —— 静默失败的唯一入口（随 `describeFor` 输出）。库本身不打印任何日志，所以这个计数是排障时的第一站。

---

## 线格式陷阱

这几条都踩过、都有抓包或崩溃作为证据。**改任何手写包体之前先读这一节。**

### 1. 存在两套 NBT 方言，不能混用

| 方言 | 用在哪 | 格式 |
|---|---|---|
| **BDS 物品 User Data 方言** | 物品描述符里的 User Data | 名字长度 **u16 小端**、整数与计数 **4 字节小端** |
| **sculk `CompoundTag` 方言** | `UpdateTrade` 的 Offers、方块实体 NBT / NpcData | **varint** 那套 |

> **血的教训**：背包虚容器第一版手写物品描述符时用了 sculk 那套方言 → **客户端解析越界直接卡死**；又因为会话没随掉线清理，重连时 tick 驱动重发一遍 → 表现为「**进不去服务器**」。
>
> 结论：**物品交给 BDS 自己的 `InventoryContentPacket` / `InventorySlotPacket` 构造**（`src/fakeinv/FakeInventoryPackets.h`）；容器名与 storage item 取值也以抓包为准（BDS 写的是容器名字节 `0`、storage 空描述符）。
> 其余域（交易菜单 / 虚拟容器 / NPC 对话）仍然全部手写包体 —— 它们那两段确实是 sculk 方言，且都逐字节对拍过。

### 2. 颜色是 **ARGB**，不是旧字节序

int32 颜色在 **v2168 是 ARGB** 打包。库早年沿用 v944 的字节序 → **R/B 通道颠倒**（填红显蓝），形状 / 文本颜色 / 背景框全部受影响。

打包 `toPacked` 与解包 `getColor` 已统一修正（`26.40.8`）。**若你此前为了凑颜色反填过值，请改回字面值。**

### 3. `SetActorDataPacket` / `UpdateAttributesPacket` 都带 `mTick`

26.40 线格式里这两个包**都带 `mTick`**，且**填 0 会被客户端当过期包丢掉** —— 出生包里元数据都对、画面却毫无变化。

自建这两个包时必须填 `currentViewTick()`。对比：customentity 域一直带着 tick 发（`sadp.mTick = tick`），所以那边一直是好的。

### 4. `NametagAlwaysShow` 的协议值**是反的**

**`0` = 常显**。别按直觉填 `1`。

### 5. 玩家在协议层上报的 y 在**眼睛**处

这是 Bedrock 特有的：`PlayerAuthInput` 里的 y 是眼位，**不是脚位**。
需要真脚位时用碰撞箱下沿取，或做「眼到脚」差值换算（站 / 潜行 / 游泳各不相同）。

### 6. 掉落物的出生包走 `AddItemActor`，不是 `AddActor`

`AddItemActor` = id **15**。只拦 `AddActor` 的话，「把掉落物换类型 / 隐藏」永远只做一半 ——
设覆盖那一刻能把当时看得到的换掉，但引擎随后自己发的 `AddItemActor` 会把物品图标**又塞回客户端**（抓包实证：`RemoveActor` + `AddActor(minecraft:cod)` 之后 **13ms** 就来一条同 `uniqueId` 的 `AddItemActor`）。

> 顺带一条配方细节：自建掉落物出生包时**不要**照抄 `ItemActor` 的 `maxHealth = 0`；用 `health 20 / scale 1` 的已验证配方。

### 7. 客户端的要求未必等于协议库的写法

交易菜单的「交易槽槽位更正」段：**协议库 `sculk` 的线格式与客户端要求不一致**（协议库多写一个字符串字段、`DurabilityCorrection` 按 varint 写，而客户端要定长 `short`）—— **照着协议库手写会直接把客户端打崩**（已踩过）。

这条不是「写错了」，而是「库和客户端对同一段的看法不同」。遇到这种情况**不要硬凑**，换一条能走通的路径（交易菜单就是这么做的：改用真实交易表或容器 UI）。

---

## 网线格式的几个「必须这样写」

| 位置 | 必须 | 否则 |
|---|---|---|
| NPC 对话框载体位置 | **`y = -66`**（世界下方） | 客户端能看到实体；换「隐身标志位」会让对话框头像**一起消失** |
| `PlayerList` 包体前缀 | 发送前校验：Add `01 01 00`、Remove `01 00 01` | 前缀异常时库**丢弃该包并记下实际前缀**（不静默） |
| 掉落物出生包属性 | `health 20 / scale 1` | 见陷阱 6 |
| 自建出生包 | flags 元数据项 + `minecraft:health` / `minecraft:scale` 属性，**且紧跟一条 `UpdateAttributesPacket`** | 客户端对出生包里的属性**只初始化不生效**；进服时心跳重发会让客户端报错 |
| `EntityView` 的 pose / color 类型 | `ColorIndex(3)` / `Color2Index(82)` = **Byte**；`Variant(2)` / `MarkVariant(43)` = **Int** | 类型取自**解码引擎自己的真实出生包**（一只真羊），不是照协议表猜的 |

---

## 物品请求有两条包通道

客户端的 `ItemStackRequest` 不是只有一种送法：

| 通道 | 形态 | 谁在用 |
|---|---|---|
| `ItemStackRequestPacket`(147) | 独立请求，动作为 cereal 形态 | 容器槽位的取放（实测**主通道**） |
| `PlayerAuthInputPacket`(144, AuthInput) | **内嵌** `mItemStackRequest`（输入标志 `PerformItemStackRequest = 36`） | 菜单界面交互（实测：**触屏时 147 可能一条都没有**） |

库**两条都挂**：AuthInput 那条用 BDS 自己的 `ItemStackRequestCereal::toActionData()` 把解析态动作转成与 147 相同的 cereal 形态，再喂进同一个派发函数。

> **AuthInput 只观察、不拦** —— 它同时承载玩家移动，拦下会把移动一起吞掉。

---

## 容器界面的时序下限

打开容器 = 「客户端侧摆方块 + 方块实体」→ 等 N tick → 发 `ContainerOpen`。那个等待是**方案固有**的（背靠背发实测打不开界面，客户端还没把这个方块应用上去）。

**实测（26.40 本机客户端）：7 tick 能开、6 打不开。** 库默认取 7（≈350ms），是参考实现 GMLIB 等效 14 tick 的一半。详见 [界面与容器 · 打开延迟](../domains/interfaces.md#打开延迟7-tick-的来历实测)。

---

## 离线回归检查

`tests/` 下的脚本把上面这些格式**钉死在字节级**，不需要启动服务端。它们**不随库发布**（在 `.gitignore` 里），部分需要本机 `logs/fullpkts/` 的真实抓包。

| 脚本 | 验什么 | 改动后必须重跑 |
|---|---|---|
| `check-no-diagnostics.bat` | **发布不变量**：开关极性（关时 marker 不进目标文件、`-DHOLOGLIB_DIAG_LOG=1` 时必须进）+ 已构建 DLL 里不得出现库内日志前缀 | `src/DiagLog.h` 或任何日志调用 |
| `check-trade-packet.bat` | 整个 `UpdateTrade` 包（9 个包级字段 + Offers NBT）与 BDS 抓包**逐字节对拍** | `trade/` 的包构造 |
| `check-trade-offers.bat` | 只对 Offers NBT 段（更快的单项检查） | 同上 |
| `check-trade-settlement.bat` | 结算判定逻辑（**43 项**，纯逻辑，不需要抓包） | `trade/TradeSettlementLogic.h` |
| `check-container-packets.bat` | 虚拟容器四个包：`ContainerOpen` 与真实抓包**整包逐字节对拍**；`UpdateBlock` / `BlockActorData` / `ContainerClose` 解回字段核对（含单箱 27 格与大箱 54 格配对） | `container/ContainerPackets.h` |
| `check-fake-inventory.bat` | 背包虚容器线格式（**10 项**）：帧落在包尾、容器 id 0 / 36 格 / 容器名字节 0 / storage 空描述符，以及物品 UserData 的**方言指纹**（`06 00 "Damage"` 而不是 `06 "Damage"`） | `fakeinv/FakeInventoryPackets.h` |
| `check-npc-carrier.bat` | 合成 NPC 载体包：AddActor 字段顺序与尾部完全消费、位置 `y=-66`、ActorData 五项标记的 id 与类型，并交叉核对真实 BDS NPC 生成包 | `npcdialog/NpcCarrierPacket.h` |
| `check-npc-playerlist.ps1` | `PlayerList` 2168 字段、可信标记位置、`Id` / `FullId` 与包体完全消费 | 皮肤 / PlayerList 发送路径 |
| `check-view-override.bat` | 视图覆盖判定逻辑（**59 项**，纯逻辑） | `view/ViewOverrideLogic.h` |
| `check-authinput.bat` | AuthInput 解码 | AuthInput 解析 |

**两条硬规矩**：

1. **真实抓包样本不随库发布。** 没有样本时脚本打印 `no captured ... found` 并以非零码退出 —— 要么把自己的抓包路径作为参数传进去，要么只跑不需要样本的检查（`check-no-diagnostics.bat` 与两个纯逻辑检查就不需要）。
2. **抓包样本不要改成「取最新」。** 例如交易菜单的检查默认取 `logs/fullpkts/pkt80_194951_007.bin`（fixture 的推导样本，确定性）；之后的抓包可能是插件自己生成的交易（配方不同），换成「最新」会让对拍失去意义。

检查的中间产物落在 `work/`（fixture 编译产物 + 生成的包字节），由脚本重建 —— 也在 `.gitignore` 里。

---

## 协议升级时要重扫什么

升级 BDS / 协议版本时，这三处的 delta 是主要工作量来源，建议定期对扫：

| 来源 | 找什么 |
|---|---|
| `.cache/schema-*/` | 字段增删（如 `lineGapHeight` 这类「以为有其实没有」的字段） |
| `bedrock_server_mod.exe` 字符串扫描 | 引擎真实使用的字段名与枚举 |
| `plugins/BedrockProtocol-main/sculk/protocol/codec` | 协议库的读写实现 |

三者一致才算真值 —— 库内多次结论（文本载荷没有行级字段、`SetActorData` 带 `mTick`、掉落物走 `AddItemActor`）都是这样三级核对出来的。

---

## 相关

- 分层与重发策略：[架构总览](overview.md)
- 各域的具体用法与坑：[能力域指南](../domains/README.md)
- 判定逻辑为什么能离线测：[README · 设计说明](../../README.md#设计说明)
