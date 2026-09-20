# HologramLib

Bedrock 协议层统一悬浮显示库（LeviLamina 26.40 / BDS 1.26.40 / 协议 2168）。把 **15 个能力域**（其中 13 个另有 LSE 导出）合并为**单一插件**，同时提供**冻结的 C++ 虚接口**与 **LSE（ll.import）兼容层**：

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
| 感知域 | `IPlayerSensing`（1.21.0） | `sensing*`（3 函数, 1.22.0 补） | 客户端设备判断（AuthInput InputMode 逐包捕获）：触屏/手柄/键鼠 |
| 千人千面 | `ICustomEntity` per-viewer 覆盖（1.21.0） | `entity*` 尾部 4 函数（1.22.0 补） | 同一实体按观看者覆盖名字牌/缩放/装备槽 |
| 逐玩家变量全息 | `IHologramText` + `{var}`（1.21.0） | — | 文本含 `{var}` 时按观看者解析（每人看到自己的 `{player}`） |
| 村民交易菜单 | `ITradeMenu`（1.21.0） | `trade*`（7 函数, 1.22.0 补） | 协议层 `UpdateTrade` 开界面（与 BDS 抓包逐字节一致）；**纯展示, 不做点击回调**（1.22.0 起） |
| NPC 对话框 | `INpcDialogue`（1.21.0） | `npcDialog*`（7 函数, 1.22.0 补） | `NpcDialoguePacket` + 合成 `minecraft:npc` 载体；按钮/关闭回传（LSE 走轮询）；多层级对话按场景名路由 |
| 虚拟容器（列表） | `IContainerMenu`（1.21.0） | `container*`（9 函数, 1.22.0 补） | 复刻 GMLIB ChestUI：客户端侧箱子方块 + 方块实体 NBT + `ContainerOpen`；小容器 27 格 / 大容器 54 格；点击回传槽位号（LSE 走轮询） |

> **假玩家 NPC 皮肤已可正常渲染（26.40.2 修复）**：`playerNpc*` 的创建/移动/朝向/缩放/视距/显隐，以及皮肤注册表（PNG 注册、在线采集、目录导入、`getSkinBlob` 导出 / `registerSkinFromBlob` 恢复）均正常工作。此前的症状是客户端不渲染所设置的皮肤、外观回退为默认模型，原因在 PlayerList 皮肤条目的 `Id` / `FullId` 为空或残留了原玩家的缓存键。

PlayerList 现在只做包体前缀校验、不回读 BDS：发送前检查单条 2168 包体前缀（Add `01 01 00`，Remove `01 00 01`），前缀异常就丢弃并记下实际前缀；成功提交到 NetworkPeer 时会写一条诊断日志 `PlayerList submitted`（**只在 `--holo_diag=y` 的排障构建里可见**，正式产物完全静默）。它表示本地校验与提交成功，不表示客户端已渲染。

除 FMBE / 自定义实体 / 交易菜单 / NPC 对话走"假实体（部分隐身、仅目标玩家可见）+ 发包"外，其余渲染都不产生真实实体、不写存档、零服务器开销。交易菜单与 NPC 对话的载体实体在界面关闭时立即删除，不落存档；虚拟容器只在**客户端侧**摆箱子方块（服务端世界与存档里都没有这个方块）。粒子发送走 vanilla `SpawnParticleEffectPacket` 批量通道（BDS tick flush 自动聚合压缩为单 Batch 数据报）。

- API 版本：**1.22.0**（`HOLOGLIB_API_VERSION 0x011C00`）
- 插件发布版本：`26.40.4`
- 版本 / API 版本 / 宏 对照：见 [`VERSION-HISTORY.md`](./VERSION-HISTORY.md)

## 更新日志

