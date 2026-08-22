# HologramLib API 参考

- 库版本：1.5.0（`HOLOGLIB_API_VERSION 0x010500`）
- 唯一公开头：`include/hologramlib/HologramLib.h`

## API 稳定性契约

| 层级 | 冻结内容 | 演进规则 |
|------|----------|----------|
| C++ 接口 | 全部纯虚方法签名与语义（ABI 稳定：实现对象在 DLL 内创建，消费者只持引用） | 只在接口**尾部追加**方法、只新增函数；永不修改/删除 |
| C++ 宏 | `HOLOGLIB_API_VERSION`、`HOLOGLIB_API`、`hologramlib` 命名空间、枚举值 | 只追加枚举值 |
| LSE 命名空间 | 三命名空间全部函数名、参数顺序、返回值类型 | 只增不改不删 |
| 版本协商 | `IHologramLib::version()`（BCD：0x010500 = 1.5.0） | 随发布递增 |

破坏兼容仅允许发生在大版本（2.0.0），届时提供全新接口名并长期保留旧接口最终冻结实现。

`src/` 目录一切内容均为内部实现，**不属于 API**。

---

## 1. C++ 接口

### 1.1 入口

```cpp
namespace hologramlib {
    class IHologramLib {
    public:
        static IHologramLib& getInstance();          // 唯一 dllexport 工厂
        IShapeDrawer&  shapes();                     // 形状渲染
        IHologramText& holograms();                  // 悬浮字全息
        IItemDetail&   itemDetails();                // 物品详情
        bool     isLseAvailable();                   // LSE 兼容层是否已挂载
        uint32_t version();                          // 0x010500
    };
}
```

通用约定：

- 所有 `id` 为 `int64_t`，创建失败返回 `< 0`
- 颜色分量 `0.0 ~ 1.0`（RGBA）
- 坐标为世界坐标；`setDuration` 单位秒
- 方法内部互斥（线程安全）；建议主线程调用（发包在调用线程执行）

### 1.2 IShapeDrawer（形状渲染）

| 方法 | 签名 | 说明 |
|------|------|------|
| createText | `(float x, float y, float z, std::string const& text) -> int64_t` | 文本形状 |
| createLine | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 线段 |
| createBox | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 盒体（两点对角） |
| createCircle | `(x, y, z, scale: float) -> int64_t` | 圆 |
| createSphere | `(x, y, z, scale: float) -> int64_t` | 球 |
| createArrow | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | 箭头 |
| setColor | `(int64_t id, float r, float g, float b, float a) -> bool` | RGBA |
| setScale | `(int64_t id, float scale) -> bool` | 缩放 |
| setDuration | `(int64_t id, float seconds) -> bool` | 存留时长 |
| setDimension | `(int64_t id, int dimId) -> bool` | 维度 |
| setLocation | `(int64_t id, float x, float y, float z) -> bool` | 位置 |
| setText | `(int64_t id, std::string const& text) -> bool` | 文本内容 |
| setRotation | `(int64_t id, float pitch, float yaw, float roll) -> bool` | 旋转（弧度） |
| clearRotation | `(int64_t id) -> bool` | 清除旋转 |
| draw | `(int64_t id) -> bool` | 全维度可见者绘制 |
| drawToPlayer | `(int64_t id, std::string const& playerName) -> bool` | 指定玩家 |
| drawToDimension | `(int64_t id, int dimId) -> bool` | 指定维度 |
| remove | `(int64_t id) -> bool` | 隐藏（保留数据） |
| update | `(int64_t id) -> bool` | 可见时原地重发（同 networkId 覆盖，无闪烁） |
| destroy | `(int64_t id) -> bool` | 销毁 |
| destroyAll | `() -> void` | 全部销毁 |
| exists | `(int64_t id) -> bool` | 存在性 |
| type | `(int64_t id) -> ShapeType` | 形状类型 |

`ShapeType` 枚举（与 LSE `getShapeType` 数值一致）：

```cpp
enum class ShapeType : int { Text=0, Line=1, Box=2, Circle=3, Sphere=4, Arrow=5 };
```

> 注：旧版 `FilledQuad = 6` 已移除；数值 6 保留空洞，永不复用。

### 1.3 IHologramText（悬浮字全息）

