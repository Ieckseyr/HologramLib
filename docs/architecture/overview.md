# 架构总览

这一页回答「它是怎么搭起来的」与「它为什么这么慢/这么快」。

> 想知道边界划在那里（什么能进库、什么不能）的原因，看 [设计说明](../design-contracts.md)。
> 想查具体线格式、发包原语、抓包验证，看 [协议层实现笔记](protocol-notes.md)。

---

## 分层

```
消费方
  ├─ LSE (JS):  ll.import("HologramLib", "holoCreate") ...
  └─ C++ 插件:  IHologramLib::getInstance().holograms() ...

        ▼
┌─ 契约层 ────────────────────────────────────────────────────┐
│ include/hologramlib/HologramLib.h                            │
│ 纯虚接口（IHologramLib + 各域接口），冻结 ABI，单向递增版本宏  │
├─ 桥接层 ────────────────────────────────────────────────────┤
│ src/HologramLibImpl.cpp    纯转发，无业务逻辑                 │
│ src/lse/LseBridge.h        exportAs → LegacyRemoteCall       │
│ src/*Exporter.cpp          统一命名空间 "HologramLib"，前缀分域│
├─ 内核层（跨域地基, 不含业务语义）─────────────────────────────┤
│ core/PacketSink.h    全库唯一发包出口: 编码 / 回读校验 / peer  │
│                      / 可靠性 / 失败计数 —— 域不自己碰 peer    │
│ core/TickDriver.h    全库唯一 Level::$tick hook + 显式顺序表   │
│ core/ServiceHost.h   域注册 / 启动 / 反向关停 / LSE 导出遍历   │
│ core/VirtualActorIds.h 虚拟 actor 的 runtimeId 号段（唯一定义）  │
│ core/Registry.h      域对象表: id 分配 / 索引 / 脏集 / 快照遍历  │
├─ 领域层 ────────────────────────────────────────────────────┤
│ PacketDebugRenderer        形状域，持有形状表 + 重发机制       │
│ FloatingTextManager        悬浮字（复用形状域，不直接发协议包）│
│ GradientLineManager        渐变线（自建线段批）               │
│ ParticleShapeManager       粒子形状（引擎同路径广播）          │
│ itemdisplay / playernpc / view / trade / container /          │
│ npcdialog / sensing / fakeinv / sulfur / itemdetail / ghost   │
├─ 协议层 ────────────────────────────────────────────────────┤
│ ProtocolPackets.cpp   DebugShape 列表 → PrimitiveShapesPacket │
│ SculkPacketSend.h     兼容层（两行转发到 core/PacketSink.h）   │
│ view/OutboundViewHook  NetworkSystem::send 钩子（只拦不发）    │
│                        心跳已并入 core/TickDriver             │
└──────────────────────────────────────────────────────────────┘
事件层: ll::event::EventBus (PlayerJoin / Disconnect / ServerLevelTick / ServerStarted)
        + LL_TYPE_INSTANCE_HOOK 实例钩子（出站发送 / 入站 AuthInput 等）
```

五个要点：

- 桥接层无状态：接口调用直接落领域层；LSE 导出与 C++ 接口走的是同一条路径，不存在「JS 专用逻辑」。
- 形状域是唯一「包里写形状」的出口：悬浮字、渐变线都复用形状域（悬浮字 = 一个多行文本形状；渐变线 = 一批线段形状）。
- 发送原语唯一：全库共用 `core/PacketSink.h`（`SculkPacketSend.h` 已是两行转发）。ItemDisplay 那份不计数、不告警的副本已删除 —— 收敛前它是唯一让「全库发包失败数」统计漏掉一整域的缺口。
- 每 tick 的驱动唯一：5 个 `Level::$tick` detour 收成一个（`core/TickDriver.*`），执行顺序由 `Order` 显式决定，不再依赖跨 TU 的静态初始化顺序。
- 生命周期唯一：每个域实现 `core::IDomain`（`start` / `stop` / `exportLse`），由 `core/ServiceHost` 统一遍历 —— 启动按声明顺序、关停反向、导出一遍。新增域只改 `ModEntry::enable()` 里的 `add` 一行。此前三张手工清单漏过一次导出（ghost），还有四个域的 `shutdown()` 从未被调用过。

