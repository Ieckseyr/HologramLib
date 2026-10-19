# DebugShape（悬浮字与形状）· LSE

协议层形状与悬浮字；不生成实体，直接发形状包。


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

## 快速开始

```js
const holoCreate   = ll.import("HologramLib", "holoCreate");
const holoAddLine  = ll.import("HologramLib", "holoAddLine");
const holoSetScale = ll.import("HologramLib", "holoSetScale");
const holoDraw     = ll.import("HologramLib", "holoDraw");

const holo = holoCreate(100.5, 66, 100.5);
holoAddLine(holo, "§e欢迎来到主城");     // § 颜色代码只作用于这一行
holoAddLine(holo, "在线: {online}");      // {online} 会替换成当前在线人数
holoSetScale(holo, 1.5);                  // 整块放大
holoDraw(holo);                           // 对全体在线玩家显示
```

渐变线的最小用法：

```js
const gradientCreate = ll.import("HologramLib", "gradientCreate");
const gradientDraw   = ll.import("HologramLib", "gradientDraw");

const trail = gradientCreate(100, 65, 100, 130, 70, 130, 20);   // 20 段拼出来
gradientDraw(trail);                                            // 默认红 → 蓝
```

## 悬浮字：创建

### 在指定坐标创建悬浮字

`holoCreate(x, y, z)`

- 参数：
  - `x` : `Float`<br>悬浮字所在的世界坐标 X
  - `y` : `Float`<br>悬浮字所在的世界坐标 Y
  - `z` : `Float`<br>悬浮字所在的世界坐标 Z
- 返回值：悬浮字 id（从 `1` 开始递增）；`<= 0` 表示创建失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "holoCreate");
    const holo = create(100.5, 66, 100.5);   // 还没显示，加完行再 draw
    ```

### 删掉一块悬浮字并同步客户端

`holoDestroy(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：删除成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const destroy = ll.import("HologramLib", "holoDestroy");
    destroy(holo);
    ```

### 删掉当前所有悬浮字

`holoDestroyAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const destroyAll = ll.import("HologramLib", "holoDestroyAll");
    destroyAll();      // 插件卸载或重载时收尾
    ```

## 悬浮字：行与内容

### 在末尾追加一行文字

`holoAddLine(id, text)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `text` : `String`<br>这一行的文本，可以用 `§` 颜色代码和 `{变量}`
- 返回值：追加成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "holoCreate");
    const addLine = ll.import("HologramLib", "holoAddLine");
    const draw = ll.import("HologramLib", "holoDraw");
    const holo = create(100.5, 66, 100.5);
    addLine(holo, "§e第一行");
    addLine(holo, "§7第二行");
    draw(holo);        // 已经显示时再 addLine 会立即刷新，不用重新 draw
    ```

### 修改某一行的文字

`holoSetLineText(id, lineIndex, text)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `lineIndex` : `Integer`<br>行号，从 `0` 开始
  - `text` : `String`<br>新的文本
- 返回值：成功 `true`；`id` 不存在或行号越界 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setLineText = ll.import("HologramLib", "holoSetLineText");
    setLineText(holo, 1, "§7在线: {online} 人");   // 改第 2 行
    ```

### 给某一行设置轮播内容池

`holoSetLinePool(id, lineIndex, content, intervalMs)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `lineIndex` : `Integer`<br>行号，从 `0` 开始
  - `content` : `Array<String>`<br>候选内容数组；传空数组 = 清除内容池，这一行回到 `holoSetLineText` 设的文本
  - `intervalMs` : `Integer`<br>轮播间隔，单位**毫秒**。池里只有 1 项、或 `intervalMs <= 0` 时不轮播，固定显示第 0 项
- 返回值：设置成功 `true`；`id` 不存在或行号越界 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setLinePool = ll.import("HologramLib", "holoSetLinePool");
    const pool = ["§a在线人数: {online}", "§eTPS: {tps}", "§b时间: {time}"];
    setLinePool(holo, 0, pool, 2000);   // 每 2 秒换一条
    ```

### 设置某一行是否解析变量

`holoSetLineParseVariables(id, lineIndex, enabled)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `lineIndex` : `Integer`<br>行号，从 `0` 开始
  - `enabled` : `Boolean`<br>`true` = 解析变量（默认）；`false` = 这一行的 `{}` 按字面量原样显示
