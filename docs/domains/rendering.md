# 能力域指南 · 渲染与文本

覆盖 6 个域：**形状渲染**、**悬浮字全息**、**渐变线**、**物品详情**、**FMBE 物品悬浮**、**粒子形状**。

这些域全部**不产生真实实体、不写存档**（唯一例外是 FMBE 物品悬浮，它用「假实体 + 发包」），客户端看到的东西只存在于客户端内存里。

| 域 | C++ | LSE | 一句话 |
|---|---|---|---|
| 形状渲染 | `IShapeDrawer` | `shape*` · 39 | 线 / 盒 / 圆 / 球 / 箭头 / 文本 |
| 悬浮字全息 | `IHologramText` | `holo*` · 26 | 多行文本，整块样式，动态行 |
| 渐变线 | — | `gradient*` · 11 | 多色渐变轨迹线 |
| 物品详情 | `IItemDetail` | `itemDetail*` · 2 | 自动翻译「钻石 x64」 |
| FMBE 物品悬浮 | `IItemDisplay` | `itemDisplay*` · 33 | 狐狸举着物品；Molang 驱动动画 |
| 粒子形状 | `IParticleShape` | `particle*` · 22 | 点 / 线 / 面 / 盒框 + 移动旋转跟随 |

---

## 形状渲染

```cpp
auto line = lib.shapes().createLine(0, 64, 0, 10, 64, 10);
lib.shapes().setColor(line, 1.0f, 0.2f, 0.2f, 1.0f);   // 分量 0.0 ~ 1.0
lib.shapes().setDuration(line, 10.0f);                 // 单位：秒
lib.shapes().draw(line);
```

**通用约定**（三个域共用）：

- 所有 `id` 为 `int64_t`，创建失败返回 `< 0`；
- 颜色分量 `0.0 ~ 1.0`（RGBA），坐标为世界坐标，`setDuration` 单位为秒；
- 方法内部互斥（线程安全），但**发包在调用线程执行 → 建议主线程调用**。

### 文本形状的三个开关（`26.40.8` 补齐）

文本形状只是形状域的一个分支，但它有三个别的形状没有的属性：

| 能力 | 方法 | 说明 |
|---|---|---|
| 背景框 | `setBackgroundColor` / `clearBackgroundColor` | 不设 = 客户端默认色 |
| 穿墙 | `setDepthTest` | `false` = 始终渲染（**默认，穿墙可见**）；`true` = 被方块 / 实体遮挡 |
| 三轴旋转 | `setRotation` / `clearRotation` | Euler `[Pitch, Yaw, Roll]`，**单位 = 度**（对齐官方脚本 API 口径，库不做换算）。设置后文本不再面向相机 |

> **这两条是修过的 bug，值得知道**：① `setRotation` 此前**静默无效**（写了 `mRotation` 却从不写 `mUseRotation`）；② `destroyBatch` / `destroyAll` 此前**只清服务端内存、不发移除包**（客户端残留显示）。若你按旧版行为写过绕行代码，现在可以删掉。

---

## 悬浮字全息

### 组合模型：多行 = 一个形状

```cpp
auto holo = lib.holograms().create(0, 70, 0);
lib.holograms().addLine(holo, "§e欢迎来到主城");
lib.holograms().addLine(holo, "在线: {online}");
lib.holograms().setScale(holo, 1.5f);
lib.holograms().draw(holo);
```

**行 = 文本**：多行合并为**单个**文本形状（`'\n'` 连接），整块共用一份颜色 / 缩放 / 背景框 / 穿墙 / 旋转。

这是协议决定的，不是实现偷懒：协议 2168 的 `TextDataPayload` 字段序为
`Text(0) / UseRotation(1) / BackgroundColor(2, optional) / DepthTest(3) / ShowBackface(4) / ShowTextBackface(5)`
—— **没有行级颜色 / 缩放 / 渐变 / 行间距字段**，`lineGapHeight` 在协议 2168 里根本不存在。

| 你想做 | 正确做法 |
|---|---|
| 某一行换颜色 | 文本内嵌 `§` 颜色代码 |
| 整块放大 | `holoSetScale` |
| 想要行间距 | 用**多个**文本形状自己排版 |

> ⚠ **历史上那 8 个行级 API 已在 `26.40.8` 整体移除**（`holoSetLineGradient` / `holoSetLineRainbow` / `holoSetLineColor` / `holoSetLineScroll` / `holoSetVerticalAnimation` / `holoSetLineSpacing` / `holoSetLineScale` / `holoTick`）。
> 它们从 `26.10.4` 起就在导出，但底层从来没有对应协议能力 —— 行级样式只有第 0 行生效、彩虹模式实测把文本涂白、行距字段不存在、自驱 tick 的偏移没有任何消费方。
> **ABI 是安全的**：这 8 个方法在 C++ 接口里保留为**废弃空槽**（非纯虚 + 内联空实现 + `[[deprecated]]`），虚表槽位与 1.25.0 完全一致，未重编译的旧消费方二进制不会错位。重新编译会得到**编译期弃用警告**；LSE 侧 `ll.import` 得 `null`（显式失败，不会静默变行为）。真正清槽留到下一个大版本。

