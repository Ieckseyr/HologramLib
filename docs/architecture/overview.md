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
├─ 领域层 ────────────────────────────────────────────────────┤
│ PacketDebugRenderer        形状域，持有形状表 + 重发机制       │
│ FloatingTextManager        悬浮字（复用形状域，不直接发协议包）│
│ GradientLineManager        渐变线（自建线段批）               │
│ ParticleShapeManager       粒子形状（引擎同路径广播）          │
│ itemdisplay / playernpc / view / trade / container /          │
│ npcdialog / sensing / fakeinv / sulfur / itemdetail / ghost   │
├─ 协议层 ────────────────────────────────────────────────────┤
│ ProtocolPackets.h    DebugShape 列表 → PrimitiveShapesPacket  │
│ SculkPacketSend.h    发送原语（全库共享）                      │
│ view/OutboundViewHook  NetworkSystem::send 钩子（只拦不发）    │
│                        + Level::$tick 心跳                    │
└──────────────────────────────────────────────────────────────┘
事件层: ll::event::EventBus (PlayerJoin / Disconnect / ServerLevelTick / ServerStarted)
        + LL_TYPE_INSTANCE_HOOK 实例钩子（出站发送 / 入站 AuthInput 等）
```

三个要点：

- 桥接层无状态：接口调用直接落领域层；LSE 导出与 C++ 接口走的是同一条路径，不存在「JS 专用逻辑」。
- 形状域是唯一「包里写形状」的出口：悬浮字、渐变线都复用形状域（悬浮字 = 一个多行文本形状；渐变线 = 一批线段形状）。
- 发送原语唯一：除 ItemDisplay 有一份独立副本（已知技术债）外，全库共用 `SculkPacketSend.h`。

---

## 源码目录

```
HologramLib/
├── include/hologramlib/HologramLib.h   # 唯一公开头（接口兼容性说明在顶部注释）
├── src/                                # 内部实现（不属于 API，可自由变更）
│   ├── HologramLibImpl.cpp             #   接口实现（委托各 Manager 单例）
│   ├── EventIdCompat.h                 #   事件 ID 对齐（MSVC 编译必需）
│   ├── DiagLog.*                       #   诊断日志开关（默认编译期关闭）
│   ├── SculkPacketSend.h               #   协议层发包原语
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
│   ├── npcdialog/                      #   NPC 对话框
│   ├── sensing/                        #   感知域
│   ├── playernpc/                      #   假玩家 NPC（皮肤注册表 / PlayerList）
│   ├── fakeinv/                        #   背包虚容器
│   ├── sulfur/                         #   硫磺立方体展示
│   ├── view/                           #   客户端视图覆盖
│   ├── lse/                            #   LSE 兼容层（运行时挂载 lrca）
│   ├── MemoryOperators.cpp             #   跨 DLL 内存配对
│   └── ModEntry.*                      #   生命周期 + LSE 双时机挂载
├── tests/                              # 离线字节级回归检查（本地工具，不随库发布）
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

> ⚠ 规模上限的第一道墙就在这里。形状数到千级、玩家上百时，15s 全量重发的包构造量会变成可观测的负载。想估自己的场景，把「重发策略分层（按维度分组 / 脏标记）」列进需求，这在维护者的优化候选里，见 [架构与开销评估](../internal/architecture-cost.md)（本地文档，不随库发布）。

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
- 心跳：`Level::$tick` 每 tick `tickPulse()`（实体重新进入视野 / 玩家换区块时重推覆盖）。
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

它在运行时通过 `GetModuleHandle` + `GetProcAddress` 解析 `lrca` 导出的 7 个 C++ 函数（MSVC mangled 符号，与导出表逐字一致）：

| 情况 | 行为 |
|---|---|
| `lrca` 未加载 / 符号缺失 | `attach()` 返回 `false`，所有导出 / 导入调用安全空操作 |
| `lrca` 存在 | LSE 脚本 `ll.import` / `ll.hasExported` 用法与旧版完全一致 |
| 原生 C++ 接口 | 不经过本桥，不受 `lrca` 存在与否影响 |

挂载是双时机的：`enable()` 时试一次，没成功就监听 `ServerStartedEvent` 兜底重试（届时所有插件均已加载）。这也是「启动顺序无任何要求」的原因。

> ⚠ 已知问题：`ghost*` 的 2 个导出虽然调用了 `exportAs`，但没有接进 `ModEntry::exportLseFunctions()`，所以运行期不注册。详见 [LSE 接口参考 · 已知问题](../reference/lse-api.md#已知问题)。

---

## 相关

- 线格式、发包原语、抓包验证：[协议层实现笔记](protocol-notes.md)
- 边界与纪律：[设计说明](../design-contracts.md)
- 维护者视角的开销评估与优化候选：[架构与开销评估](../internal/architecture-cost.md)（本地文档，不随库发布）
