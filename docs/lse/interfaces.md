# 界面 · LSE

村民交易菜单与 NPC 对话框；点击通过轮询队列回传。


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

## 交易菜单（trade*）

协议层自建交易界面：打开界面 + 摆出交易表，还可以换载体（村民 / 流浪商人）。
**本域没有点击回调** —— 结算结果只能通过 `tradePollSettlements` 取；要「点条目就有回调」的列表界面请用 `container*`。

### 打开一个交易界面

`tradeOpen(playerName, tradeType, tier, experience, realTrades, carrierIdentifier)`

- 参数：
  - `playerName` : `String`<br>目标玩家名，必须在线（不在线直接返回 `-1`）
  - `tradeType` : `String`<br>交易类型，如 `"entity.villager.butcher"`、`"entity.villager.priest"`；可填任意自定义串，客户端认不出的键会原样显示
  - `tier` : `Integer`<br>村民显示栏值，1 基：1=新手 2=学徒 3=老手 4=专家 5=大师（超出会被夹紧）
  - `experience` : `Integer`<br>经验条当前经验
  - `realTrades` : `Boolean`<br>`false`（推荐）= 纯协议层展示，服务端没有交易表，付费放不进槽、点了不成交；`true` = 给载体装真实交易表并交给服务端，玩家真的能成交、物品真的消耗
  - `carrierIdentifier` : `String`<br>载体实体类型，空串 = `minecraft:villager_v2`，也可填 `minecraft:wandering_trader`
- 返回值：菜单 id，后续函数都用它；`-1` = 打开失败（玩家不在线等）
- 返回值类型：`Integer`
- 示例：

    ```js
    const tradeOpen = ll.import("HologramLib", "tradeOpen");
    // 只展示：屠夫交易表，1 级、0 经验，载体是村民
    const id = tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
    ```

### 追加一条交易并就地重发

`tradeAddOffer(id, buyAType, buyACount, buyBType, buyBCount, sellType, sellCount, sellName, tier, locked)`

- 参数：
  - `id` : `Integer`<br>`tradeOpen` 返回的菜单 id
  - `buyAType` : `String`<br>付费物品 A 的物品 id（必填）
  - `buyACount` : `Integer`<br>付费 A 的数量，小于 1 按 1 处理
  - `buyBType` : `String`<br>第二付费物品 id，空串 = 没有第二付费项
  - `buyBCount` : `Integer`<br>付费 B 的数量，小于 1 按 1 处理
  - `sellType` : `String`<br>产出的物品 id
  - `sellCount` : `Integer`<br>产出数量，小于 1 按 1 处理
  - `sellName` : `String`<br>产出的自定义名（支持 `§` 颜色码），空串 = 无
  - `tier` : `Integer`<br>这条交易解锁的显示栏值（1 基）；高于菜单的 `tier` 时客户端把它显示为未解锁
  - `locked` : `Boolean`<br>`true` = 无论菜单 `tier` 多高，这条都显示为未解锁
- 返回值：`true` = 已追加并重发（不重开界面）；`false` = 菜单已不在（玩家离线或界面已关）
- 返回值类型：`Boolean`
- 示例：

    ```js
    const tradeAddOffer = ll.import("HologramLib", "tradeAddOffer");
    // 3 个绿宝石 + 1 个任意物品 → 1 个钻石，显示名「§b钻石」
    tradeAddOffer(id, "minecraft:emerald", 3, "", 1, "minecraft:diamond", 1, "§b钻石", 1, false);
    ```

### 改显示栏值与经验条

`tradeSetTier(id, tier, experience)`

- 参数：
  - `id` : `Integer`<br>菜单 id
  - `tier` : `Integer`<br>新的显示栏值（1 基，1=新手 … 5=大师）
  - `experience` : `Integer`<br>新的经验条当前经验
