# 版本 / API 版本 / 宏 对照

本库对外有三套编号，消费方按需使用：

| 编号 | 位置 | 用途 |
|------|------|------|
| **插件发布版本** | `manifest.json` 的 `version` | 部署用；与 git tag 同名（如 `26.40.2`），形如 `<BDS 大版本>.<小版本>.<补丁>` |
| **API 版本** | `README.md` 的「API 版本」行 | 人读的能力版本（如 `1.21.0`） |
| **`HOLOGLIB_API_VERSION`** | `include/hologramlib/HologramLib.h` | **代码里唯一的版本依据**：编译期静态断言 + `IHologramLib::version()` 运行期握手 |

`IHologramLib::version()` 返回 `HOLOGLIB_API_VERSION` 本身，所以运行期握手与编译期断言取的是同一个值。

## 编码规律

格式为 `0x01` + **次版本号（中间字节，按十六进制递增）** + 补丁位：

```
1.15.0 -> 0x011500
1.19.0 -> 0x011900
1.20.0 -> 0x011A00   ← 0x19 之后按十六进制进到 0x1A，即 1.20
1.21.0 -> 0x011B00
```

也就是**每个次版本让中间字节 +0x1**（表现为 `0x19 → 0x1A → 0x1B`），不要按十进制去读那两个十六进制字符。

补丁位通常是 `00`，所以 `1.19.1` 与 `1.19.0` 共用 `0x011900` —— **只能门到次版本**。早期版本曾用最后一位区分补丁（`0x010701` = 1.7.1）。

**只在正式发布新版本时才推高本宏。** 在同一条尚未发布的线上继续加能力域不改变它：消费方看到的"我能用的最低版本"没变，抬高宏只会让旧版消费方误判为不兼容。**`0x012100` 随 `26.40.9` 正式发布**（视图覆盖 `EntityView::skinId` 同-id 换模型 + 真实实体交互多播 + ABI 布局戳）; 此前 `0x012000` 随 `26.40.8` 发布（视图覆盖残骸清理 + `IPlayerNpc` 追加五方法 + 悬浮字/text 域收缩与补全 + `view*` 逐字段导出）; `0x011F00` 随 `26.40.7` 发布（客户端视图覆盖 `IViewOverride`）—— 再此前是 `26.40.5` 的 `0x011D00`（背包虚容器 + 硫磺立方体展示）。

> **一次例外（明记）**：`26.40.3` 发布过 1.21.0 的交易菜单点击回调，发布后随即按需求撤回 —— 交易菜单改为**纯展示**，`TradeClickEvent` / `TradeActionCallback` / `TradeRawAction` 与 `ITradeMenu` 的六个监听方法整体移除。这是**收缩而不是新增**，按本文档的约定本该走大版本（`2.0.0`）；这里抬到次版本 `1.22.0` 并在表中写明，是因为那条 API 的公开窗口只有一次发布、且没有消费方采用。若你已按 `1.21.0` 写了交易菜单的点击监听，升到 `26.40.4` 需要删掉那些调用（编译期就会报错，不会是静默的行为变化）。

> **第二次收缩（明记）**：`26.40.8` 把 `IHologramText` 的行级假功能整体移除（8 个函数 + 库内自驱 `holoTick`，详见下表 `26.40.8` 行）。这些函数从 `26.10.4` 早期域起就在导出，但底层从来没有对应的协议能力：协议 2168 的文本载荷是**整块**一份颜色/缩放，没有行间距字段，彩虹模式只会把行涂白；`holoTick` 驱动的动画偏移在库内外都没有消费方。按本库"基础轮子、不留假功能"的定位，留着它们比移除更糟——消费方会以为自己拿到了逐行能力。与 `26.40.3` 同例：收缩不单列大版本，随次版本 `1.26.0`（> 上一发布 `1.25.0`）一起发布并在表中写明。**ABI 处理（与 26.40.3 的关键区别）**：那次删的六个监听方法在接口**尾部**，不动既有槽位；这次的行级方法在 `IHologramText` **中段**，直接删除会把 `setColor` / `setLocation` / `draw` 等全部前移 —— 1.26.0 之前编译、未重编译的消费方二进制会**静默错位**。所以这 8 个方法在 C++ 接口里**保留为废弃空槽**（非纯虚 + 内联空实现 + `[[deprecated]]`，调用无动作 / 返回 false），虚表槽位与 1.25.0 完全一致；重新编译的消费方按旧写法调用得到**编译期弃用警告**（不是静默行为变化）；LSE 侧导出照旧删除。真正清槽留到下一个大版本（`2.0.0`）。

## 完整对照

