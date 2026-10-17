# 物品显示 · 把一行物品名或一个立体物品摆到世界里

`itemDetail*` 和 `itemDisplay*` 都只让**客户端**看到一件物品：服务端不会多出实体，也不写存档。

- `itemDetailShow` 在坐标上显示一行**自动翻译的物品名**（例如「钻石 x64」）。它背后就是一个悬浮字，返回的 id 还能继续用 `holo*` 精修。
- `itemDisplay*` 在坐标上摆一个**立体物品 / 方块模型**，可以换物品、三轴旋转、缩放、跟随玩家；给它设过判定体积之后还能被点击（点击经 `ghost*` 轮询取回）。

两个域都是「创建 + 显示」一步到位，不需要再调显示函数。玩家重连由库自动补发；脚本或服务端重启后这些展示就没了，要自己重新创建。

## 快速开始

```js
const itemDetailShow    = ll.import("HologramLib", "itemDetailShow");
const itemDisplayCreate = ll.import("HologramLib", "itemDisplayCreate");

// 1. 一行文字：主世界 (100.5, 65, -200.5) 显示「钻石 x64」
const tip = itemDetailShow(0, 100.5, 65, -200.5, "minecraft:diamond", 0, 64, "");

// 2. 一个立体模型：同一位置上方悬浮一颗钻石
const box = itemDisplayCreate(100.5, 66, -200.5, 0, "minecraft:diamond", 0);
```

`tip` 同时是一个悬浮字 id（可以传给 `holo*` 系列），`box` 是物品悬浮 id（传给下面所有 `itemDisplay*` 函数）。

## itemDetail* —— 一行自动翻译的物品名

### 显示一行自动翻译的物品名

`itemDetailShow(dimId, x, y, z, itemId, aux, count, customText)`

- 参数：
  - `dimId` : `Integer`<br>放进哪个维度，`0` 主世界 / `1` 下界 / `2` 末地
  - `x` : `Float`<br>世界坐标 X
  - `y` : `Float`<br>世界坐标 Y
  - `z` : `Float`<br>世界坐标 Z
  - `itemId` : `String`<br>物品标识符，例如 `minecraft:diamond`
  - `aux` : `Integer`<br>物品附加值（data 值），没有就填 `0`
  - `count` : `Integer`<br>数量；只有大于 `1` 才会加上 ` xN` 后缀
  - `customText` : `String`<br>自定义文本；空串 = 用自动翻译的物品名，非空则完全替代（支持 `§` 颜色码与 `{变量}`）
- 返回值：悬浮字 id（可以继续用 `holo*` 精修）；失败返回 `-1`（创建失败或该维度画不出来）
- 返回值类型：`Integer`
- 示例：

    ```js
    const show = ll.import("HologramLib", "itemDetailShow");
    const tip  = show(0, 100.5, 65, -200.5, "minecraft:diamond", 0, 64, "");   // 钻石 x64
    const ad   = show(0, 100.5, 66, -200.5, "minecraft:diamond", 0, 1, "§b今日特价"); // 换成自己的文案
    ```

### 删掉显示的那行物品名

`itemDetailHide(id)`

- 参数：
  - `id` : `Integer`<br>`itemDetailShow` 返回的 id
- 返回值：`true` = 已删除；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const hide = ll.import("HologramLib", "itemDetailHide");
    hide(tip);   // tip 来自「快速开始」
    ```

## itemDisplay 创建

### 创建一个悬浮物品模型

`itemDisplayCreate(x, y, z, dim, itemId, aux)`

- 参数：
  - `x` : `Float`<br>世界坐标 X
  - `y` : `Float`<br>世界坐标 Y
  - `z` : `Float`<br>世界坐标 Z
  - `dim` : `Integer`<br>维度 id，`0` / `1` / `2`
  - `itemId` : `String`<br>物品标识符，例如 `minecraft:diamond`
  - `aux` : `Integer`<br>物品附加值（data 值），没有就填 `0`
- 返回值：物品悬浮 id；失败返回负数
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "itemDisplayCreate");
    // 主世界原点上方摆一颗钻石（最后一个参数是附加值）
    const gem = create(0.5, 66, 0.5, 0, "minecraft:diamond", 0);
    ```