- `26.40.4`（API 1.22.0）：**交易菜单改为纯展示 + 新域补齐 LSE 导出**。
  - **交易菜单不再有任何点击监听**：`TradeClickEvent` / `TradeActionCallback` / `TradeRawAction` 与 `ITradeMenu` 的六个监听方法整体删除，`TradeMenuSpec` 去掉 `displayOnly` / `acceptPaymentPlacement` —— 本域从此只负责"打开界面 + 摆出交易表"。原来的 147/AuthInput 钩子与放料接住逻辑随之移除（**留在 `src/container/ContainerInteractionHooks.cpp`**：那两个钩子如今只服务虚拟容器的点击）。要"能点、点了有回调"的列表界面用虚拟容器（`IContainerMenu`）。
  - `ITradeMenu` 新增 `addOffer` / `setTier`：追加一条交易或改档位/经验条时**就地重发**交易表，不必关掉重开。
  - **四个新域补齐 LSE 导出**：`trade*`（7）、`container*`（9）、`npcDialog*`（7）、`sensing*`（3）；容器与 NPC 对话的点击在脚本侧走**轮询队列**（`containerPollClicks` / `npcDialogPollClicks`，取走并清空，条目是可切分的字符串），`ghost*` 早就用的同一套办法。
  - `entity*` 补逐客户端渲染导出：`entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` / `entityClearPlayerAppearance`。
  - API 版本因此抬到 **1.22.0**：撤回已发布过的东西属于收缩（正常该走大版本），这里抬次版本并在 [`VERSION-HISTORY.md`](./VERSION-HISTORY.md) 明记原因。按 1.21.0 写过交易菜单点击监听的代码升上来会**编译报错**（不会静默变行为）。
- `26.40.3`（API 1.21.0）：**四个新能力域（村民交易菜单 / NPC 对话框 / 虚拟容器 / 感知域）+ 逐客户端渲染 + 一次裁剪**（清单见上表 1.21.0 各行）。
  - **村民交易菜单 `ITradeMenu`**：协议层自建 `UpdateTrade` 开界面（包级字段与 Offers NBT 都与 BDS 抓包逐字节对拍），隐身载体实体驱动经验条；点击回传按配方 netId 精确定位条目（`offerIndex` / `recipeNetId`），另有与参考实现 GMLIB ChestUI 同语义的逐动作回调（`src` / `dst` / `amount`，关闭时以 `slot=-1, amount=-1` 哨兵收尾）。**（本条的点击回传已在 26.40.4 按需求撤回 —— 详见上一条。）**两条路径由 `usePacketOffers` 选：**默认纯协议层**（服务端不放交易表 → 天然只读），置 `false` 走真实交易表 + BDS `openTrading`（成交真的换物品）。载体类型 `carrierIdentifier` 可选村民或流浪商人。
  - **NPC 对话框 `INpcDialogue`**：`NpcDialoguePacket` + 合成 `minecraft:npc` 载体（放在世界下方 → 客户端看不到实体，但对话框头像照常渲染）；按钮 / 关闭回传，多层级对话按场景名路由。
  - **虚拟容器 `IContainerMenu`**：逐条复刻参考实现 GMLIB 的 `ChestUI` —— 客户端侧箱子方块（`UpdateBlock`）+ 方块实体 NBT（`BlockActorData`，物品走 `Items`、标题走 `CustomName`）+ `ContainerOpen`（类型 `Container(0)`、绑方块坐标、目标实体 `-1`）；`rows=3` 小容器 27 格、`rows=6` 大容器 54 格（`pairx` / `pairz` / `pairlead` 配对）；点击回传槽位号 = 条目下标，关闭回传 `closed=true`。**任何输入设备都能点**（点击就是一次物品拾取）。
  - **感知域 `IPlayerSensing`**：挂钩 `PlayerAuthInputPacket` 逐包捕获 `InputMode` → `KeyboardMouse` / `Touch` / `Gamepad` / `MotionController`；玩家离线即清，换设备下一包即更新。
  - **逐客户端渲染（千人千面）**：`ICustomEntity` 新增 `setPlayerNametag` / `setPlayerScale` / `setPlayerEquipmentSlot` / `clearPlayerAppearance`（出生包与增量包都按观看者覆盖值下发，装备变更即时单发无闪烁）；`IHologramText` 文本含 `{var}` 时自动切换为**逐观看者形状**，`{player}` 从此对全员绘制也逐人正确。
  - **裁剪**：移除逐客户端音效与短命飘字（讨论后不再需要，代码整体删除；两者是早期按"协议层手写包"做的实验）。
  - **修复**：①文本动画（滚动 / 弹跳）改由库内自驱（`ServerLevelTickEvent` 监听）—— 此前 `tick` 只有 LSE 导出，原生侧没有驱动源，动画既不动也不消失；②147 钩子原先只在**交易**菜单打开时才进入，虚拟容器点击因此全部漏掉；③点击上报的是背包侧槽位（现在优先报交易侧）；④`src/DiagLog.h` 遗留在发布产物里的诊断开关（`check-no-diagnostics.bat` 会挡住）。
  - **实测沉淀（详见下文各节）**：容器打开延迟 7 tick 可用 / 6 tick 不可用的下限；物品请求有 `ItemStackRequestPacket(147)` 与 `PlayerAuthInputPacket(144)` 内嵌两条通道；命中虚拟容器的动作只回传不拦（自己代答失败应答会让客户端弹错误提示）。
