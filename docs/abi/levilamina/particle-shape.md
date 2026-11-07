# 1.8 IParticleShape · 粒子形状

用原版粒子拼出点、线、矩形环、填充面、长方体线框 / 六面和多面体，并支持平移、缓动移动、旋转、自旋、缩放与跟随玩家。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().particleShapes()`
- 坐标：世界坐标，单位方块
- 锚点：点 = 坐标本身；线 = 中点；矩形 / 面 / 长方体 = 中心；多面体 = 顶点质心。平移、旋转、缩放都绕锚点
- 采样：构造时把形状采样成局部点，`step` 越小越密；**单个形状最多 `2048` 个采样点**
- 发送：逐玩家逐点发原版 `SpawnParticleEffectPacket`，由 BDS 的 tick flush 聚合成单个 Batch 数据报
- 维度：`0` 主世界 / `1` 下界 / `2` 末地

!!! info "1.14.0 起的通用粒子域"

    和 LSE 的 `particle*` 共用同一份管理器与同一套 id，两边创建的形状可以互相操作。
    周期重发、视距裁剪、自旋与缓动都由库内 tick 驱动，消费方只管创建和控制。

## 创建形状

### 摆一个粒子点

`int64_t createPoint(std::string const& owner, int dimId, float x, float y, float z, std::string const& effect, int intervalTicks, int lifetimeTicks)`

- 参数：
  - `owner` : `std::string const&`<br>创建者名字标签，**只用于 `getDebugInfo` 的诊断显示**，不影响谁能看到；可传空串
  - `dimId` : `int`<br>维度 id，`0` 主世界 / `1` 下界 / `2` 末地
  - `x` / `y` / `z` : `float`<br>粒子点的世界坐标
  - `effect` : `std::string const&`<br>粒子效果名，如 `minecraft:endrod`、`minecraft:heart_particle`；空串按 `minecraft:endrod` 处理
  - `intervalTicks` : `int`<br>整批重发间隔（tick）；小于 `1` 按 `1` 处理
  - `lifetimeTicks` : `int`<br>存活时长（tick），从创建时刻算起；`0` = 永久
- 返回值：形状 id（从 `1` 开始）；本方法没有失败路径
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& ps = hologramlib::IHologramLib::getInstance().particleShapes();

    int64_t id = ps.createPoint("MeowLand", 0, 100.5f, 66.0f, 100.5f,
                                "minecraft:endrod", 4, 0);
    ```

### 摆一条粒子线

`int64_t createLine(std::string const& owner, int dimId, float x1, float y1, float z1, float x2, float y2, float z2, float step, std::string const& effect, int intervalTicks, int lifetimeTicks)`

- 参数：
  - `owner` : `std::string const&`<br>创建者名字标签，仅诊断显示
  - `dimId` : `int`<br>维度 id
  - `x1` / `y1` / `z1` : `float`<br>第一个端点的世界坐标
  - `x2` / `y2` / `z2` : `float`<br>第二个端点的世界坐标
  - `step` : `float`<br>采样步长（方块），相邻粒子大约隔多远；小于 `0.25` 按 `0.25` 处理
  - `effect` : `std::string const&`<br>粒子效果名；空串按 `minecraft:endrod` 处理
  - `intervalTicks` : `int`<br>整批重发间隔（tick）；小于 `1` 按 `1` 处理
  - `lifetimeTicks` : `int`<br>存活时长（tick）；`0` = 永久