---

## 内核化进度

`core/Registry.h` 把「id 分配 / 索引同生共死 / 脏集 / 快照遍历 / 锁」这套板子收敛成一个模板。
`Registry<T, Mutex>` 的锁策略默认自持 `std::mutex`；多表共享临界区的域传 `core::NoLock`，临界区仍由域自己持有。
下面 10 个域有主表，已迁 7 个。

| 域 | 状态 | 说明 |
|---|---|---|
| `sulfur` | ✅ 已迁 | 主表只有 `{entityId}`，无二级索引 |
| `gradientline` | ✅ 已迁 | 主表只有线段批 + 绘制标志 |
| `particles` | ✅ 已迁 | 主表按值存 `Shape`，迁移时换成 `unique_ptr`；`tick` 改成 forEachLocked + 到期集 |
| `shape`（`PacketDebugRenderer`） | ✅ 已迁 | 主表 + `networkId` 分配 + 进服/周期重发；`networkId` 换原子递减 |
| `hologramtext` | ✅ 已迁 | `NoLock` + 域自己的 `recursive_mutex`（674c7b3 的修复保持不变）；`mVariables` / `mLastSentText` 仍在同一临界区，`peek()` 负责取裸指针 |
| `itemdisplay` | ⛔ 先重构 | 与 id 同键的 **`mConfigs` + `mRuntimes` 两张并行表** + 脏集 + 跟随。只换容器不减少复杂度，真正的重构是把两张表并成一个 Entry。 |
| `customentity` | ⛔ 先重构 | 同上：`mConfigs` + `mRuntimes` + 脏集 + 逐玩家外观覆盖。 |
| `playernpc` | ⛔ 先重构 | 同上，另有 Tab 移除队列与逐玩家朝向/装备。 |
| `container` | ✅ 已迁 | `NoLock` 策略：`mMenus` 进 Registry，`mByPlayer` / `mPlayerEpoch` / `mIdFreedTick` 仍在域自己的临界区里 |
| `npcdialog` | ✅ 已迁 | `NoLock` 策略：`mDialogs` 进 Registry，`mByPlayer` / `mCarriers` 同上 |
| `fakeinv` | — 不适用 | 主键是玩家名（`string`），不是 `int64` id。 |

剩下的 3 个都是同一类：**需要先做域内重构**（`itemdisplay` / `customentity` / `playernpc`）。
瓶颈是**两张 id 同键的并行表**（`mConfigs` + `mRuntimes`），不是容器本身 —— 只换容器会得到一个
Registry 加一张仍手写的 `mRuntimes`，反而更难维护。先把两张表并成一个 `Entry`，再整体进 Registry。

**`Registry` 的适用边界**：主表就是该域的全部状态（或状态能整块搬进 `Entry`），且锁可以自持（默认策略）或由域持有（`NoLock`）。
不满足这两条的域，(a) 前提件（`hologramtext` 的旁表），或 (b) 先并表（那三个大域）。

---

## 源码目录