### 创建并预设平移、旋转与缩放

`itemDisplayCreateAdvanced(x, y, z, dim, itemId, aux, offX, offY, offZ, rotX, rotY, rotZ, scale)`

- 参数：
  - `x` : `Float`<br>世界坐标 X，同 `itemDisplayCreate`
  - `y` : `Float`<br>世界坐标 Y，同 `itemDisplayCreate`
  - `z` : `Float`<br>世界坐标 Z，同 `itemDisplayCreate`
  - `dim` : `Integer`<br>维度 id，同 `itemDisplayCreate`
  - `itemId` : `String`<br>物品标识符，同 `itemDisplayCreate`
  - `aux` : `Integer`<br>物品附加值（data 值），同 `itemDisplayCreate`
  - `offX` : `String`<br>模型单位平移 X，写常量数字或 Molang 表达式字符串
  - `offY` : `String`<br>模型单位平移 Y，写常量数字或 Molang 表达式字符串
  - `offZ` : `String`<br>模型单位平移 Z，写常量数字或 Molang 表达式字符串
  - `rotX` : `String`<br>X 轴旋转（度），写常量数字或 Molang 表达式字符串
  - `rotY` : `String`<br>Y 轴旋转（度），写常量数字或 Molang 表达式字符串
  - `rotZ` : `String`<br>Z 轴旋转（度），写常量数字或 Molang 表达式字符串
  - `scale` : `String`<br>缩放，写常量数字或 Molang 表达式字符串
- 返回值：物品悬浮 id；失败返回负数
- 返回值类型：`Integer`
- 示例：

    ```js
    const advanced = ll.import("HologramLib", "itemDisplayCreateAdvanced");
    // 一直在转的钻石：rotY 交给 Molang 驱动，服务端不需要每 tick 推包
    const spin = advanced(0.5, 66, 0.5, 0, "minecraft:diamond", 0,
                          "0", "-4", "0", "180", "math.sin(query.life_time*90)*360", "180", "0.375");
    ```

### 创建时定好模式、视距与可见玩家

`itemDisplayCreateSeamless(x, y, z, dim, itemId, aux, offX, offY, offZ, rotX, rotY, rotZ, scale, mode, viewDistance, visiblePlayer)`

- 参数：
  - 前 13 个参数：同 `itemDisplayCreateAdvanced`
  - `mode` : `Integer`<br>`-1` = 用默认（自动判断）, `1` = item 平面渲染, `2` = block 立体渲染
  - `viewDistance` : `Float`<br>可见距离（方块）；`-1` = 用默认，`0` = 不限距离
  - `visiblePlayer` : `String`<br>只给这一个玩家看；空串 = 全员可见
- 返回值：物品悬浮 id；失败返回负数
- 返回值类型：`Integer`
- 示例：

    ```js
    const seamless = ll.import("HologramLib", "itemDisplayCreateSeamless");
    // 只给 Steve 看的立体钻石块，30 格视距
    const blk = seamless(100.5, 66, -200.5, 0, "minecraft:diamond_block", 0,
                         "0", "-4", "0", "180", "0", "180", "0.5", 2, 30, "Steve");
    ```

### 用随机 id 创建悬浮物品

`itemDisplayCreateRandom(x, y, z, dim, itemId, aux)`

- 参数：同 `itemDisplayCreate`
- 返回值：随机段（`0x10000000` 起）的物品悬浮 id，不会和自增 id 冲突；失败返回负数
- 返回值类型：`Integer`
- 示例：

    ```js
    const createRandom = ll.import("HologramLib", "itemDisplayCreateRandom");
    const gem = createRandom(0.5, 66, 0.5, 0, "minecraft:diamond", 0);
    ```

