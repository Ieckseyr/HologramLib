# 快速开始

从零把 HologramLib 跑起来：构建 → 部署 → 写第一个功能。预计 15 分钟。

> 只是用这个库（不自己编译）可以直接跳到 [部署](#3-部署)。

---

## 1. 前置条件

| 需要 | 版本 | 说明 |
|---|---|---|
| Visual Studio 2022 | MSVC x64 | 必须带 C++ 桌面开发工作负载 |
| [xmake](https://xmake.io) | 最新 | 构建系统 |
| LeviLamina | 26.40.0 | xmake 会自动拉取 |
| [SculkCatalystMC/Protocol](https://github.com/SculkCatalystMC/Protocol) | v2193 | 需要自行 CMake 构建并安装到 `plugins/BedrockProtocol-main/install` |
| Python | 3.x | 仅本地离线回归检查用（`tests/`），不参与构建 |

`Protocol` 静态库是硬依赖：形状渲染、视图覆盖、容器与交易菜单的手写包体都直接调它的 sculk 序列化。

---

## 2. 构建

```bash
xmake f -c -y
xmake -y
```

产物：

| 路径 | 是什么 |
|---|---|
| `build/windows/x64/release/HologramLib.{dll,lib}` | 编译产物（`lib` 给消费方链接用） |
| `bin/HologramLib.zip` | 发布包（部署用这个；解压后得到 `HologramLib/` 文件夹） |

> ⚠ 在普通 Windows shell（cmd / PowerShell）里跑 xmake。
> 在 Git Bash / MSYS 下，xmake 会把 host 认成 msys，按 msys 的工具链与包目录解析依赖，
> 于是找不到 MSVC，转而去重新编译 fmt / gsl 之类的基础包并失败（`cannot get program for cc`）。

Release 构建会一并产出 PDB，崩溃栈符号化需要它，部署链已包含。

---

## 3. 部署

1. 关服；
2. 解压发布包 `HologramLib.zip`，把得到的 `HologramLib` 文件夹整个放进服务端 `plugins/`，最终为 `plugins/HologramLib/`（内含 `HologramLib.dll` 与 `manifest.json`）；
3. 开服。

启动顺序没有任何要求。库在 `enable()` 期检测 LSE 兼容层，没检测到就在 `ServerStartedEvent` 兜底重试，双保险意味着它和 LegacyRemoteCall（`lrca`）谁先加载都可以。

消费方插件不需要在 `manifest.json` 里声明前置依赖：直接 `getInstance()` 即可。

### 确认加载成功

正式产物完全静默（见下节），所以看 LeviLamina 自己的插件列表即可：

```
[HologramLib] ...     ← LeviLamina 打印的插件列表里出现即加载成功
```

### 本库是纯前置：正式产物不输出任何日志

所有诊断日志（启动信息、各域执行细节、LSE 兼容层探测结果、协议校验告警）都在编译期关闭，日志字符串根本不进二进制。这是刻意的，一个前置库不该往服务端日志里写东西。

排障时临时打开（关掉后务必重新构建再发布）：

```bash
xmake f --holo_diag=y
xmake build -y
# 打开后输出走 "HologramLib" 通道，前缀形如 [NpcDialog] / [TradeMenu] / [PlayerNpc]

xmake f --holo_diag=n && xmake build -y   # 发布前复位
```

开关在 `src/DiagLog.h`：`HLIB_LOG_INFO/WARN/ERROR/DEBUG` 四个宏用 `if constexpr` 统一收口，关闭时不生成任何代码，但参数仍会被编译检查（格式串写错照样报错）。

---

## 4. 第一个 C++ 功能

### 版本门（第一件事就该做）

```cpp
#include "hologramlib/HologramLib.h"

// ① 编译期：要求某个能力域存在
static_assert(HOLOGLIB_API_VERSION >= 0x012300, "需要 1.29.0");

// ② 运行期：握手（服务端上装的可能是旧版库）
bool onEnable() {
    auto const ver = hologramlib::IHologramLib::getInstance().version();
    if (ver < 0x012300) {
        logger.error("HologramLib 版本过低: 0x{:06X}, 需要 >= 0x012300", ver);
        return false;
    }
    return true;
}
```

编译期断言 + 运行期握手取的是同一个值（`version()` 直接返回 `HOLOGLIB_API_VERSION`），所以只要两处都写，就不会出现「编译过了但运行期炸」的静默错位。

### 画一个悬浮字

```cpp
auto& lib = hologramlib::IHologramLib::getInstance();

auto holo = lib.holograms().create(0, 70, 0);
lib.holograms().addLine(holo, "§e欢迎来到主城");
lib.holograms().addLine(holo, "在线: {online}");
lib.holograms().setBackgroundColor(holo, 0.0f, 0.0f, 0.0f, 0.6f);
lib.holograms().setDepthTest(holo, true);
lib.holograms().draw(holo);
```

关于悬浮字，两个模型级的事实先记住，能省掉一轮试错：

- 多行 = 一个文本形状：多行用 `\n` 合并，整块共用一份颜色 / 缩放 / 背景框 / 穿墙 / 旋转。行级颜色用文本内嵌 `§` 代码，不要找「行级 API」，协议 2168 的文本载荷没有行级字段，历史上那批行级函数已在 `26.40.8` 收缩移除。26.51 线同理：协议 2193 只是在**整块**载荷里多了 `LineGapHeight`（行间距，库固定发 `0.0f` = 不加行距），行级颜色 / 缩放依旧不存在。
- 永久悬浮字不要设 `duration`：不设 duration 才能避免客户端倒计时自动删除。生命周期细节见 [架构总览](https://github.com/Ieckseyr/HologramLib/blob/main/docs/architecture/overview.md#悬浮字的组合模型)。

### xmake 接入

```lua
add_includedirs("../HologramLib/include")
add_linkdirs("../HologramLib/build/windows/x64/release")
add_links("HologramLib")
```

---

## 5. 第一个 LSE 脚本

LSE 侧一切都在单一命名空间 `HologramLib` 下，函数名前缀分域：
> 标准接口（`mc` / `ll` / `logger` / 事件 / NBT / 表单 / 数据库…）见 [LSE 文档站](https://lse.levimc.org/zh/apis/)；本页只讲 HologramLib 这一个命名空间。

```js
const holoCreate = ll.import("HologramLib", "holoCreate");

const id = holoCreate(0, 70, 0);
ll.import("HologramLib", "holoAddLine")(id, "§e欢迎来到主城");
ll.import("HologramLib", "holoDraw")(id);
```

先探测再调用。库是运行时挂载的，声明式前置依赖并不存在；用 `ll.hasExported` 做能力协商：

```js
if (ll.hasExported("HologramLib", "containerPollClicks")) {
    // 该能力可用
}
```

两种失败模式，都是显式的、不会静默：

| 情况 | 表现 |
|---|---|
| 库没加载 / `lrca` 没加载 | `ll.import` 得 `null` |
| 函数名不存在（旧版库） | `ll.import` 得 `null` |

所以「拿到的函数是 `null`」永远意味着「这个能力你现在没有」，而不是「调用了但没生效」。
完整的 278 个函数按域分页，见 [LSE 脚本接口](lse/index.md)。

---

## 6. 本地回归检查（可选，仅维护者）

`tests/` 下是一批离线字节级检查，不需要启动服务端，用真实抓包与真实链接库把协议格式钉死：

```powershell
./tests/check-no-diagnostics.bat    # 发布不变量：正式产物不得携带任何诊断日志
./tests/check-trade-packet.bat      # 整个 UpdateTrade 包与 BDS 抓包逐字节对拍
./tests/check-container-packets.bat # 虚拟容器四个包逐字段核对
./tests/check-npc-carrier.bat       # 合成 NPC 载体包逐字段核对
./tests/check-npc-playerlist.ps1    # PlayerList 2168 字段与可信标记
./tests/check-view-override.bat     # 视图覆盖判定逻辑（59 项）
./tests/check-trade-settlement.bat  # 交易结算逻辑（43 项）
./tests/check-fake-inventory.bat    # 背包虚容器线格式（10 项）
```

三条注意事项：

- 这些脚本是本地工作副本里的工具，不随库发布（`tests/` 在 `.gitignore` 里）。部分需要本机 `logs/fullpkts/` 下的真实抓包；没有样本时脚本打印 `no captured ... found` 并以非零码退出。
- `check-no-diagnostics.bat` 验的是发布不变量：改动 `src/DiagLog.h` 或任何日志调用后必须重跑。
- 多数检查钉住的是「某份头文件产出的字节」，所以改动对应头文件后必须重跑那一个。具体对应关系见 [协议层实现笔记](https://github.com/Ieckseyr/HologramLib/blob/main/docs/architecture/protocol-notes.md#离线回归检查)。

---

## 下一步

| 我想…… | 去看 |
|---|---|
| 挑一个能力域接进插件 | [能力域总览](domains/README.md) |
| 查某个方法的完整签名 | [C++ 接口参考](reference/cpp-api.md) · [LSE 脚本接口](lse/index.md) |
| 搞懂重发、心跳、ABI 戳这些机制 | [架构总览](https://github.com/Ieckseyr/HologramLib/blob/main/docs/architecture/overview.md) |
| 决定自己的代码该写在哪一侧 | [设计说明](https://github.com/Ieckseyr/HologramLib/blob/main/docs/design-contracts.md) |
| 排一个收不到包 / 渲染不对的问题 | [协议层实现笔记](https://github.com/Ieckseyr/HologramLib/blob/main/docs/architecture/protocol-notes.md) |
