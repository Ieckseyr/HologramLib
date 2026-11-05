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

### 整菜单重发（可选换标题）

`containerUpdate(id, title)`

- 参数：
  - `id` : `Integer`<br>菜单 id
  - `title` : `String`<br>新标题（支持 `§` 颜色码）；**空串 = 保留当前标题**，只做整菜单重发
- 返回值：`true` = 已重发；`false` = 菜单已不在或玩家离线
- 返回值类型：`Boolean`
- 说明：`containerSetTitle` 是「改标题 + 整菜单重发」；这个函数是你**不知道原标题、也想把整张菜单重发一遍**时的入口（内部就是 C++ 的 `IContainerMenu::update`）。
  它按**当前生效的 spec** 重发，所以不用重摆方块、不用重新 `containerOpen`，也没有打开延迟。
  注意：行数（27 / 54 格）在打开时就定了，`containerUpdate` **不会**改行数 —— 要换尺寸只能关掉重开
- 示例：

    ```js
    const containerUpdate = ll.import("HologramLib", "containerUpdate");
    containerUpdate(id, "");                    // 原样重发一遍（标题不变）
    containerUpdate(id, "§8任务列表 · 第 2 页");  // 换标题并重发
    ```

### 就地刷新一个带附魔的格子

`containerSetItemEnchanted(id, slot, type, count, damage, name, loreCsv, enchCsv)`

和 `containerSetItem` 完全一样，只多一个 `enchCsv`：该条目会按**真实附魔物品**下发 ——
客户端会渲染附魔光效，悬停也能看到附魔名（客户端自己翻译）。

- 参数：
  - 前七个参数同 `containerSetItem`
  - `enchCsv` : `String`<br>附魔列表，形如 `"9:5,13:2"`（**Bedrock 附魔数字 id : 等级**，逗号/竖线/分号/空格分隔）。
    空串 = 无附魔。id 取值见 Bedrock `Enchant::Type`（0 保护 / 9 锋利 / 13 火焰附加 / 27 绑定诅咒 …）；
    越界或等级 < 1 的项**整条忽略**（宁可少一条附魔，也不会让整格变成空气）
- 返回值：`true` = 已刷新；`false` = 菜单已不在、槽位越界或玩家离线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const containerSetItemEnchanted = ll.import("HologramLib", "containerSetItemEnchanted");
    // 锋利 V + 火焰附加 II
    containerSetItemEnchanted(id, 3, "minecraft:diamond_sword", 1, 0, "§c烈焰之刃", "§7第一行", "9:5,13:2");
    ```

> 附魔同时写进**方块实体那份 NBT**，所以关掉重开界面、换维度重发都不会丢光效。

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

### 预览一个真实容器（只读）

给玩家看**世界里某个真实容器**的当前内容：`containerPreview(playerName, dim, x, y, z)`。

```js
const id = Container.preview("Steve", 0, 120, 64, -35);
// 或自定义标题
const id2 = Container.previewTitled("Steve", 0, 120, 64, -35, "§8仓库 #3");
```

- 返回值：菜单 id（用 `containerClose(id)` 关）；`-1` 玩家不在线/坐标无效，`-2` 区块未加载，`-3` 该方块不是容器
- 能预览的容器：箱子 / 木桶 / 潜影盒 / 漏斗 / 熔炉 / 发射器 / 酿造台等（读 `Container::getContainerSize()` 与每格 `getItem`）
- 未加载的区块会**先按需加载一次**（`readOnly`，不写回存档），读完整块自然卸载
- 内容只读：**不能拿取**，服务端没有这个容器的会话（点击请求会被拒）
- 不触发真实开箱：没有开盖动画、比较器信号，其他玩家也看不到

内容会照抄格子的物品 id / 数量 / 耐久 / 自定义名 / lore，所以箱子里是什么，界面上就是什么。

---

### 假箱子的开合动画

`containerBlockEvent(menuId, b0, b1)` 在那只假箱子的坐标发一条 `BlockEventPacket`，让客户端播方块事件。
箱子盖 `b0=1` 开、`0` 合；参数原样透传，别的方块事件也能用。

```js
const containerBlockEvent = ll.import("HologramLib", "containerBlockEvent");
containerBlockEvent(id, 1, 0);   // 开盖（纯客户端表现，不影响服务端）
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
