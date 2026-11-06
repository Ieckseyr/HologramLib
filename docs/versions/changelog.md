# 更新日志

按发布版本倒序排列。每个条目给出 API 版本、新增或变更的导出、行为变更，以及是否需要重新编译 C++ 消费方。
发布版本与 API 版本的对应关系见[版本与ABI对照.md](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md)（维护者文档，不上文档站）。

## 版本概览

| 发布版本 | API | 协议线 | 主要内容 | 新增导出 | C++ 消费方 |
|---|---|---|---|---|---|
| `26.60.2` | 1.35.0 | 26.60 | **双平台拆分**：`core/`（协议编码 + C ABI）与 `platform/levilamina`、`platform/endstone` 两个外壳；endstone 端落地 **444 个导出**（404 真实生效 / 40 受限）；`holo_core.dll` 导出 67 个 C 符号供三方共用；`build.cmd` 一次构建两个产物；文档站重构；`lib/exports.json` 补齐 20 条此前未收录的导出。**C++ 行为与 ABI 未变。** | 0（清单补齐 20） | 无需重新编译（API 1.35.0 未变） |
| `26.60.1` | 1.35.0 | 26.60 | 死亡动画后的复活 `IPlayerRespawn`（补发 `RespawnPacket`）、容器预览 `containerPreview`、附魔条目 `containerSetItemEnchanted`、表现事件阻断 `actorBlock*`、假实体 runtimeId 查询；**LSE 导出面补全**（particle 查询/可见性 5 个、真实实体交互事件 `actor*` 2 个、`containerUpdate` / `tradeUpdate`）；内部重构（PlayerIndex、统一发包出口、全库注释清零）。 | 27 | **需重新编译**（新增 3 个接口，尾部追加，`HOLOGLIB_ABI_STAMP` 仍为 1） |
| `26.60.0` | 1.34.0 | 26.60 | 26.60 平台适配：字段级编码重写、NPC 对话载体修正、发包审计 `wireTrace*`、虚假实体 runtimeId 查询 | 10 | **需重新编译**（新增 1 个接口；尾部追加，`HOLOGLIB_ABI_STAMP` 仍为 1） |
| `26.51.7` | 1.33.0 | 26.51 | 形状扩展（圆柱/金字塔/椭球/圆锥、最远渲染距离、挂载实体）+ Boss 条 + HUD 显隐 + 方位条扩展 + 受伤死亡动画 + 侧边栏计分板 | 48 | **需重新编译**（新增 6 个接口；尾部追加，`HOLOGLIB_ABI_STAMP` 仍为 1） |
| `26.51.6` | 1.32.0 | 26.51 | 内部重构（发包 / tick / 生命周期收敛）、ABI 契约工具、支付规划修复、三个新域（掉落物 / 表单关闭 / 方位条） | 16 | **需重新编译**（虚表尾部追加：3 个新接口 + `IContainerMenu` / `IPlayerNpc` 各加方法） |
| `26.51.5` / `26.40.12` | 1.31.0 | 26.51 · 26.40 | 拉伸 / live 变换 / 动画预设扩展 / 停止配方 / 假玩家姿势装备骑乘 / FMBE 抽搐修复 / 图片平面与透明图层 / 容器会话隔离 | 若干 | 建议重新编译（虚表尾部追加） |
| `26.40.11` / `26.51.4` | 1.30.0 | 26.40 · 26.51 | 三个协议原语 + 一个时机通知 | 3 | 无需重新编译（虚表未变） |
| `26.40.10` / `26.51.3` | 1.29.0 | 26.40 · 26.51 | 视图覆盖补齐 `AddItemActor` 拦截、姿态/缩放/变种/颜色；修复元数据 tick | 1（`viewItemSpawnHooked`） | 需重新编译（`EntityView` 结构体变大） |
| `26.40.9` | 1.27.0 | 26.40 | `EntityView::skinId`（同 id 换模型）、真实实体交互事件、ABI 布局戳 | 类方法 | 需重新编译（虚表追加） |
| `26.40.8` | 1.26.0 | 26.40 | 逐观看者覆盖、四个界面域 | 若干 | 需重新编译 |

## 26.60.2 — API 1.35.0（双平台拆分 / endstone 端 / ABI）

这一版**没有改协议、没有改 C++ 接口**，改的是仓库结构：协议编码抽成平台无关的 `core/`，
LeviLamina 与 endstone 变成两个外壳，共用同一份字节。