```
HologramLib/
├── include/hologramlib/HologramLib.h   # 唯一公开头（接口兼容性说明在顶部注释）
├── src/                                # 内部实现（不属于 API，可自由变更）
│   ├── HologramLibImpl.cpp             #   接口实现（委托各 Manager 单例）
│   ├── EventIdCompat.h                 #   事件 ID 对齐（MSVC 编译必需）
│   ├── DiagLog.*                       #   诊断日志开关（默认编译期关闭）
│   ├── core/                           #   内核层（跨域地基, 不含业务语义）
│   │   ├── PacketSink.*                #     全库唯一发包出口
│   │   ├── TickDriver.*                #     全库唯一 Level::$tick hook
│   │   ├── ServiceHost.h               #     域注册与生命周期（启动顺序 = 声明顺序）
│   │   ├── VirtualActorIds.h           #     0x6D/0x6E/0x6F 号段 + 不重叠 static_assert
│   │   └── Registry.h                  #     id 分配 / 索引 / 脏集 / 快照（4 个域已迁，见「内核化进度」）
│   ├── SculkPacketSend.h               #   兼容层（转发到 core/PacketSink.h, 下个大版本删）
│   ├── PacketDebugRenderer.*           #   形状渲染（协议层）
│   ├── ProtocolShape.h / ProtocolPackets.*
│   ├── FloatingTextManager.*           #   悬浮字（整块单形状 + 逐观看者形状）
│   ├── GradientLineManager.*           #   渐变线
│   ├── itemdetail/                     #   物品详情
│   ├── itemdisplay/                    #   FMBE 物品悬浮
│   ├── customentity/                   #   自定义实体（含逐客户端外观覆盖）
│   ├── particles/                      #   通用粒子形状
│   ├── ghost/                          #   Ghost 交互路由
│   ├── trade/                          #   村民交易菜单
│   ├── container/                      #   虚拟容器
│   ├── interaction/                    #   物品请求钩子（容器域与背包域共用）
│   ├── item/                           #   菜单条目栈（MenuItemStack，容器/交易共用）
│   ├── npcdialog/                      #   NPC 对话框
│   ├── sensing/                        #   感知域
│   ├── playernpc/                      #   假玩家 NPC（皮肤注册表 / PlayerList）
│   ├── fakeinv/                        #   背包虚容器
│   ├── sulfur/                         #   硫磺立方体展示
│   ├── view/                           #   客户端视图覆盖
│   ├── lse/                            #   LSE 兼容层（运行时挂载 lrca）
│   ├── MemoryOperators.cpp             #   跨 DLL 内存配对
│   └── ModEntry.*                      #   生命周期 + LSE 双时机挂载
├── abi/                                # 冻结契约基线（槽位表 JSON + slots.md），CI 比对
├── tools/                              # 纯 python 标准库工具（无第三方依赖）
│   ├── abi_tool.py                     #   契约提取 / 比对 / 消费方镜像生成
│   ├── test_abi_tool.py                #   契约工具自测（21 项）
│   └── check_format.py                 #   只查改动行的 clang-format 门禁
├── tests/                              # 离线检查与单测（已纳入版本库）
│   ├── registry_test.cpp               #   内核层 Registry 单测（g++ 直接跑，不依赖 BDS）
│   └── check-*.bat / *.py              #   字节级对拍夹具（需本机抓包/编译环境）
├── docs/                               # 本套文档
├── xmake.lua · manifest.json
└── README.md
```

---

## 客户端内存形状模型

这是理解全部渲染行为的地基：

- DebugDrawer 形状（形状域 / 文本 / 悬浮字 / 渐变线）只存在于客户端内存。服务端不维护它的显示状态，只维护「应发送的形状表」。
- 同 `networkId` 重复发送 = 客户端原地覆盖（幂等，无副作用）。全部重发策略都建立在这条性质上：重发不闪、不叠加、不需要先删后建。
- 移除 = 同 `networkId` 的空载荷形状（`makeRemoveShape`）。

> `destroyBatch` / `destroyAll` 在 `26.40.8` 起会补发移除包，此前只清服务端内存，导致客户端残留显示。

---

## 重发策略

客户端不落盘，重连 / 丢包都必须靠服务端补发。三个时机：

| 时机 | 行为 | 位置 |
|---|---|---|
| 玩家进服 | `PlayerJoinEvent` 后 +1s / +5s 两次延迟重发（客户端加载期丢包兜底） | `PacketDebugRenderer::init()` |
| 周期兜底 | `ServerLevelTickEvent` 每 tick 累计，每 15s 全员全量静默重发 | `tickResend()` |
| 无形状时 | 持锁快查 `mShapes.empty()` 直接返回 | `tickResend()` |

复杂度：每次全量重发 = `O(形状数 × 在线玩家数)` 的包构造 + 序列化，周期 15s。悬浮字的逐观看者形状同样走这套重发（每个观看者形状单独计入形状数）。

> ⚠ 规模上限的第一道墙就在这里。形状数到千级、玩家上百时，15s 全量重发的包构造量会变成可观测的负载。想估自己的场景，把「重发策略分层（按维度分组 / 脏标记）」列进需求，这在维护者的优化候选里，见 架构与开销评估（本地文档，不随库发布）。

---

