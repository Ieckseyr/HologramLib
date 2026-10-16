# 背包伪造与硫磺立方体 · 改客户端看到的背包 / 把方块吞在立方体上

`fakeInv*` 让客户端「以为自己背包里有这些物品」：改的是**客户端显示**，服务端背包一个字不动，
玩家点那些格子会回传槽位号给你。`sulfur*` 生成一只硫磺立方体展示实体，把要展示的方块/物品「吞」在它身上
—— 这是除自定义实体之外，第二种把东西摆到世界上的做法。

两个域的共同点：**摆出来的都只是展示**，服务端数据不受影响 —— `fakeInv*` 不动真实背包，`sulfur*` 只做观感。

## 快速开始

把玩家的快捷栏改成「一把剑 + 64 个钻石」，再在定时器里取点击：

```js
const fakeInvApply      = ll.import("HologramLib", "fakeInvApply");
const fakeInvPollClicks = ll.import("HologramLib", "fakeInvPollClicks");
const fakeInvClear      = ll.import("HologramLib", "fakeInvClear");
// 槽位 0 = 快捷栏第 1 格，槽位 9 = 主背包第 1 格；每条 "槽位|物品id|数量|名字|描述行~描述行"
fakeInvApply("Steve", "0|minecraft:diamond_sword|1|§c虚容之剑|§7攻击力 +7;1|minecraft:diamond|64|§b虚容钻石", 20);

setInterval(() => {
  for (const line of fakeInvPollClicks()) {   // 每条 "player=Steve slot=0"
    const m = line.match(/player=(\S+) slot=(-?\d+)/);
    if (m) logger.info(`${m[1]} 点了伪造槽位 ${m[2]}`);
  }
}, 200);

// 收工：fakeInvClear("Steve") 撤销伪造，恢复真实背包
```

摆一只「吞着书架」的硫磺立方体（默认隐身，所以只看到那个方块）：

```js
const sulfurCreate = ll.import("HologramLib", "sulfurCreate");
const id = sulfurCreate(100.5, 65, -200.5, 0, "minecraft:bookshelf", 1, 0, "§6书架", "regular", true);
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

## 硫磺立方体展示（sulfur*）

生成一只 `minecraft:sulfur_cube`，把要展示的方块/物品装进它的主手 —— 客户端按装备渲染，
看起来就是方块被「吞」在它身上；外观档位用 `minecraft:sulfur_cube_archetype` 属性控制。
**立方体本体隐身时，被吞下去的方块照常渲染** —— `invisible` 传 `true` 的观感就是「只看到那个方块」。

### 生成一只硫磺立方体展示

`sulfurCreate(x, y, z, dim, blockType, blockCount, blockDamage, blockName, archetype, invisible)`

- 参数：
  - `x` : `Float`<br>世界坐标 X（浮点）
  - `y` : `Float`<br>世界坐标 Y（浮点）
  - `z` : `Float`<br>世界坐标 Z（浮点）
  - `dim` : `Integer`<br>维度，`0` = 主世界 / `1` = 下界 / `2` = 末地
  - `blockType` : `String`<br>被「吞下去」的物品/方块 id（空串 = 空手；建议放方块类物品，观感才是方块被吞在身上）
  - `blockCount` : `Integer`<br>数量，小于 1 按 1 处理
  - `blockDamage` : `Integer`<br>物品的附加数据值，一般填 `0`
  - `blockName` : `String`<br>自定义名（支持 `§` 颜色码），空串 = 无
  - `archetype` : `String`<br>外观档位，可填 `none` / `regular` / `bouncy` / `slow_bouncy` / `slow_flat` / `fast_flat` / `light` / `fast_sliding` / `slow_sliding` / `sticky` / `high_resistance` / `explosive` / `hot`；空串 = 不下发（保持客户端默认）
  - `invisible` : `Boolean`<br>立方体本体是否隐身；`true` = 只看到被吞的方块，`false` = 连立方体一起看到
- 返回值：展示 id，后续函数都用它；`<0` = 失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const sulfurCreate = ll.import("HologramLib", "sulfurCreate");
    const id = sulfurCreate(100.5, 65, -200.5, 0, "minecraft:bookshelf", 1, 0, "§6书架", "regular", true);
    ```

### 换掉吞下去的方块物品

`sulfurSetBlock(id, type, count, damage, name)`

- 参数：
  - `id` : `Integer`<br>展示 id
  - `type` : `String`<br>物品 id；空串 = 空手
  - `count` : `Integer`<br>数量，小于 1 按 1 处理
  - `damage` : `Integer`<br>物品的附加数据值，一般填 `0`
  - `name` : `String`<br>自定义名（支持 `§` 颜色码），空串 = 无