- `26.40.2`（API 1.20.0）：修复假玩家 NPC 皮肤不渲染（客户端回退默认模型）—— PlayerList 皮肤条目的 `Id` / `FullId` 为空或残留原玩家的缓存键；PNG 注册、在线采集、blob 恢复统一走 `finalizeSkinIds` 补全 `Id` 并重建 `FullId`，且在 Persona / 采集改写完成后才设置，不再沿用来源玩家的身份。PlayerList 发送前校验 2168 包体前缀（Add `01 01 00`，Remove `01 00 01`），前缀异常时丢弃并记录实际前缀；首次提交到 NetworkPeer 时输出 `PlayerList submitted` 日志。新增离线回归检查 `tests/check-npc-playerlist.ps1`（用实际链接的静态库产生字节 + 独立 Python 解码器核对 2168 字段、可信标记位置、`Id` / `FullId` 与包体完全消费）。配套 Protocol 静态库完成 PlayerList 2168 帧格式移植（variant 数组、`ActionType` Add=0/Remove=1、可信标记改为皮肤内三态字符串）；本插件 ABI / API 版本不变
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
│   ├── DiagLog.*                       #   诊断日志开关（默认编译期关闭, 正式产物零日志）
│   ├── SculkPacketSend.h               #   协议层发包原语（sculk 序列化 + NetworkPeer）
│   ├── PacketDebugRenderer.*           #   形状渲染（协议层）
│   ├── ProtocolShape.h / ProtocolPackets.*
│   ├── FloatingTextManager.*           #   悬浮字（含逐观看者形状 + 自驱动动画）
│   ├── GradientLineManager.*           #   渐变线
│   ├── itemdetail/                     #   物品详情
│   ├── itemdisplay/                    #   FMBE 物品悬浮
│   ├── customentity/                   #   自定义实体（含逐客户端外观覆盖）
│   ├── particles/                      #   通用粒子形状（批量发送/moveTo 动画）
│   ├── ghost/                          #   Ghost 交互路由
│   ├── trade/                          #   村民交易菜单（UpdateTrade / Offers NBT; 纯展示）
│   ├── container/                      #   虚拟容器 + 物品请求钩子（箱子方块 + 方块实体 NBT）
│   ├── npcdialog/                      #   NPC 对话框（NpcDialoguePacket + 合成载体）
│   ├── sensing/                        #   感知域（AuthInput InputMode 逐包捕获）
│   ├── playernpc/                      #   假玩家 NPC（皮肤注册表 / PlayerList / AddPlayer）
│   ├── lse/                            #   LSE 兼容层（运行时挂载 lrca）
│   ├── MemoryOperators.cpp             #   跨 DLL 内存配对
│   └── ModEntry.*                      #   生命周期 + LSE 双时机挂载
├── tests/                              # 离线字节级回归检查（独立于服务端, 产物进 work/）
├── xmake.lua
├── manifest.json
├── README.md
├── API.md                              # 完整 API 参考文档
└── VERSION-HISTORY.md                  # 版本 / API 版本 / 宏 对照
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

在**普通 Windows shell（cmd / PowerShell）**里跑 xmake：Git Bash / MSYS 下 xmake 会把 host 认成 msys，按 msys 的工具链与包目录去解析依赖，于是找不到 MSVC、转而去重新编译 fmt/gsl 之类的基础包并失败（`cannot get program for cc`）。诊断日志选项 `--holo_diag` 见下文「正式产物不输出任何日志」。

