# 粒子 · LSE

协议层粒子形状：点/线/环/面/框/多面体，支持平移、旋转、缩放、跟随。


## 粒子形状 · 创建

粒子形状按**世界坐标**定义，创建后由库自己按 tick 发射与重发，你只管控制它。七种形状：

| 函数 | 摆出来的样子 |
|---|---|
| `particleCreatePoint` | 一个点 |
| `particleCreateLine` | 两点之间的一条线 |
| `particleCreateRect` | 矩形**边框**（环线） |
| `particleCreatePlane` | 填满的矩形**面**（网格） |
| `particleCreateBox` | 长方体的 12 条边线框 |
| `particleCreateBoxFaces` | 长方体的六个面都填满 |
| `particleCreatePoly` | 自定义多面体（自己给顶点和边） |

创建参数里有几个是共通的：

- `owner` ：创建者名字标签。**只用于 `particleGetInfo` 的诊断显示，不影响谁能看到这个形状** —— 可见性由 `particleSetVisible` 决定。
- `dimId` ：维度，`0` 主世界 / `1` 下界 / `2` 末地。
- `step` ：采样步长（格），也就是相邻两个粒子之间大约隔多远；小于 `0.25` 会按 `0.25` 处理。步长越小粒子越密、发包越多。
- `effect` ：粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串会用默认的 `minecraft:endrod`。
- `interval` ：整批重发间隔（tick）。粒子本身是瞬态的，靠按这个间隔重发维持"常驻"的观感；小于 `1` 会按 `1` 处理。
- `lifetime` ：存活时长（tick）；`0` = 永久，到时间后形状自动消失。

### 摆一个粒子点

`particleCreatePoint(owner, dimId, x, y, z, effect, interval, lifetime)`

- 参数：
  - `owner` : `String`<br>创建者名字标签，**只用于 `particleGetInfo` 的诊断显示，不影响谁能看到这个形状** —— 可见性由 `particleSetVisible` 决定
  - `dimId` : `Integer`<br>维度，`0` 主世界 / `1` 下界 / `2` 末地
  - `effect` : `String`<br>粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串会用默认的 `minecraft:endrod`
  - `interval` : `Integer`<br>整批重发间隔（tick）。粒子本身是瞬态的，靠按这个间隔重发维持"常驻"的观感；小于 `1` 会按 `1` 处理
  - `lifetime` : `Integer`<br>存活时长（tick）；`0` = 永久，到时间后形状自动消失
  - `x` : `Float`<br>粒子所在的世界坐标 X
  - `y` : `Float`<br>粒子所在的世界坐标 Y
  - `z` : `Float`<br>粒子所在的世界坐标 Z
- 返回值：形状 id（从 1 开始自增）；失败返回负数
- 返回值类型：`Integer`
- 示例：

    ```js
    const particleCreatePoint = ll.import("HologramLib", "particleCreatePoint");
    const pid = particleCreatePoint("demo", 0, 100.5, 64.5, 100.5, "minecraft:endrod", 4, 0);
    ```

### 摆一条粒子线

`particleCreateLine(owner, dimId, x1, y1, z1, x2, y2, z2, step, effect, interval, lifetime)`

- 参数：
  - `owner` : `String`<br>创建者名字标签，**只用于 `particleGetInfo` 的诊断显示，不影响谁能看到这个形状** —— 可见性由 `particleSetVisible` 决定
  - `dimId` : `Integer`<br>维度，`0` 主世界 / `1` 下界 / `2` 末地
  - `step` : `Float`<br>采样步长（格），也就是相邻两个粒子之间大约隔多远；小于 `0.25` 会按 `0.25` 处理。步长越小粒子越密、发包越多
  - `effect` : `String`<br>粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串会用默认的 `minecraft:endrod`
  - `interval` : `Integer`<br>整批重发间隔（tick）。粒子本身是瞬态的，靠按这个间隔重发维持"常驻"的观感；小于 `1` 会按 `1` 处理
  - `lifetime` : `Integer`<br>存活时长（tick）；`0` = 永久，到时间后形状自动消失
  - `x1` : `Float`<br>第一个端点的世界坐标 X
  - `y1` : `Float`<br>第一个端点的世界坐标 Y
  - `z1` : `Float`<br>第一个端点的世界坐标 Z
  - `x2` : `Float`<br>第二个端点的世界坐标 X
  - `y2` : `Float`<br>第二个端点的世界坐标 Y
  - `z2` : `Float`<br>第二个端点的世界坐标 Z
