# 能力域总览

HologramLib 的能力域按用途拆成 10 页。这一页管选型：先确定用哪个域，再进对应的域指南看怎么用。把某个东西摆到世界里在本库里有六条不同的路（形状渲染、悬浮字全息、渐变线、FMBE 物品悬浮、硫磺立方体展示、自定义实体），观感、开销与能力各不相同，先看下面这张表：

| 页 | 内容 |
|---|---|
| [DebugShape（悬浮字与形状）](debugshape.md) | 形状渲染 · 悬浮字全息 · 渐变线 |
| [物品显示](item-display.md) | FMBE 物品悬浮 · 物品详情 · 拉伸 / live 变换 / 动画预设 |
| [粒子](particles.md) | 粒子形状 |
| [实体与交互](entities.md) | 自定义实体 · Ghost 交互 · 真实实体交互事件 |
| [假玩家 NPC](player-npc.md) | 假玩家 · 皮肤 / 图片平面 · 姿势 / 装备 / 骑乘 · 交互 |
| [容器与背包](containers.md) | 虚拟容器 · 背包虚容器 · 动态刷新会话隔离 |
| [界面](interfaces.md) | 村民交易菜单 · NPC 对话框 |
| [硫磺立方体](sulfur.md) | 硫磺立方体展示 |
| [视图覆盖](view.md) | 客户端视图覆盖 · 千人千面 |
| [客户端感知](sensing.md) | 输入设备感知 |

---

## 选型：我要在一个位置显示东西

| 我想显示 | 用哪个域 | 观感 | 代价 |
|---|---|---|---|
| 线框、几何体、一段文字 | 形状渲染 `IShapeDrawer`（`IShapeDrawer::createText` 分支） | 发光的调试线条 / 文字 | 客户端内存形状，不产生实体，零服务端开销 |
| 一块多行信息牌 | 悬浮字全息 `IHologramText` | 始终面向相机（可关）的文本块 | 同上；本质是「一个多行文本形状」 |
| 一条彩色轨迹线 | 渐变线 `gradient*`（仅 LSE） | 多色渐变线段 | 一批线段形状 |
| 一个物品图标 | FMBE 物品悬浮 `IItemDisplay` | 一只狐狸举着物品（或信标形态） | 假实体（部分隐身、仅目标可见）+ 发包 |
| 一个方块被吞着 | 硫磺立方体展示 `ISulfurDisplay` | 方块悬在那里，立方体默认隐身 | 完全委托 `ICustomEntity`；外观档位走 `ChangeMobProperty` |
| 一只原版生物 / 实体 | 自定义实体 `ICustomEntity` | 真实实体外观：姿态、装备槽、动画、骑乘 | 协议层实体（无 AI），服务端无对应实体 |
| 一个人形（玩家模型） | 假玩家 NPC `IPlayerNpc` | 玩家模型，皮肤可注册 / 采集 / 注入 | 纯协议假玩家；皮肤走 PlayerList |
| 让已经存在的生物长得不一样 | 客户端视图覆盖 `IViewOverride`（`EntityView::skinId`） | 换成你的模型，id 不变 | 只改客户端视图；服务端仍认它是原来那只生物 |

### 关键区别：改「客户端视图」还是「造新东西」

这是最容易选错的一处：

- 造一个新东西（自定义实体 / 假玩家 NPC / 硫磺立方体 / FMBE 物品悬浮）服务端没有对应的真实实体，玩家看得到但碰不到实体逻辑（`sulfur` 的立方体、`itemDisplay` 的狐狸都是这样）。
- 改已有东西的客户端视图（`IViewOverride`）不新增实体，只改这名玩家客户端看到的包。被改的实体还是服务端那个真身：点它 / 打它 / 瞄准它都是它本人，伤害、掉落、其他插件看到的全是真身。代价是它只随出生包来过一次的细节（变体、装备初始态）要等服务端下次同步。

「给生物换自定义模型」的正路是 `EntityView::skinId`（`26.40.9` 起），不是「隐形真身 + 逐 tick 跟随 + ghost 转发攻击」那套。后者是 PlayerNpc 载体路线的老办法，需要自己处理显隐决策、跟随与攻击转发。

---

## 选型：我要一个玩家能 interacted 的界面

