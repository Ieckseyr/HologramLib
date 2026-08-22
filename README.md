# HologramLib

Bedrock 协议层统一悬浮显示库（LeviLamina 26.10.14 / 协议 944）。

将形状渲染、悬浮字全息、物品详情三大能力域合并为**单一插件**，同时提供**冻结的 C++ 虚接口**与 **LSE（ll.import）兼容层**。渲染全部通过 `PrimitiveShapes` 协议包完成——不产生真实实体、不写存档、零服务器开销。

## 特性

- **统一接口**：`IHologramLib::getInstance()` 单例暴露 `IShapeDrawer` / `IHologramText` / `IItemDetail` 三个能力域，全库唯一公开头 `include/hologramlib/HologramLib.h`
- **API 冻结**：C++ 侧纯虚接口 + DLL 内实现（ABI 稳定）；LSE 侧单命名空间 `HologramLib` 74 个函数（`shape*` / `holo*` / `gradient*` / `itemDetail*` 四域前缀）签名永不变更，只增不改不删（详见 [API.md](API.md) 稳定性契约）
- **零前置依赖**：LegacyRemoteCall / MeowPAPI 均为运行时可选（`GetModuleHandle` + `GetProcAddress` 动态挂载，缺席时 LSE 层安全降级，原生 C++ 接口不受影响）
- **整块文本渲染**：悬浮字为单一多行文本形状（`\n` 合并 + 同 networkId 原地覆盖，无闪烁）；不含逐字符分框与极薄 Box 填充面等旧方案
- **物品名自动翻译**：ItemRegistry → ItemStack → I18n 链，兜底原样显示不阻断

## 目录

```
HologramLib/
├── include/hologramlib/HologramLib.h   # 唯一公开头（API 冻结契约在顶部注释）
├── src/                                # 内部实现（不属于 API, 可自由变更）
│   ├── HologramLibImpl.cpp             #   接口实现（委托各 Manager 单例）
│   ├── PacketDebugRenderer.*           #   形状渲染（协议层）
│   ├── ProtocolShape.h / ProtocolPackets.*
│   ├── FloatingTextManager.* / FloatingTextExporter.*
│   ├── GradientLineManager.* / GradientLineExporter.*
│   ├── itemdetail/ItemDetailManager.*
│   ├── lse/                            #   LSE 兼容层（运行时挂载 lrca）
│   ├── MemoryOperators.cpp             #   跨 DLL 内存配对
│   └── ModEntry.*                      #   生命周期 + LSE 双时机挂载
├── xmake.lua
├── README.md
└── API.md                              # 完整 API 参考文档
```

## 构建

依赖：

- Visual Studio 2022（MSVC x64）
- [xmake](https://xmake.io)
- LeviLamina 26.10.14（xmake 自动拉取）
- `BedrockProtocol-944` 本地静态库（`../BedrockProtocol-944/install/{include,lib}`，含协议 944 的 `sculk::protocol` 封包实现）

> 本仓库按白名单仅含源码与文档。构建前需在工作目录放置 `manifest.json`（modpacker 需要），模板：
>
> ```json
> {
>     "name": "HologramLib",
>     "entry": "HologramLib.dll",
>     "version": "1.5.0",
>     "type": "native",
>     "platform": "server",
>     "description": "统一悬浮显示库: 形状渲染 + 悬浮字全息 + 物品详情",
>     "author": "你的名字"
> }
> ```

```bash
xmake f -c -y
xmake -y
# 产物: build/windows/x64/release/HologramLib.{dll,lib}
# 打包: bin/HologramLib/HologramLib.dll
```

## 部署

1. 将 `bin/HologramLib/HologramLib.dll`（连同 manifest.json）放入服务端 `plugins/HologramLib/`
2. 启动服务器，日志确认：
   - `HologramLib enabling...`
   - LegacyRemoteCall 在场：`LSE compat layer attached (LegacyRemoteCall detected).`
   - LegacyRemoteCall 缺席：`LegacyRemoteCall absent: LSE (ll.import) calls disabled; native C++ API unaffected.`
3. 启动顺序无任何要求（enable 期检测 + ServerStarted 兜底双保险）

## 快速上手

### C++ 插件（推荐）

```cpp
#include "hologramlib/HologramLib.h"

auto& lib = hologramlib::IHologramLib::getInstance();

// 形状
auto line = lib.shapes().createLine(0, 64, 0, 10, 64, 10);
lib.shapes().setColor(line, 1.0f, 0.2f, 0.2f, 1.0f);
lib.shapes().setDuration(line, 10.0f);
lib.shapes().draw(line);

// 悬浮字（两行, 第二行彩虹）
auto holo = lib.holograms().create(0, 70, 0);
lib.holograms().addLine(holo, "§e欢迎来到主城");
lib.holograms().addLine(holo, "在线: {online}");
lib.holograms().setLineRainbow(holo, 1, 1.5f);
lib.holograms().draw(holo);

// 物品详情（自动翻译为 "钻石 x64"）
auto detail = lib.itemDetails().show(0, 100.5f, 65.0f, -200.5f, "minecraft:diamond", 0, 64);
```

xmake 接入：

```lua
add_includedirs("../HologramLib/include")
add_linkdirs("../HologramLib/build/windows/x64/release")
add_links("HologramLib")
```

### LSE 脚本

```js
// 统一命名空间 "HologramLib", 四域前缀:
//   shape* 形状 / holo* 悬浮字 / gradient* 渐变线 / itemDetail* 物品详情
const shapeCreateLine = ll.import("HologramLib", "shapeCreateLine");
const holoCreate      = ll.import("HologramLib", "holoCreate");
const gradientCreate  = ll.import("HologramLib", "gradientCreate");
const itemDetailShow  = ll.import("HologramLib", "itemDetailShow");
```

完整函数清单见 [API.md](API.md)。

## 许可
MIT许可
