# 1.5 IItemDisplay · 物品悬浮展示

配置驱动的物品 / 方块悬浮展示（FMBE：隐形狐狸载体 + 发包）：客户端看到一只部分隐身的狐狸
举着指定物品，服务端不生成真实实体。三轴平移、旋转、缩放都支持 **Molang 表达式**，
动画在客户端每帧计算，服务端不用每 tick 推包。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().itemDisplays()`
- 坐标：世界坐标，单位方块
- 维度：`0` 主世界 / `1` 下界 / `2` 末地
- id：`int64_t`，属于物品展示域，不能传给形状 / 悬浮字 / 实体的接口
- 变换字段：全部是 `std::string`，写**常量数字**（`"0.375"`）或 **Molang 表达式**（`"math.sin(query.life_time*90)*360"`）

下面示例里的 `id` 都指本页创建接口返回的物品展示 id。

!!! info "谁来管可见性"

    展示的生成 / 移除由库内 Level tick hook 自动同步（每 20 tick 重算一次）：
    同维度、在 `viewDistance` 内、`enabled` 为开的玩家自动收到，玩家断线自动清理。
    属性变更对已见玩家即时生效（原子 `despawn → respawn`，不会闪半截）。
    库不落盘：服务端重启后要重新创建；玩家重连由库自动补发。

## 配置结构体（ItemDisplayConfig）

创建和 `get` 都用这个结构体，字段含义如下：

| 字段 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `item` | `std::string` | `"minecraft:diamond"` | 显示的物品标识符；写错不会让创建失败，只是客户端什么都看不到 |
| `itemAux` | `int` | `0` | 物品附加值（data 值） |
| `x` / `y` / `z` | `float` | `0` / `64` / `0` | 世界坐标，单位方块 |
| `dimension` | `int` | `0` | 维度 id：`0` 主世界 / `1` 下界 / `2` 末地 |
| `offsetX` / `offsetY` / `offsetZ` | `std::string` | `"0"` / `"-4"` / `"0"` | 模型单位平移（`v.xpos` / `v.ypos` / `v.zpos`） |
| `baseOffsetX` / `baseOffsetY` / `baseOffsetZ` | `std::string` | `"0"` | 渲染像素级基础偏移（`v.xbasepos`） |
| `rotX` / `rotY` / `rotZ` | `std::string` | `"180"` / `"0"` / `"180"` | 三轴旋转，单位**度** |
| `scale` | `std::string` | `"0.375"` | 缩放；方块模式建议 `"0.5"` |
| `extendScale` | `std::string` | `"1"` | 方块模式二段缩放 |
| `extendRotX` / `extendRotY` | `std::string` | `"-90"` / `"0"` | 方块模式二段旋转，单位度 |
| `mode` | `int` | `0` | `0` = 自动判断，`1` = 物品平面渲染，`2` = 方块立体渲染 |
| `viewDistance` | `double` | `64.0` | 可见距离，单位方块；`<= 0` = 不限距离 |
| `enabled` | `bool` | `true` | `false` = 对所有玩家隐藏（id 仍有效，可随时开回来） |
| `itemNbt` | `std::string` | 空 | 物品附加数据（SNBT），例如自定义名称、附魔 |
| `itemGlint` | `bool` | `false` | 附魔光效开关（与 `itemNbt` 独立叠加） |
| `hitboxWidth` | `float` | `0.0` | AABB 判定体积宽；`0` = 无判定体积 |
| `hitboxHeight` | `float` | `0.0` | AABB 判定体积高；`0` = 无判定体积 |

!!! note "默认姿态是给狐狸头补偿过的"

    物品模式下 `offsetY = "-4"`、`rotX = rotZ = "180"`、`scale = "0.375"` 是一组配好的默认姿态；
    库内部还会给 `rotY` 自动加 `205`，补偿狐狸头的朝向 —— 你写的 `rotY` 是「最终朝向」，不用自己减。
    切到方块模式（`mode = 2`）时，默认的 `offsetY = "-4"` 与 `scale = "0.375"` 会被换成
    `0.125` 与 `0.5` 来适配方块载体，所以方块模式建议直接写 `scale = "0.5"`。

## 创建

### 创建一个物品悬浮展示

`int64_t create(ItemDisplayConfig const& config)`

- 参数：
  - `config` : `ItemDisplayConfig const&`<br>完整的展示配置，字段见上表；只填想改的字段即可，其余用默认值
- 返回值：物品展示 id（`>= 1`）；该域被 `HLIB_NO_DISPLAY` 停用时返回 `-1`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();

    hologramlib::ItemDisplayConfig cfg;
    cfg.item = "minecraft:diamond";
    cfg.x    = 100.5f;
    cfg.y    = 66.0f;
    cfg.z    = 100.5f;
    int64_t id = displays.create(cfg);
    if (id < 0) {
        return; // 没建出来
    }
    ```

