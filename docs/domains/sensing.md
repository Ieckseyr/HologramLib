# 客户端感知

从 PlayerAuthInput 捕获输入设备类型，按玩家维护 KeyboardMouse / Touch / Gamepad / MotionController。


## 感知域

```cpp
if (lib.playerSensing().isTouch("Steve")) {
    lib.containerMenus().open("Steve", menu);   // 触屏：用虚拟容器，点击必然发包
}
```

挂钩 `PlayerAuthInputPacket` **逐包**读 `InputMode` → `KeyboardMouse` / `Touch` / `Gamepad` / `MotionController`。

| 行为 | 说明 |
|---|---|
| `Unknown` 不覆盖已知值 | AuthInput 早段可能报 `Undefined`，不能让它把已知设备抹成未知 |
| 玩家离线即清 | — |
| 换设备**下一包**即更新 | 没有缓存延迟 |

**没有事件，不用轮询** —— LSE 侧直接查询：`sensingDeviceOf` / `sensingIsTouch` / `sensingDeviceCode`。

**典型用法是按设备分流界面**：触屏走虚拟容器（点击必然发包），键鼠走交易菜单（完整交互）。

## 空手挥动计数

挂钩 `ServerNetworkHandler` 的 `AnimatePacket` 入口，只读不拦：客户端每次挥手（空手挥、挥剑、
打空气、点方块、打实体）都会发 `AnimatePacket(action = Swing)`，这里按玩家计一次。

**不区分目标**：协议里只有 `Swing` 一个动作，客户端不会告诉服务端「对着什么挥」。
要判断挥空气还是挥方块，得由脚本自己结合视线 / 交互事件处理。

### 开销

每条 Swing 只做一次哈希表自增，不发包、不读世界、不碰实体。

### LSE 面

三个导出：`swingCount` / `swingTotal` / `swingReset`，见 [LSE · 客户端感知](../lse/sensing.md)。

**没有 C++ 监听回调**，`IPlayerSensing` 虚表未动（ABI 不变）。

---
