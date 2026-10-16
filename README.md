<div align="center">
  <img src="docs/assets/HologramLib.svg" alt="HologramLib — 基岩版协议层统一悬浮显示库" width="100%">
</div>

<!-- 文档站: 推送 main 后由 .github/workflows/docs.yml 发布到 https://ieckseyr.github.io/HologramLib/ -->
[中文](README.md) · [English](README.en.md) · [文档](docs/index.md)

HologramLib 把 18 个「往客户端塞东西」的能力域收进一个前置插件：同一份实现同时供 C++ 插件（冻结的纯虚接口）与 LSE 脚本（`ll.import`）调用，全部走协议层，不生成真实实体、不写存档、不碰服务端世界状态。

它是纯前置库，不是功能插件。只提供渲染原语、协议封装与时机通知；命令、表单、权限、占位符、数据持久化一律留给消费方。

| | |
|---|---|
| 插件发布版本 | `26.51.3` |
| API 版本 | `1.29.0`（`HOLOGLIB_API_VERSION 0x012300`） |
| 运行平台 | LeviLamina 26.51 · BDS 1.26.51 · 协议 2193 |
| 对外接口 | 唯一公开头 `include/hologramlib/HologramLib.h` + LSE 命名空间 `HologramLib`（268 个运行期导出） |
| 许可 | MIT（悬浮字「动态行」部分重构自 Phantom，按 LGPL-3.0） |