### 用随机 id 创建

`int64_t createRandom(ItemDisplayConfig const& config)`

- 参数：
  - `config` : `ItemDisplayConfig const&`<br>同 `create`
- 返回值：`0x10000000 ~ 0x7FFFFFFF` 随机段里的新 id（与自增段隔离）；连续 128 次都撞到已占用 id 时返回 `-1`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    int64_t id = displays.createRandom(cfg);
    ```

### 用指定 id 创建（持久化恢复用）

`int64_t createWithId(ItemDisplayConfig const& config, int64_t desiredId)`

- 参数：
  - `config` : `ItemDisplayConfig const&`<br>同 `create`
  - `desiredId` : `int64_t`<br>想占用的 id，必须 `> 0`；一般来自你自己存档里记下的 id
- 返回值：成功就是 `desiredId`；`desiredId <= 0` 或该 id 已被占用返回 `-2`
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();

    constexpr int64_t kSavedId = 1001; // 自己存档里记的展示 id
    if (!displays.isIdUsed(kSavedId)) {
        int64_t id = displays.createWithId(cfg, kSavedId);
        if (id == kSavedId) {
            // 恢复成功
        }
    }
    ```

## 查询

### 判断展示是否存在

`bool exists(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
- 返回值：存在 `true`，不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    if (!displays.exists(id)) {
        return; // 已经被销毁了
    }
    ```

### 读回当前配置

`bool get(int64_t id, ItemDisplayConfig& out) const`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `out` : `ItemDisplayConfig&`<br>出参；成功时被整体覆盖为当前配置
- 返回值：读到了 `true`；id 不存在 `false`（此时 `out` 不被改动）
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();

    hologramlib::ItemDisplayConfig cfg{};
    if (displays.get(id, cfg)) {
        // cfg.item / cfg.scale / cfg.mode ... 都是当前值
    }
    ```

### 判断 id 是否被占用

`bool isIdUsed(int64_t id) const`

- 参数：
  - `id` : `int64_t`<br>要查的物品展示 id
- 返回值：已占用 `true`（此时 `createWithId` 会返回 `-2`），空闲 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    bool free = !displays.isIdUsed(1001);
    ```

### 列出全部 id

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：当前所有物品展示的 id；一个都没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    for (int64_t one : displays.getAllIds()) {
        displays.setGlint(one, true);
    }
    ```

## 物品与渲染模式

### 换掉展示的物品

`bool setItem(int64_t id, std::string const& item, int aux)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `item` : `std::string const&`<br>新的物品标识符
  - `aux` : `int`<br>新的物品附加值（data 值），没有填 `0`
- 返回值：已改 `true`；id 不存在 `false`。换物品会**一并清掉**之前设的附加数据
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setItem(id, "minecraft:emerald", 0);
    ```

### 换物品并带附加数据

`bool setItemWithNbt(int64_t id, std::string const& item, int aux, std::string const& nbt)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `item` : `std::string const&`<br>新的物品标识符
  - `aux` : `int`<br>新的物品附加值
  - `nbt` : `std::string const&`<br>SNBT 文本；空串 = 清除附加数据。解析失败按「没有附加数据」处理并打日志
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setItemWithNbt(id, "minecraft:diamond_sword", 0, "{display:{Name:\"§b试用剑\"}}");
    ```

### 开关附魔光效

`bool setGlint(int64_t id, bool on)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `on` : `bool`<br>`true` = 加光效，`false` = 去掉
- 返回值：已设 `true`；id 不存在 `false`。与当前值相同也返回 `true`，且不会触发重发
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setGlint(id, true);
    ```

光效走 BDS 原生 `saveEnchantsToUserData`，内部注入 1 级锋利（只取光效），与 `itemNbt` 独立叠加。

### 切换渲染模式

`bool setMode(int64_t id, int mode)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `mode` : `int`<br>`0` = 自动判断（默认），`1` = 物品平面渲染，`2` = 方块立体渲染；其它值按 `1` 处理
- 返回值：已设 `true`；id 不存在 `false`。与当前值相同直接返回 `true`，不标脏、不重发
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setMode(id, 2); // 强制按立体方块渲染
    ```

`mode = 0` 时按物品自己判断：能当方块放的（且不是梯子、藤蔓这类平面方块）走 `2`，其余走 `1`。

## 位置与变换

### 移动展示到新坐标

`bool setPosition(int64_t id, float x, float y, float z, int dim)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `x` / `y` / `z` : `float`<br>新的世界坐标，单位方块
  - `dim` : `int`<br>`>= 0` 时把展示放到该维度；`< 0` = 只改坐标、维度不动