整块多行文本单一背景框渲染（`\n` 合并所有行）；变更后调用 `refresh` 原地重绘（同 networkId 覆盖，无闪烁）。行索引 0 起。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | create | `(float x, float y, float z) -> int64_t` | 创建 |
| | destroy | `(int64_t id) -> bool` | 销毁 |
| | destroyAll | `() -> void` | 全部销毁 |
| 行管理 | addLine | `(int64_t id, std::string const& text) -> bool` | 追加行 |
| | setLineText | `(int64_t id, int line, std::string const& text) -> bool` | 行文本 |
| | setLineScale | `(int64_t id, int line, float scale) -> bool` | 行缩放 |
| | removeLine | `(int64_t id, int line) -> bool` | 移除行 |
| | clearLines | `(int64_t id) -> bool` | 清空行 |
| | getLineCount | `(int64_t id) -> int` | 行数 |
| 颜色 | setColor | `(int64_t id, r, g, b, a: float) -> bool` | 整体纯色 |
| | setLineColor | `(int64_t id, int line, r, g, b, a: float) -> bool` | 行纯色 |
| | setLineGradient | `(int64_t id, int line, r1,g1,b1, r2,g2,b2: float) -> bool` | 行双色渐变 |
| | setLineRainbow | `(int64_t id, int line, float speed) -> bool` | 行彩虹 |
| 动画 | setLineScroll | `(int64_t id, int line, int direction, float speed) -> bool` | 滚动：0=无 1=左 2=右 |
| | setVerticalAnimation | `(int64_t id, int type, float speed, float range) -> bool` | 垂直：0=无 1=弹跳 2=滚动 |
| | setLineSpacing | `(int64_t id, float spacing) -> bool` | 行距 |
| 位置 | setLocation | `(int64_t id, float x, float y, float z) -> bool` | 位置 |
| | setFollowPlayer | `(int64_t id, std::string const& playerName, float offsetY) -> bool` | 跟随玩家 |
| | clearFollowPlayer | `(int64_t id) -> bool` | 取消跟随 |
| 显示 | draw / drawToDimension / drawToPlayer / remove | 同形状语义 | |
| | refresh | `(int64_t id) -> bool` | 重解析变量并原地重发 |
| 驱动 | tick | `(float deltaTime) -> void` | 动画推进（滚动/跟随） |

内置变量（PAPI 翻译后兜底解析）：`{time}` `{online}` `{tps}` `{player}`。
外部占位符 `%name%` / `{name}` 经 LseBridge 调 `MeowPAPI::translateString(WithPlayer)`（可选，缺席原样保留）。

### 1.4 IItemDetail（物品详情）

```cpp
// 在 (x,y,z) 显示 "本地化物品名 xN"（自动翻译; count<=1 无数量后缀）
// customText 非空则完全替代自动文本（支持 § 颜色码与 {变量}）
// 返回悬浮字 ID（可用 holograms() 继续精修）
int64_t show(int dimId, float x, float y, float z,
             std::string const& itemId, int aux, int count,
             std::string const& customText = "");
bool hide(int64_t id);
```

物品名解析链：`ItemRegistry（HashedString）→ ItemStack → I18n(getDescriptionId)` → `getName/getDescriptionName` → 兜底原样显示 ID（不阻断）。

---

## 2. LSE 兼容层（ll.import）

LegacyRemoteCall（lrca）在场时自动导出，三命名空间与原 DebugShape-Protocol 插件**同名同签名**，现有脚本零改动。缺席时安全降级（`ll.import` 得 null）。

类型记法：`f`=浮点 `i`=整数 `s`=字符串 `b`=布尔 `[i]`=int64 数组。除注明外 id 均为 int64。

### 2.1 DebugShape（形状，41 函数）

**创建**

| 函数 | 签名 |
|------|------|
| createText | `(x: f, y: f, z: f, text: s) -> i` |
| createLine | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |
| createBox | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |
| createCircle | `(x: f, y: f, z: f, scale: f) -> i` |
| createSphere | `(x: f, y: f, z: f, scale: f) -> i` |
| createArrow | `(x1,y1,z1: f, x2,y2,z2: f) -> i` |

**属性**

| 函数 | 签名 |
|------|------|
| setText | `(id, text: s) -> b` |
| getText | `(id) -> s` |
| setLocation | `(id, x: f, y: f, z: f) -> b` |
| getLocation | `(id) -> [f,f,f]` |
| setColor | `(id, r: f, g: f, b: f, a: f) -> b` |
| getColor | `(id) -> [f,f,f,f]` |
| setScale | `(id, scale: f) -> b` |
| setDuration | `(id, seconds: f) -> b` |
| setDimension | `(id, dimId: i) -> b` |
| setRotation | `(id, pitch: f, yaw: f, roll: f) -> b` |
| clearRotation | `(id) -> b` |
| getRotation | `(id) -> [f,f,f]` |
| getShapeType | `(id) -> i`（0..5，见 ShapeType） |

