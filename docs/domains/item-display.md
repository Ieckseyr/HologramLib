# 物品显示（FMBE）

狐狸载体 + 物品/方块展示：支持 Molang 表达式、非等比拉伸、live 变换与动画预设。


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
## 新 API（1.31.0）

- `IItemDisplay::setStretch(xz, y)` / `setStretchLive(...)`：非等比拉伸（XZ 共轴、Y 独立）；方块任意单轴用 `setExtend`。
- `setRotationLive` / `setOffsetLive` / `setScaleLive` / `setTransformLive`：只重发动画包，不 despawn、不换 id。
- `ItemDisplayAnim` 扩展：`orbit` / `tumble` / `wobble` / `heartbeat` / `pendulum` / `billboardPitch` / `lodNear` / `lodFar` / `lodMinScale` / `hideInFirstPerson`。
- `setAnimPreset(id, preset, a, b, c)`；LSE `itemDisplaySetPreset`。
- 预设名：`spin` `bob` `pulse` `sway` `orbit` `tumble` `wobble` `heartbeat` `pendulum` `billboard` `lod` `firstperson` `clear`。
- 停止：`stopAnimation` / `stopAnimationTo`；配方为同 controller 空动作 + `stopExpression=1`。