NPC 皮肤协议的离线回归检查（在 x64 Native Tools PowerShell 中运行，需要 Python；不启动或部署服务端）：

```powershell
./tests/check-npc-playerlist.ps1
# 也可显式指定要核对的头文件、静态库及样本目录：
./tests/check-npc-playerlist.ps1 -ProtocolInclude D:/path/to/include -ProtocolLibrary D:/path/to/Protocol.lib -OutputDirectory ./work/npc-wire-check
```

检查使用实际链接库生成 PlayerList 字节，再由独立 Python 解码器核对 2168 字段、可信标记位置、`Id` / `FullId` 和包体完全消费。样本保存为 `PlayerList-add.body.bin`、`PlayerList-remove.body.bin`、带包头的 `PlayerList-add.packet.bin` 与 `wire-check.json`。fixture 中的几何仅用于字节测试，不是游戏内渲染样本。头文件版本宏要求 2168；实际静态库仍以生成的字节为准。

交易菜单 / NPC 对话载体 / 虚拟容器的字节级检查：

```powershell
./tests/check-no-diagnostics.bat    # 发布不变量：正式产物不得携带任何诊断日志
./tests/check-trade-packet.bat      # 整个 UpdateTrade 包（包级字段 + Offers NBT）与 BDS 抓包逐字节对拍
./tests/check-trade-offers.bat      # 只对 Offers NBT 段（更快的单项检查）
./tests/check-npc-carrier.bat       # 合成 NPC 载体包（AddActor + NpcDialoguePacket）逐字段核对
./tests/check-container-packets.bat # 虚拟容器四个包逐字段核对（ContainerOpen 与真实抓包逐字节对拍）
```

`check-no-diagnostics.bat` 验两件事：开关极性（关闭时日志 marker 不进目标文件、`-DHOLOGLIB_DIAG_LOG=1` 时必须进）与产物不变量（已构建的 DLL 里不得出现任何库内日志前缀）。**改动 `src/DiagLog.h` 或任何日志调用后必须重跑。**

`check-trade-packet.bat` / `check-trade-offers.bat` 默认取 `logs/fullpkts/pkt80_194951_007.bin`（fixture 的推导样本，确定性）—— 不要改成"取最新"，之后的抓包可能是插件自己生成的交易（配方不同）。整包检查核对 `ContainerId` / `ContainerType`(=15 Trade) / `Size` / `TraderTier` / `EntityUniqueId` / `LastTradingPlayer` / `DisplayName` / `UseNewTradeScreen` / `UsingEconomyTrade` 九个包级字段与整段字节。

`check-npc-carrier.bat` 用 `src/npcdialog/NpcCarrierPacket.h` 产出真实字节，再由 Python 按 26.40 schema 解回字段：校验 AddActor 的字段顺序与尾部完全消费、位置 `y=-66`（世界下方 → 客户端看不到实体但 NPC 界面头像照常渲染）、ActorData 五项标记 `Name(4)` / `HasNpc(39)=1` / `NpcData(40)` / `Actions(41)` / `InteractText(100)` 的 id 与类型，并交叉核对真实 BDS NPC 生成包（`logs/fullpkts/pkt13_*.bin`）确实含同一组标记项与同一份 `NpcData` 字段集。**改动 `NpcCarrierPacket.h` 后必须重跑。**

`check-container-packets.bat` 用库内同一份 `src/container/ContainerPackets.h` 编出虚拟容器的四个包：`ContainerOpen` 与真实抓包 `logs/fullpkts/pkt46_165803_011.bin` **整包逐字节对拍**（该抓包的取值就是 fixture 的输入），`UpdateBlock` / `BlockActorData` / `ContainerClose` 按 26.40 线格式解回字段核对——含单箱子 27 格（不带配对键）与大箱子 54 格（`pairx`/`pairz`/`pairlead` 配对、两半各自 `Slot` 0..26）。**改动 `ContainerPackets.h` 后必须重跑。**

这些检查里的"逐字节对拍"都要**真实抓包样本**（`logs/fullpkts/`，BDS 自己产出的真值）。样本不随库发布：没有样本时脚本会打印 `no captured ... found` 并以非零码退出——要么把自己的抓包路径作为参数传进去，要么只跑不需要样本的检查（`check-no-diagnostics.bat` 就不需要）。