- 返回值：形状 id；锚点是线的中点（旋转 / 缩放绕中点进行）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    int64_t beam = ps.createLine("MeowLand", 0, 100.0f, 64.0f, 100.0f,
                                 110.0f, 64.0f, 100.0f,
                                 0.5f, "minecraft:flame", 4, 0);
    ```

### 摆一圈矩形环线

`int64_t createRect(std::string const& owner, int dimId, float cx, float cy, float cz, float w, float h, int axis, float step, std::string const& effect, int intervalTicks, int lifetimeTicks)`

- 参数：
  - `owner` : `std::string const&`<br>创建者名字标签，仅诊断显示
  - `dimId` : `int`<br>维度 id
  - `cx` / `cy` / `cz` : `float`<br>矩形中心的世界坐标
  - `w` : `float`<br>矩形沿平面第一个轴的尺寸（宽）；负数按 `0` 处理
  - `h` : `float`<br>矩形沿平面第二个轴的尺寸（高）；负数按 `0` 处理
  - `axis` : `int`<br>矩形所在平面：`0` = XY（`w` 沿 X、`h` 沿 Y）、`1` = YZ（`w` 沿 Z、`h` 沿 Y）、`2` = XZ（`w` 沿 X、`h` 沿 Z）；其它值按 `0` 处理
  - `step` : `float`<br>采样步长（方块）；小于 `0.25` 按 `0.25` 处理
  - `effect` : `std::string const&`<br>粒子效果名；空串按 `minecraft:endrod` 处理
  - `intervalTicks` : `int`<br>整批重发间隔（tick）；小于 `1` 按 `1` 处理
  - `lifetimeTicks` : `int`<br>存活时长（tick）；`0` = 永久
- 返回值：形状 id；只画 4 条边，不填充内部
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    // 躺在 XZ 平面上的 8×8 光环，粒子间距约 1.5 格
    int64_t ring = ps.createRect("MeowLand", 0, 100.5f, 64.1f, 100.5f,
                                 8.0f, 8.0f, 2, 1.5f, "minecraft:endrod", 2, 0);
    ```

### 铺一个填充面

`int64_t createPlane(std::string const& owner, int dimId, float cx, float cy, float cz, float w, float h, int axis, float step, std::string const& effect, int intervalTicks, int lifetimeTicks)`

- 参数：与 `createRect` 完全相同，区别是会把平面**按网格填满**（`cx` / `cy` / `cz` 是面中心）
- 返回值：形状 id；粒子数按面积增长，锚点是面中心
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    // 立在 XY 平面上的一堵 12×6 粒子墙，每 10 tick 重发
    int64_t wall = ps.createPlane("MeowLand", 0, 100.0f, 64.0f, -200.0f,
                                  12.0f, 6.0f, 0, 3.0f, "minecraft:heart_particle", 10, 0);
    ```

!!! note "面形状要慎调小 `step`"

    每格间距都会多出一行/一列采样点，面积越大、`step` 越小，点数增长越快。单形状采样点硬上限是 `2048`，
    超出的部分直接被截断（形状缺一块），所以面 / 六面建议 `step >= 1`。

### 摆一个长方体线框

`int64_t createBox(std::string const& owner, int dimId, float cx, float cy, float cz, float hx, float hy, float hz, float step, std::string const& effect, int intervalTicks, int lifetimeTicks)`

- 参数：
  - `owner` : `std::string const&`<br>创建者名字标签，仅诊断显示
  - `dimId` : `int`<br>维度 id
  - `cx` / `cy` / `cz` : `float`<br>长方体中心的世界坐标
  - `hx` / `hy` / `hz` : `float`<br>X / Y / Z 方向的**半尺寸**（边长的一半，方块）；负数按 `0` 处理
  - `step` : `float`<br>采样步长（方块）；小于 `0.25` 按 `0.25` 处理
  - `effect` : `std::string const&`<br>粒子效果名；空串按 `minecraft:endrod` 处理
  - `intervalTicks` : `int`<br>整批重发间隔（tick）；小于 `1` 按 `1` 处理
  - `lifetimeTicks` : `int`<br>存活时长（tick）；`0` = 永久
- 返回值：形状 id；只画 12 条棱，锚点是长方体中心
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    // 边长 6 的立方体线框（半尺寸 3）
    int64_t box = ps.createBox("MeowLand", 0, 100.5f, 64.5f, 100.5f,
                               3.0f, 3.0f, 3.0f, 1.0f, "minecraft:endrod", 2, 0);
    ```

### 铺满长方体的六个面

