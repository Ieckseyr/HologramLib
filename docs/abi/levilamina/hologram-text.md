# 1.3 IHologramText · 悬浮字

整块多行文本（`\n` 合并为一个形状）的创建、编辑与显示。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().holograms()`（或 `IHologramLib::text()`，视版本）
- 坐标：世界坐标，单位方块
- 颜色：`r` / `g` / `b` / `a` 四个 `float`，范围 `0.0 ~ 1.0`
- 行索引：从 `0` 开始

!!! info "1.26.0 之后的模型"

    行只负责**文本**；颜色、缩放、背景框、穿墙、旋转都是**整块属性**。
    行级样式差异请在文本里内嵌 `§` 颜色代码。
    已移除的行级方法保留为**废弃空槽**（非纯虚 + 内联空实现 + `[[deprecated]]`），
    调用返回 `false` / 无动作，虚表槽位与 1.25.0 一致。

!!! tip "变更后要 refresh"

    `create` 之后调用 `draw` 才会显示；任何内容/样式变更后调用 `refresh`
    原地重绘（同 networkId 覆盖，无闪烁）。`draw` 已绘制时也可再次调用。

## 生命周期

### 创建一个悬浮字

`int64_t create(float x, float y, float z)`

- 参数：
  - `x` : `float`<br>锚点世界坐标 X
  - `y` : `float`<br>锚点世界坐标 Y
  - `z` : `float`<br>锚点世界坐标 Z
- 返回值：悬浮字 id（从 `1` 开始递增）；`<= 0` 表示创建失败
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    #include "hologramlib/HologramLib.h"

    int64_t id = hologramlib::IHologramLib::getInstance().holograms().create(100.5f, 66.0f, 100.5f);
    if (id <= 0) {
        return; // 创建失败
    }
    auto& text = hologramlib::IHologramLib::getInstance().holograms();
    text.addLine(id, "§e主城 · 告示牌");
    text.draw(id);
    ```

### 销毁一个悬浮字

`bool destroy(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：成功 `true`；id 不存在或已销毁 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    bool ok = hologramlib::IHologramLib::getInstance().holograms().destroy(id);
    ```

### 销毁全部悬浮字

`void destroyAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    hologramlib::IHologramLib::getInstance().holograms().destroyAll();
    ```

## 行内容

### 追加一行

`bool addLine(int64_t id, std::string const& text)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `text` : `std::string const&`<br>行文本，支持 `§` 颜色代码与 `{}` 变量
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.addLine(id, "第一行");
    text.addLine(id, "§a第二行");
    ```

### 修改某一行文本

`bool setLineText(int64_t id, int lineIndex, std::string const& text)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `lineIndex` : `int`<br>行索引，`0` 起
  - `text` : `std::string const&`<br>新文本
- 返回值：成功 `true`；id 不存在或行索引越界 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setLineText(id, 0, "§c已更新");
    text.refresh(id);
    ```

### 设置动态行（内容池轮播）

`bool setLinePool(int64_t id, int lineIndex, std::vector<std::string> const& content, int intervalMs)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `lineIndex` : `int`<br>行索引
  - `content` : `std::vector<std::string> const&`<br>内容池；空数组 = 清除轮播
  - `intervalMs` : `int`<br>轮播间隔（毫秒）；`<= 0` 或池内 ≤1 项 = 不轮播
- 返回值：成功 `true`；id 不存在或行索引越界 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setLinePool(id, 0, {"§a在线", "§e欢迎", "§b主城"}, 2000);
    ```

!!! note "轮播是无状态的"

    按 `当前时间 / intervalMs` 取模选行，多个悬浮字用同一个池会**同相同步**。
    刷新由库内 `0.5s` 节流驱动，内容变了才重发。

### 设置某行是否解析变量

`bool setLineParseVariables(int64_t id, int lineIndex, bool enabled)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `lineIndex` : `int`<br>行索引
  - `enabled` : `bool`<br>`true` 解析 `{player}` 等变量（默认）；`false` 时 `{}` 按字面量显示
- 返回值：成功 `true`；id 不存在或行索引越界 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.addLine(id, "在线: {online}");
    text.setLineParseVariables(id, 0, true);
    ```

内置变量：`{player}` `{online}` `{time}` `{tps}` `{dimension}` `{x}` `{y}` `{z}`。
含 `{var}` 的文本按**观看者各一份形状**（逐玩家解析），刷新时逐玩家原地重发。

### 移除某一行

`bool removeLine(int64_t id, int lineIndex)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `lineIndex` : `int`<br>行索引
- 返回值：成功 `true`；id 不存在或行索引越界 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.removeLine(id, 1);
    text.refresh(id);
    ```

### 清空所有行

`bool clearLines(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.clearLines(id);
    text.addLine(id, "重新开始");
    text.refresh(id);
    ```

### 查询行数

