# Boss 条与 HUD 显隐

两个独立的客户端界面能力，都只发包，不需要服务端实体。

## Boss 条（bossBar*）

```js
const { BossBar } = require("mods/HologramLib/lib/HologramLib.js");

// 给 Steve 建一条 Boss 条（血量 65%，紫色，普通进度条）
const id = BossBar.add("Steve", "深渊守卫者", 0.65, 5, 0);

// 掉血
BossBar.setHealth(id, 0.3);
// 改名
BossBar.setTitle(id, "深渊守卫者 · 狂暴");
// 换样式（颜色 3=绿，样式 2=十格）
BossBar.setStyle(id, 3, 2);
// 删除
BossBar.remove(id);
```

| 函数 | 返回 | 说明 |
|---|---|---|
| `bossBarAdd(playerName, title, healthPercent, color, overlay)` | `Integer` | 创建；`healthPercent` 是 0.0~1.0 的比例；失败 `-1` |
| `bossBarSetTitle(id, title)` | `Boolean` | 改名 |
| `bossBarSetHealth(id, healthPercent)` | `Boolean` | 改血量比例（自动夹到 0~1） |
| `bossBarSetStyle(id, color, overlay)` | `Boolean` | 改颜色与样式 |
| `bossBarRemove(id)` | `Boolean` | 删除 |
| `bossBarRemoveAll(playerName)` | `Null` | 删除该玩家全部 Boss 条 |
| `bossBarCount(playerName)` | `Integer` | 该玩家当前条数 |

颜色（`color`）：`0` 粉 · `1` 蓝 · `2` 红 · `3` 绿 · `4` 黄 · `5` 紫 · `6` 品红 · `7` 白
样式（`overlay`）：`0` 进度条 · `1` 六格 · `2` 十格 · `3` 十二格 · `4` 二十格

**每条只属于一名玩家**：给所有人看要对每名在线玩家各建一条。

Boss 条的 target 必须是一个客户端认识的真实实体，所以建条时库会在玩家所在维度的虚空里（`y = -120`，世界底部之外）放一只 `minecraft:structure_void` 的无模型掉落物当锚点，删除时一并回收。这样既不占玩家视野，也能让客户端把条画出来。

## HUD 显隐（hud*）

```js
const { Hud } = require("mods/HologramLib/lib/HologramLib.js");

Hud.setElement("Steve", 4, false);   // 藏准星
Hud.setElement("Steve", 5, false);   // 藏快捷栏
Hud.setElement("Steve", 0, false);   // 藏纸娃娃（右上角小人）
Hud.setElement("Steve", 3, false);   // 藏触屏摇杆

// 一次藏多个：bit0=纸娃娃 bit4=准星 bit5=快捷栏 bit6=血条
Hud.setElements("Steve", (1 << 0) | (1 << 4) | (1 << 5) | (1 << 6), false);

// 全部恢复默认
Hud.resetAll("Steve");
```

| 函数 | 返回 | 说明 |
|---|---|---|
| `hudSetElement(playerName, element, visible)` | `Boolean` | `visible=false` 只藏这一个元素（其余保留），`true` 恢复默认 |
| `hudSetElements(playerName, mask, visible)` | `Integer` | `visible=false` 时掩码里的元素**保留**、其余隐藏，返回被隐藏的个数 |
| `hudResetAll(playerName)` | `Boolean` | 13 项全部恢复默认 |

13 个元素（`element` 与掩码位一致）：

| 值 | 元素 | 值 | 元素 |
|---|---|---|---|
| 0 | 纸娃娃 PaperDoll | 7 | 进度条 ProgressBar |
| 1 | 护甲 Armor | 8 | 饥饿 Hunger |
| 2 | 提示 ToolTips | 9 | 氧气泡 AirBubbles |
| 3 | 触屏控制 TouchControls | 10 | 坐骑血量 HorseHealth |
| 4 | 准星 Crosshair | 11 | 状态效果 StatusEffects |
| 5 | 快捷栏 HotBar | 12 | 物品名 ItemText |
| 6 | 血条 Health | | |

协议只有两种状态：`Hide`（隐藏）与 `Reset`（恢复默认），没有单独的 "Show"，所以 `visible=true` 发的是 `Reset`。

**`Hide` 时包里的元素列表是"要保留的"**（BDS 内部就是 `ServerPlayer::hideAllExcept`）：
只藏纸娃娃 = `Hide` + 除纸娃娃外的 12 项。库已经按这个语义包好了，调用方按"我想藏哪一个"来传即可。