### 指定 id 创建以恢复展示

`itemDisplayCreateWithId(x, y, z, dim, itemId, aux, desiredId)`

- 参数：
  - 前 6 个参数：同 `itemDisplayCreate`
  - `desiredId` : `Integer`<br>想用的 id，必须大于 `0` 且当前没被占用
- 返回值：成功就是 `desiredId`；`desiredId <= 0` 或已被占用返回 `-2`
- 返回值类型：`Integer`
- 示例：

    ```js
    const createWithId = ll.import("HologramLib", "itemDisplayCreateWithId");
    const id = createWithId(0.5, 66, 0.5, 0, "minecraft:diamond", 0, 42);   // 42 已被占用 → -2
    ```

## itemDisplay 属性

### 换掉展示显示的物品

`itemDisplaySetItem(id, itemId, aux)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `itemId` : `String`<br>新的物品标识符
  - `aux` : `Integer`<br>新的物品附加值（data 值）
- 返回值：`true` = 已改；id 不存在返回 `false`。换物品会一并清掉之前设的附加数据
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setItem = ll.import("HologramLib", "itemDisplaySetItem");
    setItem(box, "minecraft:emerald", 0);   // box 来自「快速开始」
    ```

### 换物品并带上附加数据

`itemDisplaySetItemWithNbt(id, itemId, aux, nbt)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `itemId` : `String`<br>新的物品标识符
  - `aux` : `Integer`<br>新的物品附加值（data 值）
  - `nbt` : `String`<br>SNBT 字符串；空串 = 清除附加数据。解析失败按「没有附加数据」处理并打日志
- 返回值：`true` = 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setNbt = ll.import("HologramLib", "itemDisplaySetItemWithNbt");
    setNbt(box, "minecraft:diamond_sword", 0, '{display:{Name:"§b试用剑"}}');
    setNbt(box, "minecraft:diamond_sword", 0, "");   // 空串 = 去掉附加数据
    ```

### 开关物品的附魔光效

`itemDisplaySetGlint(id, on)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `on` : `Boolean`<br>`true` = 加光效，`false` = 去掉；重复设同一个值不会触发重发
- 返回值：`true` = 已设；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setGlint = ll.import("HologramLib", "itemDisplaySetGlint");
    setGlint(box, true);    // 加光效
    setGlint(box, false);   // 去掉
    ```

### 把展示挪到新坐标

`itemDisplaySetPosition(id, x, y, z, dim)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `x` : `Float`<br>新的世界坐标 X
  - `y` : `Float`<br>新的世界坐标 Y
  - `z` : `Float`<br>新的世界坐标 Z
  - `dim` : `Integer`<br>目标维度；填负数 = 只改坐标、保持原维度
- 返回值：`true` = 已改；id 不存在返回 `false`。跟随中的展示被手动设位会解除跟随
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setPos = ll.import("HologramLib", "itemDisplaySetPosition");
    setPos(box, 105.5, 66, -200.5, -1);   // dim = -1：只挪坐标，维度不变
    setPos(box, 105.5, 66, -200.5, 1);    // 换到下界
    ```

### 调物品在模型里的平移

`itemDisplaySetOffset(id, ox, oy, oz)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `ox` : `String`<br>常量数字或 Molang 表达式字符串
  - `oy` : `String`<br>常量数字或 Molang 表达式字符串；物品模式的默认 Y 是 `"-4"`
  - `oz` : `String`<br>常量数字或 Molang 表达式字符串
- 返回值：`true` = 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setOffset = ll.import("HologramLib", "itemDisplaySetOffset");
    setOffset(box, "0", "-2", "0");   // 默认 Y 是 "-4"，这里换一组平移值
    ```

### 调渲染像素级的基础偏移