### 仓库拆分

```
core/                  协议编码 + shapes/registry + C ABI（平台无关）
platform/levilamina/   LL 原生插件 + LSE 兼容层
platform/endstone/     endstone Python 插件（内嵌 holo_core.dll）
build.cmd / build.py   一条命令构建两个平台的产物
```

`HologramLib.dll` 与 endstone 的 `holo_core.dll` 编自同一份 `core/`，形状、悬浮字这些能力
在两个平台发出的包体完全一致，不再有「两边各写一套」的漂移。

### endstone 端

| 项 | 数值 |
|---|---|
| 导出覆盖 | **444 / 444**（404 真实生效 · 40 受限实现，逐条写在各模块 docstring 与文档） |
| 离线测试 | **557 项**（`platform/endstone/tests/test_shape_api.py`，不需要服务器） |
| 调用方式 | `lib.holograms.add_line(...)` 这类 Python 属性，方法名 = LSE 导出名去前缀转 snake_case |

### ABI

`holo_core.dll` 导出 **67 个 C 符号**：30 个包 id 查询 + 34 个编码器 + `holo_core_api_version` /
`holo_core_version` / `holo_free`。endstone 的 C++ 插件、Python（ctypes）、LeviLamina（静态链接）
三方吃同一层，`core/c_api.h` 可直接 include，也可以 `GetProcAddress`。

### 文档站

拆成四节：**LeviLamina**（LSE 脚本接口 + C++ 接口入口）、**Endstone**（29 个域页 + 444 条 API 索引）、
**ABI 接口**（符号清单 + LeviLamina 的 C++ 包装）、**能力域**。

### 导出清单补齐

`lib/exports.json` 补了 20 条此前没收录的导出（LL 侧早就有，清单是旧的）：
`actorFreeze` / `actorPlayAnimation` / `actorPlayAnimationFull` / `actorPlayTotem` / `actorSetPose` /
`actorUnfreeze`、`blockFake*` 5 个、`easterEgg*` 2 个、`playerNpcRegisterImageSkinSized` /
`playerNpcSetImageSize`、`player*` 表现 5 个。导出数 424 → 444。

### C++ 消费方

**不需要重新编译**：`HOLOGLIB_API_VERSION` 仍是 `0x012900`（1.35.0），虚表没有变动，
ABI 基线仍是 `abi/v1.35.0.json`。

## 26.60.1 — API 1.35.0（容器预览 / 表现事件阻断 / 复活包 / 导出面补全）

### 死亡动画后的复活（`IPlayerRespawn` / `playerRespawn*`）

给玩家播死亡动画之后，客户端会进入死亡界面并向服务端发 `PlayerAction(Respawn)`；但死亡动画只是发给客户端的 `ActorEvent::DEATH`，服务端实体从未死亡，于是服务端认为不需要复活、**不发出 `RespawnPacket`** —— 客户端一直卡在死亡界面。

新增 `IPlayerRespawn` 补这一个包：

| 函数 | 说明 |
|---|---|
| `playerRespawn(playerName)` | 原地复活（`state = 2` ClientReadyToSpawn） |
| `playerRespawnState(playerName, state)` | 指定状态：`0` / `1` ReadyToSpawn / `2` ClientReadyToSpawn |
| `playerRespawnAt(playerName, x, y, z)` | 指定坐标复活 |

服务端实体的血量 / 位置 / 维度 / 背包都不变，也不会触发死亡或重生事件。
协议：`RespawnPacket`(45) = `Position`(Vec3) + `State`(uint8) + `PlayerRuntimeId`(uvarint64)。

### LSE 导出补全（本版新增的最后 9 个导出）

把「C++ 有、脚本没有」的剩余缺口补齐。补完这一版，公开头文件里**所有能被脚本表达的方法**和**所有 9 类事件**在 LSE 侧都有入口：

