# 1.8 IParticleShape · 粒子形状

完整签名（`#include "HologramLib/HologramLib.h"`）：

点/线/矩形环/填充面/长方体框/六面/多面体 + 平移/平滑移动/旋转/自旋/缩放/跟随。批量并发发送：逐玩家 vanilla `SpawnParticleEffectPacket` 经 `NetworkSystem` 入队，BDS tick flush 自动聚合压缩单 Batch 数据报（与原版粒子广播同路径）；采样/视距裁剪/周期重发由库内 tick 自驱动，消费者只管创建与控制。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 创建 | createPoint | `(std::string const& owner, int dimId, float x, float y, float z, std::string const& effect, int intervalTicks, int lifetimeTicks) -> int64_t` | 单点 |
| | createLine | `(owner, dimId, x1,y1,z1, x2,y2,z2: float, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | 线段 |
| | createRect | `(owner, dimId, float cx, cy, cz, w, h, int axis, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | 矩形环线（axis: 0=XY 1=YZ 2=XZ） |
| | createPlane | 同 createRect | 填充平面网格 |
| | createBox | `(owner, dimId, cx, cy, cz, hx, hy, hz: float, step, effect, intervalTicks, lifetimeTicks) -> int64_t` | 长方体 12 边线框（h* 为半尺寸） |
| | createBoxFaces | 同 createBox | 六面填充 |
| | createPoly | `(owner, dimId, std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | 多面体（verts=(x,y,z)×N；edges=(i,j)×M；锚点=质心） |
| 控制 | setPos | `(int64_t id, float x, float y, float z) -> bool` | 平移锚点（=粒子移动；同时解除跟随） |
| | moveBy | `(int64_t id, float dx, float dy, float dz) -> bool` | 相对平移 |
| | moveTo | `(int64_t id, float x, float y, float z, int durationTicks) -> bool` | 平滑点对点移动（easeOutCubic；解除跟随；duration<=0 立即到达） |
| | setRot | `(int64_t id, float rx, float ry, float rz) -> bool` | 欧拉角（度，ZYX 序，绕锚点） |
| | spin | `(int64_t id, float sx, float sy, float sz) -> bool` | 自旋（度/tick；0,0,0 停止） |
| | setScale | `(int64_t id, float scale) -> bool` | 各向同性缩放 |
| | follow | `(int64_t id, std::string const& playerUuid, float offX, float offY, float offZ) -> bool` | 锚点跟随玩家+偏移（每 tick 更新，跨维度） |
| | unfollow | `(int64_t id) -> bool` | 解除跟随 |
| 渲染 | setEffect | `(int64_t id, std::string const& effect) -> bool` | 粒子效果名 |
| | setVisiblePlayers | `(int64_t id, std::vector<std::string> const& playerUuids) -> bool` | 白名单（UUID；空=维度全员） |
| | clearVisiblePlayers | `(int64_t id) -> bool` | 清除白名单 |
| | setInterval | `(int64_t id, int ticks) -> bool` | 周期整批重发间隔 |
| | setViewDistance | `(int64_t id, int blocks) -> bool` | 逐玩家 3D 裁剪；0=不裁剪 |
| | setLifetime | `(int64_t id, int ticks) -> bool` | 从现在起；0=永久 |
| 生命周期 | destroy / destroyAll / exists / getAllIds | 同其他域 | — |
| 诊断 | getDebugInfo | `(int64_t id) const -> std::string` | 运行态摘要（找不到返回 "not_found"） |

约定：

- `intervalTicks` = 周期整批重发间隔（粒子瞬态，靠重发维持常驻视觉）；`lifetimeTicks` 0 = 永久
- 采样点在构造时换算为局部坐标（锚点=形状几何参考点），平移/旋转/缩放作用于局部点，下次发射自动生效
- 与 LSE `particle*`（§2.8）共用同一 Manager，id 空间互通

C++ 消费示例：

```cpp
auto& ps = hologramlib::IHologramLib::getInstance().particleShapes();
// 玩家脚下 16×16 粒子墙（XZ 平面填充, 跟随玩家）
auto id = ps.createPlane("MeowLand", 0, 0, 64, 0, 16, 16, 2, 1.0f,
                         "minecraft:endrod", 4, 0);
if (id > 0) {
    ps.follow(id, playerUuid, 0, -2, 0);
    ps.setVisiblePlayers(id, {playerUuid}); // 仅本人可见
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
