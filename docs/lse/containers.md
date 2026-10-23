# 容器与背包 · LSE

虚拟容器与背包虚容器；动态刷新只发给 UI 开着的玩家。


## 快速开始

最小可跑的例子：给玩家开一个 27 格的「任务列表」，在定时器里取点击。

```js
const containerOpen       = ll.import("HologramLib", "containerOpen");
const containerSetItem    = ll.import("HologramLib", "containerSetItem");
const containerPollClicks = ll.import("HologramLib", "containerPollClicks");

const id = containerOpen("Steve", "§8任务列表", 3);   // 27 格；rows = 6 → 54 格
containerSetItem(id, 0, "minecraft:diamond_sword", 1, 0, "§b试炼", "§7点击领取|§8第二行");

setInterval(() => {                                  // 也可以挂在 mc.listen 的 tick 上
  for (const line of containerPollClicks()) {        // 每条 "player=X menuId=N slot=S closed=0|1"
    const m = line.match(/player=(\S+) menuId=(-?\d+) slot=(-?\d+) closed=(\d)/);
    if (!m) continue;
    const [, player, menuId, slot, closed] = m;
    if (closed === "1") continue;                    // 界面关闭（此时 slot = -1）
    logger.info(`${player} 在菜单 ${menuId} 点了第 ${slot} 格`);
  }
}, 200);
```

> 打开容器有**固有延迟**（默认 7 tick ≈ 350ms）：客户端要先把这个箱子方块与它的方块实体应用上去，
> 打开界面的那条包才绑得住那个位置。延迟期间填格不会丢（逐格刷新走的是另一条包），但界面会稍晚出现。

---

### 自测

`tests/lse/HoloBoxUITest/` 里的脚本把这一页的接口按顺序跑一遍（指令 `/boxui`）：先 22 条断言，
再开一个 54 格界面，每格对应一个行为 —— 就地刷新、换标题、可交互、发物品、翻页、关闭。
点击回传和控制台的 `[PASS]` / `[FAIL]` 对着看。自己写业务时也可以照它的顺序走。

## 虚拟容器（container*）

在客户端摆一个箱子方块 + 方块实体，再开箱子界面：**服务端根本没有这个容器**，条目只是「摆在那里」。
它的点击 = 一次物品拾取，**任何输入设备都会发包**（触屏也能点），所以它最适合当「点了有回调的列表界面」。
点击/关闭同样走轮询队列 `containerPollClicks`。

### 打开一个空容器界面

`containerOpen(playerName, title, rows)`

- 参数：
  - `playerName` : `String`<br>目标玩家名，必须在线
  - `title` : `String`<br>界面标题（支持 `§` 颜色码）；空串 = 用默认标题
  - `rows` : `Integer`<br>容器行数，`3` = 小容器 27 格，`6` = 大容器 54 格
- 返回值：菜单 id（之后用 `containerSetItem` 逐格填）；`-1` = 打开失败（玩家不在线、容器 id 用尽）
- 返回值类型：`Integer`
- 示例：

    ```js
    const containerOpen = ll.import("HologramLib", "containerOpen");
    const id = containerOpen("Steve", "§8任务列表", 3);
    ```

### 就地刷新一个格子

`containerSetItem(id, slot, type, count, damage, name, loreCsv)`

- 参数：
  - `id` : `Integer`<br>菜单 id
  - `slot` : `Integer`<br>格子下标（0 起），也就是点击回传里的 `slot`
  - `type` : `String`<br>物品 id，如 `"minecraft:diamond_sword"`；空串 = 清空该格
  - `count` : `Integer`<br>数量，小于 1 按 1 处理
  - `damage` : `Integer`<br>物品的附加数据值，一般填 `0`
  - `name` : `String`<br>自定义名（支持 `§` 颜色码），空串 = 无
  - `loreCsv` : `String`<br>描述行，用 `\n` 或 `|` 分行，空串 = 没有描述行
