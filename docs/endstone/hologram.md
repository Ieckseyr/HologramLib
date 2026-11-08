# 悬浮字 · Endstone

多行悬浮字：每行独立设置文本、颜色、轮播池与变量解析。内部按 4 Hz 重下发改动过的行，只有内容真的变了才发包。

`{online}` / `{player}` 这类变量要开启 `set_line_parse_variables`；轮播用 `set_line_pool(index, [..], interval_ms)` 配刷新间隔。

## 快速开始

```python
hid = lib.holograms.create(0, 101, 0)
lib.holograms.set_scale(hid, 1.2)
lib.holograms.add_line(hid, "§6在线 §f{online}")
lib.holograms.set_line_parse_variables(hid, 0, True)
lib.holograms.add_line(hid, "§7每 2 秒轮播")
lib.holograms.set_line_pool(hid, 1, ["§aA", "§bB", "§cC"], 2000)
lib.holograms.draw(hid)
```

## 函数

### `add_line`

`lib.holograms.add_line(text_id: int, text: str)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoAddLine`
- 示例：

    ```python
    lib.holograms.add_line(1, "hello")
    ```

#### `lib.holo_api_version`

| | |
|---|---|
| 对应 LSE 导出 | `holoApiVersion` |
| 说明 | 库 API 版本常量（`1.35.0`），与 LL 版 `HOLOGLIB_API_VERSION` 对齐 |

### `clear_background_color`

`lib.holograms.clear_background_color(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoClearBackgroundColor`
- 示例：

    ```python
    lib.holograms.clear_background_color(1)
    ```

### `clear_follow_player`

`lib.holograms.clear_follow_player(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoClearFollowPlayer`
- 示例：

    ```python
    lib.holograms.clear_follow_player(1)
    ```

### `clear_lines`

`lib.holograms.clear_lines(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoClearLines`
- 示例：

    ```python
    lib.holograms.clear_lines(1)
    ```

### `clear_rotation`

`lib.holograms.clear_rotation(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoClearRotation`
- 示例：

    ```python
    lib.holograms.clear_rotation(1)
    ```

### `create`

`lib.holograms.create(x: float, y: float, z: float)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`holoCreate`
- 示例：

    ```python
    lib.holograms.create(0.0, 64.0, 0.0)
    ```

### `destroy`

`lib.holograms.destroy(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoDestroy`
- 示例：

    ```python
    lib.holograms.destroy(1)
    ```

### `destroy_all`

`lib.holograms.destroy_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`holoDestroyAll`
- 示例：

    ```python
    lib.holograms.destroy_all()
    ```

### `draw`

`lib.holograms.draw(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoDraw`
- 示例：

    ```python
    lib.holograms.draw(1)
    ```

### `draw_to_dimension`

`lib.holograms.draw_to_dimension(text_id: int, dim_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `dim_id` : `int`<br>维度 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoDrawToDimension`
- 示例：

    ```python
    lib.holograms.draw_to_dimension(1, 0)
    ```

### `draw_to_player`

`lib.holograms.draw_to_player(text_id: int, player_name: str)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoDrawToPlayer`
- 示例：

    ```python
    lib.holograms.draw_to_player(1, "Steve")
    ```

### `get_line_count`

`lib.holograms.get_line_count(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`holoGetLineCount`
- 示例：

    ```python
    lib.holograms.get_line_count(1)
    ```

### `info`

`lib.holograms.info()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`holoInfo`
- 示例：

    ```python
    lib.holograms.info()
    ```

### `refresh`

`lib.holograms.refresh(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoRefresh`
- 示例：

    ```python
    lib.holograms.refresh(1)
    ```

### `destroy`

`lib.holograms.destroy(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoRemove`
- 示例：

    ```python
    lib.holograms.destroy(1)
    ```

### `remove_line`

`lib.holograms.remove_line(text_id: int, index: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoRemoveLine`
- 示例：

    ```python
    lib.holograms.remove_line(1, 1)
    ```

### `set_background_color`

`lib.holograms.set_background_color(text_id: int, r: float, g: float, b: float, a: float = 1.0)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `r` : `float`<br>红色分量 0~1
  - `g` : `float`<br>绿色分量 0~1
  - `b` : `float`<br>蓝色分量 0~1
  - `a` : `float`<br>透明度 0~1（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetBackgroundColor`
- 示例：

    ```python
    lib.holograms.set_background_color(1, 1, 1, 1, 1)
    ```

### `set_color`

`lib.holograms.set_color(text_id: int, r: float, g: float, b: float, a: float = 1.0)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `r` : `float`<br>红色分量 0~1
  - `g` : `float`<br>绿色分量 0~1
  - `b` : `float`<br>蓝色分量 0~1
  - `a` : `float`<br>透明度 0~1（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetColor`
- 示例：

    ```python
    lib.holograms.set_color(1, 1, 1, 1, 1)
    ```

### `set_depth_test`

`lib.holograms.set_depth_test(text_id: int, enabled: bool)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `enabled` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetDepthTest`
- 示例：

    ```python
    lib.holograms.set_depth_test(1, "...")
    ```

### `set_dimension`

`lib.holograms.set_dimension(text_id: int, dim_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `dim_id` : `int`<br>维度 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetDimension`
- 示例：

    ```python
    lib.holograms.set_dimension(1, 0)
    ```

### `set_follow_player`

`lib.holograms.set_follow_player(text_id: int, player_name: str, offset_y: float = 0.0)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `offset_y` : `float`（默认 `0.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetFollowPlayer`
- 示例：

    ```python
    lib.holograms.set_follow_player(1, "Steve", 1)
    ```

### `set_line_parse_variables`

`lib.holograms.set_line_parse_variables(text_id: int, index: int, enabled: bool)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
  - `enabled` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetLineParseVariables`
- 示例：

    ```python
    lib.holograms.set_line_parse_variables(1, 1, "...")
    ```

### `set_line_pool`

`lib.holograms.set_line_pool(text_id: int, index: int, content: list[str], interval_ms: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
  - `content` : `list[str]`
  - `interval_ms` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetLinePool`
- 示例：

    ```python
    lib.holograms.set_line_pool(1, 1, "...", 1)
    ```

### `set_line_text`

`lib.holograms.set_line_text(text_id: int, index: int, text: str)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetLineText`
- 示例：

    ```python
    lib.holograms.set_line_text(1, 1, "hello")
    ```

### `set_location`

`lib.holograms.set_location(text_id: int, x: float, y: float, z: float)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetLocation`
- 示例：

    ```python
    lib.holograms.set_location(1, 0.0, 64.0, 0.0)
    ```

### `set_rotation`

`lib.holograms.set_rotation(text_id: int, pitch: float, yaw: float, roll: float = 0.0)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `pitch` : `float`<br>俯仰角（度）
  - `yaw` : `float`<br>水平朝向（度）
  - `roll` : `float`<br>翻滚角（度）（默认 `0.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetRotation`
- 示例：

    ```python
    lib.holograms.set_rotation(1, 1, 1, 1)
    ```

### `set_scale`

`lib.holograms.set_scale(text_id: int, scale: float)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`holoSetScale`
- 示例：

    ```python
    lib.holograms.set_scale(1, 1)
    ```


---

完整索引见 [Endstone API 索引](api.md)。
