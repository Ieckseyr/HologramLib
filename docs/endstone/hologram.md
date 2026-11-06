# 悬浮字 · Endstone

多行悬浮字：每行独立设置文本、颜色、轮播池与变量解析。内部按 4 Hz 重下发改动过的行，只有内容真的变了才发包。

`{online}` / `{player}` 这类变量要开启 `set_line_parse_variables`；轮播用 `set_line_pool(index, [..], interval_ms)` 配刷新间隔。

## 快速上手

```python
hid = lib.holograms.create(0, 101, 0)
lib.holograms.set_scale(hid, 1.2)
lib.holograms.add_line(hid, "§6在线 §f{online}")
lib.holograms.set_line_parse_variables(hid, 0, True)
lib.holograms.add_line(hid, "§7每 2 秒轮播")
lib.holograms.set_line_pool(hid, 1, ["§aA", "§bB", "§cC"], 2000)
lib.holograms.draw(hid)
```

## 函数（28 个）

#### `add_line`

`lib.holograms.add_line(text_id: int, text: str)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值类型：`bool`
- 对应 LSE： `holoAddLine`

#### `lib.holo_api_version`

| | |
|---|---|
| 对应 LSE 导出 | `holoApiVersion` |
| 说明 | 库 API 版本常量（`1.35.0`），与 LL 版 `HOLOGLIB_API_VERSION` 对齐 |

#### `clear_background_color`

`lib.holograms.clear_background_color(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `holoClearBackgroundColor`

#### `clear_follow_player`

`lib.holograms.clear_follow_player(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `holoClearFollowPlayer`

#### `clear_lines`

`lib.holograms.clear_lines(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `holoClearLines`

#### `clear_rotation`

`lib.holograms.clear_rotation(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `holoClearRotation`

#### `create`

`lib.holograms.create(x: float, y: float, z: float)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
- 返回值类型：`int`
- 对应 LSE： `holoCreate`

#### `destroy`

`lib.holograms.destroy(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `holoDestroy`

#### `destroy_all`

`lib.holograms.destroy_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `holoDestroyAll`

#### `draw`

`lib.holograms.draw(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `holoDraw`

#### `draw_to_dimension`

`lib.holograms.draw_to_dimension(text_id: int, dim_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `dim_id` : `int`<br>维度 id
- 返回值类型：`bool`
- 对应 LSE： `holoDrawToDimension`

#### `draw_to_player`

`lib.holograms.draw_to_player(text_id: int, player_name: str)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `holoDrawToPlayer`

#### `get_line_count`

`lib.holograms.get_line_count(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`int`
- 对应 LSE： `holoGetLineCount`

#### `info`

`lib.holograms.info()`

- 参数：无
- 返回值类型：`str`
- 对应 LSE： `holoInfo`

#### `refresh`

`lib.holograms.refresh(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `holoRefresh`

#### `destroy`

`lib.holograms.destroy(text_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `holoRemove`

#### `remove_line`

`lib.holograms.remove_line(text_id: int, index: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
- 返回值类型：`bool`
- 对应 LSE： `holoRemoveLine`

#### `set_background_color`

`lib.holograms.set_background_color(text_id: int, r: float, g: float, b: float, a: float = 1.0)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `r` : `float`<br>红色分量 0~1
  - `g` : `float`<br>绿色分量 0~1
  - `b` : `float`<br>蓝色分量 0~1
  - `a` : `float`<br>透明度 0~1（默认 `1.0`）
- 返回值类型：`bool`
- 对应 LSE： `holoSetBackgroundColor`

#### `set_color`

`lib.holograms.set_color(text_id: int, r: float, g: float, b: float, a: float = 1.0)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `r` : `float`<br>红色分量 0~1
  - `g` : `float`<br>绿色分量 0~1
  - `b` : `float`<br>蓝色分量 0~1
  - `a` : `float`<br>透明度 0~1（默认 `1.0`）
- 返回值类型：`bool`
- 对应 LSE： `holoSetColor`

#### `set_depth_test`

`lib.holograms.set_depth_test(text_id: int, enabled: bool)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `enabled` : `bool`<br>开 / 关
- 返回值类型：`bool`
- 对应 LSE： `holoSetDepthTest`

#### `set_dimension`

`lib.holograms.set_dimension(text_id: int, dim_id: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `dim_id` : `int`<br>维度 id
- 返回值类型：`bool`
- 对应 LSE： `holoSetDimension`

#### `set_follow_player`

`lib.holograms.set_follow_player(text_id: int, player_name: str, offset_y: float = 0.0)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `offset_y` : `float`（默认 `0.0`）
- 返回值类型：`bool`
- 对应 LSE： `holoSetFollowPlayer`

#### `set_line_parse_variables`

`lib.holograms.set_line_parse_variables(text_id: int, index: int, enabled: bool)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
  - `enabled` : `bool`<br>开 / 关
- 返回值类型：`bool`
- 对应 LSE： `holoSetLineParseVariables`

#### `set_line_pool`

`lib.holograms.set_line_pool(text_id: int, index: int, content: list[str], interval_ms: int)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
  - `content` : `list[str]`
  - `interval_ms` : `int`
- 返回值类型：`bool`
- 对应 LSE： `holoSetLinePool`

#### `set_line_text`

`lib.holograms.set_line_text(text_id: int, index: int, text: str)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值类型：`bool`
- 对应 LSE： `holoSetLineText`

#### `set_location`

`lib.holograms.set_location(text_id: int, x: float, y: float, z: float)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
- 返回值类型：`bool`
- 对应 LSE： `holoSetLocation`

#### `set_rotation`

`lib.holograms.set_rotation(text_id: int, pitch: float, yaw: float, roll: float = 0.0)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `pitch` : `float`<br>俯仰角（度）
  - `yaw` : `float`<br>水平朝向（度）
  - `roll` : `float`<br>翻滚角（度）（默认 `0.0`）
- 返回值类型：`bool`
- 对应 LSE： `holoSetRotation`

#### `set_scale`

`lib.holograms.set_scale(text_id: int, scale: float)`

- 参数：
  - `text_id` : `int`<br>悬浮字 id（`lib.holograms.create(...)` 的返回值）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值类型：`bool`
- 对应 LSE： `holoSetScale`


---

完整索引见 [Endstone API 索引](api.md)。
