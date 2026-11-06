# Boss 条与 HUD 显隐

两组客户端界面能力，都只发包，不需要服务端实体。

## 快速开始

```js
const { BossBar, Hud } = require("mods/HologramLib/lib/HologramLib.js");

// Boss 条：65% 血量、紫色、普通进度条
const id = BossBar.add("Steve", "深渊守卫者", 0.65, 5, 0);
BossBar.setHealth(id, 0.3);
BossBar.setTitle(id, "深渊守卫者 · 狂暴");
BossBar.remove(id);

// HUD：藏准星和快捷栏
Hud.setElement("Steve", 4, false);
Hud.setElement("Steve", 5, false);
```

## Boss 条

### 给玩家建一条 Boss 条

`bossBarAdd(playerName, title, healthPercent, color, overlay)`

- 参数：
  - `playerName` : `String`<br>玩家名（在线，大小写敏感）
  - `title` : `String`<br>显示名，支持 `§` 颜色码
  - `healthPercent` : `Float`<br>血量比例 `0.0 ~ 1.0`（`0.65` = 65%）
  - `color` : `Integer`<br>`0` 粉 · `1` 蓝 · `2` 红 · `3` 绿 · `4` 黄 · `5` 紫 · `6` 品红 · `7` 白
  - `overlay` : `Integer`<br>`0` 进度条 · `1` 六格 · `2` 十格 · `3` 十二格 · `4` 二十格
- 返回值：库内 Boss 条 id；失败返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const add = ll.import("HologramLib", "bossBarAdd");
    const id = add("Steve", "深渊守卫者", 0.65, 5, 0);
    ```

### 改标题

`bossBarSetTitle(id, title)`

- 参数：
  - `id` : `Integer`<br>`bossBarAdd` 返回的 id
  - `title` : `String`<br>新显示名
- 返回值：`true` 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "bossBarSetTitle")(id, "深渊守卫者 · 狂暴");
    ```

### 改血量比例

`bossBarSetHealth(id, healthPercent)`

- 参数：
  - `id` : `Integer`<br>Boss 条 id
  - `healthPercent` : `Float`<br>新比例，内部自动夹到 `0 ~ 1`
- 返回值：`true` 已改，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "bossBarSetHealth")(id, 0.3);
    ```

### 改颜色与样式

`bossBarSetStyle(id, color, overlay)`

- 参数：
  - `id` : `Integer`<br>Boss 条 id
  - `color` : `Integer`<br>颜色枚举，同 `bossBarAdd`
  - `overlay` : `Integer`<br>样式枚举，同 `bossBarAdd`
- 返回值：`true` 已改，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "bossBarSetStyle")(id, 3, 2);
    ```

### 删除一条

`bossBarRemove(id)`

- 参数：
  - `id` : `Integer`<br>Boss 条 id
- 返回值：`true` 已删除（锚点实体一并回收），失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "bossBarRemove")(id);
    ```

### 删除该玩家全部 Boss 条

`bossBarRemoveAll(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：无
- 返回值类型：`Null`
- 示例：

    ```js
    ll.import("HologramLib", "bossBarRemoveAll")("Steve");
    ```

### 查询条数

`bossBarCount(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：该玩家当前 Boss 条数量
- 返回值类型：`Integer`
- 示例：

    ```js
    ll.import("HologramLib", "bossBarCount")("Steve");
    ```

**每条只属于一名玩家**：要给所有人看，就对每名在线玩家各建一条。

Boss 条的 target 必须是客户端认识的真实实体，所以建条时库会在玩家所在维度的虚空里（`y = -120`）放一只 `minecraft:structure_void` 的无模型掉落物当锚点，删除时一并回收。

## HUD 显隐

### 显示或隐藏单个元素

`hudSetElement(playerName, element, visible)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `element` : `Integer`<br>元素编号 `0 ~ 12`（见下表）
  - `visible` : `Boolean`<br>`false` 只藏这一个元素（其余保留），`true` 恢复默认
- 返回值：`true` 已下发，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "hudSetElement")("Steve", 4, false);   // 藏准星
    ```

### 按掩码批量设置

`hudSetElements(playerName, mask, visible)`

- 参数：
  - `playerName` : `String`<br>玩家名
  - `mask` : `Integer`<br>位掩码，bit0=纸娃娃、bit4=准星、bit5=快捷栏、bit6=血条……（位数与元素编号一致）
  - `visible` : `Boolean`<br>`false` 时掩码里的元素**保留**、其余隐藏
- 返回值：被隐藏的元素个数
- 返回值类型：`Integer`
- 示例：

    ```js
    const mask = (1 << 0) | (1 << 4) | (1 << 5) | (1 << 6);
    ll.import("HologramLib", "hudSetElements")("Steve", mask, false);
    ```

### 全部恢复默认

`hudResetAll(playerName)`

- 参数：
  - `playerName` : `String`<br>玩家名
- 返回值：`true` 已恢复，失败 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    ll.import("HologramLib", "hudResetAll")("Steve");
    ```

## 元素编号

| 值 | 元素 | 值 | 元素 |
|---|---|---|---|
| 0 | 纸娃娃 PaperDoll | 7 | 进度条 ProgressBar |
| 1 | 护甲 Armor | 8 | 饥饿 Hunger |
| 2 | 提示 ToolTips | 9 | 氧气泡 AirBubbles |
| 3 | 触屏控制 TouchControls | 10 | 坐骑血量 HorseHealth |
| 4 | 准星 Crosshair | 11 | 状态效果 StatusEffects |
| 5 | 快捷栏 HotBar | 12 | 物品名 ItemText |
| 6 | 血条 Health | | |

## 注意

- 协议只有 `Hide`（隐藏）与 `Reset`（恢复默认）两种状态，没有单独的 "Show"，所以 `visible=true` 发的是 `Reset`。
- `Hide` 时包里的元素列表是"要保留的"（BDS 内部是 `ServerPlayer::hideAllExcept`）；库已经按"我想藏哪一个"包好了，调用方按直觉传即可。