- 返回值：设置成功 `true`；`id` 不存在或行号越界 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setLineText = ll.import("HologramLib", "holoSetLineText");
    const setParse = ll.import("HologramLib", "holoSetLineParseVariables");
    setLineText(holo, 0, "§7写法示例: {online}");
    setParse(holo, 0, false);      // 屏幕上也显示 "{online}" 这七个字符
    ```

### 删掉某一行并让后续行前移

`holoRemoveLine(id, lineIndex)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `lineIndex` : `Integer`<br>要删除的行号，从 `0` 开始
- 返回值：删除成功 `true`；`id` 不存在或行号越界 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const removeLine = ll.import("HologramLib", "holoRemoveLine");
    removeLine(holo, 0);   // 删掉第 1 行，原来的第 2 行变成行号 0
    ```

### 清空所有行并撤掉显示

`holoClearLines(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const clearLines = ll.import("HologramLib", "holoClearLines");
    clearLines(holo);      // 清空后还可以 addLine 重新排内容
    ```

### 查询悬浮字当前的行数

`holoGetLineCount(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：行数；`id` 不存在返回 `0`
- 返回值类型：`Integer`
- 示例：

    ```js
    const getLineCount = ll.import("HologramLib", "holoGetLineCount");
    logger.info(`这块悬浮字有 ${getLineCount(holo)} 行`);
    ```

## 悬浮字：整块样式

### 设置整块文字的颜色

`holoSetColor(id, r, g, b, a)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `r` : `Float`<br>红色分量，`0.0 ~ 1.0`
  - `g` : `Float`<br>绿色分量，`0.0 ~ 1.0`
  - `b` : `Float`<br>蓝色分量，`0.0 ~ 1.0`
  - `a` : `Float`<br>透明度分量，`0.0 ~ 1.0`
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setColor = ll.import("HologramLib", "holoSetColor");
    setColor(holo, 1.0, 0.9, 0.3, 1.0);   // 整块变金黄
    ```

### 设置整块文字的缩放

`holoSetScale(id, scale)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `scale` : `Float`<br>缩放倍数，必须**大于 0**（`1` = 原始大小）
- 返回值：成功 `true`；`id` 不存在或 `scale <= 0` 返回 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setScale = ll.import("HologramLib", "holoSetScale");
    setScale(holo, 2);     // 放大一倍
    ```

### 设置文字底板的颜色

`holoSetBackgroundColor(id, r, g, b, a)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `r` : `Float`<br>底板红色分量，`0.0 ~ 1.0`
  - `g` : `Float`<br>底板绿色分量，`0.0 ~ 1.0`
  - `b` : `Float`<br>底板蓝色分量，`0.0 ~ 1.0`
  - `a` : `Float`<br>底板透明度分量，`0.0 ~ 1.0`；不设置就是客户端默认色
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setBackgroundColor = ll.import("HologramLib", "holoSetBackgroundColor");
    setBackgroundColor(holo, 0, 0, 0, 0.5);   // 半透明黑底
    ```

### 把底板恢复成默认颜色

`holoClearBackgroundColor(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const clearBackgroundColor = ll.import("HologramLib", "holoClearBackgroundColor");
    clearBackgroundColor(holo);
    ```

### 设置悬浮字是否被方块遮挡

`holoSetDepthTest(id, enabled)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `enabled` : `Boolean`<br>`true` = 被方块 / 实体遮挡；`false` = 始终渲染（穿墙可见），也是默认行为
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setDepthTest = ll.import("HologramLib", "holoSetDepthTest");
    setDepthTest(holo, true);   // 走进墙后就被挡住
    setDepthTest(holo, false);  // 换回穿墙可见
    ```

### 把整块文字固定成三轴朝向

`holoSetRotation(id, pitch, yaw, roll)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `pitch` : `Float`<br>绕三个轴的旋转角度之一，单位**度**
  - `yaw` : `Float`<br>绕三个轴的旋转角度之一，单位**度**
  - `roll` : `Float`<br>绕三个轴的旋转角度之一，单位**度**
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setRotation = ll.import("HologramLib", "holoSetRotation");
    setRotation(holo, 0, 45, 0);   // 绕 Y 轴转 45 度
    ```

### 清掉旋转让文字正对镜头

`holoClearRotation(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const clearRotation = ll.import("HologramLib", "holoClearRotation");
    clearRotation(holo);   // 三个角度也一起归零
    ```

## 悬浮字：位置与跟随

### 把悬浮字挪到新的坐标

`holoSetLocation(id, x, y, z)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `x` : `Float`<br>新的世界坐标 X
  - `y` : `Float`<br>新的世界坐标 Y
  - `z` : `Float`<br>新的世界坐标 Z
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setLocation = ll.import("HologramLib", "holoSetLocation");
    setLocation(holo, 120.5, 70, 100.5);   // 已显示时会跟着移动
    ```

### 把悬浮字迁到另一个维度

`holoSetDimension(id, dimId)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `dimId` : `Integer`<br>目标维度编号 —— `0` 主世界、`1` 下界、`2` 末地
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setDimension = ll.import("HologramLib", "holoSetDimension");
    setDimension(holo, 1);   // 已显示时原地重发，不会闪
    ```

### 让悬浮字跟随某位玩家

`holoSetFollowPlayer(id, playerName, offsetY)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `playerName` : `String`<br>要跟随的玩家名（LSE 的 `realName`）
  - `offsetY` : `Float`<br>在玩家脚位 Y 上再加的高度偏移
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setFollowPlayer = ll.import("HologramLib", "holoSetFollowPlayer");
    const refresh = ll.import("HologramLib", "holoRefresh");
    setFollowPlayer(holo, "Steve", 2.5);
    // 跟随不会自动推进：想让字一直贴在玩家头上，要自己定时刷新
    setInterval(() => refresh(holo), 100);
    ```

### 取消跟随并停在当前位置

`holoClearFollowPlayer(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const clearFollowPlayer = ll.import("HologramLib", "holoClearFollowPlayer");
    clearFollowPlayer(holo);   // 停在最后一次解析到的位置
    ```

## 悬浮字：显示与刷新

### 对所有在线玩家显示悬浮字

`holoDraw(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：已显示 `true`；`id` 不存在（或没有内容可显示）`false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "holoCreate");
    const addLine = ll.import("HologramLib", "holoAddLine");
    const draw = ll.import("HologramLib", "holoDraw");
    const holo = create(100.5, 66, 100.5);
    addLine(holo, "§e主城");
    draw(holo);
    ```

### 只对某个维度显示悬浮字

`holoDrawToDimension(id, dimId)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `dimId` : `Integer`<br>维度编号 —— `0` 主世界、`1` 下界、`2` 末地
- 返回值：已显示 `true`；`id` 不存在（或没有内容可显示）`false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const drawToDimension = ll.import("HologramLib", "holoDrawToDimension");
    drawToDimension(holo, 0);   // 只有主世界玩家看得到
    ```

### 只给某一名玩家显示悬浮字

`holoDrawToPlayer(id, playerName)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
  - `playerName` : `String`<br>玩家名；玩家不在线时不会有任何显示
- 返回值：已显示 `true`；`id` 不存在、玩家不在线或没有内容可显示 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const drawToPlayer = ll.import("HologramLib", "holoDrawToPlayer");
    drawToPlayer(holo, "Steve");   // 专属提示
    ```

### 把悬浮字藏起来但保留内容

`holoRemove(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const remove = ll.import("HologramLib", "holoRemove");
    const draw = ll.import("HologramLib", "holoDraw");
    remove(holo);   // 藏起来
    draw(holo);     // 同一个 id 再显示
    ```

### 原地重新解析变量与坐标

`holoRefresh(id)`

- 参数：
  - `id` : `Integer`<br>悬浮字 id
- 返回值：刷新成功 `true`；`id` 不存在、或这块悬浮字还没显示过 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const refresh = ll.import("HologramLib", "holoRefresh");
    // 倒计时、玩家坐标这类内容不会自己变，需要按需调用
    setInterval(() => refresh(holo), 1000);   // 每秒刷新一次
    ```

## 渐变线：创建

### 创建一条多段渐变直线

`gradientCreate(x1, y1, z1, x2, y2, z2, segments)`

- 参数：
  - `x1` : `Float`<br>起点世界坐标 X
  - `y1` : `Float`<br>起点世界坐标 Y
  - `z1` : `Float`<br>起点世界坐标 Z
  - `x2` : `Float`<br>终点世界坐标 X
  - `y2` : `Float`<br>终点世界坐标 Y
  - `z2` : `Float`<br>终点世界坐标 Z
  - `segments` : `Integer`<br>分段数，段数越多过渡越平滑、线段也越多；低于 `2` 按 `2` 处理，高于 `100` 按 `100` 处理，建议 10 ~ 50
- 返回值：渐变线 id（从 `1` 开始递增）；`<= 0` 表示创建失败
- 返回值类型：`Integer`
- 示例：

    ```js
    const create = ll.import("HologramLib", "gradientCreate");
    const draw = ll.import("HologramLib", "gradientDraw");
    const trail = create(100, 65, 100, 140, 75, 140, 30);
    draw(trail);
    ```

## 渐变线：颜色

### 设置起点色与终点色

`gradientSetGradient(id, r1, g1, b1, r2, g2, b2)`

- 参数：
  - `id` : `Integer`<br>渐变线 id
  - `r1` : `Float`<br>起点颜色的红色分量，`0.0 ~ 1.0`
  - `g1` : `Float`<br>起点颜色的绿色分量，`0.0 ~ 1.0`
  - `b1` : `Float`<br>起点颜色的蓝色分量，`0.0 ~ 1.0`
  - `r2` : `Float`<br>终点颜色的红色分量，`0.0 ~ 1.0`
  - `g2` : `Float`<br>终点颜色的绿色分量，`0.0 ~ 1.0`
  - `b2` : `Float`<br>终点颜色的蓝色分量，`0.0 ~ 1.0`
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setGradient = ll.import("HologramLib", "gradientSetGradient");
    setGradient(trail, 1.0, 0.2, 0.2, 0.2, 0.4, 1.0);   // 红 → 蓝
    ```

### 让渐变线按彩虹色循环

`gradientSetRainbow(id, speed)`

- 参数：
  - `id` : `Integer`<br>渐变线 id
  - `speed` : `Float`<br>循环速度，`1.0` 时色相每秒前进 60 度；越大越快
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setRainbow = ll.import("HologramLib", "gradientSetRainbow");
    const tick = ll.import("HologramLib", "gradientTick");
    setRainbow(trail, 1.0);
    setInterval(() => tick(0.05), 50);   // 彩虹要自己按 tick 推进
    ```

### 把整条线设成同一种颜色

`gradientSetColor(id, r, g, b, a)`

- 参数：
  - `id` : `Integer`<br>渐变线 id
  - `r` : `Float`<br>红色分量，`0.0 ~ 1.0`
  - `g` : `Float`<br>绿色分量，`0.0 ~ 1.0`
  - `b` : `Float`<br>蓝色分量，`0.0 ~ 1.0`
  - `a` : `Float`<br>透明度分量，`0.0 ~ 1.0`
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setColor = ll.import("HologramLib", "gradientSetColor");
    setColor(trail, 0.3, 1.0, 0.5, 0.8);   // 半透明绿线
    ```

### 改掉渐变线的两个端点

`gradientSetEndpoints(id, x1, y1, z1, x2, y2, z2)`

- 参数：
  - `id` : `Integer`<br>渐变线 id
  - `x1` : `Float`<br>新的起点世界坐标 X
  - `y1` : `Float`<br>新的起点世界坐标 Y
  - `z1` : `Float`<br>新的起点世界坐标 Z
  - `x2` : `Float`<br>新的终点世界坐标 X
  - `y2` : `Float`<br>新的终点世界坐标 Y
  - `z2` : `Float`<br>新的终点世界坐标 Z
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const setEndpoints = ll.import("HologramLib", "gradientSetEndpoints");
    setEndpoints(trail, 100, 70, 100, 160, 80, 160);   // 已显示时会重建线段
    ```

## 渐变线：显示与销毁

### 对所有在线玩家显示渐变线

`gradientDraw(id)`

- 参数：
  - `id` : `Integer`<br>渐变线 id
- 返回值：已显示 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const create = ll.import("HologramLib", "gradientCreate");
    const draw = ll.import("HologramLib", "gradientDraw");
    const trail = create(100, 65, 100, 140, 75, 140, 30);
    draw(trail);
    ```

### 只对某个维度显示渐变线

`gradientDrawToDimension(id, dimId)`

- 参数：
  - `id` : `Integer`<br>渐变线 id
  - `dimId` : `Integer`<br>维度编号 —— `0` 主世界、`1` 下界、`2` 末地
- 返回值：已显示 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const drawToDimension = ll.import("HologramLib", "gradientDrawToDimension");
    drawToDimension(trail, 0);
    ```

### 隐藏渐变线但保留数据

`gradientRemove(id)`

- 参数：
  - `id` : `Integer`<br>渐变线 id
- 返回值：成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const remove = ll.import("HologramLib", "gradientRemove");
    const draw = ll.import("HologramLib", "gradientDraw");
    remove(trail);   // 藏起来
    draw(trail);     // 再显示
    ```

### 彻底删掉一条渐变线

`gradientDestroy(id)`

- 参数：
  - `id` : `Integer`<br>渐变线 id
- 返回值：删除成功 `true`；`id` 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const destroy = ll.import("HologramLib", "gradientDestroy");
    destroy(trail);
    ```

### 删掉当前所有渐变线

`gradientDestroyAll()`

- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const destroyAll = ll.import("HologramLib", "gradientDestroyAll");
    destroyAll();
    ```

## 渐变线：动画

### 推进彩虹动画的时间

`gradientTick(deltaTime)`

- 参数：
  - `deltaTime` : `Float`<br>这一拍过去的时间，单位**秒**；20 tps 的一拍是 `0.05`。只有彩虹模式的线会被推进，普通渐变线和纯色线不受影响
- 返回值：没有返回值
- 返回值类型：`Null`
- 示例：

    ```js
    const tick = ll.import("HologramLib", "gradientTick");
    setInterval(() => tick(0.05), 50);   // 约等于每游戏刻推进一次
    ```

## 注意事项

- **参数要按签名给全**。LSE 侧没有默认值：少给或多给参数，调用会直接拿到失败值（`Boolean` 得 `false`、`Integer` 得 `0`、`String` 得空串），不会有异常提示。
- **行级样式不存在**：多行共用一份颜色 / 缩放 / 背景框 / 穿墙 / 朝向。某一行要换颜色，就在那一行的文本里写 `§` 颜色代码（例如 `"§c红字 §a绿字"`）；整块放大用 `holoSetScale`。`26.40.8` 起 `holoSetLineScale`、`holoSetLineColor`、`holoSetLineGradient`、`holoSetLineRainbow`、`holoSetLineScroll`、`holoSetVerticalAnimation`、`holoSetLineSpacing`、`holoTick` 这 8 个行级函数已经移除，`ll.import` 会得到 `null`。
- **变量**：文本里的 `{player}`、`{online}`、`{time}`、`{tps}`、`{dimension}`、`{x}`、`{y}`、`{z}` 会被替换（装了 MeowPAPI 时还支持它提供的外部占位符）。含变量、且显示目标不是单个玩家时，库会为每个观看者各维护一份文字：**形状数量会变成观看者数**，适合队长视角、任务追踪这类少数人场景，不要用在全服广播的高频刷新上。
- **动态行按时间轮播**：内容池用「当前时间 ÷ 间隔」取模选条目，所以多块悬浮字用同一个池、同一个间隔时天然同步；改池内容会立刻反映出来，刷新本身由库按 0.5 秒节流，内容没变就不会重发。
- **跟随不会自己推进**：`holoSetFollowPlayer` 只在调用那一刻取一次玩家坐标，`holoDraw`、`holoRefresh` 也会各取一次。玩家走动时字不会跟着走，需要自己定时 `holoRefresh`（例如每 2 tick 一次）；玩家下线后字停在最后一次解析到的位置。
- **切到单玩家目标会改变变量行为**：`holoDrawToPlayer` 之后只有那一名玩家收到文字，变量也按他一个人解析。
- **渐变线的颜色改动要等下一次重发**：`gradientSetGradient`、`gradientSetColor`、`gradientSetRainbow`、`gradientTick` 只更新线段颜色值，客户端要等重新 `gradientDraw` 或库内每 15 秒的兜底重发才看到新颜色。
- **频繁 `gradientDraw` 会闪**：它会先销毁旧线段再建新的；`gradientSetEndpoints` 在已显示时也会重建线段。
- **玩家进服不用自己补发**：悬浮字和渐变线都只存在客户端内存里，玩家进服后由库自动补发（进服 1 秒、5 秒各一次，之后每 15 秒兜底重发）。
- **建议在主线程调用**（服务端 tick、事件回调里）。
- 库没装或没加载时，`ll.import` 会返回 `null`，直接调用会报错；可以先判空，或先调 `ll.hasExported("HologramLib", "holoDraw")` 做能力检查。