| 新增 | 说明 |
|---|---|
| `particleExists(id)` | 粒子形状是否还活着（寿命到点会立刻变 `false`） |
| `particleGetAllIds()` | 活着的粒子形状 id 数组 |
| `particleDestroyAll()` | 销毁全部粒子形状（全局，无归属区分） |
| `particleSetVisiblePlayers(id, uuids)` | `particleSetVisible` 的数组写法；注意粒子域白名单是**玩家 uuid**，不是名字 |
| `particleClearVisiblePlayers(id)` | 取消白名单，恢复维度全员可见 |
| `containerUpdate(menuId, title)` | 按当前 spec 整菜单重发；`title` 空串 = 保留原标题（`containerSetTitle` 少了「不知道原标题也想重发」这种用法） |
| `tradeUpdate(menuId, tradeType, tier, experience)` | 就地换贸易类型并整表重发；空串 / `<= 0` / `< 0` 分别表示保留该字段 |
| `actorPollInteractions()` | **新事件**：玩家右键/攻击**真实实体**（非库协议实体）的交互队列，`player=… action=… runtimeId=… uniqueId=… type=… pos=(…)` |
| `actorClearInteractions()` | 清空上面的队列（与 `ghostClearInteractions` 对称） |

两点边界说明：

- `actorPollInteractions` 与 `ghostPollInteractions` **互补**：命中库协议实体（自定义实体 / 物品展示 / 假人 NPC）的交互进 `ghost*`，命中真实实体（原版生物 / 玩家）的进 `actor*`，两者不会重复入队。队列上限同为 256，满了丢最旧的。
- 仍然**没有**脚本入口的只剩三类：`ICustomEntity::get` / `IPlayerNpc::get` / `IItemDisplay::get` 这种**出参结构体**（等价信息走 `*GetInfo` 的 JSON/摘要串），`add*Listener` / `setEntitySpawnCallback` 这类**回调**（脚本侧统一改成轮询队列），以及 10 个 `holoSetLine*` / `holoTick` 里已废弃的 7 个空实现。

### C++ 消费方

本版相比 26.51.7，26.60 这条线一共新增 3 个接口（`IPlayerRespawn` / `IContainerPreview` / `IActorEffectFilter`），全部在末尾追加；`HOLOGLIB_API_VERSION` 从 `0x012800` 抬到 `0x012900`，`HOLOGLIB_ABI_STAMP` 仍为 1，ABI 基线 `abi/v1.35.0.json`。带旧头文件的消费方**需要重新编译**。
导出 397 → 424。

### 修了哪些问题

这批都是审查时翻出来的毛病，只动内部实现，没加接口、没动 ABI：

- 换肤（给实体设皮肤）会把服务端卡死。
- 掉落物被捡走以后可能崩服。
- 虚拟容器开得多了，之后所有箱子都打不开。
- 关服或重载时，交易里的东西会丢。
- 交易结算队列只进不出，越堆越多。
- 背包合并会超过物品本身的堆叠上限。
- 账本把不同的物品当成同一种，数量还算错。
- 玩家重生后视图偏移不对，覆盖状态也会被误清。
- 全局覆盖有几条改包不生效。
- 个别情况下玩家会在别人客户端上凭空消失。

### 打包：`lrca` 改为可选

- **`tooth.json` 不再硬依赖 `lrca`**：删掉 `github.com/LiteLDev/LegacyRemoteCall: 0.22.*`，`lip install` 不会再顺带装 LegacyRemoteCall（原来还会装成与服务端实装不一致的 0.22）。`manifest.json` 的 `optionalDependencies` 保留，供 LeviLamina 记录加载顺序；脚本侧仍需 `lrca` 才能用导出，缺它时是安全空操作。

## 26.60.0 — API 1.34.0（26.60 平台适配）

适配 LeviLamina 26.60 / BDS 1.26.60（协议 2225）：`AddActor` / `AddPlayer` / `PlayerList` / `PlayerSkin` / `BossEvent` / `SetScore` 按 26.60 的 schema 重写；NPC 对话的合成载体不再绕过统一发包出口（之前少发 26.60 新增的可选字段，客户端解析错位会直接掉线），容器和交易两条路径也一起收敛到同一个出口。新增发包审计 `wireTrace*`，以及假实体 runtimeId 查询（`playerNpcRuntimeId` / `itemDisplayRuntimeId` / `itemActorRuntimeId`）。

### C++ 消费方

新增 1 个接口（`IPlayerRespawn`），追加在头文件尾部、访问器追加在 `IHologramLib` 尾部，`HOLOGLIB_ABI_STAMP` 仍为 `1`，但**需要重新编译**。ABI 基线 `abi/v1.34.0.json`。
导出 387 → 397。