## 悬浮字的组合模型

| 事实 | 说明 |
|---|---|
| 行 = 文本 | 多行合并为单个文本形状（`'\n'` 连接），整块一份颜色 / 缩放 / 背景框 / 穿墙 / 旋转 |
| 逐观看者形状只在必要时存在 | 文本含 `{var}` 且目标不是单玩家时，才按观看者各建一份形状（`rebuildTextShapesPerViewer`）；变量被清掉或改为单玩家目标时切回共享形状 |
| 跟随就地解析 | `26.40.8` 起：跟随位置在 `setFollowPlayer` / `draw` / `refresh` 调用时解析，无库内自驱 tick。按需 `refresh` 由消费方负责（示例：ZXPanel 每 2 tick 原地重发） |
| 永久悬浮字不设 duration | `mTotalTimeLeft` 保持空，设了会走客户端倒计时自动删除 |

---

## 自驱盘点：库内什么东西会自己动

「基础轮子」原则：库内自驱动只允许两类，送达保障（重发 / 心跳）与引擎同路径（粒子广播），不允许替消费方做动画。

| 自驱项 | 性质 | 结论 |
|---|---|---|
| 15s 兜底重发 / 进服重发 | 送达保障 | 保留 |
| 视图覆盖每 tick 心跳 | 送达保障（重推覆盖） | 保留 |
| 悬浮字动态行 0.5s 节流刷新 | 送达保障（内容变了才重发） | 保留 |
| 粒子域（采样 / 视距裁剪 / 周期重发） | BDS tick flush 聚合 Batch，与原版粒子广播同路径 | 保留 |
| `gradientTick` | 消费方驱动（LSE 导出，由脚本按 tick 调） | 保留 |
| 悬浮字自驱动画 / `holoTick` | 替消费方做动画 | `26.40.8` 已移除 |

判断一个「库内定时器」是否正当，就问：它是在保证「东西送到」，还是在替消费方决定「东西怎么动」？

---

## 视图覆盖的运行时结构

- 出站钩子：`NetworkSystem::send` / `sendToMultiple`，只决定放行 / 丢弃，从不修改引擎包字段。
- 心跳：`Level::$tick` 每 tick `tickPulse()`（实体重新进入视野 / 玩家换区块时重推覆盖）；挂点是全库唯一的 `core/TickDriver.cpp`，本域注册为 `Order::ViewOverride`（最后跑，覆盖套在已落地的实体状态上）。
- 数据来源只有协议包（入站 AuthInput、出生包载荷、Animate / ActorEvent），不读服务端实体状态（身份识别除外）。
- 判定逻辑独立在 `src/view/ViewOverrideLogic.h`，不碰 BDS 类型，离线可测（`tests/check-view-override.bat`，59 项）。

这条「把判定逻辑与引擎类型解耦，使其可离线测试」的做法在库内出现过三次：`ViewOverrideLogic.h`、`TradeSettlementLogic.h`（43 项）、`ContainerPackets.h`（逐字节对拍）。新增有判定逻辑的域时照着做。

---

## ABI：为什么「只在尾部追加」是硬约束

`IHologramLib`：新能力一律在类尾部追加虚函数，不改动 / 不重排既有方法，并递增 `HOLOGLIB_API_VERSION`。

这不是礼貌，是 ABI。虚函数在虚表里的槽位就是它的身份：

| 改动 | 后果 |
|---|---|
| 尾部追加 | 既有槽位不变 → 未重编译的消费方二进制安全 |
| 中段插入 / 删除 | 后面所有槽位整体前移 → 用旧头文件编译的消费方静默调用到错误的函数 |

> ⚠ 真实事故（已固化成机制）：`addActorInteractListener` / `removeActorInteractListener` 第一版被插在了 `IHologramLib` 中段（ghost 段之后），后面所有虚表槽位整体前移，用旧头文件编译的消费方（在 `playerNpcs()` 上取到了错的槽）直接 `0xC0000005`。
>
> 已移到类尾部，并在头文件里留了 ⚠ 说明。

`26.40.9` 起头文件携带虚表布局描述符 + 消费方编译期自检，库侧在运行期拦截布局不一致的消费方：把「中段挪槽」这类崩溃固化成机制化防护，而不是靠人记住。