| 插件发布版本 | API 版本 | `HOLOGLIB_API_VERSION` | 该版本新增的能力域 |
|---|---|---|---|
| `26.40.9` | 1.27.0 | `0x012100` | **`EntityView::skinId`（视图覆盖: 非玩家实体换成"我们的模型"）**—— 拦下的出生包 + 库自己的包替换, **同一个 runtimeId/uniqueId**: 库吃掉该实体的出生包, 发 `PlayerList(Add)`+`AddPlayer`（玩家模型 + 已注册皮肤, 可带 `scale`）。客户端看到我们的模型, 而 **id 没变 —— 服务端仍认它是那只生物, 点它/打它/瞄准都是它本人**（伤害、掉落、其他插件看到的全是真身）。配套: 该实体的 `MoveActor*`(18/111) 对这些观看者吃掉, 位置/朝向由每 tick 心跳用 `MoveActorAbsolute` 推（PlayerNpc 的轻推通道）; `PlayerList` 条目 ~20 tick 后摘掉（不进 Tab, 实体保留）; 撤销 = `clearEntity` 重发真实出生包。皮肤来自 `IPlayerNpc` 注册表（`registerSkin` / `captureSkin` / `registerSkinFromBlob`）。这是"给生物换自定义模型"的正路（PlayerNpc 载体那套要隐形真身 + 逐 tick 跟随 + ghost 转发攻击, 这条都不需要）。消费方: `MSkinventory` 的"同 id 换皮"（`config.mobRenderMode=replace`, 默认）。**真实实体交互事件**: `addActorInteractListener` / `removeActorInteractListener`（尾部追加; 多播; 复用已有收包钩子, 不新增 detour 层）; **ABI 布局戳**（虚表布局描述符 + 消费方编译期自检, 库侧运行期拦截布局不一致的消费方 —— 中段挪槽两次启动崩溃的机制化防护）; LSE `viewEntity` spec 新增 `skin=<皮肤id>`。 | **批次内追加（未发布）· 收缩（明记）**: 对话框 `avatarViewSpec` 的 **`skin=` 分支移除** —— 它会把载体换成"玩家模型"（`PlayerList(Add)`+`AddPlayer`）, 实测客户端在该载体上点进菜单会关界面（同次测试 `type=` 载体: 僵尸/鸡正常）；按"库只做机制"定位, 该路线改由消费方自建（`playerNpcs()` + `npcUniqueIdOverride`）。**保留** `type=` / `name=` 与 `avatarSkinVariant`; 两字段本身不动（ABI 不变）, 顺带移除一段从未生效的玩家模型载体 id 分配死代码。

