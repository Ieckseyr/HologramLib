# 粒子

协议层粒子形状：点/线/环/面/框/多面体，支持平移、旋转、缩放、跟随与逐玩家发送。


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