- 返回值：形状 id；失败返回负数。锚点是这条线的**中点**：之后的旋转、缩放都绕中点进行
- 返回值类型：`Integer`
- 示例：

    ```js
    const particleCreateLine = ll.import("HologramLib", "particleCreateLine");
    const pid = particleCreateLine("demo", 0, 100, 64, 100, 110, 64, 100, 0.5, "minecraft:endrod", 4, 0);
    ```

### 摆一个粒子矩形边框

`particleCreateRect(owner, dimId, cx, cy, cz, w, h, axis, step, effect, interval, lifetime)`

- 参数：
  - `owner` : `String`<br>创建者名字标签，**只用于 `particleGetInfo` 的诊断显示，不影响谁能看到这个形状** —— 可见性由 `particleSetVisible` 决定
  - `dimId` : `Integer`<br>维度，`0` 主世界 / `1` 下界 / `2` 末地
  - `step` : `Float`<br>采样步长（格），也就是相邻两个粒子之间大约隔多远；小于 `0.25` 会按 `0.25` 处理。步长越小粒子越密、发包越多
  - `effect` : `String`<br>粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串会用默认的 `minecraft:endrod`
  - `interval` : `Integer`<br>整批重发间隔（tick）。粒子本身是瞬态的，靠按这个间隔重发维持"常驻"的观感；小于 `1` 会按 `1` 处理
  - `lifetime` : `Integer`<br>存活时长（tick）；`0` = 永久，到时间后形状自动消失
  - `cx` : `Float`<br>矩形中心的世界坐标 X
  - `cy` : `Float`<br>矩形中心的世界坐标 Y
  - `cz` : `Float`<br>矩形中心的世界坐标 Z
  - `w` : `Float`<br>矩形沿平面第一个轴的尺寸（宽）；负数按 `0` 处理
  - `h` : `Float`<br>矩形沿平面第二个轴的尺寸（高）；负数按 `0` 处理
  - `axis` : `Integer`<br>矩形所在平面：`0`=XY（`w` 沿 X、`h` 沿 Y）、`1`=YZ（`w` 沿 Z、`h` 沿 Y）、`2`=XZ（`w` 沿 X、`h` 沿 Z）；其它值按 `0` 处理
- 返回值：形状 id；失败返回负数。锚点是矩形中心
- 返回值类型：`Integer`
- 示例：

    ```js
    const particleCreateRect = ll.import("HologramLib", "particleCreateRect");
    // 躺在 XZ 平面上的 8×8 光环（axis=2），粒子间隔 1.5 格
    const pid = particleCreateRect("demo", 0, 100.5, 64.1, 100.5, 8, 8, 2, 1.5, "minecraft:endrod", 2, 0);
    ```

### 摆一个粒子填充面

`particleCreatePlane(owner, dimId, cx, cy, cz, w, h, axis, step, effect, interval, lifetime)`

参数与 `particleCreateRect` 完全相同。