> **本版（26.51.3）** —— 26.51 线把 26.40 线上 `26.40.8` / `26.40.9` / `26.40.10` 三个批次整体搬了过来（API `1.25.0` → `1.29.0`；`1.28.0` 按 26.40 线约定跳过）。**26.51 侧本次唯一需要新写的适配**：`PlayerAuthInputPacketPayload::mInputData` 在 26.51 由 `std::bitset<66>` 换成 `Bedrock::EnumSet<InputData, 66>`（没有 `operator[]`），视图覆盖的输入位掩码改用 `contains()` 逐位取。**26.51 移植既有的四处适配**（文本 `LineGapHeight`、`ItemInstance::EMPTY_ITEM()`、容器应答 `presence=false`、形状 NetworkId 种子 `0x00FFFFFF`）在本次合并后逐项复核仍在。构建 0 警告；离线检查全绿（`view*` 59 项、交易结算 43 项、容器 42 项、NPC 载体 32 项、PlayerList 与 trade-offers 逐字节对拍等；`check-authinput` 需要本机抓包 `logs/authinput.hex`，本工作区没有该文件，未跑）。
>
> LSE 侧另有运行期核对：用 `ll.hasExported` 逐名扫描源码里 270 处 `exportAs` → **268 项注册**，缺的正是 `ghostClearInteractions` / `ghostPollInteractions`（与 [LSE 接口参考](docs/reference/lse-api.md#已知问题) 记的已知问题一致）。
>
> ⚠ **26.51 侧未复测的部分**：`26.40.9` / `26.40.10` 关于实体元数据与 `AddItemActor`(15) 的结论来自 **26.40 抓包**。本次只核对了协议库常量两侧一致 —— `sculk/protocol/codec/actor/ActorDataIDs.hpp` 与 `MinecraftPacketIds.hpp` 在 26.40 / 26.51 两个协议库里**逐字节相同**，`PrimitiveShapes.hpp` 只多 `mLineGapHeight`（已由 `kTextLineGapHeight` 发出）。真机渲染效果仍需 1.26.51 客户端实测。

## 能力域

| 能力域 | C++ 接口 | LSE 前缀 | 说明 |
|---|---|---|---|
| 形状渲染 | `IShapeDrawer` | `shape*` · 39 | 线 / 盒 / 圆 / 球 / 箭头 / 文本；文本支持背景框颜色、穿墙开关、三轴旋转 |
| 悬浮字全息 | `IHologramText` | `holo*` · 26 | 多行文本、整块颜色 / 缩放 / 背景框 / 穿墙 / 旋转、动态行（内容池轮播 + 行级变量解析）、跨维度迁移 |
| 渐变线 | — | `gradient*` · 11 | 多色渐变轨迹线 |
| 物品详情 | `IItemDetail` | `itemDetail*` · 2 | 自动翻译「钻石 x64」 |
| FMBE 物品悬浮 | `IItemDisplay` | `itemDisplay*` · 33 | 「狐狸 + 发包」；无感创建、白名单、视距、Molang 驱动三轴动画 |
| 自定义实体 | `ICustomEntity` | `entity*` · 37 | 协议层生成实体：姿态 / 装备槽 / 动画 / ActorLink 骑乘；逐客户端朝向 |
| Ghost 交互 | 监听器 + 轮询 | `ghost*` · 2 | 非真实实体的交互事件路由（InteractPacket 钩子；C++ 侧支持多播） |
| 粒子形状 | `IParticleShape` | `particle*` · 22 | 点 / 线 / 矩形环 / 填充面 / 盒框 / 六面 / 多面体 + moveTo / 旋转 / 自旋 / 跟随 |
| 假玩家 NPC | `IPlayerNpc` | `playerNpc*` · 29 | 纯协议假玩家；皮肤 PNG 注册 / 在线采集 / 目录导入 / 注入（含自视换肤）/ blob 持久化 |
| 感知域 | `IPlayerSensing` | `sensing*` · 3 | 逐包捕获客户端设备：触屏 / 手柄 / 键鼠 / 体感 |
| 千人千面 | `ICustomEntity` 逐观看者覆盖 | `entity*` 尾部 · 4 | 同一实体按观看者覆盖名字牌 / 缩放 / 装备槽 |
| 逐玩家变量全息 | `IHologramText` + `{var}` | — | 文本含 `{var}` 时改为逐观看者形状，每人看到自己的 `{player}` |
| 村民交易菜单 | `ITradeMenu` | `trade*` · 10 | 协议层 `UpdateTrade` 开界面（与 BDS 抓包逐字节一致）；默认纯展示，`settleLocally` 可纯协议真结算 |
| NPC 对话框 | `INpcDialogue` | `npcDialog*` · 9 | `NpcDialoguePacket` + 合成 `minecraft:npc` 载体；头像可自定义（内置皮肤变体 / 视图覆盖叠加）；多层级按场景名路由 |
| 虚拟容器（列表） | `IContainerMenu` | `container*` · 10 | 复刻 GMLIB ChestUI：客户端侧箱子方块 + 方块实体 NBT + `ContainerOpen`；小 27 格 / 大 54 格；可交互模式能真的拖动交换 |
| 背包虚容器 | `IFakeInventory` | `fakeInv*` · 9 | 协议层改写客户端看到的玩家背包内容（服务端背包一个字不动）；点击回传槽位号 |
| 硫磺立方体展示 | `ISulfurDisplay` | `sulfur*` · 10 | 生成 `minecraft:sulfur_cube`，把方块「吞」在主手；立方体默认隐身，只留内容 |
| 客户端视图覆盖 | `IViewOverride` | `view*` · 18 | 按玩家拦截出站包（只拦不发）：换类型 / 换名字牌 / 对该玩家隐藏 / 同 id 换模型 / 掉落物换类型 / 方块换外观 |

上表的 16 个前缀对应 16 个有 LSE 导出的域。剩下两个方向相反：渐变线只有 LSE 接口，逐玩家变量全息只有 C++ 接口。

LSE 侧已知问题：源码里共 270 处 `exportAs`，但 `ghost*` 的 2 处没接进注册流程（`GhostInteractRouter::exportLse()` 无人调用），运行期实际注册 268 个。脚本侧 `ll.import` 得 `null`，C++ 侧的多播监听器不受影响。详见 [LSE 接口参考](docs/reference/lse-api.md#已知问题)。

## 设计说明

接口演进规则、功能边界与视图覆盖域的限制。完整内容在 [docs/design-contracts.md](docs/design-contracts.md)。

### 接口兼容性

`IHologramLib` 与各域接口只在类尾部追加虚函数，既有方法的签名与语义不变。`HOLOGLIB_API_VERSION` 是代码侧唯一的版本依据，编译期用 `static_assert` 门住、运行期用 `version()` 握手。中段插入虚函数会让虚表槽位整体前移，消费方静默调用到错误的函数；26.40.9 起头文件带虚表布局描述符，库侧在运行期拦截布局不一致的消费方。

### 功能边界

库不落盘、不持久化消费方数据，也不替消费方决定何时重发什么。它提供时机通知（如 `setEntitySpawnCallback`）与单发原语（如 `playAnimationTo`），补发决策和数据来源都在消费方。判断一个新能力该不该进库，看它换一个消费方是否还成立。

### 视图覆盖域

出站钩子只决定这一包发不发，不修改引擎包对象里的字段；需要改内容时丢掉原包，由库自己手写协议包补发。位置、朝向、输入位取自入站 `PlayerAuthInput`、出生包载荷与 `Animate`·`ActorEvent`，不读服务端实体状态。

## 上手

### C++ 插件

```cpp
#include "hologramlib/HologramLib.h"

// 编译期门住所需能力域；运行期握手兜住「库被换成旧版」
static_assert(HOLOGLIB_API_VERSION >= 0x011B00, "需要 1.21.0 (ITradeMenu)");

auto& lib = hologramlib::IHologramLib::getInstance();

// 悬浮字：整块一个文本形状，多行用 \n 合并，行内颜色用 § 代码
auto holo = lib.holograms().create(0, 70, 0);
lib.holograms().addLine(holo, "§e欢迎来到主城");
lib.holograms().addLine(holo, "在线: {online}");           // {var} 按观看者解析
lib.holograms().setDepthTest(holo, true);                  // true = 被方块遮挡（默认穿墙可见）
lib.holograms().draw(holo);

// 形状：线 + 颜色 + 生命周期
auto line = lib.shapes().createLine(0, 64, 0, 10, 64, 10);
lib.shapes().setColor(line, 1.0f, 0.2f, 0.2f, 1.0f);
lib.shapes().draw(line);

// 虚拟容器：任何输入设备都能点的列表界面（点击 = 一次物品拾取）
hologramlib::ContainerMenuSpec menu;
menu.title = "§8任务列表";
menu.rows  = 3;                                            // 3 = 27 格；6 = 54 格
menu.items.resize(27);
menu.items[0].type = "minecraft:diamond_sword";
menu.items[0].name = "§b试炼";
auto menuId = lib.containerMenus().open("Steve", menu);
lib.containerMenus().setItem(menuId, 0, {"minecraft:barrier", 1, 0, "§c已完成", {}});
```

xmake 接入：

```lua
add_includedirs("../HologramLib/include")
add_linkdirs("../HologramLib/build/windows/x64/release")
add_links("HologramLib")
```

### LSE 脚本

```js
// 统一命名空间 "HologramLib"，16 个前缀分域
const holoCreate      = ll.import("HologramLib", "holoCreate");
const containerOpen   = ll.import("HologramLib", "containerOpen");
const sensingIsTouch  = ll.import("HologramLib", "sensingIsTouch");

// 按客户端设备分流：触屏走虚拟容器（点击必然发包）
if (sensingIsTouch("Steve")) {
    const id = containerOpen("Steve", "§8任务列表", 3);
    // 点击在守护 / 定时里取（取走并清空）
    for (const line of ll.import("HologramLib", "containerPollClicks")()) {
        // "player=Steve menuId=1 slot=0 closed=0"
    }
}
```

## 安装

1. 关服。
2. 解压发布包 `HologramLib.zip`，把得到的 `HologramLib` 文件夹整个放进服务端 `plugins/`，最终为 `plugins/HologramLib/`（内含 `HologramLib.dll` 与 `manifest.json`）。
3. 开服。启动顺序没有要求：库在 enable 期检测 LSE 兼容层，没检测到就在 `ServerStarted` 兜底重试。

消费方插件只需在 `enable()` 里 `getInstance()`，不必声明前置依赖。LSE 兼容层在运行期探测 `lrca`，没有时所有导出调用是安全空操作，原生 C++ 接口不受影响。

正式产物完全静默：所有诊断日志编译期关闭，日志字符串不进二进制。排障时临时打开 `xmake f --holo_diag=y && xmake build -y`，发布前复位（`--holo_diag=n`）。

## 文档

文档在 [`docs/`](docs/index.md)。推送到主分支后会自动发布到 [GitHub Pages](docs/index.md)。

| 我想…… | 去看 |
|---|---|
| 把库跑起来、写第一个功能 | [快速开始](docs/getting-started.md) |
| 知道某个能力域怎么用、有哪些实测坑 | [能力域指南](docs/domains/README.md) |
| 查某个接口的完整签名 | [C++ 接口参考](docs/reference/cpp-api.md) · [LSE 接口参考](docs/reference/lse-api.md) |
| 确认版本兼容性 / 会不会 ABI 断裂 | [版本与 ABI 对照](docs/versions/api-version-matrix.md) |
| 理解内部分层与运行时开销 | [架构总览](docs/architecture/overview.md) · [协议层实现笔记](docs/architecture/protocol-notes.md) |
| 看某个版本改了什么 | [更新日志](docs/versions/changelog.md) |
| 给这个库提交代码 | [设计说明](docs/design-contracts.md) · [架构总览](docs/architecture/overview.md) |

英文入口层覆盖 [文档首页](docs/index.en.md)、[快速开始](docs/getting-started.en.md)、[能力域总览](docs/domains/README.en.md)、[架构总览](docs/architecture/overview.en.md)；未翻译的页面在站点上回落到中文。

## 版本

| 编号 | 位置 | 用途 |
|---|---|---|
| 插件发布版本 | `manifest.json` 的 `version` | 部署用，与 git tag 同名（形如 `26.51.3`） |
| API 版本 | 本文档上方的 `1.29.0` | 人读的能力版本 |
| `HOLOGLIB_API_VERSION` | `include/hologramlib/HologramLib.h` | 代码里唯一的版本依据，`0x012300` |

三者的完整对应表、编码规律，以及两次 API 收缩的原因，见 [版本与 ABI 对照](docs/versions/api-version-matrix.md)。

## 许可

MIT 许可（见 [LICENSE](LICENSE)）。

悬浮字「动态行」实现（`FloatingTextLine` 的内容池 / 轮播间隔 / 行级变量开关、按间隔时间取模轮播、0.5s 节流刷新与逐玩家重发、内容去重）重构自 [Phantom](https://github.com/GroupMountain/Phantom)（LeviLamina 基岩版悬浮字插件，GNU Lesser General Public License v3.0）。

依 LGPL-3.0 要求标注来源：该重构部分及其演绎作品按 LGPL-3.0 分发，版权归 Phantom 贡献者（GroupMountain）所有；库其余部分仍为 MIT。分发包含该部分的产物时需同时满足 LGPL-3.0：保留版权与许可声明、提供对应源码（来源位置：`src/FloatingTextManager.h` 文件头、`src/FloatingTextManager.cpp` 相应函数上方标注、本 README 与 `docs/versions/api-version-matrix.md`），并保证该部分可被替换 / 独立重建。