### 动态行（内容池轮播 + 行级变量）

```cpp
lib.holograms().setLinePool(holo, 0, {"§a在线人数: {online}", "§eTPS: {tps}"}, 2000); // 2000ms 一轮
lib.holograms().setLineParseVariables(holo, 1, true);   // 行级变量开关，默认开
```

- 内容池按间隔**时间取模轮播**，**无状态** —— 多个悬浮字用同一个池、同一间隔时天然同相；
- 刷新由库内 **0.5s 节流**承担，且**内容变了才重发**；
- 变量集：`{player}` / `{online}` / `{dimension}` / `{x}` / `{y}` / `{z}`。

> 这一部分**重构自 [Phantom](https://github.com/GroupMountain/Phantom)**（LGPL-3.0），来源标注见项目 README 与 `src/FloatingTextManager.h`。

### 逐玩家变量（千人千面）

文本一旦含 `{var}` 且目标不是单玩家，库就**改为维护每个观看者一份形状**，各自收到按自己解析的结果（观看者离线 / 离开维度时那份形状就地销毁）。

```cpp
lib.holograms().addLine(holo, "§b{player}§r 你好");   // 每人看到自己的名字
```

两个必须知道的代价：

- **形状数变成 N 倍**（N = 观看者数），重发复杂度随之 N 倍 —— 适合队长视角、任务追踪这类**少数人场景**，不适合全服广播式的高频刷新；
- 变量被清掉、或改成单玩家目标时，会自动切回共享形状。

### 生命周期与重发

- **永久悬浮字不要设 `duration`** —— 不设才能避免客户端倒计时自动删除；
- 客户端不落盘，所以重连必须补发。补发策略集中在形状域（见 [架构总览](../architecture/overview.md#重发策略)）；
- **跟随玩家不再依赖库内 tick**（`26.40.8` 起）：位置在 `setFollowPlayer` / `draw` / `refresh` 调用时**就地解析**，按需刷新由消费方负责（示例：ZXPanel 每 2 tick 原地重发一次）。

---

## 渐变线（仅 LSE）

`gradient*` 有 11 个函数，没有 C++ 接口。它自建一批线段形状，用多色渐变模拟轨迹线。

`gradientTick` 是**消费方驱动**的（LSE 导出，由脚本按 tick 调用）—— 库不替消费方做动画。

---

## 物品详情

`itemDetail*`（2 个函数）把物品堆翻译成人读文本，例如 `minecraft:diamond` + 数量 64 → 「钻石 x64」。适合做侧边栏 / 提示文本，不做渲染。

---

## FMBE 物品悬浮

```cpp
hologramlib::ItemDisplayConfig cfg;
cfg.x = 100.5f; cfg.y = 65.0f; cfg.z = -200.5f;
cfg.rotY = "math.sin(query.life_time*90)*360";   // Molang 表达式
auto disp = lib.itemDisplays().create(cfg);
```

「狐狸 + 发包」：生成一只**部分隐身的假实体**举着物品，只对目标玩家可见。三轴旋转 / 平移 / 缩放都支持 **Molang 表达式**（拿 `query.life_time` 之类驱动），所以动画不需要服务端每 tick 推包。

其他能力：无感创建（`createSeamless`）、可见玩家白名单、视距、`scaleTo`。

> ⚠ **技术债（已知）**：ItemDisplay 有一份**独立的发送实现副本**，没有走全库共享的 `SculkPacketSend.h`。`SculkPacketSend.h` 头注释里记着「验证稳定后统一迁移」。改动发包逻辑时别忘了这一份。

---

## 粒子形状

```cpp
auto wall = lib.particleShapes().createPlane(
    "Steve", 0, 100, 64, -200, 48, 32, /*axis=*/2, /*step=*/3,
    "minecraft:heart_particle", /*interval=*/10, /*lifetime=*/0);

lib.particleShapes().setVisiblePlayers(wall, {"uuid1", "uuid2"});  // 空 = 维度全员
lib.particleShapes().moveTo(wall, 150, 64, -200, 100);            // 5 秒平滑滑移
```

形状：点 / 线 / 矩形环 / 填充面 / 盒框 / 六面 / 多面体。
变换：`moveTo`（锚点 `easeOutCubic` 插值，**整面随锚点移动**）、旋转、自旋、跟随玩家。

**发送路径**：走 vanilla `SpawnParticleEffectPacket` 批量通道 —— BDS 的 tick flush 会自动聚合压缩成单个 Batch 数据报。这条路径与「原版粒子广播」相同，所以库内自驱的采样 / 视距裁剪 / 周期重发属于**引擎同路径**，不是替消费方做动画。

---

## 相关

- 完整签名：[C++ 接口参考](../reference/cpp-api.md)（§1.2 形状 / §1.3 悬浮字 / §1.4 物品详情 / §1.5 物品悬浮 / §1.8 粒子）
- LSE 函数清单：[LSE 接口参考](../reference/lse-api.md)（§2.1 ~ §2.5、§2.8）
- 重发与开销：[架构总览](../architecture/overview.md) · [协议层实现笔记](../architecture/protocol-notes.md)