**显示**

| 函数 | 签名 |
|------|------|
| draw | `(id) -> b` |
| drawToPlayer | `(id, playerName: s) -> b` |
| drawToDimension | `(id, dimId: i) -> b` |
| drawBatch | `(ids: [i]) -> b` |
| remove | `(id) -> b` |
| removeToPlayer | `(id, playerName: s) -> b` |
| removeToDimension | `(id, dimId: i) -> b` |
| update | `(id) -> b` |
| updateToPlayer | `(id, playerName: s) -> b` |
| updateToDimension | `(id, dimId: i) -> b` |

**生命周期与查询**

| 函数 | 签名 |
|------|------|
| destroy | `(id) -> b` |
| destroyAll | `() -> nil` |
| destroyBatch | `(ids: [i]) -> b` |
| findTextByLocation | `(x: f, y: f, z: f, radius: f) -> [i]` |
| findTextByLocationAndContent | `(x: f, y: f, z: f, radius: f, text: s) -> i` |
| getAllShapeIds | `() -> [i]` |
| exists | `(id) -> b` |

### 2.2 FloatingText（悬浮字，23 函数）

| 函数 | 签名 |
|------|------|
| create | `(x: f, y: f, z: f) -> i` |
| destroy | `(id) -> b` |
| destroyAll | `() -> nil` |
| addLine | `(id, text: s) -> b` |
| setLineText | `(id, lineIndex: i, text: s) -> b` |
| setLineScale | `(id, lineIndex: i, scale: f) -> b` |
| removeLine | `(id, lineIndex: i) -> b` |
| clearLines | `(id) -> b` |
| getLineCount | `(id) -> i` |
| setColor | `(id, r,g,b,a: f) -> b` |
| setLineColor | `(id, lineIndex: i, r,g,b,a: f) -> b` |
| setLineGradient | `(id, lineIndex: i, r1,g1,b1,r2,g2,b2: f) -> b` |
| setLineRainbow | `(id, lineIndex: i, speed: f) -> b` |
| setLineScroll | `(id, lineIndex: i, direction: i, speed: f) -> b` |
| setVerticalAnimation | `(id, type: i, speed: f, range: f) -> b` |
| setLineSpacing | `(id, spacing: f) -> b` |
| setLocation | `(id, x: f, y: f, z: f) -> b` |
| setFollowPlayer | `(id, playerName: s, offsetY: f) -> b` |
| clearFollowPlayer | `(id) -> b` |
| tick | `(deltaTime: f) -> nil`（动画驱动） |
| draw | `(id) -> b` |
| drawToDimension | `(id, dimId: i) -> b` |
| drawToPlayer | `(id, playerName: s) -> b` |
| remove | `(id) -> b` |
| refresh | `(id) -> b` |

### 2.3 GradientLine（渐变线，11 函数）

| 函数 | 签名 |
|------|------|
| create | `(x1,y1,z1: f, x2,y2,z2: f, segments: i) -> i` |
| setGradient | `(id, r1,g1,b1,r2,g2,b2: f) -> b` |
| setRainbow | `(id, speed: f) -> b` |
| setColor | `(id, r,g,b,a: f) -> b` |
| setEndpoints | `(id, x1,y1,z1: f, x2,y2,z2: f) -> b` |
| draw | `(id) -> b` |
| drawToDimension | `(id, dimId: i) -> b` |
| remove | `(id) -> b` |
| destroy | `(id) -> b` |
| destroyAll | `() -> nil` |
| tick | `(deltaTime: f) -> nil`（彩虹动画驱动） |

---

## 3. 已删除 API（相对旧 DebugShape-Protocol）

以下旧 API 在本库中**永久移除**，数值/名称保留空洞永不复用：

| 删除项 | 原因 |
|--------|------|
| `DebugShape::createFilledQuad` | 极薄 Box 模拟填充面方案废弃 |
| `DebugShape::createFilledQuadBatch` | 每像素一个极薄 Box 的批量填充面方案废弃 |
| `LSEShapeType::FilledQuad (=6)` | 枚举移除，6 保留空洞 |

旧脚本经 `safeImport` 容错调用上述 API 将得到 null，不会报错。

---

## 4. 消费者版本协商示例

```cpp
#include "hologramlib/HologramLib.h"

static_assert(HOLOGLIB_API_VERSION >= 0x010500, "need HologramLib >= 1.5.0");

auto& lib = hologramlib::IHologramLib::getInstance();
if (lib.version() < 0x010500) { /* 运行时能力协商 */ }
```