> `abi/v1.34.0.json` 和这一版的构建产物里已经有 `IPlayerRespawn`；它的用法说明统一写在 26.60.1 的「死亡动画后的复活」一节。

## 26.51.7 — API 1.33.0（形状扩展、Boss 条、HUD、实体表现事件、侧边栏）

### 形状（`IShapeDrawerExt` / `shape*`）

协议层一直支持 10 种形状，这一版把漏掉的四种补上，并开放两个一直没用上的字段。

| 新增 | 说明 |
|---|---|
| `shapeCreateCylinder(x, y, z, radiusX, radiusZ, height, segments)` | 圆柱；半径是两个 Vec2（Radius X / Radius Z） |
| `shapeCreatePyramid(x, y, z, width, height, depth)` | 金字塔，底面 `width × depth` |
| `shapeCreateEllipsoid(x, y, z, radiusX, radiusY, radiusZ, segmentsPerAxis)` | 椭球，三轴半径独立 |
| `shapeCreateCone(x, y, z, radiusX, radiusZ, height, segments)` | 圆锥，X/Z 半径不同可做椭圆底面 |
| `shapeSetMaxRenderDistance(id, distance)` | 超出距离客户端不再绘制；`<= 0` 取消限制 |
| `shapeAttachEntity(id, entityUniqueId)` / `shapeClearAttachedEntity(id)` | 挂到实体上，客户端跟随，不必每 tick 改坐标 |

脚本侧形状编号：`0..5` 沿用旧值、`6` 是历史空洞、新增从 `7` 起（`7` 圆柱 / `8` 金字塔 / `9` 椭球 / `10` 圆锥）；
协议侧是 `6..9`。两套编号的映射只在 `src/shape/ShapeIdMap.h` 一处维护，并配离线测试。

`shapeCreate*` 只登记形状，必须再 `shapeDraw(id)` 才会发包 —— 这一点和其余形状一致。

### Boss 条（`IBossBar` / `bossBar*`）

按玩家维护的血量条，协议 `BossEventPacket`：

| 函数 | 说明 |
|---|---|
| `bossBarAdd(playerName, title, healthPercent, color, overlay)` | 创建；`healthPercent` 是 0.0~1.0 的比例 |
| `bossBarSetTitle` / `bossBarSetHealth` / `bossBarSetStyle` | 改名 / 改血量比例 / 改颜色与样式 |
| `bossBarRemove` / `bossBarRemoveAll` / `bossBarCount` | 删除 / 全部删除 / 计数 |
| `bossBarGetInfo(id)` / `bossBarProbe()` | 诊断用：看字段值与手写包体的校验结果 |

客户端的条必须对应一个它认识的真实实体，所以建条时库会在玩家所在维度的虚空里（`y = -120`）
生成一只 `minecraft:structure_void` 的无模型掉落物当锚点，删除时一并回收 —— 不占玩家视野，
调用方不需要管实体。

### HUD 显隐（`IHud` / `hud*`）

`SetHudPacket` 的 13 项元素全部开放：纸娃娃、护甲、提示、触屏控制、准星、快捷栏、血条、
进度条、饥饿、氧气泡、坐骑血量、状态效果、物品名。协议只有 `Hide` / `Reset` 两态，
所以 `visible=true` 发的是 `Reset`。

### 实体表现事件（`IActorEffect` / `actor*`）

`ActorEventPacket`：`actorPlayHurt` / `actorPlayDeath` / `actorPlayInstantDeath`，外加通用
`actorPlayEvent(runtimeId, eventId, data)` 与只发给单名玩家的 `actorPlayEventFor`。
另补 `actorRuntimeIdOfPlayer` / `actorRuntimeIdOfEntity`，脚本侧才有办法拿到 runtimeId。

### 侧边栏计分板（`ISidebar` / `sidebar*`）

只发包、不动服务端记分板。objective 名按玩家派生，行是假玩家条目，`score` 只作排序键。

| 函数 | 说明 |
|---|---|
| `sidebarSet(playerName, title, [lines])` | 显示或更新；空数组等于移除 |
| `sidebarSetTitle` / `sidebarSetLine` | 改标题 / 改某一行 |
| `sidebarRemove` / `sidebarRemoveAll` | 移除 |
| `sidebarIsShown` / `sidebarLineCount` | 查询 |

### 方位条扩展（`ILocatorBarExt`）—— 接口可用，客户端不渲染

