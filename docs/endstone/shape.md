# 形状 · Endstone

形状域用 `PrimitiveShapes`（包 id 328）一次下发一组几何体，客户端自己画。悬浮的线框、球、圆、箭头、文字都走这里，不生成实体、不写存档。

每个形状有独立 id，改位置/颜色/缩放后要再 `draw()` 一次才会重新下发；`draw_to_player()` 只发给一个人，`draw_to_dimension()` 发给整个维度。

## 快速开始

```python
lib = self.server.plugin_manager.get_plugin("hologramlib")

sid = lib.api.create_box(0, 64, 0, 3, 67, 3)   # 一个 3×3×3 的线框
lib.api.set_color(sid, 0.2, 1.0, 0.4)          # 换颜色
lib.api.draw(sid)                              # 下发

text_id = lib.api.create_text(0, 70, 0, "§eHello")
lib.api.draw(text_id)
```

## 函数

### `attach_entity`

`lib.api.attach_entity(shape_id: int, entity_unique_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `entity_unique_id` : `int`<br>实体的 uniqueId（网络层 id，不是库内 id）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeAttachEntity`
- 示例：

    ```python
    lib.api.attach_entity(1, 1)
    ```

### `clear_attached_entity`

`lib.api.clear_attached_entity(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeClearAttachedEntity`
- 示例：

    ```python
    lib.api.clear_attached_entity(1)
    ```

### `clear_background_color`

`lib.api.clear_background_color(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeClearBackgroundColor`
- 示例：

    ```python
    lib.api.clear_background_color(1)
    ```

### `clear_rotation`

`lib.api.clear_rotation(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeClearRotation`
- 示例：

    ```python
    lib.api.clear_rotation(1)
    ```

### `create_arrow`

`lib.api.create_arrow(x1: float, y1: float, z1: float, x2: float, y2: float, z2: float)`

- 参数：
  - `x1` : `float`<br>起点 X
  - `y1` : `float`<br>起点 Y
  - `z1` : `float`<br>起点 Z
  - `x2` : `float`<br>终点 X
  - `y2` : `float`<br>终点 Y
  - `z2` : `float`<br>终点 Z
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateArrow`
- 示例：

    ```python
    lib.api.create_arrow(0.0, 64.0, 0.0, 1.0, 65.0, 1.0)
    ```

### `create_box`

`lib.api.create_box(x1: float, y1: float, z1: float, x2: float, y2: float, z2: float)`

- 参数：
  - `x1` : `float`<br>起点 X
  - `y1` : `float`<br>起点 Y
  - `z1` : `float`<br>起点 Z
  - `x2` : `float`<br>终点 X
  - `y2` : `float`<br>终点 Y
  - `z2` : `float`<br>终点 Z
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateBox`
- 示例：

    ```python
    lib.api.create_box(0.0, 64.0, 0.0, 1.0, 65.0, 1.0)
    ```

### `create_circle`

`lib.api.create_circle(x: float, y: float, z: float, scale: float = 1.0)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateCircle`
- 示例：

    ```python
    lib.api.create_circle(0.0, 64.0, 0.0, 1)
    ```

### `create_cone`

`lib.api.create_cone(x: float, y: float, z: float, radius_x: float, radius_z: float, height: float, segments: int = 16)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `radius_x` : `float`
  - `radius_z` : `float`
  - `height` : `float`<br>高
  - `segments` : `int`（默认 `16`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateCone`
- 示例：

    ```python
    lib.api.create_cone(0.0, 64.0, 0.0, 1, 1, 1, 1)
    ```

### `create_cylinder`

`lib.api.create_cylinder(x: float, y: float, z: float, radius_x: float, radius_z: float, height: float, segments: int = 16)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `radius_x` : `float`
  - `radius_z` : `float`
  - `height` : `float`<br>高
  - `segments` : `int`（默认 `16`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateCylinder`
- 示例：

    ```python
    lib.api.create_cylinder(0.0, 64.0, 0.0, 1, 1, 1, 1)
    ```

### `create_ellipsoid`

`lib.api.create_ellipsoid(x: float, y: float, z: float, radius_x: float, radius_y: float, radius_z: float, segments: int = 12)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `radius_x` : `float`
  - `radius_y` : `float`
  - `radius_z` : `float`
  - `segments` : `int`（默认 `12`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateEllipsoid`
- 示例：

    ```python
    lib.api.create_ellipsoid(0.0, 64.0, 0.0, 1, 1, 1, 1)
    ```

