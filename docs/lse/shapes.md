# 形状渲染 · 在客户端画出线段、方框、圆球、箭头和文字

这一域在指定的世界坐标上画出线、盒、圆、球、箭头和文本，**只有客户端看得见**：服务端世界里没有实体、没有碰撞，也不写进存档。适合做区域标记、建筑辅助线、范围提示和临时公告文字。

每个形状用创建时返回的整数 `id` 管理。设置属性（文字、位置、颜色…）改的是服务端记下的值，已经显示出来的形状要再调用一次 `shapeUpdate` 或 `shapeDraw`，客户端才会看到变化。

## 快速开始

```js
const shapeCreateLine = ll.import("HologramLib", "shapeCreateLine");
const shapeSetColor   = ll.import("HologramLib", "shapeSetColor");
const shapeDraw       = ll.import("HologramLib", "shapeDraw");

// 一条从 (100,65,100) 到 (110,65,110) 的红线，对全体在线玩家显示
const lineId = shapeCreateLine(100, 65, 100, 110, 65, 110);
shapeSetColor(lineId, 1.0, 0.2, 0.2, 1.0);   // RGBA，四个分量都是 0.0 ~ 1.0
shapeDraw(lineId);
```

坐标一律是世界坐标；颜色一律是 `0.0 ~ 1.0` 的四个分量。

## 创建

### 创建一个文本形状

`shapeCreateText(x, y, z, text)`

- 参数：
  - `x` : `Float`<br>文本所在的世界坐标 X
  - `y` : `Float`<br>文本所在的世界坐标 Y
  - `z` : `Float`<br>文本所在的世界坐标 Z
  - `text` : `String`<br>要显示的文本，支持 `§` 颜色代码与 `\n` 换行
- 返回值：形状 id（从 `1` 开始递增）；`<= 0` 表示创建失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const draw   = ll.import("HologramLib", "shapeDraw");

    const textId = create(100.5, 66, 100.5, "§e主城 · 告示牌");
    draw(textId);
    ```

### 在两点之间画一条线段

`shapeCreateLine(x1, y1, z1, x2, y2, z2)`

- 参数：
  - `x1` : `Float`<br>起点世界坐标 X
  - `y1` : `Float`<br>起点世界坐标 Y
  - `z1` : `Float`<br>起点世界坐标 Z
  - `x2` : `Float`<br>终点世界坐标 X
  - `y2` : `Float`<br>终点世界坐标 Y
  - `z2` : `Float`<br>终点世界坐标 Z
- 返回值：形状 id；`<= 0` 表示创建失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateLine");
    const id = create(100, 65, 100, 110, 65, 110);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 用两个对角点创建长方体线框

`shapeCreateBox(x1, y1, z1, x2, y2, z2)`

- 参数：
  - `x1` : `Float`<br>第一个角的世界坐标 X
  - `y1` : `Float`<br>第一个角的世界坐标 Y
  - `z1` : `Float`<br>第一个角的世界坐标 Z
  - `x2` : `Float`<br>对角的世界坐标 X（两个点的先后顺序无所谓，边长取两点的坐标差）
  - `y2` : `Float`<br>对角的世界坐标 Y
  - `z2` : `Float`<br>对角的世界坐标 Z
- 返回值：形状 id；`<= 0` 表示创建失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateBox");
    const id = create(100, 64, 100, 110, 70, 110);   // 10×6×10 的框
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 在指定坐标创建一个圆

`shapeCreateCircle(x, y, z, scale)`

- 参数：
  - `x` : `Float`<br>圆心世界坐标 X
  - `y` : `Float`<br>圆心世界坐标 Y
  - `z` : `Float`<br>圆心世界坐标 Z
  - `scale` : `Float`<br>圆的大小，相当于创建时就设好形状缩放，之后也能用 `shapeSetScale` 改
- 返回值：形状 id；`<= 0` 表示创建失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateCircle");
    const id = create(100, 65, 100, 3);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 在指定坐标创建一个球

`shapeCreateSphere(x, y, z, scale)`

- 参数：
  - `x` : `Float`<br>球心世界坐标 X
  - `y` : `Float`<br>球心世界坐标 Y
  - `z` : `Float`<br>球心世界坐标 Z
  - `scale` : `Float`<br>球的大小，相当于创建时就设好形状缩放，之后也能用 `shapeSetScale` 改
- 返回值：形状 id；`<= 0` 表示创建失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateSphere");
    const id = create(100, 65, 100, 2);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 画一个从起点指向终点的箭头

`shapeCreateArrow(x1, y1, z1, x2, y2, z2)`

- 参数：
  - `x1` : `Float`<br>箭尾世界坐标 X
  - `y1` : `Float`<br>箭尾世界坐标 Y
  - `z1` : `Float`<br>箭尾世界坐标 Z
  - `x2` : `Float`<br>箭头指向的世界坐标 X
  - `y2` : `Float`<br>箭头指向的世界坐标 Y
  - `z2` : `Float`<br>箭头指向的世界坐标 Z
- 返回值：形状 id；`<= 0` 表示创建失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateArrow");
    const id = create(100, 66, 100, 110, 66, 100);   // 沿 +X 指过去
    ll.import("HologramLib", "shapeDraw")(id);
    ```

