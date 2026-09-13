# HologramLib

Bedrock 协议层统一悬浮显示库（LeviLamina 26.40 / BDS 1.26.40 / 协议 2168）。将九大能力域合并为**单一插件**，同时提供**冻结的 C++ 虚接口**与 **LSE（ll.import）兼容层**：

| 能力域 | C++ 接口 | LSE 前缀 | 说明 |
|--------|----------|----------|------|
| 形状渲染 | `IShapeDrawer` | `shape*`（36 函数） | 线/盒/圆/球/箭头/文本，协议层 PrimitiveShapes 包 |
| 悬浮字全息 | `IHologramText` | `holo*`（26 函数） | 多行文本、彩虹、变量占位符，跨维度迁移 |
| 渐变线 | — | `gradient*`（11 函数） | 多色渐变轨迹线 |
| 物品详情 | `IItemDetail` | `itemDetail*`（2 函数） | 自动翻译 "钻石 x64" |
| FMBE 物品悬浮 | `IItemDisplay`（1.6.0） | `itemDisplay*`（33 函数） | 狐狸+发包；无感创建（createSeamless）、白名单、视距、scaleTo |
| 自定义实体 | `ICustomEntity`（1.10.0） | `entity*`（33 函数） | 协议层生成实体：姿态/装备槽/动画/ActorLink 骑乘；逐客户端朝向（1.20.0） |
| Ghost 交互 | 监听器 + 轮询 | `ghost*`（2 函数） | 非真实实体的交互事件路由（InteractPacket hook, 1.12.0）；C++ 侧支持多播监听（1.19.1） |
| 粒子形状 | `IParticleShape`（1.14.0） | `particle*`（22 函数） | 点/线/矩形环/填充面/盒框/六面/多面体 + moveTo/旋转/自旋/跟随 |
| 假玩家 NPC | `IPlayerNpc`（1.16.0） | `playerNpc*`（25 函数） | 纯协议假玩家；皮肤 PNG 注册/在线采集/目录导入/自定义模型；逐客户端朝向（1.20.0） |

> **假玩家 NPC 的皮肤暂无效（未解决）**：`playerNpc*` 的创建/移动/朝向/缩放/视距/显隐，以及皮肤注册表（PNG 注册、在线采集、目录导入、`getSkinBlob` 导出/`registerSkinFromBlob` 恢复）均正常工作，数据链路完整；但客户端目前不会渲染所设置的皮肤，NPC 外观回退为默认模型。

当前源码已补齐 PNG / 在线采集 / blob 恢复的 `Id` 与 `FullId`，PlayerList 只进行包体前缀校验，不执行 BDS 回读；这些修正仍待客户端验证。发送前检查单条 2168 PlayerList 包体前缀（Add `01 01 00`，Remove `01 00 01`），前缀异常时丢弃并记录实际前缀；首次成功提交到 NetworkPeer 时输出 `PlayerList submitted` 日志。它表示本地校验和提交成功，不表示客户端已渲染。PlayerList / AddPlayer 均跳过 BDS 回读，Tab 保留策略保持原样。

除 FMBE/自定义实体走"假实体 + 发包"外，其余渲染均不产生真实实体、不写存档、零服务器开销；粒子发送走 vanilla `SpawnParticleEffectPacket` 批量通道（BDS tick flush 自动聚合压缩为单 Batch 数据报）。

- API 版本：**1.20.0**（`HOLOGLIB_API_VERSION 0x011A00`）
- 插件发布版本：`26.40.1`

## 更新日志

- `26.40.1`（API 1.20.0）：适配 LeviLamina 26.40 / BDS 1.26.40（协议 2168，形状渲染改用 Protocol v2168 静态库）；修复事件 ID 与官方 LeviLamina 不一致导致监听器全部收不到事件（`src/EventIdCompat.h`）；新增逐客户端朝向（`setPlayerRotation` / `clearPlayerRotation` / `clearPlayerRotations`，实体与 NPC 通用）与轻量朝向更新（`setRotationLight`）；新增 ghost 交互多播监听（1.19.1）；NPC 创建/脏刷新合并到 tick 末尾统一发包（同一 tick 内多次下发会让客户端收到密集"新玩家"而断线）；不再下发 PlayerList 移除（客户端在皮肤条目仍活跃时移除该条目会崩，实体照常消失）；PlayerList / AddPlayer 发送跳过 BDS 回读校验
- `26.10.7`（API 1.19.0）：新增皮肤从内存导出（`getSkinBlob`/`registerSkinFromBlob`，消费方自行持久化）；目录批量导入皮肤（一个子文件夹 = PNG + 可选 `.json` 模型）；`PlayerNpcSkin.geometryData` 自定义几何模型；库移除磁盘存储，改为纯 API；新增 NPC 缩放（`PlayerNpcConfig.scale` + `playerNpcSetScale`，0.0625~10，碰撞箱等比）；修复 NPC 视距裁剪/脏刷新/Tab 移除失效（tick hook 未注册）；修复 NPC 重生（缩放/换肤等脏刷新）皮肤丢失变默认史蒂夫（过期 Tab 移除条目误删新皮肤条目）
- `26.10.6`（API 1.17.1）：皮肤采集永久存储修复

## 目录

```
HologramLib/
├── include/hologramlib/HologramLib.h   # 唯一公开头（API 冻结契约在顶部注释）
├── src/                                # 内部实现（不属于 API, 可自由变更）
│   ├── HologramLibImpl.cpp             #   接口实现（委托各 Manager 单例）
│   ├── EventIdCompat.h                 #   事件 ID 对齐（MSVC 编译必需）
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
- LeviLamina 26.40.0（xmake 自动拉取）
- [SculkCatalystMC/Protocol](https://github.com/SculkCatalystMC/Protocol) v2168 静态库（自行 CMake 构建并安装到 `../BedrockProtocol-main/install`）

```bash
xmake f -c -y
xmake -y
# 产物: build/windows/x64/release/HologramLib.{dll,lib}
# 打包: bin/HologramLib/HologramLib.dll
```

NPC 皮肤协议的离线回归检查（在 x64 Native Tools PowerShell 中运行，需要 Python；不启动或部署服务端）：

```powershell
./tests/check-npc-playerlist.ps1
# 也可显式指定要核对的头文件、静态库及样本目录：
./tests/check-npc-playerlist.ps1 -ProtocolInclude D:/path/to/include -ProtocolLibrary D:/path/to/Protocol.lib -OutputDirectory ./work/npc-wire-check
```

检查使用实际链接库生成 PlayerList 字节，再由独立 Python 解码器核对 2168 字段、可信标记位置、`Id` / `FullId` 和包体完全消费。样本保存为 `PlayerList-add.body.bin`、`PlayerList-remove.body.bin`、带包头的 `PlayerList-add.packet.bin` 与 `wire-check.json`。fixture 中的几何仅用于字节测试，不是游戏内渲染样本。头文件版本宏要求 2168；实际静态库仍以生成的字节为准。

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
// 统一命名空间 "HologramLib", 九域前缀:
//   shape* / holo* / gradient* / itemDetail*
//   itemDisplay* / entity* / ghost* / particle* / playerNpc*
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