`itemDisplaySetBaseOffset(id, ox, oy, oz)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `ox` : `String`<br>常量数字或 Molang 表达式字符串；默认是 `"0"`
  - `oy` : `String`<br>常量数字或 Molang 表达式字符串；默认是 `"0"`
  - `oz` : `String`<br>常量数字或 Molang 表达式字符串；默认是 `"0"`
- 返回值：`true` = 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setBase = ll.import("HologramLib", "itemDisplaySetBaseOffset");
    setBase(box, "0", "8", "0");   // 默认都是 "0"，这里在平移之外再叠一层基础偏移
    ```

### 设置三轴旋转角度

`itemDisplaySetRotation(id, rx, ry, rz)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `rx` : `String`<br>常量数字或 Molang 表达式字符串（度）；物品模式默认 `rotX` 是 `"180"`（物品模式下库内会自动补偿朝向）
  - `ry` : `String`<br>常量数字或 Molang 表达式字符串（度）；物品模式默认 `rotY` 是 `"0"`
  - `rz` : `String`<br>常量数字或 Molang 表达式字符串（度）；物品模式默认 `rotZ` 是 `"180"`
- 返回值：`true` = 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setRot = ll.import("HologramLib", "itemDisplaySetRotation");
    setRot(box, "180", "0", "180");   // 回到物品模式的默认姿态
    setRot(box, "180", "45", "180");  // 偏航转 45 度
    ```

### 叠加偏航旋转增量

`itemDisplayRotateY(id, delta)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `delta` : `Float`<br>要叠加的角度（度），正负都行
- 返回值：`true` = 已叠加；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const rotateY = ll.import("HologramLib", "itemDisplayRotateY");
    rotateY(box, 45);    // 在当前偏航上再转 45 度
    rotateY(box, -45);   // 转回来
    ```

### 设置展示缩放大小

`itemDisplaySetScale(id, scale)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `scale` : `String`<br>常量数字或 Molang 表达式字符串；物品模式默认 `"0.375"`，方块模式建议 `"0.5"`
- 返回值：`true` = 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setScale = ll.import("HologramLib", "itemDisplaySetScale");
    setScale(box, "0.6");
    setScale(box, "0.5");   // 方块模式常用的缩放
    ```

### 按系数放大或缩小

`itemDisplayScaleBy(id, factor)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `factor` : `Float`<br>大于 `1` 放大，`0 < factor < 1` 缩小；`<= 0` 会被直接拒绝
- 返回值：`true` = 已改；`factor <= 0`、id 不存在，或乘完结果不大于 `0` 时返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const scaleBy = ll.import("HologramLib", "itemDisplayScaleBy");
    scaleBy(box, 1.5);        // 放大 1.5 倍
    scaleBy(box, 1 / 1.5);    // 再缩回去
    ```

### 一步跳到目标缩放

`itemDisplayScaleTo(id, targetScale)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `targetScale` : `Float`<br>目标缩放；小于 `0.01`（或不是有限数）会被钳到 `0.01`
- 返回值：只有方块渲染路径能用：`mode` 为 `1`（item 平面）返回 `false`，id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const scaleTo = ll.import("HologramLib", "itemDisplayScaleTo");
    // 它本身只做单次跳变；想要渐变动画就自己分几步调用
    scaleTo(box, 0.8);
    ```

### 设置方块模式的二段变换

`itemDisplaySetExtend(id, scale, rx, ry)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `scale` : `String`<br>常量数字或 Molang 表达式字符串；只在方块模式生效，默认 `"1"`
  - `rx` : `String`<br>常量数字或 Molang 表达式字符串；只在方块模式生效，默认 `"-90"`
  - `ry` : `String`<br>常量数字或 Molang 表达式字符串；只在方块模式生效，默认 `"0"`
- 返回值：`true` = 已改；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setExtend = ll.import("HologramLib", "itemDisplaySetExtend");
    setExtend(box, "1", "-90", "0");   // 默认值
    ```