- 返回值：`true` = 已刷新（发一条单格更新，客户端就地换掉那一格，无延迟、无闪烁）；`false` = 菜单已不在、槽位越界或玩家离线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const containerSetItem = ll.import("HologramLib", "containerSetItem");
    containerSetItem(id, 0, "minecraft:diamond_sword", 1, 0, "§b试炼", "§7点击领取|§8第二行");
    containerSetItem(id, 1, "", 1, 0, "", "");   // 清空 1 号格
    ```

### 就地换掉界面标题

`containerSetTitle(id, title)`

- 参数：
  - `id` : `Integer`<br>菜单 id
  - `title` : `String`<br>新标题（支持 `§` 颜色码）
- 返回值：`true` = 已换（重发方块实体 + 界面包，不重摆方块、不重开界面）；`false` = 菜单已不在或玩家离线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const containerSetTitle = ll.import("HologramLib", "containerSetTitle");
    containerSetTitle(id, "§8任务列表 · 第 2 页");
    ```

### 打开或关闭可交互模式

`containerSetInteractive(id, on)`

- 参数：
  - `id` : `Integer`<br>菜单 id
  - `on` : `Boolean`<br>`true` = 可交互（物品真的留在新格子，只接整条请求都落在本容器里的动作）；`false`（默认）= 只回传点击，物品被服务端打回原位
- 返回值：`true` = 已切换；`false` = 菜单已不在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const containerSetInteractive = ll.import("HologramLib", "containerSetInteractive");
    containerSetInteractive(id, true);   // 变成「能真的搬东西」的容器
    ```

### 取走并清空容器点击事件

`containerPollClicks()`

- 返回值：每条形如 `player=X menuId=N slot=S closed=0|1`；`slot` = 被点击的格子下标，`closed=1` 表示界面被关闭（此时 `slot=-1`）；队列空时返回空数组
- 返回值类型：`Array<String>`
- 示例：

    ```js
    const containerPollClicks = ll.import("HologramLib", "containerPollClicks");
    setInterval(() => {
      for (const line of containerPollClicks()) logger.info(line);
    }, 200);
    ```

### 丢弃容器队列里的点击

`containerClearClicks()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const containerClearClicks = ll.import("HologramLib", "containerClearClicks");
    containerClearClicks();   // 重开界面前把旧点击扔掉
    ```

### 查询容器菜单是否还在

`containerIsOpen(id)`

- 参数：
  - `id` : `Integer`<br>菜单 id
- 返回值：`true` = 菜单存在；`false` = 已关闭或不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const containerIsOpen = ll.import("HologramLib", "containerIsOpen");
    if (containerIsOpen(id)) containerSetTitle(id, "§8任务列表");
    ```

### 列出所有打开的容器菜单

`containerGetIds()`

- 返回值：菜单 id 数组；没有打开的菜单时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const containerGetIds = ll.import("HologramLib", "containerGetIds");
    logger.info(`当前有 ${containerGetIds().length} 个容器界面`);
    ```

### 关掉一个容器界面

`containerClose(id)`

- 参数：
  - `id` : `Integer`<br>菜单 id
- 返回值：`true` = 已关闭（客户端那一格方块会被改回真实方块）；`false` = 该 id 不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const containerClose = ll.import("HologramLib", "containerClose");
    containerClose(id);
    ```

### 关掉所有容器界面

`containerCloseAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const containerCloseAll = ll.import("HologramLib", "containerCloseAll");
    containerCloseAll();
    ```

---

## 背包虚容器（fakeInv*）

伪造内容覆盖玩家背包的 36 格：`0..8` 是快捷栏，`9..35` 是主背包。
点击走轮询队列 `fakeInvPollClicks`，**只有「伪造且非空」的格子**被点击才会有回传。

### 下发伪造背包并周期重发

`fakeInvApply(playerName, itemsSpec, refreshIntervalTicks)`

- 参数：
  - `playerName` : `String`<br>目标玩家名，必须在线
  - `itemsSpec` : `String`<br>整份伪造内容。每条 `槽位|物品id|数量|名字|描述行~描述行`，条目之间用 `;` 或换行分隔；槽位 `0..35`（越界的条目会被跳过），数量留空按 1，描述行用 `~` 分隔
  - `refreshIntervalTicks` : `Integer`<br>周期重发间隔（tick）；`0` = 关掉周期重发，小于 5 的值按 5 执行（最快 5 tick）
