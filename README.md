# HologramLib

Bedrock 协议层统一悬浮显示库（LeviLamina 26.10.14 / 协议 944）。将九大能力域合并为**单一插件**，同时提供**冻结的 C++ 虚接口**与 **LSE（ll.import）兼容层**：

| 能力域 | C++ 接口 | LSE 前缀 | 说明 |
|--------|----------|----------|------|
| 形状渲染 | `IShapeDrawer` | `shape*`（36 函数） | 线/盒/圆/球/箭头/文本，协议层 PrimitiveShapes 包 |
| 悬浮字全息 | `IHologramText` | `holo*`（26 函数） | 多行文本、彩虹、变量占位符，跨维度迁移 |
| 渐变线 | — | `gradient*`（11 函数） | 多色渐变轨迹线 |
| 物品详情 | `IItemDetail` | `itemDetail*`（2 函数） | 自动翻译 "钻石 x64" |
| FMBE 物品悬浮 | `IItemDisplay`（1.6.0） | `itemDisplay*`（30 函数） | 狐狸+发包；无感创建（createSeamless）、白名单、视距、scaleTo |
| 自定义实体 | `ICustomEntity`（1.10.0） | `entity*`（33 函数） | 协议层生成实体：姿态/装备槽/动画/ActorLink 骑乘 |
| Ghost 交互 | 监听器 + 轮询 | `ghost*`（2 函数） | 非真实实体的交互事件路由（InteractPacket hook, 1.12.0） |
| 粒子形状 | `IParticleShape`（1.14.0） | `particle*`（21 函数） | 点/线/矩形环/填充面/盒框/六面/多面体 + moveTo/旋转/自旋/跟随 |
| 假玩家 NPC | `IPlayerNpc`（1.16.0） | `playerNpc*`（24 函数） | 纯协议假玩家；皮肤 PNG 注册/在线采集/目录导入/自定义模型 |

除 FMBE/自定义实体走"假实体 + 发包"外，其余渲染均不产生真实实体、不写存档、零服务器开销；粒子发送走 vanilla `SpawnParticleEffectPacket` 批量通道（BDS tick flush 自动聚合压缩为单 Batch 数据报）。

- API 版本：**1.19.0**（`HOLOGLIB_API_VERSION 0x011900`）
- 插件发布版本：`26.10.7`

## 更新日志

- `26.10.7`（API 1.19.0）：新增皮肤从内存导出（`getSkinBlob`/`registerSkinFromBlob`，消费方自行持久化）；目录批量导入皮肤（一个子文件夹 = PNG + 可选 `.json` 模型）；`PlayerNpcSkin.geometryData` 自定义几何模型；库移除磁盘存储，改为纯 API；新增 NPC 缩放（`PlayerNpcConfig.scale` + `playerNpcSetScale`，0.0625~10，碰撞箱等比）；修复 NPC 视距裁剪/脏刷新/Tab 移除失效（tick hook 未注册）；修复 NPC 重生（缩放/换肤等脏刷新）皮肤丢失变默认史蒂夫（过期 Tab 移除条目误删新皮肤条目）
- `26.10.6`（API 1.17.1）：皮肤采集永久存储修复

## 目录

```
HologramLib/
├── include/hologramlib/HologramLib.h   # 唯一公开头（API 冻结契约在顶部注释）
├── src/                                # 内部实现（不属于 API, 可自由变更）
│   ├── HologramLibImpl.cpp             #   接口实现（委托各 Manager 单例）
│   ├── PacketDebugRenderer.*           #   形状渲染（协议层）
│   ├── ProtocolShape.h / ProtocolPackets.*
│   ├── FloatingTextManager.*           #   悬浮字
│   ├── GradientLineManager.*           #   渐变线
│   ├── itemdetail/                     #   物品详情
│   ├── itemdisplay/                    #   FMBE 物品悬浮
│   ├── customentity/                   #   自定义实体
│   ├── particles/                      #   通用粒子形状（批量发送/moveTo 动画）
│   ├── ghost/                          #   Ghost 交互路由
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

```bash
xmake f -c -y
xmake -y
# 产物: build/windows/x64/release/HologramLib.{dll,lib}
# 打包: bin/HologramLib/HologramLib.dll
```

## 部署

1. 关服 → 将 `bin/HologramLib/HologramLib.dll`（连同 manifest.json）放入服务端 `plugins/HologramLib/` → 开服
2. 启动日志确认：
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

// FMBE 物品悬浮（狐狸+发包; 三轴旋转/平移/缩放支持 Molang 表达式）
hologramlib::ItemDisplayConfig cfg;
cfg.x = 100.5f; cfg.y = 65.0f; cfg.z = -200.5f;
cfg.rotY = "math.sin(query.life_time*90)*360";   // 旋转动画
auto disp = lib.itemDisplays().create(cfg);

// 粒子形状: 填充面 + 白名单 + 平滑移动（锚点 easeOutCubic 插值, 整面随锚点移动）
auto wall = lib.particleShapes().createPlane(
    "Steve", 0, 100, 64, -200, 48, 32, /*axis=*/2, /*step=*/3,
    "minecraft:heart_particle", /*interval=*/10, /*lifetime=*/0);
lib.particleShapes().setVisiblePlayers(wall, {"uuid1", "uuid2"}); // 空 = 维度全员
lib.particleShapes().moveTo(wall, 150, 64, -200, 100);            // 5 秒平滑滑移
```

xmake 接入：

```lua
add_includedirs("../HologramLib/include")
add_linkdirs("../HologramLib/build/windows/x64/release")
add_links("HologramLib")
```

### LSE 脚本

```js
// 统一命名空间 "HologramLib", 八域前缀:
//   shape* / holo* / gradient* / itemDetail*
//   itemDisplay* / entity* / ghost* / particle*
const shapeCreateLine = ll.import("HologramLib", "shapeCreateLine");
const holoCreate      = ll.import("HologramLib", "holoCreate");
const itemDisplayCreateBeacon = ll.import("HologramLib", "itemDisplayCreateBeacon");
const particleMoveTo  = ll.import("HologramLib", "particleMoveTo");
```

完整函数清单见 [API.md](API.md)。

## 依赖

- LeviLamina
- [SculkCatalystMC/Protocol](https://github.com/SculkCatalystMC/Protocol)

## 许可

MIT 许可（见 [LICENSE](LICENSE)）