- 返回值：`true` = 已改并就地重发；`false` = 菜单已不在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const tradeSetTier = ll.import("HologramLib", "tradeSetTier");
    tradeSetTier(id, 3, 120);   // 抬到 3 级、经验条给 120
    ```

### 切换纯协议层真结算

`tradeSetSettle(id, on)`

- 参数：
  - `id` : `Integer`<br>菜单 id
  - `on` : `Boolean`<br>`true` = 开真结算（库接住付费放置/取回/成交，从玩家背包扣付费、把产物发进背包）；`false` = 回到纯展示
- 返回值：`true` = 已切换（只对 `realTrades = false` 的那条路径有效）；`false` = 菜单已不在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const tradeSetSettle = ll.import("HologramLib", "tradeSetSettle");
    tradeSetSettle(id, true);    // 只有纯协议层路径（realTrades = false）有效
    ```

### 查询菜单是否真结算

`tradeIsSettle(id)`

- 参数：
  - `id` : `Integer`<br>菜单 id
- 返回值：`true` = 真结算开着；`false` = 没开，或菜单已不在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const tradeIsSettle = ll.import("HologramLib", "tradeIsSettle");
    if (tradeIsSettle(id)) logger.info("这个菜单会真的扣物品");
    ```

### 取走并清空结算队列

`tradePollSettlements()`

- 返回值：每次成交（成功或失败）各一条，每条形如 `player=Steve menuId=3 offer=1 ok=1 reason=`；`offer` = 成交的是第几条交易（`-1` = 定位不到），`ok=1` = 已真结算（付费已扣、产物已发），`ok=0` 时 `reason` 给原因（如 `not-owner` / `payment-missing` / `inventory-full`）；队列空时返回空数组
- 返回值类型：`Array<String>`
- 示例：

    ```js
    const tradePollSettlements = ll.import("HologramLib", "tradePollSettlements");
    setInterval(() => {
      for (const line of tradePollSettlements()) {
        const m = line.match(/player=(\S+) menuId=(-?\d+) offer=(-?\d+) ok=(\d) reason=(\S*)/);
        if (m && m[4] === "1") logger.info(`${m[1]} 成交了第 ${m[3]} 条交易`);
      }
    }, 200);
    ```

### 查询菜单是否还开着

`tradeIsOpen(id)`

- 参数：
  - `id` : `Integer`<br>菜单 id
- 返回值：`true` = 菜单存在；`false` = 已关闭或从来不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const tradeIsOpen = ll.import("HologramLib", "tradeIsOpen");
    logger.info(tradeIsOpen(id) ? "界面还开着" : "已经关了");
    ```

### 列出所有打开的交易菜单

`tradeGetIds()`

- 返回值：菜单 id 数组；没有打开的菜单时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const tradeGetIds = ll.import("HologramLib", "tradeGetIds");
    for (const menuId of tradeGetIds()) logger.info(`菜单 ${menuId}`);
    ```

### 关掉一个交易菜单

`tradeClose(id)`

- 参数：
  - `id` : `Integer`<br>菜单 id
- 返回值：`true` = 已关闭（客户端界面收起，载体实体一起删掉）；`false` = 该 id 不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const tradeClose = ll.import("HologramLib", "tradeClose");
    tradeClose(id);
    ```

### 关掉所有交易菜单

`tradeCloseAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const tradeCloseAll = ll.import("HologramLib", "tradeCloseAll");
    tradeCloseAll();   // 例如在服务器关闭或重载脚本时收尾
    ```

---

## NPC 对话框（npcDialog*）

下发对话界面（NPC 名 + 正文 + 按钮表），并合成一只只为该玩家可见的 NPC 载体实体。
按钮表完全由服务端逐玩家生成 —— 「按权限/tag/任务状态决定按钮」就是在调用前自己算好按钮串。
点击/关闭走轮询队列 `npcDialogPollClicks`。

