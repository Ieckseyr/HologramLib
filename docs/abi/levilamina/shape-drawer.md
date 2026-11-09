# 1.2 IShapeDrawer · 形状渲染

协议层的调试形状：不生成实体、不写存档，直接给客户端发 `PrimitiveShapes`（DebugShape）包，
画出线框、圆、球、箭头和文本。适合做区域标记、寻路指引、显示边界和调试可视化。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().shapes()`
- 坐标：世界坐标，单位方块
- 颜色：`r` / `g` / `b` / `a` 四个 `float`，范围 `0.0 ~ 1.0`
- 维度：`0` 主世界 / `1` 下界 / `2` 末地
- 形状 id：`int64_t`，从 `1` 开始递增，只属于形状域
- 形状类型：`ShapeType` 枚举（`0` 文本、`1` 线、`2` 盒、`3` 圆、`4` 球、`5` 箭头）

下面示例里的 `id` 都指本页 `create*` 返回的形状 id。

!!! tip "属性改完要 update（或重新 draw）"

    形状的 `set*` 只改服务端保存的值，**不发包**。已经 `draw` 过的形状改完要调一次
    `update(id)` 原地重发；还没显示过的形状直接 `draw` 即可。悬浮字域是自动 `refresh`，
    形状域不是 —— 这是最常见的「改了没反应」原因。

!!! info "创建不会失败，也没有尺寸校验"

    六个 `create*` 不做参数校验、也没有资源上限，返回的 id 一定 `>= 1`。
    画不出来通常是后面漏了 `draw`，或者坐标 / 维度写错。

## 创建形状

### 画一段文本

`int64_t createText(float x, float y, float z, std::string const& text)`

- 参数：
  - `x` / `y` / `z` : `float`<br>文本锚点的世界坐标，单位方块；`.5` 通常落在方块中心
  - `text` : `std::string const&`<br>要显示的文本，支持 `§` 颜色代码与 `\n` 换行
- 返回值：形状 id（`>= 1`）；创建不做校验，不会失败
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createText(100.5f, 66.0f, 100.5f, "§e主城 · 告示牌");
    shapes.draw(id);
    ```

!!! note "文本形状是唯一带「底板」的形状"

    它额外支持背景框颜色（`setBackgroundColor` / `clearBackgroundColor`）和穿墙开关
    （`setDepthTest`），这三个方法对线、盒、圆、球、箭头返回 `false`。

### 画一条线段

`int64_t createLine(float x1, float y1, float z1, float x2, float y2, float z2)`

- 参数：
  - `x1` / `y1` / `z1` : `float`<br>起点世界坐标，也是这个形状的锚点
  - `x2` / `y2` / `z2` : `float`<br>终点世界坐标（绝对坐标，不是偏移）
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createLine(100.0f, 65.0f, 100.0f, 110.0f, 65.0f, 110.0f);
    shapes.draw(id);
    ```

### 画一个长方体线框

`int64_t createBox(float x1, float y1, float z1, float x2, float y2, float z2)`

- 参数：
  - `x1` / `y1` / `z1` : `float`<br>第一个角的世界坐标
  - `x2` / `y2` / `z2` : `float`<br>对角的世界坐标；两个点的先后顺序无所谓，边长取坐标差的绝对值
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createBox(100.0f, 64.0f, 100.0f, 110.0f, 70.0f, 110.0f); // 10×6×10 的框
    shapes.draw(id);
    ```

### 画一个圆

`int64_t createCircle(float x, float y, float z, float scale)`

- 参数：
  - `x` / `y` / `z` : `float`<br>圆心的世界坐标
  - `scale` : `float`<br>圆的大小；等价于创建时就设好 `setScale`，之后还能再改
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createCircle(100.5f, 65.0f, 100.5f, 3.0f);
    shapes.draw(id);
    ```

### 画一个球

`int64_t createSphere(float x, float y, float z, float scale)`

- 参数：
  - `x` / `y` / `z` : `float`<br>球心的世界坐标
  - `scale` : `float`<br>球的大小；等价于创建时就设好 `setScale`
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createSphere(100.5f, 66.0f, 100.5f, 2.0f);
    shapes.draw(id);
    ```

### 画一个箭头

