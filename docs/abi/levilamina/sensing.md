# 1.13 感知域 · 逐玩家变量

从客户端每 tick 上报的 `PlayerAuthInputPacket` 里读输入设备类型，并让悬浮字里的 `{var}` 按观看者各自解析。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().playerSensing()`
- 数据源：`PlayerAuthInputPacket::mInputMode`（`1` 键鼠 / `2` 触屏 / `3` 手柄 / `4` 体感，其它值 = `Unknown`）
- 生命周期：玩家离线即清；还没上报过 = `Unknown`（`0`）
- 查询成本：一次哈希查表，随时可调

!!! note "没有事件、不用轮询"

    设备类型只是库内内存里的一个映射，`inputDeviceOf` / `isTouch` 是纯查询，需要时直接问。
    典型用途：按设备分流 UI —— 触屏走容器列表（点击必然发包），键鼠 / 手柄走交易界面
    （触屏在纯协议层交易里走不到成交，这是实测结论）。

## 输入设备

### 查询玩家当前使用的输入设备

`ClientInputDevice inputDeviceOf(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名（精确匹配）
- 返回值：`ClientInputDevice` 枚举值；玩家不在线、还没上报过输入、或上报值不认识都返回 `Unknown`（`0`）
- 返回值类型：`ClientInputDevice`
- 示例：

    ```cpp
    auto& sensing = hologramlib::IHologramLib::getInstance().playerSensing();

    switch (sensing.inputDeviceOf("Steve")) {
    case hologramlib::ClientInputDevice::Touch:
        // 触屏：走容器列表
        break;
    case hologramlib::ClientInputDevice::KeyboardMouse:
    case hologramlib::ClientInputDevice::Gamepad:
        // 键鼠 / 手柄：走交易界面
        break;
    default:
        // 还没上报过：按最保守的那条路走
        break;
    }
    ```

### 判断玩家是否触屏

`bool isTouch(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：触屏 `true`；不是触屏、玩家不在线或还没上报过 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (sensing.isTouch("Steve")) {
        open_container_list("Steve");   // 触屏：容器点击必然发包
    } else {
        open_trade_menu("Steve");       // 键鼠 / 手柄：交易界面
    }
    ```

#### 设备枚举对照

| 枚举 | 值 | 对应的 `mInputMode` | 说明 |
|---|---|---|---|
| `ClientInputDevice::Unknown` | `0` | 其它 / 未上报 | 玩家不在线、还没上报，或客户端给了不认识的值 |
| `ClientInputDevice::KeyboardMouse` | `1` | `1` | 键鼠 |
| `ClientInputDevice::Touch` | `2` | `2` | 触屏 |
| `ClientInputDevice::Gamepad` | `3` | `3` | 手柄 |
| `ClientInputDevice::MotionController` | `4` | `4` | 体感手柄 |

## 逐玩家变量（悬浮字里的 `{var}`）

文本里含 `{var}` 占位符、且这条悬浮字不是只画给某一个玩家时，库自动切成**逐观看者形状**：
每个在线玩家收到一份按自己名字解析的独立文本，`{player}` 对每个人都是他自己。

内置变量：`{player}` `{online}` `{time}` `{tps}` `{dimension}` `{x}` `{y}` `{z}`。

```cpp
auto& text = hologramlib::IHologramLib::getInstance().holograms();

int64_t id = text.create(100.5f, 70.0f, 100.5f);
text.addLine(id, "§e欢迎回来，§a{player}");
text.addLine(id, "§7在线：§f{online}");
text.setLineParseVariables(id, 0, true);   // 默认就是 true
text.draw(id);
```

- 没有 `{var}` 的文本仍是**共享单形状**，性能和以前一样
- 玩家离线 / 离开该维度时，他那一份形状就地销毁
- 改完内容调 `refresh(id)`，按观看者逐份原地重发（同 networkId，无闪烁）
- 想让某一行原样显示 `{}`，`setLineParseVariables(id, lineIndex, false)`

!!! tip "变量是逐玩家解析，不是逐玩家设置"

    文本只写一份，每个观看者看到的 `{player}` 是他自己的名字。
    想让某个人看到完全不同的内容，用 `drawToPlayer` 单独开一条，或者用下面 `ICustomEntity` 的逐玩家覆盖。

## 千人千面（`ICustomEntity` 的逐玩家外观）

`ICustomEntity` 有一套按玩家 uuid 保存的覆盖，出生包与增量包都按覆盖值下发，未覆盖的玩家用 config 值：

| 方法 | 签名 | 说明 |
|---|---|---|
| `setPlayerNametag` | `bool (int64_t id, std::string const& playerName, std::string const& text)` | 只改这名玩家看到的名字牌；空串 = 清掉覆盖 |
| `setPlayerScale` | `bool (int64_t id, std::string const& playerName, float scale)` | 逐玩家缩放，有效 `0.0625 ~ 10`（超出钳到边界）；`<= 0` = 清掉覆盖 |
| `setPlayerEquipmentSlot` | `bool (int64_t id, std::string const& playerName, int slot, std::string const& name, int aux, std::string const& nbt)` | 逐玩家装备槽，`slot 0..5`；`name` 空 = 该槽回退 config；即时单发，不重建实体、不闪烁 |
| `clearPlayerAppearance` | `bool (int64_t id, std::string const& playerName)` | 清掉这名玩家的名字牌 / 缩放 / 装备覆盖 |

这些方法返回 `false` 的常见原因：实体不存在、玩家不在线，或**这名玩家当前并没有看到这只实体**
（覆盖只对「看得见它」的人有意义）。

## 已移除的能力

曾短暂实现过**逐客户端音效**与**短命飘字**，经讨论已整体移除（见 README 更新日志）。
现在要做「只给一个人看的提示」，用 `IHologramText::drawToPlayer`；要做「响一下」，
用原版 `/playsound` 或服务端的音效接口，不要在这个域里找。

## 版本注记

| API · 插件版本 | 变化 |
|---|---|
| 1.21.0 · 26.40.3 | `IPlayerSensing` 首次发布（`inputDeviceOf` / `isTouch`）；同批 `ICustomEntity` / `IHologramText` 追加逐客户端渲染（按观看者覆盖名字牌 / 缩放 / 装备、含 `{var}` 的文本按观看者解析）；同批移除逐客户端音效与短命飘字 |

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

#include <string>

void greet_and_route(std::string const& playerName, float x, float y, float z) {
    auto& lib     = hologramlib::IHologramLib::getInstance();
    auto& sensing = lib.playerSensing();
    auto& text    = lib.holograms();

    // 1) 逐玩家变量的悬浮字：每个人看到自己的名字
    int64_t id = text.create(x, y + 2.2f, z);
    text.addLine(id, "§6欢迎，§a{player}");
    text.addLine(id, "§7当前在线：§f{online}");
    text.setScale(id, 1.1f);
    text.setDepthTest(id, false);   // 穿墙可见
    text.draw(id);

    // 2) 按输入设备分流 UI
    switch (sensing.inputDeviceOf(playerName)) {
    case hologramlib::ClientInputDevice::Touch:
        open_container_list(playerName);   // 触屏：容器列表（点击必然发包）
        break;
    case hologramlib::ClientInputDevice::KeyboardMouse:
    case hologramlib::ClientInputDevice::Gamepad:
        open_trade_menu(playerName);       // 键鼠 / 手柄：交易界面
        break;
    default:
        break;                             // 还没上报过设备：这一轮不弹界面
    }
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