### 机器守契约（26.51.6 起）

「只在尾部追加」不再只靠注释、版本表与记忆力 —— 契约有了基线文件和一个退出码：

| 命令 | 作用 |
|---|---|
| `python tools/abi_tool.py check` | 与 `abi/v1.31.0.json` 比对；中段插入 / 删除 / 改签名 / 改结构体 / 改枚举值 → **非 0 退出** |
| `python tools/abi_tool.py extract --json-out abi/v1.32.0.json` | 发版后固化新基线 |
| `python tools/abi_tool.py extract --md-out abi/slots.md` | 生成人读槽位表（消费方可以直接 diff 自己的镜像） |
| `python tools/abi_tool.py check -H <某份头文件>` | 拿任意一份头文件对基线（例：确认消费方手里的镜像是否还跟得上） |
| `python tools/abi_tool.py mirror --out HologramLibAbi.h` | 给消费方生成镜像头（同槽位序、`HOLOGLIB_API` 置空、命名空间可换成宏），生成后自证槽位表一致 |
| `python tools/test_abi_tool.py` | 工具自身的 21 项自测（中段插入 / 尾部追加 ±抬版本 / 结构体 / 枚举 / 镜像 / manifest 漂移），CI 一起跑 |

`.github/workflows/build.yml` 的 static-checks 在每次 push / PR 上跑前两条（ABI 契约 + 工具自测，外加内核层单测与格式门禁）。尾部追加会打印新增槽位并通过；
只要动了既有槽位，它在 CI 上红 —— 而不是在用户机器上 `0xC0000005`。
破坏槽位时如果 `HOLOGLIB_ABI_STAMP` 没跟着 +1，也判失败：那道运行期自检是最后一道防线，它失效必须先被发现。

### 收缩时怎么办

删除已发布的方法同样会动槽位。库内的两条处理办法（都是「明记收缩」）：

| 场景 | 处理 | 实例 |
|---|---|---|
| 删的是尾部方法 | 直接删，抬次版本 + 在版本表写明 | `26.40.3` 交易菜单的六个监听方法 |
| 删的是中段方法 | 保留为废弃空槽（非纯虚 + 内联空实现 + `[[deprecated]]`），虚表槽位不变，真正清槽留到大版本 | `26.40.8` 的 8 个悬浮字行级方法 |

两次收缩的原因与迁移方式见 [版本与 ABI 对照](../versions/api-version-matrix.md)，这套接口只在尾部追加的约定的完整说明见 [设计说明](../design-contracts.md)。

---

## LSE 兼容层：运行时挂载，不是硬依赖

HologramLib 不以 LegacyRemoteCall（`lrca`）/ MeowPAPI 为前置依赖：无链接时符号、无 manifest 声明。

它在运行时通过 `GetModuleHandle` + `GetProcAddress` 解析 `lrca` 导出的 **6 个** C++ 函数（`exportFunc` / `importFunc` / `hasFunc` / `removeFunc` / `removeNameSpace` / `onCallError`）。
解析分两级：先按精确的 MSVC mangled 符号取，取不到再按导出表前缀模糊匹配（跨 lrca 编译版本的 STL mangling 有差异）；`exportFunc` + `importFunc` 两个取到即算挂载成功。

| 情况 | 行为 |
|---|---|
| `lrca` 未加载 / 符号缺失 | `attach()` 返回 `false`，所有导出 / 导入调用安全空操作 |
| `lrca` 存在 | LSE 脚本 `ll.import` / `ll.hasExported` 用法与旧版完全一致 |
| 原生 C++ 接口 | 不经过本桥，不受 `lrca` 存在与否影响 |

挂载是双时机的：`enable()` 时试一次，没成功就监听 `ServerStartedEvent` 兜底重试（届时所有插件均已加载）。这也是「启动顺序无任何要求」的原因。


---

## 相关

- 线格式、发包原语、抓包验证：[协议层实现笔记](protocol-notes.md)
- 边界与纪律：[设计说明](../design-contracts.md)
- 维护者视角的开销评估与优化候选：架构与开销评估（本地文档，不随库发布）