按钮串格式：按钮之间用 `;` 分隔（空段跳过），每个按钮内部用 `|` 分成
`label|actionId|mode|命令1,命令2`（后三项可省）；`mode`：`0`=普通按钮 `1`=关闭 `2`=打开；`label` 留空会显示成「按钮」。

### 打开一个 NPC 对话框

`npcDialogOpen(playerName, npcName, sceneName, dialogue, buttonsSpec, carrierIdentifier)`

- 参数：
  - `playerName` : `String`<br>目标玩家名，必须在线
  - `npcName` : `String`<br>对话框里显示的名字（支持 `§` 颜色码），空串 = 默认名
  - `sceneName` : `String`<br>场景名，多层级对话靠它路由（点击回传里会带回来），空串 = 默认场景
  - `dialogue` : `String`<br>正文
  - `buttonsSpec` : `String`<br>按钮串（格式见本节开头）
  - `carrierIdentifier` : `String`<br>载体实体类型，空串 = `minecraft:npc`；必须是 NPC 家族（客户端的对话界面与该实体类型绑定，换成村民等原版实体弹不出来）
- 返回值：对话 id（同一玩家重复调用会先关掉上一层）；`-1` = 打开失败（玩家不在线、载体生成失败）
- 返回值类型：`Integer`
- 示例：

    ```js
    const npcDialogOpen = ll.import("HologramLib", "npcDialogOpen");
    const id = npcDialogOpen("Steve", "§e村长", "main", "要来点任务吗？",
                             "§a接受|main#0|0|;;§c离开|main#1|1|", "");
    ```

### 就地换掉正文与按钮

`npcDialogUpdate(id, dialogue, buttonsSpec)`

- 参数：
  - `id` : `Integer`<br>对话 id
  - `dialogue` : `String`<br>新正文
  - `buttonsSpec` : `String`<br>新按钮串（格式同 `npcDialogOpen`）
- 返回值：`true` = 已重发（NPC 名、场景名、载体类型保留不变）；`false` = 该对话已不在或玩家离线
- 返回值类型：`Boolean`
- 示例：

    ```js
    const npcDialogUpdate = ll.import("HologramLib", "npcDialogUpdate");
    npcDialogUpdate(id, "任务进度：3/10", "§a交任务|main#0|0|");   // 界面仍开着时的微调
    ```

### 换对话框的头像与模型

`npcDialogSetAvatar(id, skinVariant, avatarViewSpec)`

- 参数：
  - `id` : `Integer`<br>对话 id
  - `skinVariant` : `Integer`<br>NPC 内置皮肤变体，`0..59`；`-1` = 默认（不下发皮肤项）
  - `avatarViewSpec` : `String`<br>在载体上叠一层视图覆盖，spec 语法同 `viewEntity`（如 `"name=§6酒保"`），空串 = 不叠
- 返回值：`true` = 已重开（就地更新，不闪烁）；`false` = 该对话已不在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const npcDialogSetAvatar = ll.import("HologramLib", "npcDialogSetAvatar");
    npcDialogSetAvatar(id, -1, "name=§6酒保;always=1");
    ```

### 取走并清空按钮点击事件

`npcDialogPollClicks()`

- 返回值：每条形如 `player=X dialogId=N scene=S button=I actionId=A closed=0|1 commands=...`；`button` = 点了第几个按钮（`-1` = 非按钮事件），`actionId` = 按钮串第二段原样带回，`closed=1` = 玩家关闭了对话，`commands` = 按钮上挂的命令（多条用 `,` 分隔，命令内部的空格与逗号被替换成 `+`）；队列空时返回空数组
- 返回值类型：`Array<String>`
- 示例：

    ```js
    const npcDialogPollClicks = ll.import("HologramLib", "npcDialogPollClicks");
    setInterval(() => {
      for (const line of npcDialogPollClicks()) {
        const m = line.match(/player=(\S+) dialogId=(-?\d+) scene=(\S*) button=(-?\d+) actionId=(\S*) closed=(\d)/);
        if (m && m[6] === "0") logger.info(`${m[1]} 点了按钮 ${m[4]}（actionId=${m[5]}）`);
      }
    }, 200);
    ```

### 丢弃对话队列里的点击

`npcDialogClearClicks()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const npcDialogClearClicks = ll.import("HologramLib", "npcDialogClearClicks");
    npcDialogClearClicks();
    ```

### 查询这个对话是否还在

`npcDialogIsOpen(id)`

- 参数：
  - `id` : `Integer`<br>对话 id
- 返回值：`true` = 对话存在；`false` = 已关闭或不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const npcDialogIsOpen = ll.import("HologramLib", "npcDialogIsOpen");
    if (npcDialogIsOpen(id)) npcDialogUpdate(id, "……", "");
    ```