`int64_t createBoxFaces(std::string const& owner, int dimId, float cx, float cy, float cz, float hx, float hy, float hz, float step, std::string const& effect, int intervalTicks, int lifetimeTicks)`

- 参数：与 `createBox` 完全相同（`hx` / `hy` / `hz` 仍是半尺寸）
- 返回值：形状 id；六个面都填满，锚点是长方体中心
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    // 半尺寸 2 的实心盒（六个面），step 0.5 → 每个面 5×5 个点
    int64_t shell = ps.createBoxFaces("MeowLand", 0, 100.5f, 64.5f, 100.5f,
                                      2.0f, 2.0f, 2.0f, 0.5f, "minecraft:endrod", 4, 0);
    ```

!!! note "`createBoxFaces` 的 `step` 是「面内密度」不是格子数"

    其余形状的 `step` 以方块为单位；六面填充分两步：先在半尺寸的归一化坐标里从 `-1` 到 `1` 按 `step` 递增，
    再乘以对应的半尺寸。所以 `step = 1` 一个面只有 3×3 个点，`step = 0.25` 是 9×9。
    想要「每 1 格一个点」，请按 `1 / max(hx, hy, hz)` 折算。

### 用顶点和边拼多面体

`int64_t createPoly(std::string const& owner, int dimId, std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step, std::string const& effect, int intervalTicks, int lifetimeTicks)`

- 参数：
  - `owner` : `std::string const&`<br>创建者名字标签，仅诊断显示
  - `dimId` : `int`<br>维度 id
  - `verts` : `std::vector<float> const&`<br>顶点表，按 `(x, y, z)` 三个一组平铺，**世界坐标**；至少 2 个顶点、长度必须是 3 的倍数
  - `edges` : `std::vector<std::int32_t> const&`<br>边表，按 `(i, j)` 两个一组平铺，是 `verts` 的下标；长度必须是偶数、每个下标都要落在顶点范围内
  - `step` : `float`<br>每条边的采样步长（方块）；小于 `0.25` 按 `0.25` 处理
  - `effect` : `std::string const&`<br>粒子效果名；空串按 `minecraft:endrod` 处理
  - `intervalTicks` : `int`<br>整批重发间隔（tick）；小于 `1` 按 `1` 处理
  - `lifetimeTicks` : `int`<br>存活时长（tick）；`0` = 永久
- 返回值：形状 id；**顶点或边不合法时返回 `-1`**（顶点少于 2 个、`edges` 为空或长度为奇数、下标越界）。锚点是顶点质心
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    // 一个三角形线框：3 个顶点、3 条边
    std::vector<float> verts = {
        100.0f, 64.0f, 100.0f,
        104.0f, 64.0f, 100.0f,
        100.0f, 64.0f, 104.0f,
    };
    std::vector<std::int32_t> edges = {0, 1, 1, 2, 2, 0};

    int64_t tri = ps.createPoly("MeowLand", 0, verts, edges, 0.5f,
                                "minecraft:endrod", 4, 0);
    // tri < 0 表示顶点/边不合法
    ```

## 位置与移动

### 直接设置锚点

`bool setPos(int64_t id, float x, float y, float z)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `x` / `y` / `z` : `float`<br>新的锚点世界坐标
- 返回值：成功 `true`；id 不存在 `false`。会同时解除跟随、取消正在进行的缓动
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.setPos(id, 200.5f, 70.0f, 100.5f);
    ```

### 相对平移

`bool moveBy(int64_t id, float dx, float dy, float dz)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `dx` / `dy` / `dz` : `float`<br>相对当前锚点的位移量（方块）；可以传负数
- 返回值：成功 `true`；id 不存在 `false`。不改写跟随状态 —— 跟随中发射锚点仍由玩家位置决定，这次位移看不到效果
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.moveBy(id, 0.0f, 0.5f, 0.0f); // 抬高半格
    ```

### 平滑移动到目标点