### 切换物品的渲染模式

`itemDisplaySetMode(id, mode)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `mode` : `Integer`<br>`0` = auto（默认，按物品自己判断）, `1` = item 平面渲染, `2` = block 立体渲染
- 返回值：`true` = 已设；id 不存在返回 `false`。值和当前一样时直接返回 `true` 且不重发
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setMode = ll.import("HologramLib", "itemDisplaySetMode");
    setMode(box, 2);   // 强制按立体方块渲染
    setMode(box, 0);   // 交回自动判断
    ```

### 打开或关闭这个展示

`itemDisplaySetEnabled(id, enabled)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `enabled` : `Boolean`<br>`false` = 玩家看不到（id 仍然有效，随时可以开回来），`true` = 恢复显示
- 返回值：`true` = 已设；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setEnabled = ll.import("HologramLib", "itemDisplaySetEnabled");
    setEnabled(box, false);   // 先藏起来
    setEnabled(box, true);    // 再显示
    ```

### 设置展示的可见距离

`itemDisplaySetViewDistance(id, dist)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `dist` : `Float`<br>距离，单位方块；`<= 0` = 不限距离
- 返回值：`true` = 已设；id 不存在返回 `false`。值和当前一样时不重发
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setView = ll.import("HologramLib", "itemDisplaySetViewDistance");
    setView(box, 32);   // 32 格内可见
    setView(box, 0);    // 不限距离
    ```

## itemDisplay 可见玩家白名单

### 只让名单里的玩家看到

`itemDisplaySetVisiblePlayers(id, playerNames)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `playerNames` : `Array<String>`<br>玩家名数组，按 LSE 的 `realName` 匹配；空数组 = 清除限制、全员可见
- 返回值：`true` = 已设；id 不存在返回 `false`。维度与视距条件仍然叠加生效
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setVisiblePlayers = ll.import("HologramLib", "itemDisplaySetVisiblePlayers");
    setVisiblePlayers(box, ["Steve", "Alex"]);   // 只给这两个人看
    setVisiblePlayers(box, []);                  // 清空 = 全员可见
    ```

### 白名单只填一个玩家

`itemDisplaySetVisiblePlayer(id, playerName)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `playerName` : `String`<br>唯一可见的玩家名（`realName`）
- 返回值：`true` = 已设；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setVisiblePlayer = ll.import("HologramLib", "itemDisplaySetVisiblePlayer");
    setVisiblePlayer(box, "Steve");   // 只给 Steve 看
    ```

### 清除白名单恢复全员可见

`itemDisplayClearVisiblePlayers(id)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
- 返回值：`true` = 已清除（本来就没有限制也算成功）；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const clearVisible = ll.import("HologramLib", "itemDisplayClearVisiblePlayers");
    clearVisible(box);   // 所有人都能看到了
    ```

## itemDisplay 跟随与点击

### 让展示跟着玩家实时移动

`itemDisplayFollow(id, playerName, offX, offY, offZ)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `playerName` : `String`<br>目标玩家名（`realName`）
  - `offX` : `Float`<br>在玩家坐标上叠加的偏移量 X
  - `offY` : `Float`<br>在玩家坐标上叠加的偏移量 Y
  - `offZ` : `Float`<br>在玩家坐标上叠加的偏移量 Z
- 返回值：`true` = 已开始跟随；id 不存在返回 `false`。玩家下线会自动解除跟随（展示留在原地），手动 `itemDisplaySetPosition` 也会解除
- 返回值类型：`Boolean`
- 示例：

    ```js
    const follow = ll.import("HologramLib", "itemDisplayFollow");
    follow(box, "Steve", 0, 2.5, 0);   // 跟在 Steve 头顶上方
    ```

### 解除跟随并留在原地

`itemDisplayUnfollow(id)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
- 返回值：没有跟随关系也返回 `true`（可以放心重复调用）
- 返回值类型：`Boolean`
- 示例：

    ```js
    const unfollow = ll.import("HologramLib", "itemDisplayUnfollow");
    unfollow(box);   // 不再跟着玩家走
    ```