**交易菜单是纯展示：不做任何点击事件监听。** 打开界面、把交易表摆出来给人看，就到此为止 —— 客户端点了哪一条、往付费槽里放了什么，库一律不读、不拦、不回传。`26.40.3` 里短暂存在过的三套回调（`addClickListener` / `addActionListener` / `addRawActionListener` 与 `TradeClickEvent`、`TradeRawAction`）已在 `26.40.4` 整体删除；需要"能点、点了有回调"的列表界面请用虚拟容器（`IContainerMenu`）——它的点击就是一次物品拾取，任何输入设备都会发包。

交易接口的显示栏值用 **1 基**（1=新手 … 5=大师，wire 上是 0..4，由库换算并夹紧）；`TradeMenuOffer::locked = true` 可强制某条显示为未解锁。经验条由载体实体的 `TradeTier` / `MaxTradeTier` / `TradeExperience` 元数据驱动 —— 三项都是 `Int`，与真实村民生成包（`logs/fullpkts/pkt13_*_002.bin`，`minecraft:villager_v2`）实测一致，其中 `MaxTradeTier` 恒为 4（大师）。

交易菜单两条路径由 `TradeMenuSpec::usePacketOffers` 选，**默认是纯协议层展示**：只发我们自己构造的 `UpdateTrade`，服务端不放交易表 —— 天然只读（玩家往付费槽放东西的请求会被 BDS 拒掉、物品弹回，界面停在"不可成交"，这正是纯展示要的）。置 `false` 则给载体装真实交易表并走 BDS 的 `openTrading`，**玩家是真的在交易**（物品真的消耗）。两条路径下 `UpdateTrade` 都会在几 tick 后发送：载体实体是异步送达客户端的，背靠背发时客户端还不认识那个实体，界面绑不上去。

容器条目内容是**动态可刷**的：`ITradeMenu::addOffer(menuId, offer)` 追加一条、`setTier(menuId, tier, experience)` 改档位/经验条，两者都**就地重发交易表**（复用同一个界面与载体，不重开、不等延迟）。

虚拟容器（`IContainerMenu`）的大小由 `ContainerMenuSpec::rows` 选：`3` = 小容器（单箱子 27 格），`6` = 大容器（大箱子 54 格，两个配对方块 + `pairx`/`pairz`/`pairlead`）。条目物品放在**方块实体 NBT 的 Items 列表**里（不是逐格 `InventorySlot`），标题走其 `CustomName`；点击回传给出槽位号 = 条目下标，关闭回传 `closed = true`。`useMinecart = true` 保留的是旧路径（只发一只合成箱子矿车），实测客户端不弹界面，仅供对比排查。

### 容器打开延迟的来历与可调项（实测对比参考实现）

打开一次容器的时序是「客户端侧摆方块 + 方块实体」→ 等 N tick → 发 `ContainerOpen`。那个等待**不是移植时写错的**：

| 实现 | 时序 | 打开耗时 |
|---|---|---|
| 参考实现 GMLIB `ChestUI::sendTo` | `updateBlock` + `updateBlockActor` → **`ticks(10)`** → `ContainerOpen` → **`ticks(4)`** → 逐格补玩家物品栏/光标 | **14 tick ≈ 700ms** |
| 本库 `ContainerMenuManager` | 摆方块 + 方块实体 → **`ticks(openDelayTicks)`（默认 7）** → `ContainerOpen` | **7 tick ≈ 350ms** |

（GMLIB 的 `ChestForm` 只是 `ChestUI` 的薄壳：`ChestForm::sendTo` 直接调 `pImpl->mChestUI->sendTo(pl)`，没有更快的第二条路。）

本库比参考实现少 7 tick：条目放在**方块实体 NBT** 里，不需要 GMLIB 那一步逐格补格（省 4 tick）；剩下的等待本身也压到实测下限（省 3 tick）。那段等待是方案固有：客户端要先把这个方块与它的方块实体应用上去，`ContainerOpen` 才绑得住这个位置（背靠背发实测打不开界面）。

三个可调处：