- 参数：
  - `owner` : `String`<br>创建者名字标签，**只用于 `particleGetInfo` 的诊断显示，不影响谁能看到这个形状** —— 可见性由 `particleSetVisible` 决定
  - `dimId` : `Integer`<br>维度，`0` 主世界 / `1` 下界 / `2` 末地
  - `cx` : `Float`<br>平面中心的世界坐标 X
  - `cy` : `Float`<br>平面中心的世界坐标 Y
  - `cz` : `Float`<br>平面中心的世界坐标 Z
  - `w` : `Float`<br>平面第一个轴的尺寸；负数按 `0` 处理
  - `h` : `Float`<br>平面第二个轴的尺寸；负数按 `0` 处理
  - `axis` : `Integer`<br>平面朝向，`0`=XY `1`=YZ `2`=XZ；其它值按 `0` 处理
  - `step` : `Float`<br>采样步长（格），也就是相邻两个粒子之间大约隔多远；小于 `0.25` 会按 `0.25` 处理。步长越小粒子越密、发包越多。面形状要慎调小 `step` —— 粒子数量按面积增长
  - `effect` : `String`<br>粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串会用默认的 `minecraft:endrod`
  - `interval` : `Integer`<br>整批重发间隔（tick）。粒子本身是瞬态的，靠按这个间隔重发维持"常驻"的观感；小于 `1` 会按 `1` 处理
  - `lifetime` : `Integer`<br>存活时长（tick）；`0` = 永久，到时间后形状自动消失
- 返回值：形状 id；失败返回负数。锚点是平面中心
- 返回值类型：`Integer`
- 示例：

    ```js
    const particleCreatePlane = ll.import("HologramLib", "particleCreatePlane");
    // 立在 XY 平面上的一堵粒子墙（axis=0），每 10 tick 重发
    const pid = particleCreatePlane("demo", 0, 100, 64, -200, 12, 6, 0, 3, "minecraft:heart_particle", 10, 0);
    ```

### 摆一个粒子长方体线框

`particleCreateBox(owner, dimId, cx, cy, cz, hx, hy, hz, step, effect, interval, lifetime)`

- 参数：
  - `owner` : `String`<br>创建者名字标签，**只用于 `particleGetInfo` 的诊断显示，不影响谁能看到这个形状** —— 可见性由 `particleSetVisible` 决定
  - `dimId` : `Integer`<br>维度，`0` 主世界 / `1` 下界 / `2` 末地
  - `step` : `Float`<br>采样步长（格），也就是相邻两个粒子之间大约隔多远；小于 `0.25` 会按 `0.25` 处理。步长越小粒子越密、发包越多
  - `effect` : `String`<br>粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串会用默认的 `minecraft:endrod`
  - `interval` : `Integer`<br>整批重发间隔（tick）。粒子本身是瞬态的，靠按这个间隔重发维持"常驻"的观感；小于 `1` 会按 `1` 处理
  - `lifetime` : `Integer`<br>存活时长（tick）；`0` = 永久，到时间后形状自动消失
  - `cx` : `Float`<br>长方体中心的世界坐标 X
  - `cy` : `Float`<br>长方体中心的世界坐标 Y
  - `cz` : `Float`<br>长方体中心的世界坐标 Z
  - `hx` : `Float`<br>X 方向的**半尺寸**（格子数），即边长的一半；负数按 `0` 处理
  - `hy` : `Float`<br>Y 方向的**半尺寸**（格子数），即边长的一半；负数按 `0` 处理
  - `hz` : `Float`<br>Z 方向的**半尺寸**（格子数），即边长的一半；负数按 `0` 处理
- 返回值：形状 id；失败返回负数。锚点是长方体中心
- 返回值类型：`Integer`
- 示例：

    ```js
    const particleCreateBox = ll.import("HologramLib", "particleCreateBox");
    // 边长 6 的立方体线框（半尺寸 3）
    const pid = particleCreateBox("demo", 0, 100.5, 64.5, 100.5, 3, 3, 3, 1, "minecraft:endrod", 2, 0);
    ```

### 摆一个粒子六面长方体

`particleCreateBoxFaces(owner, dimId, cx, cy, cz, hx, hy, hz, step, effect, interval, lifetime)`

参数与 `particleCreateBox` 完全相同。