> **ABI 事故（本次, 明记）**：`addActorInteractListener` / `removeActorInteractListener` 第一版被**插在了 `IHologramLib` 中段**（ghost 段之后）——后面所有虚表槽位整体前移, 用旧头文件编译的消费方（MeowHolographicRenderer 在 `playerNpcs()` 上取到了错的槽）直接 `0xC0000005`。已移到类尾部并在头文件里留了 ⚠ 说明。**教训: "只在尾部追加"不是礼貌, 是 ABI** —— 中段插入 = 所有预编译消费方静默错位。
| `26.40.8` | 1.26.0 | `0x012000` | **视图覆盖旧世界残骸清理**（行为不变, "只拦不发"自 26.40.7 起即如此 —— 重写过期的"原包改写"顶注、删除就地改写死代码（`dataListOf` / `setItemById` / `cloneItems` / `restoreItems`）、判定链路全 const 化; 换类型/名字牌/隐藏/方块覆盖一律丢原包 + 库自己手写协议包）; `view*` LSE 导出补齐**逐字段**（6→16 函数; 唯一 id 判定修正 —— 玩家 id 本身是负数）。**`IPlayerNpc` 尾部追加五方法**（`setPositionLight` 轻量位置更新, 与 `setRotationLight` 同通道; `injectSkin` / `injectSkinAll` 玩家皮肤注入, 用目标自己的 UUID 发 `PlayerList(Add)` 含"自己看自己"; `playAnimation` / `playAnimationTo` 按名触发动画 + `setEntitySpawnCallback`; 既有 ABI 不变）。**悬浮字域收缩**（见上方"第二次收缩"）: 移除行级缩放/颜色/渐变/彩虹/滚动/垂直动画/行间距 8 个函数与库内自驱 `holoTick`。**文本能力补全**: `holo*` 新增 `setScale` / `setBackgroundColor` / `clearBackgroundColor` / `setDepthTest` / `setRotation` / `clearRotation`（背景框颜色 / 穿墙开关 / 三轴 Euler **度** 旋转）; `shape*` 新增 `shapeSetBackgroundColor` / `shapeClearBackgroundColor` / `shapeSetDepthTest`（仅文本形状）。**修复**: `setRotation` 补 `UseRotation` + 双面渲染位（此前文本旋转静默无效）; `destroyBatch` / `destroyAll` 补发移除包（此前只清内存）; **颜色打包修正**: int32 颜色按 v2168 的 **ARGB** 打包（此前沿用 v944 字节序 → R/B 颠倒、填红显蓝; 形状/文本颜色/背景框全部受影响, 打包与解包已同步修正）; **视图覆盖**（Disguise 实证的两处修复 + 一处接线）: `asPlayer` 判空漏项（"伪装成某个玩家"被当成撤销、报"无法伪装"）; 覆盖不再作用于"自己"（本人不发/不吃关于自己实体的包 —— 此前玩家变生物会把自己客户端卡死; **换肤类例外: 含本人**, 自视也变）; 被替换玩家的位移推送补上（`MoveActorAbsolute`, 此前是死代码、替身站桩不动）。**动态行回归**（**重构自 Phantom**（github.com/GroupMountain/Phantom）, **LGPL-3.0** —— 已按其许可要求在代码/README 标注来源; 该部分按 LGPL-3.0 分发, 库其余部分仍 MIT）: `IHologramText` 尾部追加 `setLinePool`（内容池**时间取模轮播**, 无状态）/ `setLineParseVariables`（行级变量开关）, 库内 **0.5s 节流**刷新、**内容变了才重发**; 并修**逐观看者（含 `{var}`）文本的刷新只改内存不发包**（此前更新要等 15s 兜底重发才到客户端）; 变量集补 `{dimension}` / `{x}` / `{y}` / `{z}`。LSE 导出 `holo*` 26→24→**26**（收缩后补回动态行 `setLinePool` / `setLineParseVariables`）/ `shape*` 36→39 / **`playerNpc*` 25→29**（补 `playerNpcInjectSkin(All)` 皮肤注入与 `playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64` 皮肤 blob 持久化配对 —— "玩家自视换肤"的脚本侧正路）；**NPC 对话框头像自定义**: `NpcDialogSpec` 尾部追加 `avatarSkinVariant`（NPC 内置皮肤变体 0..59 → 载体 ActorData `SkinId(104)`; -1 = 不加项）/ `avatarViewSpec`（载体上叠加 IViewOverride 语义: 换类型/换标题）, LSE 补 `npcDialogSetAvatar`（就地重开, 0 闪烁）—— `npcDialog*` 8→9, 总计 268 |
| `26.40.7` | 1.25.0 | `0x011F00` | **新增客户端视图覆盖 `IViewOverride`**（协议层拦截）。两条纪律: **只拦不发** —— 出站钩子挂在 `NetworkSystem::send` / `sendToMultiple`, 只决定「放行 / 丢弃」, **从不修改引擎包字段**; 需要改的一律丢原包 + 库**自己手写协议包**补发（sculk 构造 → 回读校验 → 原始字节发送）。**数据来源只有协议包**（入站 `PlayerAuthInput` 的位置/朝向/输入位、出生包载荷、`Animate`·`ActorEvent`），不读服务端实体状态（例外: 身份识别）。能做: 生物换类型、**玩家变生物 = 替换**（吃掉 `AddPlayer`、用**同一 runtimeId/uniqueId** 发一只该类型实体, `MovePlayer` 对该观看者吃掉、位置由 `MoveActorAbsolute` 推 → **服务端仍认他是真玩家, 打他就是打他本人**）、玩家换皮肤（重发 `PlayerList`, 可立刻撤销）、换名字牌、隐藏、方块换外观; **外加每 tick 心跳**（实体重新进入视野 / 玩家换区块时重推覆盖）。**不进库**: 背包镜像等"读引擎状态的功能项"（消费方用容器域协议能力自己拼）。LSE 导出 `view*`（6 函数）。判定逻辑独立在 `src/view/ViewOverrideLogic.h`, 离线可测（`tests/check-view-override.bat`, 54 项）。**没做的**: 逐玩家缩放/发光/隐身（26.40 元数据表里没有这些项）。|
| `26.40.6` | 1.24.0 | `0x011E00` | **交易菜单新增"纯协议层真结算"**：`TradeMenuSpec::settleLocally` + `ITradeMenu::setSettleLocally` / `isSettleLocally` / 结算回传（`TradeSettlementEvent` 监听 + `tradePollSettlements`）。界面仍是自建 `UpdateTrade`（服务端不放交易表），但库接住客户端的付费放置 / 取回 / 成交请求并**真的**从玩家背包扣付费、把产物写进背包（关界面退还暂存付费）；光标（"点一下拿起 / 再点一下放进槽"）由库托管。判定逻辑独立在 `src/trade/TradeSettlementLogic.h`，离线可测（`tests/check-trade-settlement.bat`，43 项）。**已知限制**：客户端要求的"交易槽槽位更正"在本协议版本与协议库线格式不一致（手写会打崩客户端），客户端可能把放料撤回 —— 稳定成交请用 `usePacketOffers=false`（真实交易表）或容器 UI |
| `26.40.5` | 1.23.0 | `0x011D00` | 新增 `IFakeInventory`（背包虚容器：协议层改写客户端看到的玩家背包内容、点伪造物品回传槽位号、与交易菜单/虚拟容器共存）与 `ISulfurDisplay`（硫磺立方体展示：把方块"吞"在主手 + 隐身默认开 + 外观档位属性），两者都带 LSE 导出；虚拟容器新增**可交互模式**（容器内拖动/交换物品会被库接住并保留）；`ICustomEntity::setMobProperty` 开放实体属性同步 |
| `26.40.4` | 1.22.0 | `0x011C00` | **交易菜单改为纯展示**（暂无有效监听方法），新增 `ITradeMenu::addOffer` / `setTier`（就地重发交易表）；四个新域补齐 **LSE 导出**（`trade*` / `container*` / `npcDialog*` / `sensing*`，容器与对话的点击走轮询队列），`entity*` 补逐客户端渲染导出（`entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` / `entityClearPlayerAppearance`）|
| `26.40.3` | 1.21.0 | `0x011B00` | 新增 `ITradeMenu`（村民交易菜单：协议层交易界面）、`INpcDialogue`（NPC 对话界面：场景/正文/按钮 JSON、点击与关闭回传、合成 NPC 载体）、`IContainerMenu`（虚拟容器/列表：复刻 GMLIB ChestUI —— 客户端侧箱子方块 + 方块实体 NBT + 绑方块坐标的 ContainerOpen；小容器 27 格、大容器 54 格配对；点击回传槽位号）与 `IPlayerSensing`（感知域：AuthInput InputMode 逐包捕获客户端设备）；`ICustomEntity` / `IHologramText` 追加逐客户端渲染（按观看者覆盖名字牌/缩放/装备、含 `{var}` 的文本按观看者解析）；移除逐客户端音效与短命飘字 |
| `26.40.2` | 1.20.0 | `0x011A00` | 修复 NPC 皮肤不渲染（`Id`/`FullId` 补全、PlayerList 2168 帧格式）|
| `26.40.1` | 1.20.0 | `0x011A00` | 适配 LeviLamina 26.40 / BDS 1.26.40（协议 2168）；逐客户端朝向（`setPlayerRotation` 等）|
| `26.10.7` | 1.19.0 | `0x011900` | 皮肤内存导出/目录导入、`geometryData` 自定义模型、NPC 缩放；1.19.1 追加 ghost 交互多播监听 |
| `26.10.6` | 1.15.0 | `0x011500` | 皮肤采集永久存储修复 |
| `26.10.5` | 1.15.0 | `0x011500` | `IParticleShape`（粒子形状）域 |
| `26.10.4` / `26.10.3` | 1.9.0 | `0x010900` | 早期域（`IShapeDrawer` / `IHologramText` / `IItemDetail` 等）|
| `26.10.1` | 1.7.1 | `0x010701` | 最初版本 |

> 早期几个 tag 的 README 没有「API 版本」行，上表按当时的宏与 changelog 归纳；要精确到某个 tag，用
> `git show <tag>:include/hologramlib/HologramLib.h | grep HOLOGLIB_API_VERSION`。

## 消费方怎么用

```cpp
#include <hologramlib/HologramLib.h>

// 1) 编译期：要求某个能力域存在
static_assert(HOLOGLIB_API_VERSION >= 0x011B00, "需要 1.21.0 (ITradeMenu)");

// 2) 运行期：插件 enable 时握手（库可能被替换成旧版）
auto const ver = hologramlib::IHologramLib::getInstance().version();
if (ver < 0x011B00) {
    logger.error("HologramLib 版本过低: 0x{:06X}, 需要 >= 0x011B00", ver);
    return false;
}
```

## 新增域的追加约定

`IHologramLib` 是**冻结契约**：新能力一律在类尾部追加虚函数，不改动/重排既有方法（既有方法的 ABI 因此保持不变），并递增 `HOLOGLIB_API_VERSION`。每个新增域在 README 的更新日志里记一行，并在此表登记。
