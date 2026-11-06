# 1.2 IShapeDrawer · 形状渲染

完整签名（`#include "HologramLib/HologramLib.h"`）：

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
| setRotation | `(int64_t id, float pitch, float yaw, float roll) -> bool` | 三轴旋转（度; 文本形状自动启用 useRotation + 双面渲染） |

### 1.33.0 追加：IShapeDrawerExt（形状扩展）

形状扩展单独成接口（`lib.shapeExt()`），不往 `IShapeDrawer` 中间插方法 —— 中间插会挪动后面所有接口的虚表槽位。

| 方法 | 签名 | 说明 |
|------|------|------|
| createCylinder | `(float x, float y, float z, float radiusX, float radiusZ, float height, int segments = 16) -> int64_t` | 圆柱 |
| createPyramid | `(float x, float y, float z, float width, float height, float depth) -> int64_t` | 金字塔（底面 width×depth） |
| createEllipsoid | `(float x, float y, float z, float radiusX, float radiusY, float radiusZ, int segmentsPerAxis = 12) -> int64_t` | 椭球 |
| createCone | `(float x, float y, float z, float radiusX, float radiusZ, float height, int segments = 16) -> int64_t` | 圆锥 |
| setMaxRenderDistance | `(int64_t id, float distance) -> bool` | 最远渲染距离；`<= 0` 清空该字段 |
| setAttachedEntity | `(int64_t id, int64_t entityUniqueId) -> bool` | 挂到实体上（客户端跟随） |
| clearAttachedEntity | `(int64_t id) -> bool` | 解除挂载 |

`ShapeType` 同步追加 `Cylinder = 7` / `Pyramid = 8` / `Ellipsoid = 9` / `Cone = 10`；`6` 是历史空洞，不复用。
虚表与枚举都是尾部追加，`HOLOGLIB_ABI_STAMP` 仍为 1，但新增了接口，消费方需要重新编译。 |
| clearRotation | `(int64_t id) -> bool` | 清除旋转（恢复面向相机） |
| draw | `(int64_t id) -> bool` | 全维度可见者绘制 |
| drawToPlayer | `(int64_t id, std::string const& playerName) -> bool` | 指定玩家 |
| drawToDimension | `(int64_t id, int dimId) -> bool` | 指定维度 |
| remove | `(int64_t id) -> bool` | 隐藏（保留数据） |
| update | `(int64_t id) -> bool` | 可见时原地重发（同 networkId 覆盖，无闪烁） |
| destroy | `(int64_t id) -> bool` | 销毁 |
| destroyAll | `() -> void` | 全部销毁 |
| exists | `(int64_t id) -> bool` | 存在性 |
| type | `(int64_t id) -> ShapeType` | 形状类型 |
| setBackgroundColor | `(int64_t id, float r, float g, float b, float a) -> bool` | 背景框颜色（1.26.0; 仅文本形状, 不设 = 客户端默认色） |
| clearBackgroundColor | `(int64_t id) -> bool` | 回客户端默认背景色（1.26.0） |
| setDepthTest | `(int64_t id, bool enabled) -> bool` | 穿墙可见性（1.26.0; true = 被方块/实体遮挡, false = 始终渲染） |

```cpp
enum class ShapeType : int { Text=0, Line=1, Box=2, Circle=3, Sphere=4, Arrow=5 };
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
