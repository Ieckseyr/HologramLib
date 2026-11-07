# 1.18 其它接口 · Boss 条 / HUD / 表现事件 / 侧边栏 / 隐身 / 复活 / 容器预览

这一页收拢 8 个"单功能、方法少"的接口。它们同样通过 `hologramlib::IHologramLib::getInstance()` 取：

```cpp
#include "hologramlib/HologramLib.h"
namespace hl = hologramlib;

auto& lib = hl::IHologramLib::getInstance();
```

| 接口 | 访问器 | 作用 |
|---|---|---|
| `IBossBar` | `bossBars()` | 每个玩家独立显示的 Boss 血条 |
| `IHud` | `hud()` | 客户端 HUD 元素显隐 |
| `IActorEffect` | `actorEffects()` | 给实体播受伤/死亡/任意 `ActorEvent` |
| `ISidebar` | `sidebar()` | 每人独立的侧边栏计分板 |
| `IPlayerVisibility` | `playerVisibility()` | 屏蔽某玩家对其他人的出站包 |
| `IPlayerRespawn` | `playerRespawn()` | 补发 `RespawnPacket`，把客户端拉出死亡界面 |
| `IContainerPreview` | `containerPreview()` | 按坐标只读预览真实容器 |
| `IActorEffectFilter` | `actorEffectFilter()` | 拦截/屏蔽表现事件 |

所有方法都是"只发包，不动服务端状态"：不改血量、不改记分板、不改背包。

---

## IBossBar · Boss 血条

### 给玩家建一条 Boss 条

`int64_t add(std::string const& playerName, std::string const& title, float healthPercent, BossBarColor color = BossBarColor::Purple, BossBarOverlay overlay = BossBarOverlay::Progress)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名（在线，大小写敏感）
  - `title` : `std::string const&`<br>显示名，支持 `§` 颜色码
  - `healthPercent` : `float`<br>血量比例 `0.0 ~ 1.0`（`0.65` = 65%）
  - `color` : `BossBarColor`<br>`Pink` / `Blue` / `Red` / `Green` / `Yellow` / `Purple`（默认）/ `Magenta` / `White`
  - `overlay` : `BossBarOverlay`<br>`Progress`（默认）/ `Notched6` / `Notched10` / `Notched12`
- 返回值：Boss 条 id（从 `1` 递增）；玩家不在线或参数非法返回 `0`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& bar = lib.bossBars();
    int64_t id = bar.add("Steve", "§c深渊守卫者", 0.65f);
    if (id > 0) {
        bar.setTitle(id, "§c深渊守卫者 · 狂暴");
        bar.setHealth(id, 0.3f);
    }
    ```

### 修改标题

`bool setTitle(int64_t id, std::string const& title)`

- 参数：
  - `id` : `int64_t`<br>Boss 条 id
  - `title` : `std::string const&`<br>新标题
- 返回值：已下发 `true`；id 不存在或玩家已离线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    bar.setTitle(id, "§e新的名字");
    ```

### 修改血量

`bool setHealth(int64_t id, float healthPercent)`

- 参数：
  - `id` : `int64_t`<br>Boss 条 id
  - `healthPercent` : `float`<br>`0.0 ~ 1.0`，超出会被夹紧
- 返回值：已下发 `true`；id 不存在或玩家已离线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    bar.setHealth(id, 0.5f);
    ```

### 修改颜色与样式

`bool setStyle(int64_t id, BossBarColor color, BossBarOverlay overlay)`

- 参数：
  - `id` : `int64_t`<br>Boss 条 id
  - `color` : `BossBarColor`<br>颜色枚举
  - `overlay` : `BossBarOverlay`<br>样式枚举