- 返回值：`true` = 已下发；`false` = 玩家不在线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const fakeInvApply = ll.import("HologramLib", "fakeInvApply");
    // 快捷栏 0 号一把剑、1 号 64 个钻石；槽位 9 = 主背包第一格
    fakeInvApply("Steve", "0|minecraft:diamond_sword|1|§c虚容之剑|§7攻击力 +7;1|minecraft:diamond|64|§b虚容钻石", 20);
    ```

### 改伪造背包里的一个格子

`fakeInvSetSlot(playerName, slot, type, count, damage, name, loreCsv)`

- 参数：
  - `playerName` : `String`<br>目标玩家名
  - `slot` : `Integer`<br>槽位 `0..35`（`0..8` 快捷栏，`9..35` 主背包）
  - `type` : `String`<br>物品 id；空串 = 清空该格
  - `count` : `Integer`<br>数量，小于 1 按 1 处理
  - `damage` : `Integer`<br>物品的附加数据值，一般填 `0`
  - `name` : `String`<br>自定义名（支持 `§` 颜色码），空串 = 无
  - `loreCsv` : `String`<br>描述行，用 `~` 分隔，空串 = 没有描述行
- 返回值：`true` = 已改（单格更新，无延迟）；`false` = 该玩家还没 `apply` 过、玩家不在线或槽位越界
- 返回值类型：`Boolean`
- 示例：

    ```js
    const fakeInvSetSlot = ll.import("HologramLib", "fakeInvSetSlot");
    fakeInvSetSlot("Steve", 1, "minecraft:emerald", 3, 0, "§a奖励", "§7今天刚发~§8别乱花");
    ```

### 立刻重发一遍伪造背包

`fakeInvRefresh(playerName)`

- 参数：
  - `playerName` : `String`<br>目标玩家名
- 返回值：`true` = 已重发（打开界面后发现被盖掉时用）；`false` = 该玩家没有生效中的伪造，或玩家不在线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const fakeInvRefresh = ll.import("HologramLib", "fakeInvRefresh");
    fakeInvRefresh("Steve");   // 刚打开箱子/交易界面时补一发
    ```

### 查询玩家有没有伪造背包

`fakeInvIsActive(playerName)`

- 参数：
  - `playerName` : `String`<br>目标玩家名
- 返回值：`true` = 伪造生效中；`false` = 没有
- 返回值类型：`Boolean`
- 示例：

    ```js
    const fakeInvIsActive = ll.import("HologramLib", "fakeInvIsActive");
    if (!fakeInvIsActive("Steve")) fakeInvApply("Steve", "0|minecraft:paper|1|§7任务凭证", 20);
    ```

### 列出伪造生效中的玩家

`fakeInvGetPlayers()`

- 返回值：玩家名数组；一个都没有时是空数组
- 返回值类型：`Array<String>`
- 示例：

    ```js
    const fakeInvGetPlayers = ll.import("HologramLib", "fakeInvGetPlayers");
    logger.info(`正在伪造 ${fakeInvGetPlayers().length} 名玩家的背包`);
    ```

### 取走并清空伪造格点击

`fakeInvPollClicks()`

- 返回值：每条形如 `player=X slot=S`；`slot` = 被点击的伪造槽位；队列空时返回空数组
- 返回值类型：`Array<String>`
- 示例：

    ```js
    const fakeInvPollClicks = ll.import("HologramLib", "fakeInvPollClicks");
    setInterval(() => {
      for (const line of fakeInvPollClicks()) logger.info(line);
    }, 200);
    ```

### 丢弃队列里全部点击

`fakeInvClearClicks()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const fakeInvClearClicks = ll.import("HologramLib", "fakeInvClearClicks");
    fakeInvClearClicks();
    ```

### 撤销伪造并重发真实背包

`fakeInvClear(playerName)`

- 参数：
  - `playerName` : `String`<br>目标玩家名
- 返回值：`true` = 已撤销；`false` = 该玩家本来就没有伪造
- 返回值类型：`Boolean`
- 示例：

    ```js
    const fakeInvClear = ll.import("HologramLib", "fakeInvClear");
    fakeInvClear("Steve");   // 玩家看到的就是真实背包了
    ```

### 撤销所有玩家的伪造

`fakeInvClearAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const fakeInvClearAll = ll.import("HologramLib", "fakeInvClearAll");
    fakeInvClearAll();
    ```

---