### 列出所有打开的对话

`npcDialogGetIds()`

- 返回值：对话 id 数组；没有打开的对话时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const npcDialogGetIds = ll.import("HologramLib", "npcDialogGetIds");
    for (const dialogId of npcDialogGetIds()) npcDialogClose(dialogId);
    ```

### 关掉一个对话界面

`npcDialogClose(id)`

- 参数：
  - `id` : `Integer`<br>对话 id
- 返回值：`true` = 已关闭（客户端界面收起，载体实体一起删掉）；`false` = 该 id 不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const npcDialogClose = ll.import("HologramLib", "npcDialogClose");
    npcDialogClose(id);
    ```

### 关掉所有 NPC 对话框

`npcDialogCloseAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const npcDialogCloseAll = ll.import("HologramLib", "npcDialogCloseAll");
    npcDialogCloseAll();
    ```

---

## 注意事项

- **服务端没有这些界面。** 点击只会回传槽位号，物品不会被真的拿走；把物品放进虚拟槽时服务端真实槽位对不上，
  客户端会把这次预测撤回（物品弹回原位）—— 这是默认行为，不是出错。只有开了
  `containerSetInteractive` / `tradeSetSettle`（或用 `realTrades = true`）才会真的改变背包。
- **事件全在轮询队列里**：`containerPollClicks`、`npcDialogPollClicks`、`tradePollSettlements` 都是「取走并清空」，
  所以要按 tick 或 `setInterval` 持续取；不取就会一直堆在队列里。
- **换页不要重新 `containerOpen`**：重新打开会先关旧菜单、恢复真方块，每次都要再付一次打开延迟。
  翻页属于同一界面的内容变更，用 `containerSetItem` 逐格改即可；只换标题用 `containerSetTitle`。
- **NPC 对话换页必须重新 `npcDialogOpen`**：点任意按钮后客户端会自行收起界面，`npcDialogUpdate` 唤不回来。
- **对话按钮上的命令不会自己执行**：`npcDialogPollClicks` 回传的 `commands` 只是按钮上挂的文本，
  合成 NPC 在服务端没有实体代跑命令，要执行得自己来（例如 `player.runCommand`）。
- **对话头像换模型有边界**：`npcDialogSetAvatar` 的 `avatarViewSpec` 里 `skin=` 分支已移除 ——
  把载体换成玩家模型后，客户端在该载体上点进菜单会关掉界面；要玩家模型请另建假玩家并把对话挂到它的 id 上。
- **交易菜单要稳定成交**：用 `realTrades = true`（服务端自己结算）或改用虚拟容器。
  纯协议真结算（`tradeSetSettle(id, true)`）下扣款与发产物是真的，但客户端仍可能把这次放料撤回、
  成交按钮点不动 —— 协议要求的槽位更正在本协议版本对不上，写错会把客户端打崩，所以没有实现。
- **触屏玩家**：纯协议交易界面下付费拖不进交易槽（客户端反复弹回），请用真实交易表路径或容器界面。
  按设备分流可以先查 `sensingIsTouch`（见[视图覆盖与感知](view.md)）。