- 返回值：已改 `true`；id 不存在 `false`。跟随中的展示被手动设位会解除跟随
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setPosition(id, 105.5f, 66.0f, -200.5f, -1); // 只挪坐标
    displays.setPosition(id, 105.5f, 66.0f, -200.5f, 1);  // 搬到下界
    ```

### 调模型单位平移

`bool setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `ox` / `oy` / `oz` : `std::string const&`<br>常量数字或 Molang 表达式；对应 `v.xpos` / `v.ypos` / `v.zpos`
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setOffset(id, "0", "-2", "0"); // 默认 Y 是 "-4"
    ```

### 调渲染像素级基础偏移

`bool setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `ox` / `oy` / `oz` : `std::string const&`<br>常量数字或 Molang 表达式；对应 `v.xbasepos` / `v.ybasepos` / `v.zbasepos`
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setBaseOffset(id, "0", "8", "0");
    ```

### 设置三轴旋转

`bool setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `rx` / `ry` / `rz` : `std::string const&`<br>三轴角度（度），常量数字或 Molang 表达式；`ry` 是最终朝向，库内部会自己补偿狐狸头
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setRotation(id, "180", "45", "180"); // 偏航转 45 度
    ```

### 叠加偏航增量

`bool rotateY(int64_t id, float delta)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `delta` : `float`<br>要叠加的角度，单位**度**；正负都行
- 返回值：已叠加 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.rotateY(id, 45.0f);  // 在当前偏航上再转 45 度
    displays.rotateY(id, -45.0f); // 转回来
    ```

`rotY` 是纯数字时直接做加法；是 Molang 表达式时包成 `(表达式)+delta`，仍然是客户端每帧算。

### 设置缩放

`bool setScale(int64_t id, std::string const& scale)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `scale` : `std::string const&`<br>常量数字或 Molang 表达式；物品模式默认 `"0.375"`，方块模式建议 `"0.5"`
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setScale(id, "0.6");
    ```

### 按系数放大或缩小

`bool scaleBy(int64_t id, double factor)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `factor` : `double`<br>倍数：`> 1` 放大，`0 < factor < 1` 缩小
- 返回值：已改 `true`；`factor <= 0`、id 不存在，或常量缩放乘完结果 `<= 0` 时 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.scaleBy(id, 1.5);     // 放大 1.5 倍
    displays.scaleBy(id, 1 / 1.5); // 再缩回去
    ```

`scale` 是常量时直接相乘；是 Molang 表达式时包成 `(表达式)*factor`，不会展开表达式内部。

### 设置方块模式的二段变换

`bool setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `scale` : `std::string const&`<br>二段缩放，默认 `"1"`；**只在方块模式生效**
  - `rx` : `std::string const&`<br>二段旋转 X（度），默认 `"-90"`；只在方块模式生效
  - `ry` : `std::string const&`<br>二段旋转 Y（度），默认 `"0"`；只在方块模式生效
- 返回值：已改 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setExtend(id, "1", "-90", "0"); // 默认值
    ```

## 非等比拉伸

### 设置 XZ / Y 两段拉伸

`bool setStretch(int64_t id, std::string const& xz, std::string const& y)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `xz` : `std::string const&`<br>XZ 共轴的拉伸系数，常量数字或 Molang 表达式
  - `y` : `std::string const&`<br>Y 轴单独的拉伸系数，常量数字或 Molang 表达式
