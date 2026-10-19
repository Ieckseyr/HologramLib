# DebugShape · 悬浮字与形状显示

协议层的形状与悬浮字：不生成实体、不写存档，直接给客户端发 DebugShape / 悬浮字包。


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

> **26.51 线（协议 2193）**：`TextDataPayload` 在 `BackgroundColor` 与 `DepthTest` 之间**新增了 `LineGapHeight`(float)**
> —— 这是 2168 → 2193 在这张表上唯一的差异。26.51 上**必须显式发出该字段**（不发会让其后所有字段错位、客户端解析
> 失败），库发的是 `kTextLineGapHeight = 0.0f`（= 协议默认值，即不额外加行距，见 `src/ProtocolShape.h`）。
> 行级颜色 / 缩放 / 渐变在 2193 上仍不存在；要让多行排得松一些，调的就是这一个常量。

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
- 客户端不落盘，所以重连必须补发。补发策略集中在形状域（见 [架构总览](https://github.com/Ieckseyr/HologramLib/blob/main/docs/architecture/overview.md#重发策略)）；
- **跟随玩家不再依赖库内 tick**（`26.40.8` 起）：位置在 `setFollowPlayer` / `draw` / `refresh` 调用时**就地解析**，按需刷新由消费方负责（示例：ZXPanel 每 2 tick 原地重发一次）。

---

## 渐变线（仅 LSE）

`gradient*` 有 11 个函数，没有 C++ 接口。它自建一批线段形状，用多色渐变模拟轨迹线。

`gradientTick` 是**消费方驱动**的（LSE 导出，由脚本按 tick 调用）—— 库不替消费方做动画。

---
## 当前状态（1.31.0）

- 形状与悬浮字默认开启；`HLIB_NO_SHAPE=1` 可停发全部 `PrimitiveShapesPacket`。
- 文本形状载荷按 26.51 协议包含 `LineGapHeight`，不发会导致客户端错位。
- 形状颜色按协议 ARGB 打包；`setRotation` 对文本形状生效。
