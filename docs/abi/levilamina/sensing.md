# 1.13 感知域 · 逐玩家变量

完整签名（`#include "HologramLib/HologramLib.h"`）：

**感知域 `IHologramLib::playerSensing()`**：挂钩 `PlayerAuthInputPacket`（每玩家每 tick）逐包捕获 `InputMode`。
`inputDeviceOf(playerName)` 返回 `ClientInputDevice`（`Unknown/KeyboardMouse/Touch/Gamepad/MotionController`），
`isTouch(playerName)` 便捷判断。玩家离线即清；未上报 = `Unknown`。典型用途：按设备路由 UI
（触屏走容器列表，键鼠/手柄走交易界面 —— 触屏在纯协议层交易里走不到成交是实测结论）。

**千人千面** `ICustomEntity` 追加：`setPlayerNametag(id, player, text)`（空 = 清覆盖）、
`setPlayerScale(id, player, scale)`（<=0 = 清覆盖）、`setPlayerEquipmentSlot(id, player, slot, name, aux, nbt)`
（slot 0..5，name 空 = 该槽回退 config，即时单发无闪烁）、`clearPlayerAppearance(id, player)`。
机制与 `setPlayerRotation` 相同：覆盖按玩家 uuid 保存，出生包与增量包都按覆盖值下发，未覆盖玩家用 config 值。

**逐玩家变量全息**：全息文本含 `{var}` 占位符且绘制目标不是单玩家时，库自动切换为**逐观看者形状**
——每个在线玩家收到按自己名字解析的独立文本（`{player}` 对每人正确）；离线/离开维度的观看者形状
就地销毁。无变量时仍是共享单形状（性能不变）。文本动画（滚动/弹跳）由库内 `ServerLevelTickEvent`
监听自驱，无需消费方调用 tick。

> 曾短暂实现过**逐客户端音效**与**短命飘字**，经讨论已整体移除（见 README 更新日志）。

---

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