### `create_line`

`lib.api.create_line(x1: float, y1: float, z1: float, x2: float, y2: float, z2: float)`

- 参数：
  - `x1` : `float`<br>起点 X
  - `y1` : `float`<br>起点 Y
  - `z1` : `float`<br>起点 Z
  - `x2` : `float`<br>终点 X
  - `y2` : `float`<br>终点 Y
  - `z2` : `float`<br>终点 Z
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateLine`
- 示例：

    ```python
    lib.api.create_line(0.0, 64.0, 0.0, 1.0, 65.0, 1.0)
    ```

### `create_pyramid`

`lib.api.create_pyramid(x: float, y: float, z: float, width: float, height: float, depth: float)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `width` : `float`<br>宽
  - `height` : `float`<br>高
  - `depth` : `float`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreatePyramid`
- 示例：

    ```python
    lib.api.create_pyramid(0.0, 64.0, 0.0, 1, 1, 1)
    ```

### `create_sphere`

`lib.api.create_sphere(x: float, y: float, z: float, scale: float = 1.0)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateSphere`
- 示例：

    ```python
    lib.api.create_sphere(0.0, 64.0, 0.0, 1)
    ```

### `create_text`

`lib.api.create_text(x: float, y: float, z: float, text: str)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeCreateText`
- 示例：

    ```python
    lib.api.create_text(0.0, 64.0, 0.0, "hello")
    ```

### `destroy`

`lib.api.destroy(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeDestroy`
- 示例：

    ```python
    lib.api.destroy(1)
    ```

### `destroy_all`

`lib.api.destroy_all()`

- 参数：无
- 返回值：清空所有形状（一次批量发删除包）。返回清理数量。
- 返回值类型：`int`
- 对应 LSE 导出：`shapeDestroyAll`
- 示例：

    ```python
    lib.api.destroy_all()
    ```

### `destroy_batch`

`lib.api.destroy_batch(ids: list[int])`

- 参数：
  - `ids` : `list[int]`<br>形状 id 列表
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeDestroyBatch`
- 示例：

    ```python
    lib.api.destroy_batch("...")
    ```

### `draw`

`lib.api.draw(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeDraw`
- 示例：

    ```python
    lib.api.draw(1)
    ```

### `draw_batch`

`lib.api.draw_batch(ids: list[int])`

- 参数：
  - `ids` : `list[int]`<br>形状 id 列表
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeDrawBatch`
- 示例：

    ```python
    lib.api.draw_batch("...")
    ```

### `draw_to_dimension`

`lib.api.draw_to_dimension(shape_id: int, dim_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `dim_id` : `int`<br>维度 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeDrawToDimension`
- 示例：

    ```python
    lib.api.draw_to_dimension(1, 0)
    ```

### `draw_to_player`

`lib.api.draw_to_player(shape_id: int, player_name: str)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeDrawToPlayer`
- 示例：

    ```python
    lib.api.draw_to_player(1, "Steve")
    ```

### `exists`

`lib.api.exists(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeExists`
- 示例：

    ```python
    lib.api.exists(1)
    ```

### `find_text_by_location`

`lib.api.find_text_by_location(x: float, y: float, z: float, radius: float = 1.0)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `radius` : `float`（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`shapeFindTextByLocation`
- 示例：

    ```python
    lib.api.find_text_by_location(0.0, 64.0, 0.0, 1)
    ```

### `find_text_by_location_and_content`

`lib.api.find_text_by_location_and_content(x: float, y: float, z: float, radius: float, text: str)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `radius` : `float`
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeFindTextByLocationAndContent`
- 示例：

    ```python
    lib.api.find_text_by_location_and_content(0.0, 64.0, 0.0, 1, "hello")
    ```

### `get_all_shape_ids`

`lib.api.get_all_shape_ids()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`shapeGetAllShapeIds`
- 示例：

    ```python
    lib.api.get_all_shape_ids()
    ```

### `get_color`

`lib.api.get_color(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[float]`
- 对应 LSE 导出：`shapeGetColor`
- 示例：

    ```python
    lib.api.get_color(1)
    ```

### `get_location`

`lib.api.get_location(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[float]`
- 对应 LSE 导出：`shapeGetLocation`
- 示例：

    ```python
    lib.api.get_location(1)
    ```

### `get_rotation`

`lib.api.get_rotation(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[float]`
- 对应 LSE 导出：`shapeGetRotation`
- 示例：

    ```python
    lib.api.get_rotation(1)
    ```

### `get_shape_type`

`lib.api.get_shape_type(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`shapeGetShapeType`
- 示例：

    ```python
    lib.api.get_shape_type(1)
    ```

### `get_text`

`lib.api.get_text(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`shapeGetText`
- 示例：

    ```python
    lib.api.get_text(1)
    ```

### `remove`

`lib.api.remove(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeRemove`
- 示例：

    ```python
    lib.api.remove(1)
    ```

### `remove_to_dimension`

`lib.api.remove_to_dimension(shape_id: int, dim_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `dim_id` : `int`<br>维度 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeRemoveToDimension`
- 示例：

    ```python
    lib.api.remove_to_dimension(1, 0)
    ```

### `remove_to_player`

`lib.api.remove_to_player(shape_id: int, player_name: str)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeRemoveToPlayer`
- 示例：

    ```python
    lib.api.remove_to_player(1, "Steve")
    ```

### `set_background_color`

`lib.api.set_background_color(shape_id: int, r: float, g: float, b: float, a: float = 1.0)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `r` : `float`<br>红色分量 0~1
  - `g` : `float`<br>绿色分量 0~1
  - `b` : `float`<br>蓝色分量 0~1
  - `a` : `float`<br>透明度 0~1（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetBackgroundColor`
- 示例：

    ```python
    lib.api.set_background_color(1, 1, 1, 1, 1)
    ```

### `set_color`

`lib.api.set_color(shape_id: int, r: float, g: float, b: float, a: float = 1.0)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `r` : `float`<br>红色分量 0~1
  - `g` : `float`<br>绿色分量 0~1
  - `b` : `float`<br>蓝色分量 0~1
  - `a` : `float`<br>透明度 0~1（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetColor`
- 示例：

    ```python
    lib.api.set_color(1, 1, 1, 1, 1)
    ```

### `set_depth_test`

`lib.api.set_depth_test(shape_id: int, enabled: bool)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `enabled` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetDepthTest`
- 示例：

    ```python
    lib.api.set_depth_test(1, "...")
    ```

### `set_dimension`

`lib.api.set_dimension(shape_id: int, dim_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `dim_id` : `int`<br>维度 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetDimension`
- 示例：

    ```python
    lib.api.set_dimension(1, 0)
    ```

### `set_duration`

`lib.api.set_duration(shape_id: int, seconds: float)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `seconds` : `float`<br>持续秒数
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetDuration`
- 示例：

    ```python
    lib.api.set_duration(1, 1)
    ```

### `set_location`

`lib.api.set_location(shape_id: int, x: float, y: float, z: float)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetLocation`
- 示例：

    ```python
    lib.api.set_location(1, 0.0, 64.0, 0.0)
    ```

### `set_max_render_distance`

`lib.api.set_max_render_distance(shape_id: int, distance: float)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `distance` : `float`<br>距离（格）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetMaxRenderDistance`
- 示例：

    ```python
    lib.api.set_max_render_distance(1, 1)
    ```

### `set_rotation`

`lib.api.set_rotation(shape_id: int, pitch: float, yaw: float, roll: float = 0.0)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `pitch` : `float`<br>俯仰角（度）
  - `yaw` : `float`<br>水平朝向（度）
  - `roll` : `float`<br>翻滚角（度）（默认 `0.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetRotation`
- 示例：

    ```python
    lib.api.set_rotation(1, 1, 1, 1)
    ```

### `set_scale`

`lib.api.set_scale(shape_id: int, scale: float)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetScale`
- 示例：

    ```python
    lib.api.set_scale(1, 1)
    ```

### `set_text`

`lib.api.set_text(shape_id: int, text: str)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeSetText`
- 示例：

    ```python
    lib.api.set_text(1, "hello")
    ```

### `update`

`lib.api.update(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeUpdate`
- 示例：

    ```python
    lib.api.update(1)
    ```

### `update_to_dimension`

`lib.api.update_to_dimension(shape_id: int, dim_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `dim_id` : `int`<br>维度 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeUpdateToDimension`
- 示例：

    ```python
    lib.api.update_to_dimension(1, 0)
    ```

### `update_to_player`

`lib.api.update_to_player(shape_id: int, player_name: str)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`shapeUpdateToPlayer`
- 示例：

    ```python
    lib.api.update_to_player(1, "Steve")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