- `ContainerMenuSpec::openDelayTicks`（默认 **7**）——**只在"打开"那一次生效**。**实测（26.40 本机客户端）：7 能正常开界面、6 打不开**，所以默认取 7（≈350ms，是 GMLIB 等效 14 tick 的一半）。下限与客户端/机器/负载有关，换环境可能要回调大，故保留为可调项。
- `IContainerMenu::update(menuId, spec)`——翻页/换整页内容：复用同一个载体方块，**不拆界面、不重摆方块、不等待**，只重发方块实体 NBT（新条目/标题）+ `ContainerOpen` 让客户端重读。走 `open()` 重开则每次都要付一次打开延迟（它会先关旧菜单、恢复真方块）。
- `IContainerMenu::setItem(menuId, slot, item)`——**按槽动态刷新**：只发一条 `InventorySlotPacket`（与真实箱子同步内容用的是同一种包），客户端就地换掉那一格，**不重发方块实体、不重发 ContainerOpen**，所以无延迟也无闪烁。适合"任务完成打勾 / 数量变化 / 价格变化"这类单格改动；`item.type` 为空即清空该槽。只改标题用 `setTitle(menuId, title)`（内部就是"改完 spec 再 update"）。

脚本侧（LSE）用 `containerOpen` / `containerSetItem` / `containerSetTitle` 打开与填格，点击则在**轮询队列**里取：`containerPollClicks()` 返回并清空待处理点击，每条是 `player=X menuId=N slot=S closed=0|1`（`closed=1` 时 `slot=-1`）。队列与 C++ 监听器收到的是同一份事件。

### 物品请求有两条包通道（实测，26.40）

虚拟容器的点击不是只有一种送法 —— 客户端的 `ItemStackRequest` 有两条通道：

| 通道 | 形态 | 谁在用 |
|---|---|---|
| `ItemStackRequestPacket`(147) | 独立请求, 动作为 cereal 形态 | 容器槽位的取放（实测主通道） |
| `PlayerAuthInputPacket`(144, AuthInput) | **内嵌** `mItemStackRequest`（`PlayerAuthInputPacketPayload::mItemStackRequest`, 输入标志 `PerformItemStackRequest = 36`） | 菜单界面的交互（实测: 触屏时 147 可能一条都没有, 只有这条） |

库**两条都挂**（`src/container/ContainerInteractionHooks.cpp`）: AuthInput 那条用 BDS 自己的 `ItemStackRequestCereal::toActionData()` 把解析态动作转成与 147 相同的 cereal 形态, 再喂进同一个派发函数。AuthInput 只观察、不拦 —— 它同时承载玩家移动, 拦下会把移动一起吞掉, 而内嵌请求由 BDS 自己处理。

**命中容器的动作只回传、不拦**（与参考实现 GMLIB 完全一致）：虚拟容器在服务端并不存在, 放行后 BDS 自己就会失败并让客户端把预测撤回（物品在界面上闪一下回到原位），回调照常收到。实测教训：若这里自己代答一条失败应答（`ItemStackNetResult` 3），客户端会**弹一个错误提示**；交给 BDS 走它自己的失败路径反而是安静的。交易菜单不参与这两个钩子 —— 它不读任何客户端请求。

### NPC 对话框：按钮回传与一个实测限制

`INpcDialogue::open` 下发 `NpcDialoguePacket`（场景名 + 正文 + NPC 名 + 按钮 JSON）并合成一个 `minecraft:npc` 载体实体；
客户端点按钮 / 关闭时回传（`NpcRequestPacket`），回调给出 `sceneName` + `buttonIndex` + `actionId` + `commands` + `closed`。
多层级对话就是"点击回传带回 `sceneName`，调用方据此发下一层"。

三个实测要点：

- **载体必须放在世界下方**（`y=-66`）：客户端看不到实体，但对话框里的头像照常渲染；换成"隐身标志位"会让头像一起消失（实测）。
- **点任意按钮时客户端就会自行收起界面**，而 `INpcDialogue::update()` 只重发一次 `NpcDialoguePacket(Open)` —— 客户端不会因此重新弹出。所以"点按钮就地换页"对 NPC 对话**不可行**：要让客户端重新显示，必须走 `open()`（删旧载体 + 建新载体 + 再发一次 Open）。`update()` 只适合界面仍开着时的内容微调。
- 合成 NPC 在服务端侧没有 BDS 实体代跑命令：`NpcDialogButton::commands` 只是展示在按钮上，要执行得由调用方自己跑（例如 `player.runCommand`）。

