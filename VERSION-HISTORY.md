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

**只在正式发布新版本时才推高本宏。** 在同一条尚未发布的线上继续加能力域不改变它：消费方看到的"我能用的最低版本"没变，抬高宏只会让旧版消费方误判为不兼容。**`0x011D00` 随 `26.40.5` 正式发布**（背包虚容器 `IFakeInventory` + 硫磺立方体展示 `ISulfurDisplay`）—— 此前是 `26.40.4` 的 `0x011C00`。

> **一次例外（明记）**：`26.40.3` 发布过 1.21.0 的交易菜单点击回调，发布后随即按需求撤回 —— 交易菜单改为**纯展示**，`TradeClickEvent` / `TradeActionCallback` / `TradeRawAction` 与 `ITradeMenu` 的六个监听方法整体移除。这是**收缩而不是新增**，按本文档的约定本该走大版本（`2.0.0`）；这里抬到次版本 `1.22.0` 并在表中写明，是因为那条 API 的公开窗口只有一次发布、且没有消费方采用。若你已按 `1.21.0` 写了交易菜单的点击监听，升到 `26.40.4` 需要删掉那些调用（编译期就会报错，不会是静默的行为变化）。

## 完整对照

| 插件发布版本 | API 版本 | `HOLOGLIB_API_VERSION` | 该版本新增的能力域 |
|---|---|---|---|
| `26.40.5` | 1.23.0 | `0x011D00` | 新增 `IFakeInventory`（背包虚容器：协议层改写客户端看到的玩家背包内容、点伪造物品回传槽位号、与交易菜单/虚拟容器共存）与 `ISulfurDisplay`（硫磺立方体展示：把方块"吞"在主手 + 隐身参数 + 外观档位属性），两者都带 LSE 导出；`ICustomEntity::setMobProperty` 开放实体属性同步 |
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
static_assert(HOLOGLIB_API_VERSION >= 0x012100, "需要 1.21.0 (ITradeMenu)");

// 2) 运行期：插件 enable 时握手（库可能被替换成旧版）
auto const ver = hologramlib::IHologramLib::getInstance().version();
if (ver < 0x012100) {
    logger.error("HologramLib 版本过低: 0x{:06X}, 需要 >= 0x012100", ver);
    return false;
}
```

## 新增域的追加约定

`IHologramLib` 是**冻结契约**：新能力一律在类尾部追加虚函数，不改动/重排既有方法（既有方法的 ABI 因此保持不变），并递增 `HOLOGLIB_API_VERSION`。每个新增域在 README 的更新日志里记一行，并在此表登记。