`bool moveTo(int64_t id, float x, float y, float z, int durationTicks)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `x` / `y` / `z` : `float`<br>目标锚点世界坐标
  - `durationTicks` : `int`<br>滑行时长（tick）；`<= 0` = 立刻到达（等价 `setPos`）
- 返回值：成功 `true`；id 不存在 `false`。缓动是 easeOutCubic（先快后慢），整块形状随锚点移动
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.moveTo(id, 150.0f, 64.0f, -200.0f, 100); // 约 5 秒滑到目标点
    ```

### 跟随玩家

`bool follow(int64_t id, std::string const& playerUuid, float offX, float offY, float offZ)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `playerUuid` : `std::string const&`<br>要跟随的玩家 **UUID 字符串**（不是玩家名）；解析不出 UUID 直接拒绝
  - `offX` / `offY` / `offZ` : `float`<br>相对玩家位置的偏移（方块）；脚下光圈常用 `0, 0.3, 0`
- 返回值：成功 `true`；UUID 不合法或 id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.follow(id, player.getUuid().asString(), 0.0f, 0.3f, 0.0f);
    ```

!!! note "跟随会跨维度"

    每 tick 发射前用「玩家当前位置 + 偏移」当锚点。玩家换维度时，形状会跟着把 `dimId` 改成玩家的新维度，
    并重新收集该维度里的观看者 —— 不需要消费方自己迁维度。
    玩家离线时这一轮直接跳过（不发射，形状记录还在），玩家回来后继续跟。

### 解除跟随

`bool unfollow(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：成功 `true`；id 不存在 `false`。解除后锚点回到形状自己记录的坐标（跟随时没有改写它），形状会跳回创建 / 上次 `setPos` 的位置
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.unfollow(id);
    ps.setPos(id, 100.5f, 64.5f, 100.5f); // 想留在原地就自己再设一次
    ```

## 变换

### 设置三轴旋转

`bool setRot(int64_t id, float rx, float ry, float rz)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `rx` / `ry` / `rz` : `float`<br>三轴欧拉角（度），绕锚点旋转，ZYX 顺序
- 返回值：成功 `true`；id 不存在 `false`。设置的是**绝对角度**，会覆盖 `spin` 累计出来的角度
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.setRot(id, 0.0f, 45.0f, 0.0f); // 绕 Y 轴转 45 度
    ```

### 让形状持续自转

`bool spin(int64_t id, float sx, float sy, float sz)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `sx` / `sy` / `sz` : `float`<br>三轴自旋速率（度/tick）；传 `0, 0, 0` = 停止自转
- 返回值：成功 `true`；id 不存在 `false`。自旋在库内 tick 里累加到旋转角上
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.spin(id, 0.0f, 3.0f, 0.0f); // 每 tick 绕 Y 转 3 度
    ps.spin(id, 0.0f, 0.0f, 0.0f); // 停
    ```

### 以锚点为中心缩放

`bool setScale(int64_t id, float scale)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `scale` : `float`<br>缩放倍数；`<= 0.01` 按 `0.01` 处理，没有上限
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.setScale(id, 2.0f);
    ps.setScale(id, 1.0f); // 还原
    ```

## 外观与发送

### 更换粒子效果

`bool setEffect(int64_t id, std::string const& effect)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `effect` : `std::string const&`<br>新的粒子效果名，如 `minecraft:flame`；**空串会被拒绝**
- 返回值：成功 `true`；`effect` 为空或 id 不存在 `false`。从下一次发射开始生效
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.setEffect(id, "minecraft:flame");
    ```

### 限定可见玩家

`bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerUuids)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `playerUuids` : `std::vector<std::string> const&`<br>玩家 **UUID** 列表；解析不出 UUID 的项会被丢掉；**空数组 = 恢复该维度全员可见**
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.setVisiblePlayers(id, {playerUuid});            // 只有这个玩家看得到
    ps.setVisiblePlayers(id, {uuid1, uuid2});          // 两个玩家
    ps.setVisiblePlayers(id, {});                      // 维度全员
    ```

### 清除可见玩家白名单

