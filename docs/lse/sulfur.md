# 硫磺立方体 · LSE

硫磺立方体展示：主手拿方块 + 默认隐身 + 外观档位。


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
