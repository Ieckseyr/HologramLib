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

## 攻击 / 挥动监听

挂钩 `ServerNetworkHandler` 的 `AnimatePacket` 入口，只读不拦。数据源是客户端每次挥动都会发的
`AnimatePacket(action = Swing)` —— 空手挥、挥剑、打空气、点方块、打实体都走这一条。

目标判定分两步（协议本身不告诉你「挥的是什么」）：

| 步骤 | 依据 | 结论 |
|---|---|---|
| 1 | 包里的 `Swing Source`（`ActorSwingSource`，1.26 起有） | `Attack` → 实体；其余视为「打方块或打空气」 |
| 2 | 从眼睛沿视线做 5 格射线（引擎 `BlockSource::clip`，`ShapeType::Outline`） | 命中方块 → `block`（带坐标 + 命中面）；否则 → `air` |

`entity` 分支再用包里的 Target Actor Runtime ID 去 `Level::getRuntimeActorList()` 里反查 uniqueId；
找不到就留 0（客户端不保证带 target）。

### 开销

- 每条 Swing 一次射线（≤5 格，引擎内部按块步进）+ 一次手持物品读取。
- 只有 `Attack` 分支才遍历 actor 列表反查 uniqueId。
- 事件进有界双端队列（上限 1024），满了丢最早的；计数独立于队列。

### LSE 面

九个导出：`swingPollEvents` / `swingClearEvents` / `swingLastEvent` / `swingLastTarget` /
`swingLastBlock` / `swingCount` / `swingTotal` / `swingDropped` / `swingReset`，
用法与事件行格式见 [LSE · 客户端感知](../lse/sensing.md#攻击--挥动监听swing)。

**没有 C++ 监听回调**：目前只做 LSE 轮询面，`IPlayerSensing` 虚表未动（ABI 不变）。

---