## 属性

### 修改文本形状的文字

`shapeSetText(id, text)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `text` : `String`<br>新的文本；只在文本形状上有意义，其它类型调了不会有变化
- 返回值：写入成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setText = ll.import("HologramLib", "shapeSetText");
    const id = create(100, 66, 100, "§e第一版");
    setText(id, "§a改过的文字");            // 服务端值已改
    ll.import("HologramLib", "shapeUpdate")(id);   // 已显示时要重发一次
    ```

### 读取文本形状的文字

`shapeGetText(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：当前文本；`id` 不存在或不是文本形状时是空串 `""`
- 返回值类型：`String`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const getText = ll.import("HologramLib", "shapeGetText");
    const id = create(100, 66, 100, "§e主城");
    const text = getText(id);   // "§e主城"
    ```

### 移动形状到新的坐标

`shapeSetLocation(id, x, y, z)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `x` : `Float`<br>新的世界坐标 X
  - `y` : `Float`<br>新的世界坐标 Y
  - `z` : `Float`<br>新的世界坐标 Z
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateLine");
    const setLocation = ll.import("HologramLib", "shapeSetLocation");
    const id = create(100, 65, 100, 110, 65, 110);
    setLocation(id, 200, 70, 200);
    ll.import("HologramLib", "shapeUpdate")(id);   // 已显示时才需要
    ```

### 查询形状当前的坐标

`shapeGetLocation(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：`[x, y, z]`；`id` 不存在时是空数组
- 返回值类型：`Array<Float>`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateLine");
    const getLocation = ll.import("HologramLib", "shapeGetLocation");
    const id = create(100, 65, 100, 110, 65, 110);
    const [x, y, z] = getLocation(id);
    logger.info(`形状在 ${x}, ${y}, ${z}`);
    ```

### 设置形状的红绿蓝与透明度