- 返回值：已改 `true`；id 不存在或运行时数据缺失 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setStretch(id, "1", "2"); // 只把高度拉成两倍
    ```

### 实时改拉伸（不重建）

`bool setStretchLive(int64_t id, std::string const& xz, std::string const& y)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `xz` / `y` : `std::string const&`<br>同 `setStretch`
- 返回值：已重发 `true`；id 不存在，或当前没有任何玩家能看到这个展示时 `false`（参数仍已写入）
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setStretchLive(id, "1.5", "1");
    ```

`setStretch` 标脏、下一 tick 走重建（对已见玩家原子 `despawn → respawn`）；
`setStretchLive` 只重发动画包，不重建、不换 id。

## 实时变换（不重建）

这一组是 1.31.0 追加的：只重发 FMBE 动画包，不 `despawn`、不换 id，适合每 tick 驱动的动画。

!!! note "没有观看者时返回 false"

    这些接口都返回「有没有包发出去」：当前没有任何玩家能看到该展示时返回 `false`，
    但**参数已经写进去了**，等下一个玩家看到时按新值生效。别把 `false` 当成「参数没生效」。

### 实时改旋转

`bool setRotationLive(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `rx` / `ry` / `rz` : `std::string const&`<br>三轴角度（度），常量数字或 Molang 表达式
- 返回值：已重发 `true`；id 不存在或当前无观看者 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setRotationLive(id, "180", "90", "180");
    ```

### 实时改平移

`bool setOffsetLive(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `ox` / `oy` / `oz` : `std::string const&`<br>模型单位平移，常量数字或 Molang 表达式
- 返回值：已重发 `true`；id 不存在或当前无观看者 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setOffsetLive(id, "0", "-3", "0");
    ```

### 实时改缩放

`bool setScaleLive(int64_t id, std::string const& scale)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `scale` : `std::string const&`<br>常量数字或 Molang 表达式
- 返回值：已重发 `true`；id 不存在或当前无观看者 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setScaleLive(id, "0.5");
    ```

### 一次改完平移 / 旋转 / 缩放

`bool setTransformLive(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz, std::string const& rx, std::string const& ry, std::string const& rz, std::string const& scale)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `ox` / `oy` / `oz` : `std::string const&`<br>模型单位平移
  - `rx` / `ry` / `rz` : `std::string const&`<br>三轴角度（度）
  - `scale` : `std::string const&`<br>缩放
- 返回值：已重发 `true`；id 不存在或当前无观看者 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setTransformLive(id, "0", "-4", "0", "180", "45", "180", "0.375");
    ```

`setTransformLive` 等价于三个 live 接口各调一次，但只发一份包 —— 每 tick 驱动时优先用它。

## 动画

`ItemDisplayAnim` 是一组「持续循环」的动画参数（基础字段 26.40.12 起；
`orbit` / `tumble` / `wobble` / `heartbeat` / `pendulum` / LOD / 第一人称隐藏这批在 1.31.0 补齐）。
库把它们编译成 Molang 表达式并 live 重发，客户端每帧算。单位：`spin` 度/秒，`period` 秒，
`phase` 秒（多个展示用它错开相位）。组合顺序：`spin → billboard → bob / pulse / sway`。

| 字段 | 类型 | 默认值 | 含义 |
|---|---|---|---|
| `spinSpeed` | `float` | `0.0` | 匀速自转，度/秒；`0` = 不转 |
| `spinAxis` | `int` | `1` | 自转轴：`0` = X，`1` = Y，`2` = Z |
| `bobAmplitude` | `float` | `0.0` | 上下浮动幅度（模型单位）；`0` = 不浮 |
| `bobPeriod` | `float` | `2.0` | 浮动周期（秒） |
| `pulseAmplitude` | `float` | `0.0` | 缩放呼吸幅度（比例，`0.2` = ±20%） |
| `pulsePeriod` | `float` | `2.0` | 呼吸周期（秒） |
| `swayAmplitude` | `float` | `0.0` | 摇摆幅度（度，同时作用于 X/Z） |
| `swayPeriod` | `float` | `3.0` | 摇摆周期（秒） |
| `phase` | `float` | `0.0` | 相位偏移（秒） |
| `billboard` | `bool` | `false` | 面向相机（仅物品模式） |
| `orbitRadius` | `float` | `0.0` | 绕基准点画圈（模型单位）；`0` = 不绕 |
| `orbitPeriod` | `float` | `4.0` | 绕圈周期（秒） |
| `orbitPlane` | `int` | `2` | 绕圈平面：`0` = XY，`1` = YZ，`2` = XZ |
| `tumbleSpeed` | `float` | `0.0` | X/Z 双轴翻滚（度/秒）；`0` = 不翻 |
| `wobbleAmplitude` | `float` | `0.0` | 双正弦微晃幅度（度） |
| `wobbleSpeed` | `float` | `1.0` | 微晃倍率 |
| `heartbeatAmplitude` | `float` | `0.0` | 心跳缩放（比例，`0.15` = 峰值 +15%） |
| `heartbeatPeriod` | `float` | `1.2` | 心跳周期（秒） |
| `pendulumAmplitude` | `float` | `0.0` | 绕 Y 轴钟摆幅度（度） |
| `pendulumPeriod` | `float` | `3.0` | 钟摆周期（秒） |
| `billboardPitch` | `bool` | `false` | 面向相机时同时跟随俯仰 |
| `lodNear` | `float` | `0.0` | 距离 LOD 近端（格）；与 `lodFar` 同时 `> 0` 才生效 |
| `lodFar` | `float` | `0.0` | 距离 LOD 远端（格） |
| `lodMinScale` | `float` | `1.0` | 远端最小缩放 |
| `hideInFirstPerson` | `bool` | `false` | 第一人称时隐藏 |

### 应用动画结构体

`bool setAnim(int64_t id, ItemDisplayAnim const& anim)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `anim` : `ItemDisplayAnim const&`<br>整套动画参数，字段见上表；全为默认值时等于不动
- 返回值：已重发 `true`；id 不存在或当前无观看者 `false`（参数仍已写入）
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();

    hologramlib::ItemDisplayAnim anim;
    anim.spinSpeed     = 90.0f; // 每秒转 90 度
    anim.spinAxis      = 1;     // 绕 Y 轴
    anim.bobAmplitude  = 0.5f;  // 上下浮动半个模型单位
    anim.bobPeriod     = 2.0f;
    displays.setAnim(id, anim);
    ```

第一次调 `setAnim` 时，库会把当前的平移 / 旋转 / 缩放记成「基准」；`clearAnim` 就是回到这份基准。

### 用预设名设置动画

`bool setAnimPreset(int64_t id, std::string const& preset, float a, float b, int c)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `preset` : `std::string const&`<br>预设名，见下表；不认识的预设返回 `false`
  - `a` : `float`<br>主参数（幅度 / 速度 / 半径 / 开关），含义随预设
  - `b` : `float`<br>周期或速度；`<= 0.01` 时用该预设的默认周期
  - `c` : `int`<br>整数参数（轴 / 平面 / 百分比），含义随预设
- 返回值：已应用 `true`；`preset` 未知、id 不存在或当前无观看者 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setAnimPreset(id, "spin", 120.0f, 0.0f, 1); // 绕 Y 每秒 120 度
    ```

预设与参数含义：

| `preset` | `a` | `b` | `c` |
|---|---|---|---|
| `"spin"` | 自转速度（度/秒） | 不用 | 轴 `0/1/2`；越界用 `1` |
| `"bob"` | 浮动幅度（模型单位） | 周期（秒，默认 `2.0`） | 不用 |
| `"pulse"` | 呼吸幅度（比例） | 周期（秒，默认 `2.0`） | 不用 |
| `"sway"` | 摇摆幅度（度） | 周期（秒，默认 `3.0`） | 不用 |
| `"orbit"` | 半径（模型单位） | 周期（秒，默认 `4.0`） | 平面 `0/1/2`；越界用 `2` |
| `"tumble"` | 翻滚速度（度/秒） | 不用 | 不用 |
| `"wobble"` | 微晃幅度（度） | 速度倍率（默认 `1.0`） | 不用 |
| `"heartbeat"` | 心跳幅度（比例） | 周期（秒，默认 `1.2`） | 不用 |
| `"pendulum"` | 钟摆幅度（度） | 周期（秒，默认 `3.0`） | 不用 |
| `"billboard"` | `!= 0` 开启面向相机 | 不用 | `!= 0` 同时跟随俯仰 |
| `"lod"` | 近端距离（格） | 远端距离（格） | 远端最小缩放百分比（`50` = `0.5`，`<= 0` 用 `0.5`） |
| `"firstperson"` | `!= 0` 第一人称隐藏 | 不用 | 不用 |
| `"clear"` | 不用 | 不用 | 不用（等价 `clearAnim`） |

### 清除动画

`bool clearAnim(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
- 返回值：已恢复 `true`；id 不存在或当前无观看者 `false`；本来没有动画时直接返回 `true`（空操作）
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.clearAnim(id); // 回到 setAnim 之前的平移 / 旋转 / 缩放
    ```

## 跟随与点击

### 跟随玩家

`bool follow(int64_t id, std::string const& playerName, float offX, float offY, float offZ)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `playerName` : `std::string const&`<br>目标玩家名（`realName`），精确匹配
  - `offX` / `offY` / `offZ` : `float`<br>在玩家实时坐标上叠加的偏移，单位方块
- 返回值：已开始跟随 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.follow(id, "Steve", 0.0f, 2.5f, 0.0f); // 跟在 Steve 头顶
    ```

跟随是 1.17.0 的能力：每 tick 同步目标实时坐标（跨维度会自动 `respawn` 过去），
对已见玩家发 `MoveActorAbsolute`（不是 teleport，客户端插值）平滑移动，全程不重建；
玩家下线自动解除（展示留在原地），手动 `setPosition` 也会解除。

### 解除跟随

`bool unfollow(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
- 返回值：始终 `true`（本来就没有跟随关系也返回 `true`，可以放心重复调用）
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.unfollow(id);
    ```

### 设置可命中的判定体积

`bool setHitbox(int64_t id, float width, float height)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `width` : `float`<br>判定体积宽，单位方块；`<= 0` 按 `0` 处理
  - `height` : `float`<br>判定体积高，单位方块；`<= 0` 按 `0` 处理。两者都 `0` = 不可命中（默认）
- 返回值：已设 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setHitbox(id, 1.0f, 1.0f); // 变成能被点到的方块
    displays.setHitbox(id, 0.0f, 0.0f); // 恢复不可命中
    ```

1.17.0 追加的 AABB 判定体积：通过 `SetActorData` 的 `R53`（宽）/ `R54`（高）下发，
对已见玩家即时生效、不重建；数值同时写进配置，`respawn` 后自动按当前值重发。
命中后的交互 / 攻击经 `ghost*` 事件取回（`domain=itemDisplay`）。

## 开关与可见距离

### 打开或关闭展示

`bool setEnabled(int64_t id, bool enabled)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `enabled` : `bool`<br>`false` = 从所有客户端撤掉（id 仍有效），`true` = 重新显示
- 返回值：已设 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setEnabled(id, false); // 先藏起来
    displays.setEnabled(id, true);  // 再显示
    ```

### 设置可见距离

`bool setViewDistance(int64_t id, double dist)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
  - `dist` : `double`<br>距离，单位方块；`<= 0` = 不限距离
- 返回值：已设 `true`；id 不存在 `false`。与当前值相同直接返回 `true`，不标脏、不重发
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.setViewDistance(id, 32.0); // 32 格内可见
    displays.setViewDistance(id, 0.0);  // 不限距离
    ```

## 销毁

### 销毁一个展示

`bool destroy(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>物品展示 id
- 返回值：确实销毁了 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.destroy(id);
    ```

### 销毁全部展示

`void destroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    auto& displays = hologramlib::IHologramLib::getInstance().itemDisplays();
    displays.destroyAll(); // 关服 / 重载插件时收场
    ```

## 只用 LSE 暴露的能力

以下几个能力有 LSE 导出、但**没有**进 C++ 接口，需要时请走脚本层：

- `createSeamless`：建展示时一次定好模式、视距与可见玩家，避免「建完再改」的那次重建闪烁；
- `scaleTo`：单次跳变到目标缩放（`mode = 1` 时不可用）；要做渐变动画就自己分步调用
  `scaleTo` 驱动（例如 5 步 easeOutCubic），库不替消费方做动画；
- 可见玩家白名单（`setVisiblePlayers` / `setVisiblePlayer` / `clearVisiblePlayers`）；
- `playCreeperSwelling` 彩蛋与 `emitForEntity`（让某玩家把一只已有实体看成 FMBE 模型）。

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

// 在 (100.5, 66, 100.5) 摆一颗一直转、会呼吸、能点到的钻石
int64_t spawn_spinning_gem() {
    auto& lib      = hologramlib::IHologramLib::getInstance();
    auto& displays = lib.itemDisplays();

    hologramlib::ItemDisplayConfig cfg;
    cfg.item    = "minecraft:diamond";
    cfg.x       = 100.5f;
    cfg.y       = 66.0f;
    cfg.z       = 100.5f;
    cfg.dimension = 0;
    cfg.scale     = "0.5";
    cfg.itemGlint = true;

    int64_t id = displays.create(cfg);
    if (id < 0) {
        return -1;
    }

    hologramlib::ItemDisplayAnim anim;
    anim.spinSpeed      = 90.0f; // 每秒 90 度
    anim.spinAxis       = 1;     // 绕 Y 轴
    anim.pulseAmplitude = 0.15f; // 缩放 ±15%
    anim.pulsePeriod    = 1.5f;
    displays.setAnim(id, anim);

    displays.setHitbox(id, 0.8f, 0.8f); // 能被点到，点击走 ghost* 事件
    return id;
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