`int64_t createArrow(float x1, float y1, float z1, float x2, float y2, float z2)`

- 参数：
  - `x1` / `y1` / `z1` : `float`<br>箭尾的世界坐标，也是这个形状的锚点
  - `x2` / `y2` / `z2` : `float`<br>箭头指向的世界坐标（绝对坐标）
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createArrow(100.0f, 66.0f, 100.0f, 110.0f, 66.0f, 100.0f); // 沿 +X 指
    shapes.draw(id);
    ```

## 通用属性

### 设置颜色

`bool setColor(int64_t id, float r, float g, float b, float a)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `r` / `g` / `b` : `float`<br>红 / 绿 / 蓝分量，`0.0 ~ 1.0`
  - `a` : `float`<br>透明度，`0.0 ~ 1.0`；`0.0` 完全透明（等于看不见），`1.0` 不透明
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.setColor(id, 1.0f, 0.2f, 0.2f, 1.0f); // 红色
    shapes.update(id);
    ```

### 设置缩放

`bool setScale(int64_t id, float scale)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `scale` : `float`<br>缩放倍数；`1.0` 是创建时的大小，`0.5` 缩一半，`2.0` 放大一倍
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.setScale(id, 1.5f);
    shapes.update(id);
    ```

### 设置自动消失时间

`bool setDuration(int64_t id, float seconds)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `seconds` : `float`<br>存留时长，单位**秒**；**不调用这个方法 = 永久显示**
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createText(100.5f, 66.0f, 100.5f, "§c10 秒后消失");
    shapes.setDuration(id, 10.0f);
    shapes.draw(id);
    ```

!!! note "duration 是客户端的倒计时"

    时长写在包里，由客户端自己数：不设 `duration` 的形状不会自动消失，
    设了就是一次性倒计时，时间到客户端自己丢掉它（服务端记录还在，可以再 `draw`）。

### 设置所属维度

`bool setDimension(int64_t id, int dimId)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `dimId` : `int`<br>维度 id：`0` 主世界 / `1` 下界 / `2` 末地
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.setDimension(id, 0);
    shapes.drawToDimension(id, 0);
    ```

### 移动位置

`bool setLocation(int64_t id, float x, float y, float z)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `x` / `y` / `z` : `float`<br>新的锚点世界坐标
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.setLocation(id, 200.5f, 70.0f, 200.5f);
    shapes.update(id);
    ```

!!! warning "锚点不等于「整体平移」"

    不同形状的锚点含义不同，`setLocation` 只改锚点：

    - 文本 / 圆 / 球：就是它们的位置，整体跟着走；
    - 盒子：锚点是**中心**，整体跟着走；
    - 线 / 箭头：锚点是**起点**，终点的绝对坐标不变 —— 只改起点会让线被拉长或改变方向。
      终点没有读取接口，要整体平移请在消费方自己保存两个端点，重新 `createLine` / `createArrow`。

### 修改文本内容

`bool setText(int64_t id, std::string const& text)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `text` : `std::string const&`<br>新的文本，支持 `§` 颜色代码与 `\n` 换行
- 返回值：写入成功 `true`；id 不存在 `false`。**非文本形状也返回 `true`，但内容不会生效**
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.setText(id, "§a改过的文字");
    shapes.update(id); // 已显示时要重发一次
    ```

### 设置三轴旋转

`bool setRotation(int64_t id, float pitch, float yaw, float roll)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `pitch` : `float`<br>俯仰角，单位**度**
  - `yaw` : `float`<br>偏航角，单位**度**
  - `roll` : `float`<br>翻滚角，单位**度**
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.setRotation(id, 90.0f, 0.0f, 0.0f); // 平躺的文字
    shapes.update(id);
    ```

!!! note "文本形状会自动打开双面渲染"

    对文本形状调 `setRotation` 时，库同时打开 `useRotation` 与正反面渲染，
    否则旋转后从背面看会消失。设过旋转的文本不再面向相机；要恢复面向相机请用 `clearRotation`。

### 恢复面向相机

`bool clearRotation(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.clearRotation(id);
    shapes.update(id);
    ```

## 文本形状专属

背景框与穿墙这两个能力是 1.26.0 补进文本形状的；三个方法对非文本形状都返回 `false`。

### 设置背景框颜色

`bool setBackgroundColor(int64_t id, float r, float g, float b, float a)`

- 参数：
  - `id` : `int64_t`<br>形状 id，必须是文本形状
  - `r` / `g` / `b` : `float`<br>底板颜色分量，`0.0 ~ 1.0`
  - `a` : `float`<br>底板透明度，`0.0 ~ 1.0`
- 返回值：写入成功 `true`；id 不存在**或不是文本形状** `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createText(100.5f, 66.0f, 100.5f, "§f黑底白字");
    shapes.setBackgroundColor(id, 0.0f, 0.0f, 0.0f, 0.6f);
    shapes.draw(id);
    ```

### 恢复默认背景框

`bool clearBackgroundColor(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id，必须是文本形状
- 返回值：写入成功 `true`；id 不存在或不是文本形状 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.clearBackgroundColor(id);
    shapes.update(id);
    ```

不设置背景框 = 用客户端默认色，不是「透明」。

### 设置穿墙可见性

`bool setDepthTest(int64_t id, bool enabled)`

- 参数：
  - `id` : `int64_t`<br>形状 id，必须是文本形状
  - `enabled` : `bool`<br>`true` = 被方块 / 实体遮挡；`false` = 始终渲染（穿墙可见），**也是默认值**
- 返回值：写入成功 `true`；id 不存在或不是文本形状 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.setDepthTest(id, true); // 藏在墙后就看不见
    shapes.update(id);
    ```

!!! note "默认是穿墙"

    新建的文本形状 `depthTest` 是 `false`（穿墙可见），和库外常见的「先遮挡」预期相反。
    要做「走近才显形」的效果，记得显式 `setDepthTest(id, true)`。

## 显示控制

### 对全体玩家显示

`bool draw(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：已发出 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createBox(100.0f, 64.0f, 100.0f, 110.0f, 70.0f, 110.0f);
    shapes.draw(id);
    ```

`draw` 会把形状的显示目标记为「全体」，之前用 `drawToPlayer` / `drawToDimension` 设过的目标被覆盖。

### 只对一名玩家显示

`bool drawToPlayer(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `playerName` : `std::string const&`<br>玩家名（`realName`），精确匹配；不在线时不会有任何显示
- 返回值：已发出 `true`；id 不存在或玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    int64_t id = shapes.createText(100.5f, 66.0f, 100.5f, "§b只有你能看到");
    shapes.drawToPlayer(id, "Steve");
    ```

### 只对某个维度显示

`bool drawToDimension(int64_t id, int dimId)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `dimId` : `int`<br>维度 id：`0` 主世界 / `1` 下界 / `2` 末地
- 返回值：已发出 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.drawToDimension(id, 0); // 只发给主世界玩家，并把形状维度设为 0
    ```

### 隐藏形状

`bool remove(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：已发出移除包 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.remove(id); // 只隐藏，数据和 id 都还在
    shapes.draw(id);   // 用同一个 id 再显示回来
    ```

`remove` 只让客户端不再渲染，服务端数据、id、属性全部保留；`destroy` 才会真正删掉。

### 原地重发

`bool update(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：id 存在就返回 `true`；还没显示过的形状不做任何事（也不报错）；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.setText(id, "§e倒计时: 9");
    shapes.update(id); // 改完不上屏，必须重发
    ```

!!! warning "update 按全体重发"

    `update` 不按之前的显示目标过滤：就算一开始是 `drawToPlayer(id, "Steve")`，
    调 `update(id)` 也会**发给所有在线玩家**。要维持单人显示，不要调用 `update`，
    改为重新 `drawToPlayer(id, "Steve")`（它同样会重发当前数据）。

## 销毁与查询

### 销毁形状

`bool destroy(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：确实销毁了 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.destroy(id);
    ```

### 销毁全部形状

`void destroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    shapes.destroyAll(); // 关服、重载插件时收尾
    ```

### 判断形状是否存在

`bool exists(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：存在 `true`，不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    if (shapes.exists(id)) {
        shapes.remove(id);
    }
    ```

### 查询形状类型

`ShapeType type(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：该 id 的类型；id 不存在时是 `static_cast<ShapeType>(-1)`
- 返回值类型：`ShapeType`
- 示例：

    ```cpp
    auto& shapes = hologramlib::IHologramLib::getInstance().shapes();
    if (static_cast<int>(shapes.type(id)) < 0) {
        return; // id 不存在
    }
    if (shapes.type(id) == hologramlib::ShapeType::Box) {
        // 这是一个盒子
    }
    ```

`ShapeType` 取值：`Text = 0`、`Line = 1`、`Box = 2`、`Circle = 3`、`Sphere = 4`、`Arrow = 5`；
扩展形状追加 `Cylinder = 7`、`Pyramid = 8`、`Ellipsoid = 9`、`Cone = 10` —— `6` 是历史空洞，不复用。

## 扩展形状（IShapeDrawerExt）

1.33.0 把圆柱、金字塔、椭球、圆锥和两个通用属性放进了**单独的接口**
`IShapeDrawerExt`，通过 `lib.shapeExt()` 取，不往 `IShapeDrawer` 中间插方法 ——
中间插会挪动后面所有域在 `IHologramLib` 虚表里的槽位。

!!! warning "库比头文件旧时要用封装"

    ABI 规则是「只追加、要删留空槽、换库重编」。消费方按 1.36.0 头文件编译、却连到
    更旧的库（`version() < 0x012B00`）时，`shapeExt()` 这个槽在旧库里还不存在。
    这种场景用 `AbiCompat.h` 的薄封装（只做版本检查，不猜槽位）降级：

```cpp
#include "hologramlib/AbiCompat.h"

auto* ext = hologramlib::abi::shapeExt(hologramlib::IHologramLib::getInstance());
if (ext != nullptr) {
    int64_t id = ext->createCylinder(100.5f, 64.0f, 100.5f, 2.0f, 2.0f, 5.0f);
    ext->setMaxRenderDistance(id, 64.0f);
}
```

扩展形状的 `setColor` / `setScale` / `setDuration` / `setDimension` / `setLocation` /
`setRotation` / `draw` / `remove` / `update` / `destroy` 等全部复用 `IShapeDrawer` 的方法
（id 是同一个空间），所以下面只列新增的创建与属性。
`setMaxRenderDistance` 和 `setAttachedEntity` 对**任意**形状 id 生效，不只是扩展形状。
虚表与枚举都是尾部追加，`HOLOGLIB_ABI_STAMP` 仍为 `1`，但新增了接口 —— 用旧头文件编译的
消费方需要重新编译。

### 画一个圆柱

`int64_t createCylinder(float x, float y, float z, float radiusX, float radiusZ, float height, int segments = 16)`

- 参数：
  - `x` / `y` / `z` : `float`<br>圆柱底面的中心世界坐标
  - `radiusX` : `float`<br>X 方向的半径，单位方块
  - `radiusZ` : `float`<br>Z 方向的半径，单位方块；不等于 `radiusX` 时就是椭圆柱
  - `height` : `float`<br>圆柱高度，单位方块
  - `segments` : `int`<br>圆周分段数，默认 `16`；超出 `3 ~ 255` 会被钳进这个区间
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& ext = hologramlib::IHologramLib::getInstance().shapeExt();
    int64_t id = ext.createCylinder(100.5f, 64.0f, 100.5f, 2.0f, 2.0f, 5.0f, 24);
    hologramlib::IHologramLib::getInstance().shapes().draw(id);
    ```

### 画一个金字塔

`int64_t createPyramid(float x, float y, float z, float width, float height, float depth)`

- 参数：
  - `x` / `y` / `z` : `float`<br>金字塔底面的中心世界坐标
  - `width` : `float`<br>底面 X 方向边长，单位方块
  - `height` : `float`<br>高度，单位方块
  - `depth` : `float`<br>底面 Z 方向边长，单位方块
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& ext = hologramlib::IHologramLib::getInstance().shapeExt();
    int64_t id = ext.createPyramid(100.5f, 64.0f, 100.5f, 4.0f, 3.0f, 4.0f);
    hologramlib::IHologramLib::getInstance().shapes().draw(id);
    ```

### 画一个椭球

`int64_t createEllipsoid(float x, float y, float z, float radiusX, float radiusY, float radiusZ, int segmentsPerAxis = 12)`

- 参数：
  - `x` / `y` / `z` : `float`<br>椭球中心的世界坐标
  - `radiusX` / `radiusY` / `radiusZ` : `float`<br>三个轴上的半径，单位方块；三者相等就是球
  - `segmentsPerAxis` : `int`<br>每轴分段数，默认 `12`；超出 `3 ~ 255` 会被钳进这个区间
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& ext = hologramlib::IHologramLib::getInstance().shapeExt();
    int64_t id = ext.createEllipsoid(100.5f, 66.0f, 100.5f, 3.0f, 2.0f, 1.0f);
    hologramlib::IHologramLib::getInstance().shapes().draw(id);
    ```

### 画一个圆锥

`int64_t createCone(float x, float y, float z, float radiusX, float radiusZ, float height, int segments = 16)`

- 参数：
  - `x` / `y` / `z` : `float`<br>圆锥底面的中心世界坐标
  - `radiusX` : `float`<br>X 方向的底面半径，单位方块
  - `radiusZ` : `float`<br>Z 方向的底面半径，单位方块
  - `height` : `float`<br>高度，单位方块
  - `segments` : `int`<br>圆周分段数，默认 `16`；超出 `3 ~ 255` 会被钳进这个区间
- 返回值：形状 id（`>= 1`）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& ext = hologramlib::IHologramLib::getInstance().shapeExt();
    int64_t id = ext.createCone(100.5f, 64.0f, 100.5f, 2.0f, 2.0f, 4.0f);
    hologramlib::IHologramLib::getInstance().shapes().draw(id);
    ```

### 设置最远渲染距离

`bool setMaxRenderDistance(int64_t id, float distance)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `distance` : `float`<br>最远渲染距离，单位方块；`<= 0` = 清掉这个限制（不做距离裁剪）
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& ext = hologramlib::IHologramLib::getInstance().shapeExt();
    ext.setMaxRenderDistance(id, 64.0f); // 64 格外客户端不画
    hologramlib::IHologramLib::getInstance().shapes().update(id);
    ```

### 挂到实体上

`bool setAttachedEntity(int64_t id, int64_t entityUniqueId)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `entityUniqueId` : `int64_t`<br>目标实体的 **uniqueId**（不是 runtimeId，也不是库内 id）
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& ext = hologramlib::IHologramLib::getInstance().shapeExt();
    ext.setAttachedEntity(id, 12345);
    hologramlib::IHologramLib::getInstance().shapes().update(id);
    ```

挂上之后客户端让形状跟着实体走，消费方不必每 tick 改坐标；形状的锚点仍然生效（相对实体计算）。

### 解除挂载

`bool clearAttachedEntity(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：写入成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& ext = hologramlib::IHologramLib::getInstance().shapeExt();
    ext.clearAttachedEntity(id);
    hologramlib::IHologramLib::getInstance().shapes().update(id);
    ```

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

// 给一块 10×6×10 的区域描个绿框，上面挂一行字，10 秒后自动消失
void mark_region() {
    auto& lib    = hologramlib::IHologramLib::getInstance();
    auto& shapes = lib.shapes();

    int64_t box = shapes.createBox(100.0f, 64.0f, 100.0f, 110.0f, 70.0f, 110.0f);
    shapes.setColor(box, 0.2f, 1.0f, 0.2f, 0.6f);
    shapes.setDuration(box, 10.0f);
    shapes.draw(box);

    int64_t title = shapes.createText(105.0f, 71.0f, 105.0f, "§a施工区域");
    shapes.setScale(title, 1.5f);
    shapes.setBackgroundColor(title, 0.0f, 0.0f, 0.0f, 0.5f);
    shapes.setDepthTest(title, false); // 穿墙可见
    shapes.setDuration(title, 10.0f);
    shapes.draw(title);

    // 圆柱：走扩展接口（同版本直接取；跨版本请按上一节的 AbiCompat 写法）
    auto& ext = lib.shapeExt();
    int64_t pillar = ext.createCylinder(105.0f, 64.0f, 105.0f, 1.0f, 1.0f, 6.0f, 24);
    shapes.setColor(pillar, 0.2f, 0.8f, 1.0f, 0.4f);
    shapes.draw(pillar);
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
