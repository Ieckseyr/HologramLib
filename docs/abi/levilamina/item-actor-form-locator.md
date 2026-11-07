# 1.17 IItemActor / IFormControl / ILocatorBar

协议层掉落物、强制关闭客户端表单，外加一个**已经移除**的方位条接口。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：
  - `IItemActor` → `hologramlib::IHologramLib::getInstance().itemActors()`
  - `IFormControl` → `hologramlib::IHologramLib::getInstance().formControl()`
  - `ILocatorBar` → **没有**（1.36.0 已从 ABI 删除；1.37.0 起 27 / 32 号槽以保留槽形式存在，无任何方法）
- 掉落物只存在于客户端：不参与拾取、不进存档、没有服务端物理
- 表单关闭只发一条包：不区分表单来源，服务端也不记录表单状态

## IItemActor · 协议层掉落物

在客户端显示一只掉落物实体，用来做奖励飞出、抽奖、过场道具这类展示；
需要玩家真正拾取的物品请用原版掉落物。

`ItemActorSpec`：

| 字段 | 类型 | 默认 | 说明 |
|---|---|---|---|
| `itemId` | `std::string` | 空 | 物品 id，如 `minecraft:diamond`；**必填**，空串 = 生成失败 |
| `aux` | `int` | `0` | 附加数据值（损害值），一般 `0` |
| `count` | `int` | `1` | 数量，`< 1` 按 `1`；按 `uint16` 下发，实践上别超过 `127`（物品堆叠上限） |
| `x` / `y` / `z` | `float` | `0` | 世界坐标；`.5` 落在方块中心 |
| `dimension` | `int` | `0` | 维度 `0` 主世界 / `1` 下界 / `2` 末地 |
| `vx` / `vy` / `vz` | `float` | `0` | 初速度（方块 / tick） |

!!! note "物品不带 NBT"

    物品只包含 id、数量与 aux —— 没有自定义名、没有 lore、没有附魔。
    需要自定义名称请用 [物品悬浮展示](item-display.md)。

### 生成一只掉落物

`int64_t spawn(ItemActorSpec const& spec)`

- 参数：
  - `spec` : `ItemActorSpec const&`<br>物品、坐标、维度与初速度（字段见上表）