- 返回值：`true` = 已换（会重建一次实体 = 有一次重生）；`false` = 该展示 id 不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const sulfurSetBlock = ll.import("HologramLib", "sulfurSetBlock");
    sulfurSetBlock(id, "minecraft:gold_block", 1, 0, "§6金块");   // 换成吞金块
    ```

### 换掉立方体的外观档位

`sulfurSetArchetype(id, archetype)`

- 参数：
  - `id` : `Integer`<br>展示 id
  - `archetype` : `String`<br>外观档位，取值同 `sulfurCreate` 的 `archetype`；空串 = 清掉属性，回到客户端默认外观
- 返回值：`true` = 已换（不重建实体，立刻生效）；`false` = 该展示 id 不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const sulfurSetArchetype = ll.import("HologramLib", "sulfurSetArchetype");
    sulfurSetArchetype(id, "sticky");   // 换成黏糊糊那档
    ```

### 设置立方体本体是否隐身

`sulfurSetInvisible(id, on)`

- 参数：
  - `id` : `Integer`<br>展示 id
  - `on` : `Boolean`<br>`true` = 隐身（只剩被吞的方块可见）；`false` = 显示立方体本体
- 返回值：`true` = 已设置；`false` = 该展示 id 不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const sulfurSetInvisible = ll.import("HologramLib", "sulfurSetInvisible");
    sulfurSetInvisible(id, false);   // 想看到立方体本体
    ```

### 设置展示实体的缩放

`sulfurSetScale(id, scale)`

- 参数：
  - `id` : `Integer`<br>展示 id
  - `scale` : `Float`<br>缩放值，有效范围 `0.0625~10`，超出会被自动钳到范围内
- 返回值：`true` = 已设置；`false` = 该展示 id 不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const sulfurSetScale = ll.import("HologramLib", "sulfurSetScale");
    sulfurSetScale(id, 1.5);
    ```

### 查询这个展示是否还在

`sulfurExists(id)`

- 参数：
  - `id` : `Integer`<br>展示 id
- 返回值：`true` = 存在；`false` = 已销毁或不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const sulfurExists = ll.import("HologramLib", "sulfurExists");
    if (sulfurExists(id)) sulfurSetInvisible(id, true);
    ```

### 列出所有硫磺立方体展示

`sulfurGetIds()`

- 返回值：展示 id 数组；一个都没有时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const sulfurGetIds = ll.import("HologramLib", "sulfurGetIds");
    for (const showId of sulfurGetIds()) logger.info(`展示 ${showId}`);
    ```

### 查询展示背后的实体 id

`sulfurGetEntityId(id)`

- 参数：
  - `id` : `Integer`<br>展示 id
- 返回值：背后的实体 id（诊断用）；`-1` = 该展示不存在
- 返回值类型：`Integer`
- 示例：

    ```js
    const sulfurGetEntityId = ll.import("HologramLib", "sulfurGetEntityId");
    logger.info(`展示 ${id} 的实体 id = ${sulfurGetEntityId(id)}`);
    ```

### 销毁一只硫磺立方体展示

`sulfurDestroy(id)`

- 参数：
  - `id` : `Integer`<br>展示 id
- 返回值：`true` = 已销毁；`false` = 该 id 不存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const sulfurDestroy = ll.import("HologramLib", "sulfurDestroy");
    sulfurDestroy(id);
    ```

### 销毁所有硫磺立方体展示

`sulfurDestroyAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const sulfurDestroyAll = ll.import("HologramLib", "sulfurDestroyAll");
    sulfurDestroyAll();
    ```

---

## 注意事项

- **伪造背包只影响客户端显示。** 玩家真正能用、能吃的是服务端真实物品；物品也不会因为点击被拿走，
  服务端判定失败后客户端会把这次预测撤回（这也是默认表现，不是出错）。
- **别把某格伪造成与真实物品一模一样**：那一格的操作会被服务端当真执行。想让某格纯展示，
  就让它与真实物品不同。
- **该格真实物品被别的原因改动**（捡东西 / 用物品 / 别的插件改背包）时，周期重发会把伪造内容盖回去。
- **打开箱子界面或交易界面时服务端会重发背包内容**，伪造会被盖掉 —— 靠 `refreshIntervalTicks` 周期重发，
  或用 `fakeInvRefresh` 手动补一发。
- **护甲（槽位 6）与副手（槽位 34）不在本域范围内**，它们各有独立的容器。
- 玩家下线时伪造记录会被清掉，重连后看到的是真实背包，不需要手动撤销。
- 点击事件在轮询队列里：`fakeInvPollClicks` 是「取走并清空」，要按 tick 或 `setInterval` 持续取。
- **「吞生物」没有做**：这类展示实体没有 AI，真正吞并/消化做不到，本域只做方块与隐身。

## 相关

- 域指南：[界面与容器](../domains/interfaces.md#背包虚容器)（伪造背包的机制与边界）
- 也想改方块外观：[视图覆盖与感知](view.md)
- 完整的 C++ 侧说明：[C++ 接口参考](../reference/cpp-api.md)