| 需求 | 用哪个域 | 为什么 |
|---|---|---|
| 列表界面：点条目就有回调，任何输入设备都能点 | 虚拟容器 `IContainerMenu` | 点击 = 一次物品拾取，触屏也会发包；小 27 格 / 大 54 格 |
| 摆一张村民交易表给玩家看 | 村民交易菜单 `ITradeMenu` | 协议层 `UpdateTrade`，包体与 BDS 抓包逐字节一致 |
| 交易要真的扣款发货 | `ITradeMenu` + `settleLocally`，或 `usePacketOffers=false` | 前者库自己结算，后者交给 BDS 结算（推荐后者，见 [界面与容器](interfaces.md#三条路径)） |
| 多层级对话 / 带按钮的 NPC 剧情 | NPC 对话框 `INpcDialogue` | `NpcDialoguePacket` + 合成载体；按场景名路由层级 |
| 让客户端「以为自己背包里有这些东西」 | 背包虚容器 `IFakeInventory` | 协议层改写客户端看到的背包内容；服务端背包一个字不动 |
| 想知道玩家用什么设备，按设备分流 | 感知域 `IPlayerSensing` | 逐包捕获 `InputMode`：触屏 / 手柄 / 键鼠 |

共性边界：这些「虚拟界面」在服务端并不存在。玩家在界面里点东西，服务端那格的真实物品对不上，BDS 会判失败并让客户端把预测撤回。库不代答失败应答，代答会让客户端弹错误提示，交给 BDS 走它自己的失败路径反而是安静的。

---

## 选型：我要粒子 / 临时特效

| 需求 | 用哪个域 |
|---|---|
| 点、线、矩形环、填充面、盒框、六面、多面体 | 粒子形状 `IParticleShape` |
| 整面随锚点平滑移动、旋转、自旋、跟随 | 同上：`moveTo` / 旋转 / `setFollowPlayer` |
| 只给部分玩家看 | 同上：`setVisiblePlayers` 白名单（空 = 维度全员） |
| 逐客户端音效、短命飘字 | 没有这个能力：`26.40.3` 已整体删除，见 [更新日志](../versions/changelog.md) |

粒子发包走 vanilla `SpawnParticleEffectPacket` 批量通道，BDS 的 tick flush 会自动聚合成单个 Batch 数据报。

---

## 域之间的依赖

绝大多数域互相独立，只有这几条关系需要注意：

| 域 | 关系 |
|---|---|
| 硫磺立方体 `sulfur` | 完全委托 `ICustomEntity` 实现：缩放 / 视距 / 白名单 / 逐客户端朝向直接继承，新增的协议内容只有 `ChangeMobProperty` |
| 悬浮字 `holo` | 复用形状域，不直接发协议包（悬浮字 = 一个多行文本形状） |
| 渐变线 `gradient`（LSE） | 同样是自建线段批，走形状域 |
| 千人千面 / 逐玩家变量 | 是 `ICustomEntity` / `IHologramText` 上的逐观看者覆盖，不是独立域 |
| 容器 / 背包虚容器 | 共用同一套物品请求钩子（`src/interaction/PlayerInteractionHooks.cpp`），靠容器 id 区分互不抢 |
| 视图覆盖 | 需要皮肤时读 `IPlayerNpc` 的注册表（`registerSkin` / `captureSkin` / `registerSkinFromBlob`） |

---

## 每个域的「最低版本」

写 `static_assert` 门时用这些值：

| 版本 | 新增 |
|---|---|
| `0x010900` · 1.9.0 | 早期域：`IShapeDrawer` / `IHologramText` / `IItemDetail` |
| `0x011500` · 1.15.0 | `IParticleShape` |
| `0x011900` · 1.19.0 | 皮肤内存导出 / 目录导入、`geometryData` 自定义模型、NPC 缩放 |
| `0x011A00` · 1.20.0 | 适配 26.40；逐客户端朝向 |
| `0x011B00` · 1.21.0 | `ITradeMenu` / `INpcDialogue` / `IContainerMenu` / `IPlayerSensing` + 逐客户端渲染 |
| `0x011C00` · 1.22.0 | 交易菜单改纯展示；四域补 LSE 导出 |
| `0x011D00` · 1.23.0 | `IFakeInventory` / `ISulfurDisplay` |
| `0x011E00` · 1.24.0 | 交易菜单纯协议真结算 |
| `0x011F00` · 1.25.0 | `IViewOverride` |
| `0x012000` · 1.26.0 | 视图覆盖残骸清理；`IPlayerNpc` 追加五方法；悬浮字收缩与补全 |
| `0x012100` · 1.27.0 | `EntityView::skinId`；真实实体交互监听；ABI 布局戳 |
| `0x012300` · 1.29.0 | `AddItemActor` 纳入拦截；`EntityView` 补姿态 / 缩放 / 变种 / 颜色 |
| `26.51.5` | 1.31.0 | `0x012500` | **26.51 线合并 26.40.12**：拉伸、live 变换、动画预设、停止配方、假玩家姿势/装备/骑乘、FMBE 抽搐修复、图片平面与透明图层、容器会话隔离、PlayerSkin trusted 与皮肤 Id 重建。 |
| `26.51.4` | 1.30.0 | `0x012400` | **只加 3 个纯原语/通知，不含任何功能实现**：`viewPollSpawnEvents`（出生事件通知） · `itemDisplayCreateFull`（建展示即带 NBT/光效） · `itemDisplayEmitForEntity`（让某玩家把某只已有实体看成 FMBE 模型）。判定/状态/策略一律留在消费方（`ItemPhys`）。只加 LSE 导出，虚表未动 → `HOLOGLIB_ABI_STAMP` 不变。 |

注：`0x012200`（= API 1.28.0）没有出现在上表里，该编号被跳过、从未作为发布版本出现过。原因与说明见 [版本与 ABI 对照](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md)。

完整对照（含两次「明记收缩」的原因与迁移方式）见 [版本与 ABI 对照](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.md)。