- 参数：
  - `owner` : `String`<br>创建者名字标签，**只用于 `particleGetInfo` 的诊断显示，不影响谁能看到这个形状** —— 可见性由 `particleSetVisible` 决定
  - `dimId` : `Integer`<br>维度，`0` 主世界 / `1` 下界 / `2` 末地
  - `cx` : `Float`<br>长方体中心的世界坐标 X
  - `cy` : `Float`<br>长方体中心的世界坐标 Y
  - `cz` : `Float`<br>长方体中心的世界坐标 Z
  - `hx` : `Float`<br>X 方向的半尺寸；负数按 `0` 处理
  - `hy` : `Float`<br>Y 方向的半尺寸；负数按 `0` 处理
  - `hz` : `Float`<br>Z 方向的半尺寸；负数按 `0` 处理
  - `step` : `Float`<br>采样步长（格），也就是相邻两个粒子之间大约隔多远；小于 `0.25` 会按 `0.25` 处理。步长越小粒子越密、发包越多。六个面都要填，粒子数量是线框的很多倍
  - `effect` : `String`<br>粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串会用默认的 `minecraft:endrod`
  - `interval` : `Integer`<br>整批重发间隔（tick）。粒子本身是瞬态的，靠按这个间隔重发维持"常驻"的观感；小于 `1` 会按 `1` 处理
  - `lifetime` : `Integer`<br>存活时长（tick）；`0` = 永久，到时间后形状自动消失
- 返回值：形状 id；失败返回负数。锚点是长方体中心
- 返回值类型：`Integer`
- 示例：

    ```js
    const particleCreateBoxFaces = ll.import("HologramLib", "particleCreateBoxFaces");
    const pid = particleCreateBoxFaces("demo", 0, 100.5, 64.5, 100.5, 2, 2, 2, 1, "minecraft:endrod", 4, 0);
    ```

### 摆一个自定义多面体

`particleCreatePoly(owner, dimId, verts, edges, step, effect, interval, lifetime)`

- 参数：
  - `owner` : `String`<br>创建者名字标签，**只用于 `particleGetInfo` 的诊断显示，不影响谁能看到这个形状** —— 可见性由 `particleSetVisible` 决定
  - `dimId` : `Integer`<br>维度，`0` 主世界 / `1` 下界 / `2` 末地
  - `step` : `Float`<br>采样步长（格），也就是相邻两个粒子之间大约隔多远；小于 `0.25` 会按 `0.25` 处理。步长越小粒子越密、发包越多
  - `effect` : `String`<br>粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串会用默认的 `minecraft:endrod`
  - `interval` : `Integer`<br>整批重发间隔（tick）。粒子本身是瞬态的，靠按这个间隔重发维持"常驻"的观感；小于 `1` 会按 `1` 处理
  - `lifetime` : `Integer`<br>存活时长（tick）；`0` = 永久，到时间后形状自动消失
  - `verts` : `String`<br>顶点串，世界坐标，格式 `"x,y,z;x,y,z;..."`，至少 2 个顶点
  - `edges` : `String`<br>边串，顶点索引对，格式 `"i-j;i-j"`，索引从 `0` 开始、必须落在顶点范围内
- 返回值：形状 id；顶点少于 2 个、边串里一条可用边都没有、或索引越界时返回 `-1`。锚点是全部顶点的**质心**，旋转与缩放绕质心进行
- 返回值类型：`Integer`
- 示例：

    ```js
    const particleCreatePoly = ll.import("HologramLib", "particleCreatePoly");
    // 一个四面体：4 个顶点、6 条边
    const pid = particleCreatePoly(
      "demo", 0,
      "100,64,100;102,64,100;101,64,102;101,66,101",
      "0-1;1-2;2-0;0-3;1-3;2-3",
      1, "minecraft:endrod", 4, 0
    );
    ```

## 粒子形状 · 移动与变换

所有变换都绕形状的**锚点**进行（线=中点、矩形/面/长方体=中心、多面体=质心）。

### 瞬移粒子形状

`particleSetPos(id, x, y, z)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `x` : `Float`<br>形状锚点的新世界坐标 X
  - `y` : `Float`<br>形状锚点的新世界坐标 Y
  - `z` : `Float`<br>形状锚点的新世界坐标 Z
- 返回值：`true` 成功；id 不存在 `false`。下一次发射时整批粒子出现在新位置
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSetPos = ll.import("HologramLib", "particleSetPos");
    particleSetPos(pid, 200.5, 70, 100.5);
    ```