### 建展示时直接带上物品 NBT / 附魔光效（1.30.0 / 26.40.11 起）

`itemDisplayCreateFull(x, y, z, dim, itemId, aux, offX, offY, offZ, rotX, rotY, rotZ, scale, nbt, glint)`

- 参数：前 13 个与 `itemDisplayCreateAdvanced` 相同，后面两个：
  - `nbt` : `String`<br>物品 SNBT（自定义名 / 附魔等），空串 = 无
  - `glint` : `Boolean`<br>`true` = 加附魔光效
- 返回值：物品悬浮 id；失败返回负数
- 说明：效果等同 `itemDisplayCreateAdvanced` + `itemDisplaySetItemWithNbt` + `itemDisplaySetGlint`，
  但**不产生那一次"建完再改"的 respawn** —— 后者会让刚出现的展示闪一下。
- 示例：

    ```js
    const createFull = ll.import("HologramLib", "itemDisplayCreateFull");
    const id = createFull(0.5, 66, 0.5, 0, "minecraft:diamond_sword", 0,
                          "0", "-4", "0", "180", "0", "180", "0.375",
                          '{"ench":[{"id":9s,"lvl":1s}]}', true);
    ```

### 让某玩家把一只已有实体看成 FMBE 模型（1.30.0 起）

`itemDisplayEmitForEntity(playerName, uniqueId, itemId, aux, offsetY, rotX, rotY, rotZ, scale, nbt, glint)`

- 参数：
  - `playerName` : `String`<br>只对这个玩家生效
  - `uniqueId` : `Integer`<br>**已有实体**的 uniqueId（掉落物就是它自己的 id）
  - `itemId` / `aux` / `nbt` / `glint`：要显示成什么物品
  - `offsetY` / `rotX` / `rotY` / `rotZ` / `scale`：FMBE 外观参数（`rotY` 传 `"0"` = 用实体当前速度方向）
- 返回值：实际渲染模式 `1`=物品平面 `2`=3D 方块；失败 `-1`
- 说明：**一次性发** `RemoveActor + AddActor(fox) + MobEquipment + SetActorData + 动画`，用调用方给的 id，
  **不保存任何状态**；何时发、发给谁、落地怎么躺平、尺寸多少，全由调用方决定。
- 示例：

    ```js
    const emit = ll.import("HologramLib", "itemDisplayEmitForEntity");
    emit("Steve", 12345, "minecraft:diamond", 0, "-4", "180", "0", "180", "0.375", "", false);
    ```

### 给展示加可命中的判定体积

`itemDisplaySetHitbox(id, width, height)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
  - `width` : `Float`<br>判定体积的宽
  - `height` : `Float`<br>判定体积的高；两个都填 `0` = 恢复成不可命中（默认行为）
- 返回值：`true` = 已设；id 不存在返回 `false`。命中后的事件经 `ghost*` 轮询取回，域是 `itemDisplay`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setHitbox = ll.import("HologramLib", "itemDisplaySetHitbox");
    setHitbox(box, 1, 1);   // 变成能被点到的方块
    setHitbox(box, 0, 0);   // 恢复不可命中
    ```

## itemDisplay 查询与删除

### 查询展示是否还存在