`bool clearVisiblePlayers(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：成功 `true`；id 不存在 `false`。等价于传空数组给 `setVisiblePlayers`
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.clearVisiblePlayers(id);
    ```

### 设置重发间隔

`bool setInterval(int64_t id, int ticks)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `ticks` : `int`<br>整批重发间隔（tick）；小于 `1` 按 `1` 处理
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.setInterval(id, 10); // 每 0.5 秒重发一次，降低发包量
    ```

!!! note "为什么必须周期重发"

    粒子本身是瞬态的（播完就没了），常驻观感靠按 `intervalTicks` **整批重发**维持。
    `intervalTicks = 1` 表示每 tick 都发一遍，最流畅也最费带宽；静态装饰建议 `4 ~ 10`。

### 设置逐玩家视距裁剪

`bool setViewDistance(int64_t id, int blocks)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `blocks` : `int`<br>裁剪半径（方块）；`0` = 不裁剪。按**单个采样点**到玩家的 3D 距离判断
- 返回值：成功 `true`；id 不存在 `false`。负数按 `0` 处理
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.setViewDistance(id, 32); // 只给 32 格内的玩家发这个形状的点
    ```

### 设置存活时长

`bool setLifetime(int64_t id, int ticks)`

- 参数：
  - `id` : `int64_t`<br>形状 id
  - `ticks` : `int`<br>从现在起还能活多少 tick；`<= 0` = 改成永久
- 返回值：成功 `true`；id 不存在 `false`。到期后形状自动销毁（id 失效）
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.setLifetime(id, 200); // 10 秒后自动消失
    ps.setLifetime(id, 0);   // 改成永久
    ```

## 查询与销毁

### 查询形状是否存在

`bool exists(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：存在 `true`；不存在或已到期 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (ps.exists(id)) {
        ps.setScale(id, 1.5f);
    }
    ```

### 列出全部形状 id

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：当前所有活着的形状 id；没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    for (int64_t id : ps.getAllIds()) {
        ps.destroy(id);
    }
    ```

### 取运行态摘要

`std::string getDebugInfo(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：一行诊断文字（类型、`owner`、维度、锚点、旋转、自旋、缩放、点数、`step`、效果、可见范围、视距、间隔、到期 tick、是否在缓动）；**id 不存在返回空串**
- 返回值类型：`std::string`
- 示例：

    ```cpp
    logger.info("{}", ps.getDebugInfo(id));
    // particle shape #3: kind=plane owner=MeowLand dim=0 anchor=(100.50,64.00,-200.00) ...
    ```

### 销毁一个形状

`bool destroy(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>形状 id
- 返回值：销毁成功 `true`；id 不存在 `false`。已经发出去的粒子会自然消散，不会补「清除」包
- 返回值类型：`bool`
- 示例：

    ```cpp
    ps.destroy(id);
    ```

### 销毁全部形状

`void destroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    ps.destroyAll();
    ```

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

void spawn_player_aura(std::string const& playerUuid) {
    auto& ps = hologramlib::IHologramLib::getInstance().particleShapes();

    // 脚下 6×6 的光环：XZ 平面、每 3 tick 重发、永久
    int64_t ring = ps.createRect("aura", 0, 0.0f, 0.0f, 0.0f,
                                 6.0f, 6.0f, 2, 1.0f, "minecraft:endrod", 3, 0);
    if (ring <= 0) {
        return;
    }

    // 锚点先随便给，follow 之后每 tick 用玩家位置覆盖
    ps.setPos(ring, 0.0f, 0.0f, 0.0f);
    ps.follow(ring, playerUuid, 0.0f, 0.1f, 0.0f);
    ps.setRot(ring, 0.0f, 0.0f, 0.0f);
    ps.spin(ring, 0.0f, 2.0f, 0.0f);          // 缓慢自转
    ps.setVisiblePlayers(ring, {playerUuid}); // 只有本人看得到
    ps.setViewDistance(ring, 0);              // 跟随目标不需要裁剪
    ps.setLifetime(ring, 20 * 60);            // 1 分钟后自动消失
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