### 相对移动形状

`particleMoveBy(id, dx, dy, dz)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `dx` : `Float`<br>X 方向的位移量（格）
  - `dy` : `Float`<br>Y 方向的位移量（格）
  - `dz` : `Float`<br>Z 方向的位移量（格）
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleMoveBy = ll.import("HologramLib", "particleMoveBy");
    particleMoveBy(pid, 0, 0.5, 0);   // 抬高半格
    ```

### 让形状缓动滑到目标点

`particleMoveTo(id, x, y, z, durationTicks)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `x` : `Float`<br>目标锚点世界坐标 X
  - `y` : `Float`<br>目标锚点世界坐标 Y
  - `z` : `Float`<br>目标锚点世界坐标 Z
  - `durationTicks` : `Integer`<br>滑行时长（tick）；`<= 0` = 立刻到达（等价 `particleSetPos`）
- 返回值：`true` 表示已经开始移动；id 不存在 `false`。整面、整体一起移动
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleMoveTo = ll.import("HologramLib", "particleMoveTo");
    particleMoveTo(pid, 150, 64, -200, 100);   // 5 秒滑到目标点
    ```

### 设置形状三轴欧拉角

`particleSetRot(id, rx, ry, rz)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `rx` : `Float`<br>X 轴的旋转角（度），绕锚点旋转
  - `ry` : `Float`<br>Y 轴的旋转角（度），绕锚点旋转
  - `rz` : `Float`<br>Z 轴的旋转角（度），绕锚点旋转
- 返回值：`true` 成功；id 不存在 `false`。设置的是绝对角度，会覆盖 `particleSpin` 已经累计出来的角度
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSetRot = ll.import("HologramLib", "particleSetRot");
    particleSetRot(pid, 0, 45, 0);   // 绕 Y 轴转 45 度
    ```

### 让形状持续自转

`particleSpin(id, sx, sy, sz)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `sx` : `Float`<br>X 轴的自旋速率（度/tick）；`0,0,0` = 停止自转
  - `sy` : `Float`<br>Y 轴的自旋速率（度/tick）；`0,0,0` = 停止自转
  - `sz` : `Float`<br>Z 轴的自旋速率（度/tick）；`0,0,0` = 停止自转
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSpin = ll.import("HologramLib", "particleSpin");
    particleSpin(pid, 0, 3, 0);    // 每 tick 绕 Y 转 3 度
    particleSpin(pid, 0, 0, 0);    // 停
    ```

### 以锚点为中心缩放形状

`particleSetScale(id, s)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `s` : `Float`<br>缩放倍数；小于等于 `0.01` 的值按 `0.01` 处理
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSetScale = ll.import("HologramLib", "particleSetScale");
    particleSetScale(pid, 2);
    particleSetScale(pid, 1);   // 还原
    ```

### 让形状跟随玩家

`particleFollow(id, uuid, offX, offY, offZ)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `uuid` : `String`<br>要跟随的玩家 **uuid 字符串**（不是玩家名）；传进来的字符串解析不出 uuid 会被直接拒绝
  - `offX` : `Float`<br>相对玩家的 X 方向偏移量（格），例如脚下光圈用 `0, 0.3, 0`
  - `offY` : `Float`<br>相对玩家的 Y 方向偏移量（格），例如脚下光圈用 `0, 0.3, 0`
  - `offZ` : `Float`<br>相对玩家的 Z 方向偏移量（格），例如脚下光圈用 `0, 0.3, 0`
- 返回值：`true` 成功；`uuid` 不是合法 uuid、或 id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleFollow = ll.import("HologramLib", "particleFollow");
    particleFollow(pid, player.uuid, 0, 0.3, 0);   // 形状锚点 = 玩家位置 + 偏移
    ```

### 解除跟随

