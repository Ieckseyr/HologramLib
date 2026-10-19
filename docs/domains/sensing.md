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

---