脚本侧（LSE）用 `npcDialogOpen` 打开（按钮串 `label|actionId|mode|命令1,命令2`，按钮之间 `;` 分隔）、`npcDialogPollClicks()` 取点击，每条是 `player=X dialogId=N scene=S button=I actionId=A closed=0|1 commands=...`。

### 感知域与逐客户端渲染

- `IPlayerSensing`（`playerSensing()`）：挂钩 `PlayerAuthInputPacket` 逐包读 `InputMode` → `KeyboardMouse` / `Touch` / `Gamepad` / `MotionController`；`Unknown` 不覆盖已知值（AuthInput 早段可能报 `Undefined`），玩家离线即清。适合"按设备分流界面"——触屏走虚拟容器，键鼠走交易菜单。LSE 侧用 `sensingDeviceOf` / `sensingIsTouch` / `sensingDeviceCode` 查询（没有事件，不用轮询）。
- 千人千面（逐客户端渲染）：`ICustomEntity` 的 `setPlayerNametag` / `setPlayerScale` / `setPlayerEquipmentSlot` / `clearPlayerAppearance` 按观看者覆盖外观（出生包与增量包都按覆盖值下发，装备变更即时单发无闪烁），LSE 侧同名的 `entitySetPlayerNametag` 等四个导出；`IHologramText` 的文本一旦含 `{var}`，库就改为**维护每个观看者一份形状**，各自收到按自己解析的结果（观看者离线 / 离开维度时那份形状就地销毁）。都是逐玩家发包，适合少数人场景（队长视角、任务追踪），不适合全服广播式的高频刷新。

## 部署

1. 关服 → 将 `bin/HologramLib/HologramLib.dll`（连同 manifest.json）放入服务端 `plugins/HologramLib/` → 开服
2. 启动顺序无任何要求（enable 期检测 + ServerStarted 兜底双保险）

### 本库是纯前置：正式产物不输出任何日志

正式构建**完全静默** —— 所有诊断日志（启动信息、各域的执行细节、LSE 兼容层探测结果、协议校验告警）都编译期关闭，日志字符串不进二进制。确认加载看 LeviLamina 自身的插件列表即可；消费方（如 MeowTradeTest）自己有日志通道。

需要排障时**临时**打开诊断日志（关掉后务必重新构建再发布）：

```powershell
xmake f --holo_diag=y   # 默认 false
xmake build -y
# 打开后输出走 "HologramLib" 通道，前缀形如 [NpcDialog] / [TradeMenu] / [PlayerNpc]
xmake f --holo_diag=n && xmake build -y   # 发布前复位
```

开关在 `src/DiagLog.h`：`HLIB_LOG_INFO/WARN/ERROR/DEBUG` 四个宏用 `if constexpr` 统一收口，关闭时参数仍会被编译检查（格式串写错照样报错），但不生成任何代码。

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

// ── 1.21.0 追加的四个界面 / 查询域 ──

// 虚拟容器：任何输入设备都能点的列表界面（点击 = 一次物品拾取, 回传槽位号 = 条目下标）
hologramlib::ContainerMenuSpec menu;
menu.title = "§8任务列表";
menu.rows  = 3;                                  // 3 = 小容器 27 格; 6 = 大容器 54 格
menu.items.resize(27);
menu.items[0].type = "minecraft:diamond_sword";
menu.items[0].name = "§b试炼";
menu.items[0].lore = {"§7点击领取"};
auto menuId = lib.containerMenus().open("Steve", menu);
lib.containerMenus().addClickListener([](hologramlib::ContainerClickEvent const& e) {
    if (e.closed) return;                        // 界面关闭（此时 slot = -1）
    // e.slot = 被点击的条目下标
});
lib.containerMenus().setItem(menuId, 0, {"minecraft:barrier", 1, 0, "§c已完成", {}}); // 单格刷新: 无延迟无闪烁
lib.containerMenus().update(menuId, page2Spec);                                       // 换整页: 不拆界面不等待