`particleUnfollow(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：`true` 成功；id 不存在 `false`。解除后锚点回到形状自己记录的坐标（跟随时并没有改写它），所以形状会跳回创建/上次 `particleSetPos` 的位置
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleUnfollow = ll.import("HologramLib", "particleUnfollow");
    particleUnfollow(pid);
    particleSetPos(pid, 100.5, 64.5, 100.5);   // 想留在原地就自己再设一次位置
    ```

## 粒子形状 · 外观与可见性

### 更换粒子效果

`particleSetEffect(id, effect)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `effect` : `String`<br>新的粒子效果名，如 `minecraft:flame`；空串被拒绝
- 返回值：`true` 成功；`effect` 为空或 id 不存在 `false`。从下一次发射开始生效
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSetEffect = ll.import("HologramLib", "particleSetEffect");
    particleSetEffect(pid, "minecraft:flame");
    ```

### 限定粒子可见玩家

`particleSetVisible(id, playersCsv)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `playersCsv` : `String`<br>逗号分隔的玩家 uuid 串，如 `"uuid1,uuid2"`；空串 = 形状所在维度里所有人都能看到。解析不出 uuid 的项会被丢掉
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSetVisible = ll.import("HologramLib", "particleSetVisible");
    particleSetVisible(pid, player.uuid);        // 只有这个玩家看得到
    particleSetVisible(pid, `${player.uuid},${other.uuid}`);   // 两个玩家
    particleSetVisible(pid, "");                 // 恢复维度全员可见
    ```

### 改粒子重发间隔

`particleSetInterval(id, ticks)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `ticks` : `Integer`<br>新的重发间隔（tick）；小于 `1` 会按 `1` 处理
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSetInterval = ll.import("HologramLib", "particleSetInterval");
    particleSetInterval(pid, 2);    // 更连贯，但发包更密
    particleSetInterval(pid, 10);   // 省一点带宽
    ```

### 设置粒子逐玩家裁剪半径

`particleSetViewDistance(id, blocks)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `blocks` : `Integer`<br>裁剪半径（格），玩家离得远就不发；`0` = 不裁剪（负值按 `0` 处理）
- 返回值：`true` 成功；id 不存在 `false`。创建时的默认值：点形状是 `0`（不裁剪），其余形状是 `96` 格
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSetViewDistance = ll.import("HologramLib", "particleSetViewDistance");
    particleSetViewDistance(pid, 48);   // 48 格外的玩家收不到
    particleSetViewDistance(pid, 0);    // 不限
    ```

### 重设粒子存活时长

`particleSetLifetime(id, ticks)`

- 参数：
  - `id` : `Integer`<br>形状 id
  - `ticks` : `Integer`<br>从现在起还能存活多少 tick；`0` = 改成永久
- 返回值：`true` 成功；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleSetLifetime = ll.import("HologramLib", "particleSetLifetime");
    particleSetLifetime(pid, 200);   // 再存在 10 秒
    particleSetLifetime(pid, 0);     // 改成永久
    ```

## 粒子形状 · 查询与销毁

### 立刻销毁粒子形状并停发

`particleDestroy(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：确实销毁了 `true`；id 不存在 `false`
- 返回值类型：`Boolean`
- 示例：

    ```js
    const particleDestroy = ll.import("HologramLib", "particleDestroy");
    particleDestroy(pid);
    ```

### 读取粒子形状摘要

`particleGetInfo(id)`

- 参数：
  - `id` : `Integer`<br>形状 id
- 返回值：摘要文字（诊断探针；形状类型、`owner`、维度、锚点、旋转、自旋、缩放、粒子点数、`step`、效果名、可见范围、视距、重发间隔、结束时间、是否正在平滑移动）；id 不存在返回**空字符串** `""`
- 返回值类型：`String`
- 示例：

    ```js
    const particleGetInfo = ll.import("HologramLib", "particleGetInfo");
    logger.info(particleGetInfo(pid));   // 例：particle shape #1: kind=box owner=demo dim=0 anchor=(100.50,64.50,100.50) ...
    ```