`shapeSetColor(id, r, g, b, a)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `r` : `Float`<br>红色分量，`0.0 ~ 1.0`
  - `g` : `Float`<br>绿色分量，`0.0 ~ 1.0`
  - `b` : `Float`<br>蓝色分量，`0.0 ~ 1.0`
  - `a` : `Float`<br>透明度分量，`0.0 ~ 1.0`
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateBox");
    const setColor = ll.import("HologramLib", "shapeSetColor");
    const id = create(100, 64, 100, 110, 70, 110);
    setColor(id, 0.2, 1.0, 0.2, 0.6);      // 半透明绿色
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 读回形状当前的颜色

`shapeGetColor(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：`[r, g, b, a]`，各分量 `0.0 ~ 1.0`；`id` 不存在时是空数组。颜色按 8 位保存，读回值可能与写入值差 `±1/255`
- 返回值类型：`Array<Float>`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateCircle");
    const getColor = ll.import("HologramLib", "shapeGetColor");
    const id = create(100, 65, 100, 3);
    const [r, g, b, a] = getColor(id);
    logger.info(`颜色 ${r} ${g} ${b} ${a}`);
    ```

### 设置形状的缩放倍数

`shapeSetScale(id, scale)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `scale` : `Float`<br>缩放倍数，`1` 是创建时的大小
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateSphere");
    const setScale = ll.import("HologramLib", "shapeSetScale");
    const id = create(100, 66, 100, 2);
    setScale(id, 4);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 让形状若干秒后自动消失

`shapeSetDuration(id, seconds)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `seconds` : `Float`<br>持续时间，单位**秒**。不调用这个函数 = 永久显示
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setDuration = ll.import("HologramLib", "shapeSetDuration");
    const id = create(100, 66, 100, "§c10 秒后消失");
    setDuration(id, 10);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 设置形状所属的维度

`shapeSetDimension(id, dimId)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `dimId` : `Integer`<br>维度编号 —— `0` 主世界、`1` 下界、`2` 末地
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateLine");
    const setDimension = ll.import("HologramLib", "shapeSetDimension");
    const id = create(100, 65, 100, 110, 65, 110);
    setDimension(id, 0);
    ll.import("HologramLib", "shapeDrawToDimension")(id, 0);   // 只给主世界玩家看
    ```

### 把形状固定成三轴朝向

`shapeSetRotation(id, pitch, yaw, roll)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `pitch` : `Float`<br>绕三个轴的旋转角度之一，单位**度**
  - `yaw` : `Float`<br>绕三个轴的旋转角度之一，单位**度**
  - `roll` : `Float`<br>绕三个轴的旋转角度之一，单位**度**
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setRotation = ll.import("HologramLib", "shapeSetRotation");
    const id = create(100, 66, 100, "§e平躺的文字");
    setRotation(id, 90, 0, 0);             // 文本形状会自动打开双面渲染
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 清掉旋转让形状面向镜头

`shapeClearRotation(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const clearRotation = ll.import("HologramLib", "shapeClearRotation");
    const id = create(100, 66, 100, "§e又面向镜头了");
    clearRotation(id);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 查询形状当前的旋转角度

`shapeGetRotation(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：`[pitch, yaw, roll]`（度）；没设过旋转（仍然面向镜头）或 `id` 不存在时是空数组
- 返回值类型：`Array<Float>`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const getRotation = ll.import("HologramLib", "shapeGetRotation");
    const id = create(100, 66, 100, "§e文字");
    const rot = getRotation(id);   // 没设过旋转 → []
    logger.info(`旋转项数 ${rot.length}`);
    ```

### 查询这个 id 是哪种形状

`shapeGetShapeType(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：类型编号：`0` 文本、`1` 线、`2` 盒、`3` 圆、`4` 球、`5` 箭头；`id` 不存在返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateBox");
    const getShapeType = ll.import("HologramLib", "shapeGetShapeType");
    const id = create(100, 64, 100, 110, 70, 110);
    if (getShapeType(id) === 2) logger.info("这是一个盒子");
    ```

### 设置文本底板的颜色

`shapeSetBackgroundColor(id, r, g, b, a)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `r` : `Float`<br>底板红色分量，`0.0 ~ 1.0`
  - `g` : `Float`<br>底板绿色分量，`0.0 ~ 1.0`
  - `b` : `Float`<br>底板蓝色分量，`0.0 ~ 1.0`
  - `a` : `Float`<br>底板透明度分量，`0.0 ~ 1.0`；不设置就是客户端默认色
- 返回值：成功 `true`；`id` 不存在**或不是文本形状**返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setBackgroundColor = ll.import("HologramLib", "shapeSetBackgroundColor");
    const id = create(100, 66, 100, "§f黑底白字");
    setBackgroundColor(id, 0, 0, 0, 0.6);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 把文本底板恢复默认颜色

`shapeClearBackgroundColor(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：成功 `true`；`id` 不存在或不是文本形状返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const clearBackgroundColor = ll.import("HologramLib", "shapeClearBackgroundColor");
    const id = create(100, 66, 100, "§f回到默认底色");
    clearBackgroundColor(id);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### 设置文本是否被方块遮挡

`shapeSetDepthTest(id, enabled)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `enabled` : `Boolean`<br>`true` = 被方块 / 实体遮挡；`false` = 始终渲染（穿墙可见），也是默认行为
- 返回值：成功 `true`；`id` 不存在或不是文本形状返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setDepthTest = ll.import("HologramLib", "shapeSetDepthTest");
    const id = create(100, 66, 100, "§e藏在墙后的文字");
    setDepthTest(id, true);                // 被挡住就看不见
    ll.import("HologramLib", "shapeDraw")(id);
    ```

## 显示

### 对所有在线玩家显示形状

`shapeDraw(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：已发出 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateArrow");
    const draw = ll.import("HologramLib", "shapeDraw");
    const id = create(100, 66, 100, 110, 66, 100);
    draw(id);
    ```

### 只让某一名玩家看到形状

`shapeDrawToPlayer(id, playerName)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `playerName` : `String`<br>玩家名（LSE 的 `realName`）。玩家不在线时不会有任何显示
- 返回值：已发出 `true`；`id` 不存在或玩家不在线 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const drawToPlayer = ll.import("HologramLib", "shapeDrawToPlayer");
    const id = create(100, 66, 100, "§b只有你能看到");
    drawToPlayer(id, "Steve");
    ```

### 只对某个维度显示形状

`shapeDrawToDimension(id, dimId)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `dimId` : `Integer`<br>维度编号 —— `0` 主世界、`1` 下界、`2` 末地
- 返回值：已发出 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateBox");
    const drawToDimension = ll.import("HologramLib", "shapeDrawToDimension");
    const id = create(100, 64, 100, 110, 70, 110);
    drawToDimension(id, 0);      // 只有主世界玩家看得到
    ```

### 一次显示多个形状

`shapeDrawBatch(ids)`

- 参数：
  - `ids` : `Array<Integer>`<br>形状 id 数组；数组里的 id 不存在会被跳过（数组为空也返回 `true`）
- 返回值：已发出 `true`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const createText = ll.import("HologramLib", "shapeCreateText");
    const createLine = ll.import("HologramLib", "shapeCreateLine");
    const drawBatch = ll.import("HologramLib", "shapeDrawBatch");
    const a = createText(100, 66, 100, "§eA");
    const b = createLine(100, 65, 100, 110, 65, 110);
    drawBatch([a, b]);
    ```

### 对所有玩家隐藏形状

`shapeRemove(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const draw = ll.import("HologramLib", "shapeDraw");
    const remove = ll.import("HologramLib", "shapeRemove");
    remove(id);        // 藏起来
    draw(id);          // 用同一个 id 再显示回来
    ```

### 只让某一名玩家看不到形状

`shapeRemoveToPlayer(id, playerName)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `playerName` : `String`<br>玩家名；玩家不在线返回 `false`
- 返回值：成功 `true`；`id` 不存在或玩家不在线 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const removeToPlayer = ll.import("HologramLib", "shapeRemoveToPlayer");
    removeToPlayer(id, "Steve");
    ```

### 让某个维度的玩家看不到形状

`shapeRemoveToDimension(id, dimId)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `dimId` : `Integer`<br>维度编号 —— `0` 主世界、`1` 下界、`2` 末地
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const removeToDimension = ll.import("HologramLib", "shapeRemoveToDimension");
    removeToDimension(id, 0);
    ```

### 把改过的属性重发给观看者

`shapeUpdate(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：`id` 存在就返回 `true`；形状还没显示过时不做任何事（也不会报错），`id` 不存在返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setText = ll.import("HologramLib", "shapeSetText");
    const update = ll.import("HologramLib", "shapeUpdate");
    const id = create(100, 66, 100, "§e倒计时: 10");
    ll.import("HologramLib", "shapeDraw")(id);
    setText(id, "§e倒计时: 9");
    update(id);        // 只改文字不会自己上屏，要重发
    ```

### 把改过的属性重发给某名玩家

`shapeUpdateToPlayer(id, playerName)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `playerName` : `String`<br>玩家名；玩家不在线返回 `false`
- 返回值：`id` 存在返回 `true`（没显示过时不发）；`id` 不存在或玩家不在线 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const updateToPlayer = ll.import("HologramLib", "shapeUpdateToPlayer");
    updateToPlayer(id, "Steve");
    ```

### 把改过的属性重发给某个维度

`shapeUpdateToDimension(id, dimId)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `dimId` : `Integer`<br>维度编号 —— `0` 主世界、`1` 下界、`2` 末地
- 返回值：`id` 存在返回 `true`（没显示过时不发）；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const updateToDimension = ll.import("HologramLib", "shapeUpdateToDimension");
    updateToDimension(id, 0);
    ```

## 查询

### 找出某个点附近的文本形状

`shapeFindTextByLocation(x, y, z, radius)`

- 参数：
  - `x` : `Float`<br>要查询的中心世界坐标 X
  - `y` : `Float`<br>要查询的中心世界坐标 Y
  - `z` : `Float`<br>要查询的中心世界坐标 Z
  - `radius` : `Float`<br>查询半径（含边界）；只有文本形状会参与匹配，线 / 盒 / 圆 / 球 / 箭头不返回
- 返回值：命中的文本形状 id 数组；一个都没有时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const find = ll.import("HologramLib", "shapeFindTextByLocation");
    const ids = find(100, 65, 100, 10);
    for (const id of ids) logger.info(`附近文本形状 ${id}`);
    ```

### 按内容查找附近的文本形状

`shapeFindTextByLocationAndContent(x, y, z, radius, text)`

- 参数：
  - `x` : `Float`<br>中心世界坐标 X
  - `y` : `Float`<br>中心世界坐标 Y
  - `z` : `Float`<br>中心世界坐标 Z
  - `radius` : `Float`<br>查询半径（含边界）
  - `text` : `String`<br>要比对的完整文本（完全相等才算命中）
- 返回值：第一个命中的文本形状 id；没找到返回 `-1`
- 返回值类型：`Integer`
- 示例：

    ```js
    const find = ll.import("HologramLib", "shapeFindTextByLocationAndContent");
    const id = find(100, 65, 100, 10, "§e主城");
    if (id !== -1) ll.import("HologramLib", "shapeDestroy")(id);
    ```

### 列出当前所有形状的 id

`shapeGetAllShapeIds()`

- 返回值：全部形状 id（所有类型）；一个形状都没有时是空数组
- 返回值类型：`Array<Integer>`
- 示例：

    ```js
    const getAll = ll.import("HologramLib", "shapeGetAllShapeIds");
    const ids = getAll();
    logger.info(`当前有 ${ids.length} 个形状`);
    ```

### 判断形状 id 是否还存在

`shapeExists(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：存在 `true`；不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const exists = ll.import("HologramLib", "shapeExists");
    if (!exists(id)) logger.info("这个形状已经销毁了");
    ```

## 销毁

### 从两端一起删掉形状

`shapeDestroy(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：删除成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const destroy = ll.import("HologramLib", "shapeDestroy");
    destroy(id);
    ```

### 删掉当前所有形状

`shapeDestroyAll()`

- 返回值：没有返回值；调用后客户端上残留的形状会一并消失
- 返回值类型：`Null`
- 示例：

    ```js
    const destroyAll = ll.import("HologramLib", "shapeDestroyAll");
    destroyAll();      // 插件卸载或重载时收尾
    ```

### 一次删掉一批形状

`shapeDestroyBatch(ids)`

- 参数：
  - `ids` : `Array<Integer>`<br>形状 id 数组；数组里不存在的 id 会被跳过（数组为空也返回 `true`）
- 返回值：已处理 `true`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const destroyBatch = ll.import("HologramLib", "shapeDestroyBatch");
    destroyBatch([a, b, c]);
    ```

## 注意事项

- **参数要按签名给全**。LSE 侧没有默认值：少给或多给参数，调用会直接拿到失败值（`Boolean` 得 `false`、`Integer` 得 `0`、`String` 得空串），不会有异常提示。
- **改完属性要重发**：`shapeSetText` / `shapeSetLocation` / `shapeSetColor` / `shapeSetScale` / `shapeSetRotation` / `shapeSetBackgroundColor` / `shapeSetDepthTest` / `shapeSetDimension` 都只改服务端记的值，已经显示的形状要再调 `shapeUpdate`（或 `shapeUpdateToPlayer` / `shapeUpdateToDimension`），或者直接再 `shapeDraw` 一次。
- **文本形状专属的三项**：背景框颜色（`shapeSetBackgroundColor`）、穿墙开关（`shapeSetDepthTest`）、三轴旋转的朝向开关。线 / 盒 / 圆 / 球 / 箭头调前两个会直接返回 `false`。
- **颜色按 8 位保存**：写入 `0.5` 后用 `shapeGetColor` 读回可能是 `0.498` 或 `0.502`，比较颜色时不要用严格相等。
- **`shapeSetDuration` 是倒计时删除**：设了以后客户端会在时间到时自己把形状删掉，要做永久显示就不要调用它。
- **可见范围由显示函数决定**：`shapeDraw` 发给所有在线玩家（不限维度），`shapeDrawToDimension` 只发给该维度，`shapeDrawToPlayer` 只发给那名玩家；之后玩家进服时的自动补发也沿用最后一次的设置。
- **玩家进服不用自己补发**：这些形状只存在客户端内存里，玩家进服后由库自动补发（进服 1 秒、5 秒各一次，之后每 15 秒兜底重发）。
- **建议在主线程调用**（服务端 tick、事件回调里）。
- 库没装或没加载时，`ll.import` 会返回 `null`，直接调用会报错；可以先判空，或先调 `ll.hasExported("HologramLib", "shapeDraw")` 做能力检查。