// 村民交易菜单：纯展示（默认纯协议层 = 服务端无交易表, 天然只读）, 没有任何点击回调
hologramlib::TradeMenuSpec trade;
trade.tradeType = "entity.villager.butcher";
hologramlib::TradeMenuOffer offer;
offer.buyA.type = "minecraft:emerald"; offer.buyA.count = 3;
offer.sell.type = "minecraft:diamond"; offer.sell.count = 1;
offer.sell.name = "§b钻石";
trade.offers.push_back(offer);
auto tradeId = lib.tradeMenus().open("Steve", trade);
lib.tradeMenus().addOffer(tradeId, anotherOffer);      // 追加一条并就地重发（不必重开界面）
lib.tradeMenus().setTier(tradeId, 2, 40);              // 改显示栏值 / 经验条

// NPC 对话框：按钮表由服务端逐玩家生成, 点击 / 关闭回传（多层级按 sceneName 路由）
hologramlib::NpcDialogSpec dlg;
dlg.npcName = "§e村长";
dlg.dialogue = "要来点任务吗？";
dlg.buttons.push_back({"§a接受", {}, "main#0", 0});
dlg.buttons.push_back({"§c离开", {}, "main#1", 1});
auto dialogId = lib.npcDialogs().open("Steve", dlg);

// 感知域：按客户端设备分流（触屏 / 手柄 / 键鼠）
if (lib.playerSensing().isTouch("Steve")) {
    lib.containerMenus().open("Steve", menu);    // 触屏: 用虚拟容器, 点击必然发包
}
```

xmake 接入：

```lua
add_includedirs("../HologramLib/include")
add_linkdirs("../HologramLib/build/windows/x64/release")
add_links("HologramLib")
```

### LSE 脚本

```js
// 统一命名空间 "HologramLib", 十三个 LSE 前缀（其余能力域只有 C++ 接口）:
//   shape* / holo* / gradient* / itemDetail* / itemDisplay*
//   entity* / ghost* / particle* / playerNpc* / trade*
//   container* / npcDialog* / sensing*
const shapeCreateLine = ll.import("HologramLib", "shapeCreateLine");
const holoCreate      = ll.import("HologramLib", "holoCreate");
const itemDisplayCreateBeacon = ll.import("HologramLib", "itemDisplayCreateBeacon");
const particleMoveTo  = ll.import("HologramLib", "particleMoveTo");

// ── 1.22.0 起可用的界面 / 查询域（脚本侧）──
const containerOpen       = ll.import("HologramLib", "containerOpen");
const containerSetItem    = ll.import("HologramLib", "containerSetItem");
const containerPollClicks = ll.import("HologramLib", "containerPollClicks");
const sensingIsTouch      = ll.import("HologramLib", "sensingIsTouch");
const npcDialogOpen       = ll.import("HologramLib", "npcDialogOpen");
const tradeOpen           = ll.import("HologramLib", "tradeOpen");
const tradeAddOffer       = ll.import("HologramLib", "tradeAddOffer");

// 任务列表: 触屏也能点（点击就是一次物品拾取, 回传槽位号 = 条目下标）
const listId = containerOpen("Steve", "§8任务列表", 3);          // 3 = 27 格; 6 = 54 格
containerSetItem(listId, 0, "minecraft:diamond_sword", 1, 0, "§b试炼", "§7点击领取|§8第二行");

// 点击在守护/定时里取（取走并清空）: "player=Steve menuId=1 slot=0 closed=0"
for (const line of containerPollClicks()) { /* 自行切分处理 */ }

// 交易菜单（纯展示, 没有任何回调）: 打开后逐条补, 或直接摆满
if (!sensingIsTouch("Steve")) {
    const tid = tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
    tradeAddOffer(tid, "minecraft:emerald", 3, "", 1, "minecraft:diamond", 1, "§b钻石", 1, false);
}

// NPC 对话: 按钮串 = label|actionId|mode|命令1,命令2（按钮之间用 ; 分隔）
npcDialogOpen("Steve", "§e村长", "main", "要来点任务吗？", "§a接受|main#0|0|;;§c离开|main#1|1|", "");
```

完整函数清单见 [API.md](API.md)。

## 依赖

- LeviLamina
- [SculkCatalystMC/Protocol](https://github.com/SculkCatalystMC/Protocol)

## 许可

MIT 许可（见 [LICENSE](LICENSE)）