`itemDisplayExists(id)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
- 返回值：`true` = 存在
- 返回值类型：`Boolean`
- 示例：

    ```js
    const exists = ll.import("HologramLib", "itemDisplayExists");
    if (exists(box)) logger.info("展示还在");
    ```

### 查询 id 是否已被占用

`itemDisplayIsIdUsed(id)`

- 参数：
  - `id` : `Integer`<br>要查的物品悬浮 id
- 返回值：`true` = 已被占用（此时 `itemDisplayCreateWithId` 会返回 `-2`）
- 返回值类型：`Boolean`
- 示例：

    ```js
    const isUsed = ll.import("HologramLib", "itemDisplayIsIdUsed");
    if (!isUsed(42)) createWithId(0.5, 66, 0.5, 0, "minecraft:diamond", 0, 42);
    ```

### 列出所有展示的 id

`itemDisplayGetAllIds()`

- 返回值：所有展示的 id；一个都没有时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const allIds = ll.import("HologramLib", "itemDisplayGetAllIds");
    for (const id of allIds()) logger.info(`展示 ${id}`);
    ```

### 找同维度里最近的展示

`itemDisplayFindNearest(x, y, z, dim, maxDist)`

- 参数：
  - `x` : `Float`<br>查找中心的世界坐标 X
  - `y` : `Float`<br>查找中心的世界坐标 Y
  - `z` : `Float`<br>查找中心的世界坐标 Z
  - `dim` : `Integer`<br>只在哪个维度里找
  - `maxDist` : `Float`<br>搜索半径（方块）；`<= 0` = 不限距离
- 返回值：最近的展示 id；这个维度里没有匹配返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const nearest = ll.import("HologramLib", "itemDisplayFindNearest");
    const near = nearest(100.5, 66, -200.5, 0, 10);   // 主世界 10 格内最近的展示
    ```

### 取运行态摘要用于排查

`itemDisplayGetInfo(id)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
- 返回值：摘要文本，包含 id、维度、坐标、模式、开关、物品、视距、可见名单与已见玩家数；id 不存在返回 `not_found`
- 返回值类型：`String`
- 示例：

    ```js
    const getInfo = ll.import("HologramLib", "itemDisplayGetInfo");
    logger.info(getInfo(box));
    ```

### 删掉一个物品悬浮展示

`itemDisplayDestroy(id)`

- 参数：
  - `id` : `Integer`<br>物品悬浮 id
- 返回值：`true` = 已删除；id 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const destroy = ll.import("HologramLib", "itemDisplayDestroy");
    destroy(box);
    ```

### 删掉全部物品悬浮展示

`itemDisplayDestroyAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const destroyAll = ll.import("HologramLib", "itemDisplayDestroyAll");
    destroyAll();   // 清场
    ```

## 注意事项

- **默认姿态由库给定**：物品模式 `offsetY` 是 `"-4"`、`rotX` 与 `rotZ` 是 `"180"`、`scale` 是 `"0.375"`（方块模式建议 `"0.5"`）；默认可见距离是 64 格。
- **Molang 表达式**：平移 / 旋转 / 缩放参数都可以直接写表达式（例如 `"math.sin(query.life_time*90)*360"`），动画在客户端算，不需要服务端每 tick 推包。
- **属性变更会合并**：同一 tick 里连改多项，只会重建一次（不会反复闪）。但重建会把展示重发一遍，所以「每 tick 跟着东西走」要用 `itemDisplayFollow`，别在 tick 循环里反复调 `itemDisplaySetPosition`。
- **`itemDisplayScaleTo` 只做单次跳变**：它本身不产生渐变动画，想要缓动效果就自己分几步调用；`mode` 为 `1`（item 平面）时它返回 `false`。
- **点击需要判定体积**：只有设过 `itemDisplaySetHitbox` 的非零宽高，客户端射线才能命中；命中后的交互/攻击经 `ghost*` 轮询取回（条目里 `domain=itemDisplay`）。
- **id 不通用**：`itemDisplay*` 的 id 只属于物品悬浮域，不要传给 `holo*` / `entity*` 等其它域。反过来，`itemDetailShow` 返回的 id 才是一个悬浮字 id。
- **库不落盘**：脚本或服务端重启后展示就没了，要重新创建；玩家重连不用管，库会自动补发。