- 返回值：已下发 `true`；id 不存在或玩家已离线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    bar.setStyle(id, hl::BossBarColor::Red, hl::BossBarOverlay::Notched10);
    ```

### 移除一条

`bool remove(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>Boss 条 id
- 返回值：已下发移除 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    bar.remove(id);
    ```

### 移除某玩家的全部

`void removeAll(std::string const& playerName)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    bar.removeAll("Steve");
    ```

### 查询数量

`int count(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：该玩家当前的 Boss 条数量；玩家不在线返回 `0`
- 返回值类型：`int`
- 示例：

    ```cpp
    int n = bar.count("Steve");
    ```

---

## IHud · HUD 显隐

`HudElement` 编号：`PaperDoll=0` `Armor=1` `ToolTips=2` `TouchControls=3` `Crosshair=4` `HotBar=5` `Health=6` `ProgressBar=7` `Hunger=8` `AirBubbles=9`（共 13 个，`0..12`）。

### 设置单个元素

`bool setElement(std::string const& playerName, HudElement element, bool visible)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
  - `element` : `HudElement`<br>要改的元素
  - `visible` : `bool`<br>`true` 显示 / `false` 隐藏
- 返回值：已下发 `true`；玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& hud = lib.hud();
    hud.setElement("Steve", hl::HudElement::Crosshair, false);
    hud.setElement("Steve", hl::HudElement::HotBar, false);
    ```

### 批量设置（掩码）

`int setElements(std::string const& playerName, int mask, bool visible)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
  - `mask` : `int`<br>位掩码，第 `n` 位对应 `HudElement(n)`
  - `visible` : `bool`<br>`true` = 全部恢复显示（忽略 `mask`，重置 13 个）；`false` = 只保留 `mask` 里置位的元素，其余隐藏
- 返回值：受影响元素数；`visible=true` 固定返回 `13`；玩家不在线返回 `0`
- 返回值类型：`int`
- 示例：

    ```cpp
    // 只保留准星(4)与快捷栏(5)，其余隐藏
    int changed = hud.setElements("Steve", (1 << 4) | (1 << 5), false);

    // 全部恢复
    hud.setElements("Steve", 0, true);
    ```

### 恢复全部

`bool resetAll(std::string const& playerName)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：已下发重置 `true`；玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    hud.resetAll("Steve");
    ```

---

## IActorEffect · 实体表现事件

目标一律是实体的 **runtimeId**（`IPlayerSensing` / `IPlayerNpc` 一类接口返回的那个），不是插件层 id。

### 受伤动画

`bool playHurt(int64_t entityRuntimeId)`

- 参数：
  - `entityRuntimeId` : `int64_t`<br>目标实体 runtimeId
- 返回值：已下发 `true`；实体不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.actorEffects().playHurt(rid);   // 对应 ActorEvent::HURT = 2
    ```

### 死亡动画

`bool playDeath(int64_t entityRuntimeId)`

- 参数：
  - `entityRuntimeId` : `int64_t`<br>目标实体 runtimeId
- 返回值：已下发 `true`；实体不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.actorEffects().playDeath(rid);
    ```

### 立即消失

`bool playInstantDeath(int64_t entityRuntimeId)`

- 参数：
  - `entityRuntimeId` : `int64_t`<br>目标实体 runtimeId
- 返回值：已下发 `true`；实体不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.actorEffects().playInstantDeath(rid);
    ```

### 发任意 ActorEvent

`bool play(int64_t entityRuntimeId, int eventId, int data = 0)`

- 参数：
  - `entityRuntimeId` : `int64_t`<br>目标实体 runtimeId
  - `eventId` : `int`<br>`ActorEvent` 枚举值
  - `data` : `int`<br>事件附带数据，默认 `0`
- 返回值：已下发 `true`；实体不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.actorEffects().play(rid, 2, 0);   // 2 = HURT
    ```

!!! note "播死亡动画不会杀死服务端实体"

    只是给客户端发了一个包。要让玩家从死亡界面回来，用下面的 `IPlayerRespawn`。

---

## IPlayerRespawn · 复活

配合死亡动画使用：客户端进入死亡界面后会发 `PlayerAction(Respawn)`，但服务端认为玩家一直活着，
**不会回 `RespawnPacket`**，客户端就卡住了。这一组补的就是那个包。

### 原地复活

`bool respawn(std::string const& playerName, int state = 2)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
  - `state` : `int`<br>`PlayerRespawnState`，默认 `2`（客户端就绪）
- 返回值：已下发 `true`；玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.actorEffects().playDeath(rid);
    // 约 1 秒后把客户端拉回来
    lib.playerRespawn().respawn("Steve");
    ```

### 指定坐标复活

`bool respawnAt(std::string const& playerName, float x, float y, float z, int state = 2)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
  - `x` / `y` / `z` : `float`<br>复活点世界坐标
  - `state` : `int`<br>同 `respawn`
- 返回值：已下发 `true`；玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.playerRespawn().respawnAt("Steve", 100.5f, 66.0f, 100.5f);
    ```

!!! note "服务端实体完全不动"

    血量、位置、维度、背包都不变，也不会触发任何死亡/重生事件。它只影响客户端界面。

---

## ISidebar · 侧边栏计分板

只发包，不读也不改 BDS 自己的记分板。行索引 `0` 在顶部。

### 显示或更新

`bool set(std::string const& playerName, std::string const& title, std::vector<std::string> const& lines)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名（在线）
  - `title` : `std::string const&`<br>顶栏标题，支持 `§`
  - `lines` : `std::vector<std::string> const&`<br>行内容，自上而下；空数组等于移除
- 返回值：已下发 `true`；玩家不在线或参数非法 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.sidebar().set("Steve", "§6深渊竞技场", {"§7第一行", "§7第二行"});
    ```

### 只改标题

`bool setTitle(std::string const& playerName, std::string const& title)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
  - `title` : `std::string const&`<br>新标题
- 返回值：已下发 `true`；该玩家当前没有侧边栏 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.sidebar().setTitle("Steve", "§c血战阶段");
    ```

### 只改某一行

`bool setLine(std::string const& playerName, int index, std::string const& text)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
  - `index` : `int`<br>行索引，`0` 起
  - `text` : `std::string const&`<br>新文本
- 返回值：已下发 `true`；没有侧边栏或索引越界 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.sidebar().setLine("Steve", 0, "§7新的第一行");
    ```

### 移除

`bool remove(std::string const& playerName)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：已下发移除 `true`；本来就没有 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.sidebar().remove("Steve");
    ```

### 移除全部

`void removeAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.sidebar().removeAll();
    ```

### 是否在显示

`bool isShown(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：有侧边栏 `true`；没有 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    bool shown = lib.sidebar().isShown("Steve");
    ```

### 行数

`int lineCount(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：当前行数；没有侧边栏返回 `0`
- 返回值类型：`int`
- 示例：

    ```cpp
    int n = lib.sidebar().lineCount("Steve");
    ```

---

## IPlayerVisibility · 出站包屏蔽

两种方式互不替代：**掩码**只停更新（已出生的人不会消失），**隐身**会让其他客户端立刻看不到。

### 设置掩码

`bool setMask(std::string const& playerName, PlayerPacketMask const& mask, bool alsoSelf = false)`

- 参数：
  - `playerName` : `std::string const&`<br>被屏蔽的玩家
  - `mask` : `PlayerPacketMask const&`<br>五个开关：`spawn` / `tab` / `motion` / `state` / `skin`，`true` = 屏蔽该类包
  - `alsoSelf` : `bool`<br>`true` 时该玩家自己也收不到自己的这些包（默认 `false`）
- 返回值：已应用 `true`；玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    hl::PlayerPacketMask mask;
    mask.spawn  = true;
    mask.tab    = true;
    lib.playerVisibility().setMask("Steve", mask);
    ```

### 隐身

`bool setHidden(std::string const& playerName, bool hidden)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
  - `hidden` : `bool`<br>`true` 让其他人立刻看不到；`false` 取消
- 返回值：已应用 `true`；玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.playerVisibility().setHidden("Steve", true);
    ```

### 恢复

`bool restore(std::string const& playerName)`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：已恢复 `true`；玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    lib.playerVisibility().restore("Steve");   // 清掩码 + 取消隐身 + 重发出生包
    ```

### 查询掩码

`PlayerPacketMask maskOf(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：当前掩码副本；没有记录时全 `false`
- 返回值类型：`PlayerPacketMask`
- 示例：

    ```cpp
    auto m = lib.playerVisibility().maskOf("Steve");
    bool tabHidden = m.tab;
    ```

### 查询隐身

`bool isHidden(std::string const& playerName) const`

- 参数：
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：处于隐身 `true`；否则 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    bool hidden = lib.playerVisibility().isHidden("Steve");
    ```

---

## IContainerPreview · 容器预览

### 按坐标预览

`int64_t preview(std::string const& playerName, int dim, int x, int y, int z, std::string const& title = {})`

- 参数：
  - `playerName` : `std::string const&`<br>目标玩家
  - `dim` : `int`<br>维度 id（`0` 主世界 / `1` 下界 / `2` 末地）
  - `x` / `y` / `z` : `int`<br>方块坐标
  - `title` : `std::string const&`<br>界面标题，空 = 用默认标题
- 返回值：预览 id（`>0`）；玩家不在线、坐标没有容器或参数非法 `<=0`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    int64_t pid = lib.containerPreview().preview("Steve", 0, 100, 64, 100, "§6仓库");
    ```

!!! note "只读"

    预览不会把容器内容同步到服务端；玩家在界面里的操作不会写回真实容器。

---

## IActorEffectFilter · 表现事件拦截

用于"服务端想自己处理受伤/死亡表现"的场景：拦掉库自动发的表现事件包。

### 全局拦截

`void block(int eventId)`

- 参数：
  - `eventId` : `int`<br>`ActorEvent` 枚举值
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.actorEffectFilter().block(3);   // 3 = DEATH
    ```

### 取消全局拦截

`void unblock(int eventId)`

- 参数：
  - `eventId` : `int`<br>`ActorEvent` 枚举值
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.actorEffectFilter().unblock(3);
    ```

### 对某个玩家拦截

`void blockForPlayer(std::string const& playerName, int eventId)`

- 参数：
  - `playerName` : `std::string const&`<br>观察者玩家名（谁收不到这个包）
  - `eventId` : `int`<br>`ActorEvent` 枚举值
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.actorEffectFilter().blockForPlayer("Steve", 2);   // Steve 看不到受伤
    ```

### 取消对某个玩家的拦截

`void unblockForPlayer(std::string const& playerName, int eventId)`

- 参数：
  - `playerName` : `std::string const&`<br>观察者玩家名
  - `eventId` : `int`<br>`ActorEvent` 枚举值
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.actorEffectFilter().unblockForPlayer("Steve", 2);
    ```

### 对某个实体拦截

`void blockForEntity(int64_t runtimeId, int eventId)`

- 参数：
  - `runtimeId` : `int64_t`<br>目标实体 runtimeId
  - `eventId` : `int`<br>`ActorEvent` 枚举值
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.actorEffectFilter().blockForEntity(rid, 2);
    ```

### 取消对某个实体的拦截

`void unblockForEntity(int64_t runtimeId, int eventId)`

- 参数：
  - `runtimeId` : `int64_t`<br>目标实体 runtimeId
  - `eventId` : `int`<br>`ActorEvent` 枚举值
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.actorEffectFilter().unblockForEntity(rid, 2);
    ```

### 清空全部拦截

`void clear()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    lib.actorEffectFilter().clear();
    ```

### 统计

`std::string stats() const`

- 参数：无
- 返回值：一行可读统计（各类拦截的条目数），用于排查
- 返回值类型：`std::string`
- 示例：

    ```cpp
    logger.info("filter: {}", lib.actorEffectFilter().stats());
    ```

---

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

namespace hl = hologramlib;

void demo(hl::IHologramLib& lib, std::string const& player) {
    // Boss 条
    int64_t bar = lib.bossBars().add(player, "§c守卫者", 1.0f);
    lib.bossBars().setHealth(bar, 0.4f);

    // 藏准星与快捷栏
    lib.hud().setElement(player, hl::HudElement::Crosshair, false);
    lib.hud().setElement(player, hl::HudElement::HotBar, false);

    // 侧边栏
    lib.sidebar().set(player, "§6竞技场", {"§7第一行", "§e积分: 0"});

    // 受伤动画 + 拦截死亡表现
    lib.actorEffects().playHurt(rid);
    lib.actorEffectFilter().block(3);

    // 从死亡界面拉回来
    lib.playerRespawn().respawn(player);
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