`locatorBarSetIcon` / `SetIconSize` / `AttachEntity` / `ClearAttach` / `SetGroup` / `SyncGroup`
与两个通用导出（`anchorAcquire` / `anchorRelease`）都在，包也能发出去并通过回读校验，但
**客户端不画服务端下发的航点**：这条 HUD 只显示服务端"玩家方位"系统里注册过的玩家
（正常进服的玩家有，本库的假玩家也没有）。服务端注册要经过 `ServerWaypoint` /
`ServerWaypointGroup`，这两个类没有导出构造函数，插件侧走不通原生路径。

做标记点、玩家方位这类需求请用 Boss 条或侧边栏。

### 其它

- `wireStats()` / `wireResetStats()`：发包总账（`encoded` / `sent` / `noPeer` / `invalid`），
  `invalid` 增长说明有包过不了 BDS 回读校验
- `viewUniqueIdOf` 改为直接查在线玩家表（原来走视图覆盖注册表，普通玩家恒 `-1`）
- 虚空锚点实体池（内部）：`minecraft:structure_void` 掉落物，放在 `y = -120`

### C++ 消费方

新增 6 个接口（`IShapeDrawerExt` / `IBossBar` / `IHud` / `ILocatorBarExt` / `IActorEffect` / `ISidebar`），
全部追加在头文件尾部，`HOLOGLIB_ABI_STAMP` 仍为 `1`，但**需要重新编译**：旧头文件里没有这些访问器。
ABI 基线 `abi/v1.33.0.json`。

## 26.51.6 — API 1.32.0（内部重构、三个新域与修复）

### 新增域（API 1.31.0 → 1.32.0）

只加能力，没有改动既有行为。**虚表尾部追加**：3 个新接口 + `IContainerMenu` / `IPlayerNpc` 各加 1~2 个方法，
`HOLOGLIB_ABI_STAMP` 仍为 1；带旧头文件编译的 C++ 消费方需要重新编译（`abi/v1.32.0.json` 已更新）。
LSE 导出 298 → **339**。

#### 新域

- **掉落物 `IItemActor`**（`itemActor*` 8 个）：在客户端显示掉落物，服务端没有对应实体。
  `setMotion` 可随时修改速度与方向，`take` 播放拾取动画后删除。
  物品只含 id、数量与 aux，不携带 NBT；需要自定义名称用 `itemDisplay*`。
  实体号段新增 `0x70`，不参与交互事件路由。
- **表单关闭 `IFormControl`**（`form*` 2 个）：发送 `ClientboundCloseForm` 关闭客户端当前表单。
- **隐身 `IPlayerVisibility`**（`playerHide*` 5 个）：控制其他客户端是否接收某个玩家的数据包。
  掩码可分别屏蔽出生 / Tab / 移动 / 状态 / 皮肤；`setHidden` 使已出生的玩家立即消失；
  `restore` 清除记录并重新发送出生包。
- **方位条 `ILocatorBar`**（`locatorBar*` 6 个）：按玩家维护航点，`key` 由调用方命名，库据此生成 UUID，
  更新与删除无需保存 UUID。`hideFromOthers` 转发到 `IPlayerVisibility`。

#### 既有接口尾部追加

- `IContainerMenu::blockEvent(menuId, b0, b1)`（`containerBlockEvent`）：在假箱子坐标发送 `BlockEvent`。
- `IPlayerNpc::playEmote(id, pieceId, lengthTicks)`（`playerNpcPlayEmote`）：播放原版表情动作，
  只发送给已见过该 NPC 的玩家。

#### 脚本导入层

发布包内新增 `lib/HologramLib.js`、`lib/HologramLib.d.ts`、`lib/exports.json`。
脚本可用 `require("mods/HologramLib/lib/HologramLib.js")` 后按分组或按函数名调用，
也可继续使用 `ll.import`。函数首次使用时才与库绑定；缺少 `lrca` 或函数不存在时调用被忽略并记录警告。

另有 `holoInfo()` / `holoApiVersion()` 两个版本信息导出。

#### 工具

- `tools/test_abi_tool.py` 的"尾部追加"用例改成**动态取最后一条虚函数**作锚点 —— 之前写死字符串，
  头文件尾部一长就插到中间去，自测反而误报。

#### 验证

`xmake build` 通过；`registry_test` / `click_event_test` / `abi_tool check` / `test_abi_tool` /
`check_exports --check-docs` 全过。