`int getLineCount(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：行数；id 不存在返回 `0`
- 返回值类型：`int`
- 示例：

    ```cpp
    int lines = text.getLineCount(id);
    ```

## 颜色

### 设置文字颜色（整块）

`bool setColor(int64_t id, float r, float g, float b, float a)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `r` / `g` / `b` / `a` : `float`<br>RGBA，`0.0 ~ 1.0`
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setColor(id, 1.0f, 0.85f, 0.2f, 1.0f);
    text.refresh(id);
    ```

### 设置背景框颜色

`bool setBackgroundColor(int64_t id, float r, float g, float b, float a)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `r` / `g` / `b` / `a` : `float`<br>RGBA，`0.0 ~ 1.0`；`a = 0` 表示透明
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setBackgroundColor(id, 0.0f, 0.0f, 0.0f, 0.5f);
    ```

### 清除背景框颜色

`bool clearBackgroundColor(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.clearBackgroundColor(id);
    ```

## 样式

### 设置整块缩放

`bool setScale(int64_t id, float scale)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `scale` : `float`<br>缩放倍数，`1.0` 为原始大小
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setScale(id, 1.5f);
    text.refresh(id);
    ```

### 设置穿墙可见性

`bool setDepthTest(int64_t id, bool enabled)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `enabled` : `bool`<br>`false` = 穿墙可见（默认）；`true` = 被方块/实体遮挡
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setDepthTest(id, true); // 被方块/实体遮挡
    ```

### 设置固定朝向

`bool setRotation(int64_t id, float pitch, float yaw, float roll)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `pitch` / `yaw` / `roll` : `float`<br>三轴角度（度）
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setRotation(id, 0.0f, 90.0f, 0.0f);
    ```

### 恢复面向相机

`bool clearRotation(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.clearRotation(id);
    ```

## 位置

### 设置位置

`bool setLocation(int64_t id, float x, float y, float z)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `x` / `y` / `z` : `float`<br>新的世界坐标
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setLocation(id, 101.0f, 66.0f, 101.0f);
    text.refresh(id);
    ```

### 跟随玩家

`bool setFollowPlayer(int64_t id, std::string const& playerName, float offsetY)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `playerName` : `std::string const&`<br>玩家名（精确匹配）
  - `offsetY` : `float`<br>相对玩家脚部的 Y 偏移
- 返回值：成功 `true`；id 不存在或玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setFollowPlayer(id, "Steve", 2.2f);
    text.refresh(id);
    ```

### 取消跟随

`bool clearFollowPlayer(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.clearFollowPlayer(id);
    ```

### 迁移维度

`bool setDimension(int64_t id, int dimId)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `dimId` : `int`<br>目标维度 id（`0` 主世界 / `1` 下界 / `2` 末地）
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.setDimension(id, 1);
    ```

!!! note "已绘制时的行为"

    已绘制时会同步底层形状维度，并按原绘制目标**原地重发**（同 networkId，无闪烁）。

## 显示控制

### 对全体显示

`bool draw(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：成功 `true`；id 不存在或没有行 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.addLine(id, "Hello");
    text.draw(id);
    ```

### 对某个维度显示

`bool drawToDimension(int64_t id, int dimId)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `dimId` : `int`<br>目标维度 id
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.drawToDimension(id, 0);
    ```

### 对单个玩家显示

`bool drawToPlayer(int64_t id, std::string const& playerName)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
  - `playerName` : `std::string const&`<br>玩家名
- 返回值：成功 `true`；id 不存在或玩家不在线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.drawToPlayer(id, "Alex");
    ```

### 移除显示

`bool remove(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.remove(id);
    ```

### 原地重绘

`bool refresh(int64_t id)`

- 参数：
  - `id` : `int64_t`<br>悬浮字 id
- 返回值：成功 `true`；id 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    text.addLine(id, "在线: {online}");
    text.refresh(id);
    ```

## 已废弃（1.26.0 移除，空槽保留）

以下方法仍在虚表中（保证旧二进制槽位不错位），但**调用无效果**、返回 `false`。
新代码请用替代方案。

| 方法 | 替代 |
|---|---|
| `setLineScale(int64_t, int, float)` | `setScale(id, scale)` |
| `setLineColor(int64_t, int, float, float, float, float)` | 文本内嵌 `§` 颜色代码 |
| `setLineGradient(...)` | 文本内嵌 `§` 颜色代码 |
| `setLineRainbow(...)` | 文本内嵌 `§` 颜色代码 |
| `setLineScroll(...)` | 无（库内自驱已删） |
| `setVerticalAnimation(...)` | 无（库内自驱已删） |
| `setLineSpacing(...)` | 无（协议无行间距字段） |
| `tick(float)` | 无（跟随/变量在 `draw` / `refresh` 时就地解析） |

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

void spawn_welcome_hologram() {
    auto& text = hologramlib::IHologramLib::getInstance().holograms();

    int64_t id = text.create(100.5f, 67.0f, 100.5f);
    if (id <= 0) {
        return;
    }

    text.addLine(id, "§6§l主城");
    text.addLine(id, "在线: §a{online}");
    text.setLinePool(id, 1, {"§a欢迎回来", "§e祝你玩得开心"}, 3000);
    text.setColor(id, 1.0f, 1.0f, 1.0f, 1.0f);
    text.setBackgroundColor(id, 0.0f, 0.0f, 0.0f, 0.45f);
    text.setScale(id, 1.2f);
    text.setDepthTest(id, false);
    text.draw(id);
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