- 返回值：库内掉落物 id（`>= 0`，后续调用都用它）；`-1` = `itemId` 为空、物品 id 不认识、**该维度没有玩家**，或包编码失败
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& actors = hologramlib::IHologramLib::getInstance().itemActors();

    hologramlib::ItemActorSpec spec;
    spec.itemId    = "minecraft:diamond";
    spec.count     = 1;
    spec.x         = 100.5f;
    spec.y         = 65.0f;
    spec.z         = 100.5f;
    spec.dimension = 0;
    spec.vx        = 0.0f;
    spec.vy        = 0.4f;    // 往上飞一点
    spec.vz        = 0.0f;

    int64_t id = actors.spawn(spec);
    if (id < 0) {
        return;               // 该维度没人在线 / 物品 id 不对
    }
    ```

只发给**同维度**的玩家；生成时该维度一个玩家都没有就直接返回 `-1`，不会先记下来等人来。

### 改速度与方向

`bool setMotion(int64_t id, float vx, float vy, float vz)`

- 参数：
  - `id` : `int64_t`<br>掉落物 id
  - `vx` / `vy` / `vz` : `float`<br>新速度（方块 / tick）；`0, 0, 0` = 停止
- 返回值：已设置 `true`；`false` = id 不存在（或包编码失败）
- 返回值类型：`bool`
- 示例：

    ```cpp
    actors.setMotion(id, -0.3f, 0.2f, 0.0f);   // 1 秒后换个方向
    ```

### 传送到指定坐标

`bool moveTo(int64_t id, float x, float y, float z)`

- 参数：
  - `id` : `int64_t`<br>掉落物 id
  - `x` / `y` / `z` : `float`<br>目标坐标（按 teleport 下发，客户端不会插值）
- 返回值：已传送 `true`；`false` = id 不存在（或包编码失败）
- 返回值类型：`bool`
- 示例：

    ```cpp
    actors.moveTo(id, 100.0f, 70.0f, 100.0f);
    ```

### 播放拾取动画并删除

`bool take(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>掉落物 id
  - `playerName` : `std::string const&`<br>拾取动画的接收者（动画飞向他）
- 返回值：已播放并删除 `true`；`false` = id 不存在或玩家不在线
- 返回值类型：`bool`
- 示例：

    ```cpp
    actors.take(id, "Steve");   // 飞向 Steve 然后消失，但不会给他物品
    ```

!!! warning "take 不发物品"

    `take` 只是**播放动画 + 删除实体**，物品不会进任何人的背包 —— 实际发放由调用方自己处理。
    删除包发给同维度的所有玩家，目标玩家额外收到 `TakeItemActor`。

### 删除一只掉落物

`bool destroy(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>掉落物 id
- 返回值：已删除 `true`；`false` = id 不存在
- 返回值类型：`bool`
- 示例：

    ```cpp
    actors.destroy(id);
    ```

### 删除全部掉落物

`void destroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    actors.destroyAll();
    ```

### 查询掉落物是否还在

`bool exists(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>掉落物 id
- 返回值：存在 `true`；已删除或不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (actors.exists(id)) {
        actors.setMotion(id, 0.0f, 0.0f, 0.0f);
    }
    ```

### 列出所有掉落物

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：当前所有掉落物 id；一只都没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    for (int64_t actorId : actors.getAllIds()) {
        actors.destroy(actorId);
    }
    ```

## IFormControl · 强制关闭客户端表单

只管「关掉」：表单的创建仍由 LSE、LeviLamina Form API 或 MeowMenu 负责。
适用于玩家无法用 ESC 关闭的界面、被其他插件覆盖的界面、过场前清屏、玩家下线前收尾。

### 关闭一名玩家的表单

`bool closeForm(std::string const& playerName)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名（在线，大小写敏感）
- 返回值：已发出关闭包 `true`；`false` = 玩家不在线（或服务端还没就绪）
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& forms = hologramlib::IHologramLib::getInstance().formControl();

    if (!forms.closeForm("Steve")) {
        return;   // 玩家不在线
    }
    ```

玩家没开表单时调用也返回 `true`（客户端收到关闭包不会报错）。

### 关闭所有在线玩家的表单

`int closeAllForms()`

- 参数：无
- 返回值：实际发出的关闭包数量；`0` = 没有在线玩家（或服务端还没就绪）
- 返回值类型：`int`
- 示例：

    ```cpp
    auto& forms = hologramlib::IHologramLib::getInstance().formControl();

    int n = forms.closeAllForms();
    // n = 这次关掉了几个人
    ```

## ILocatorBar · 当前不可用（已移除）

!!! warning "当前没有可用的实现"

    方位条这一组接口**已经整体删除**：`ILocatorBar`、`ILocatorBarExt` 两个类，连同 `IHologramLib` 上的
    `locatorBar()` / `locatorBarExt()` 两个访问器，在 1.36.0（插件 26.60.3）移除。

    现在头文件里没有这两个类，`getInstance()` 上也没有这两个访问器 —— 写代码时**不要再找它们**，
    也不要拿旧头文件去编译（会静默调到别的域上）。

#### 为什么做不了

- 客户端的方位条只在**有真实玩家**参与时才出现；服务端没有真实玩家时，这条 HUD 根本不画
- 自建航点必须锚定到客户端认识的实体（真实玩家 / 已下发的假玩家 / 生物），否则没有可对齐的对象
- 想走原生路径，需要构造 `ServerWaypoint` / `ServerWaypointGroup` 并注册进玩家的方位组，
  但这两个类的构造函数没有导出，`Waypoint::mWorldPos` 的 `WorldPosition` 也禁止构造

结论：协议包发得出去（字段、编码、回读校验都正常），客户端就是不渲染 ——
**插件层没有可靠办法创建「客户端认的」锚点**。

#### 曾经有什么（都不再可用）

| 历史方法 | 现状 |
|---|---|
| `setWaypoint(playerName, key, x, y, z, dimension, color, visible)` | 已删除 |
| `removeWaypoint(playerName, key)` | 已删除 |
| `clearWaypoints(playerName)` | 已删除 |
| `waypointCount(playerName)` | 已删除 |
| `hideFromOthers(playerName, hidden)` | 已删除（当时转发到 `IPlayerVisibility`） |
| `isHiddenFromOthers(playerName)` | 已删除 |

LSE 侧对应的 14 个 `locatorBar*` 导出同批删除；Endstone 的 `locatorbar.py` 一并移除。

#### ABI 影响

1.36.0 是**中部删除**：虚表因此整体前移两个槽位，按旧头文件编译的消费方必须重新编译，
否则调用会落到别的域上静默失效。

1.37.0 已把这个错误纠正回来：27 / 32 号槽以空的 `IReservedDomain` 放回
（`reservedSlot27()` / `reservedSlot32()`），虚表恢复单调布局，槽位留给后续新域。
`AbiCompat.h` 不再猜槽位——它只做运行时版本检查。

#### 现在怎么指路

`IShapeDrawer::createArrow`（线框箭头）、`IHologramText`（悬浮字）、`IParticleShape`（粒子）都能做指向提示。
26.60 线上不要再写方位条。

## 版本注记

| API · 插件版本 | 变化 |
|---|---|
| 1.32.0 · 26.51.6 | `IItemActor`、`IFormControl`、`ILocatorBar` 三个新域尾部追加；用旧头文件编译的消费方需要重新编译 |
| 1.36.0 · 26.60.3 | 删除 `ILocatorBar` / `ILocatorBarExt` 与两个访问器（虚表中部变短，**必须重新编译**）；`IItemActor` / `IFormControl` 的签名不变 |
| 1.37.0 · 26.60.4 | 27 / 32 号槽恢复为保留槽 `reservedSlot27()` / `reservedSlot32()`（空 `IReservedDomain`，无方法）；ABI stamp 1 → 2，**必须重新编译** |

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <string>

void drop_reward_and_close_forms(std::string const& playerName) {
    auto& lib = hologramlib::IHologramLib::getInstance();

    // 1) 在玩家上方生成一颗钻石，往上飞
    hologramlib::ItemActorSpec spec;
    spec.itemId    = "minecraft:diamond";
    spec.count     = 1;
    spec.x         = 100.5f;
    spec.y         = 66.0f;
    spec.z         = 100.5f;
    spec.dimension = 0;
    spec.vy        = 0.4f;

    int64_t actorId = lib.itemActors().spawn(spec);
    if (actorId < 0) {
        return;   // 该维度没人在线
    }

    // 2) 过一会儿飞向玩家并消失（物品要自己发）
    lib.itemActors().take(actorId, playerName);

    // 3) 收尾：把这个玩家当前打开的表单关掉
    lib.formControl().closeForm(playerName);

    // 4) 方位条不要指望：ILocatorBar 已移除，指路请用形状 / 悬浮字 / 粒子
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
