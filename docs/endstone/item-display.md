# 物品展示 · Endstone

3D 物品展示：把物品贴在空间里旋转、缩放、拉伸，支持跟随玩家、只对某人可见、发光（附魔光效）、自定义 NBT。适合做旋转展台、掉落物特效、UI 装饰。

`set_*_live` 是「立即生效」版本，`tick()` 驱动旋转和跟随（插件已自动调用）。

彩蛋版见 `easter_egg(displayId, durationTicks, rotate)` —— 朋友的⑨：_昨日一去不回，噢耶耶，开心比什么都贵........_

## 快速开始

```python
did = lib.itemdisplay.create(0, 66, 0, 0, "minecraft:diamond_sword")
lib.itemdisplay.set_scale(did, 2.0)
lib.itemdisplay.set_anim(did, 30.0, 1)      # 每 tick 绕 Y 轴 30°
lib.itemdisplay.set_glint(did, True)
```

## 函数

### `clear_anim`

`lib.itemdisplay.clear_anim(display_id: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayClearAnim`
- 示例：

    ```python
    lib.itemdisplay.clear_anim(1)
    ```

### `clear_visible_players`

`lib.itemdisplay.clear_visible_players(display_id: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayClearVisiblePlayers`
- 示例：

    ```python
    lib.itemdisplay.clear_visible_players(1)
    ```

### `create`

`lib.itemdisplay.create(x, y, z, dim, item, aux = 0)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `item` : `String`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `Int`<br>物品数据值（damage / aux）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayCreate`
- 示例：

    ```python
    lib.itemdisplay.create(0.0, 64.0, 0.0, 0, "minecraft:diamond", "...")
    ```

### `create_advanced`

`lib.itemdisplay.create_advanced(x, y, z, dim, item, aux, off_x, off_y, off_z, rot_x, rot_y, rot_z, scale)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `item` : `String`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `Int`<br>物品数据值（damage / aux）
  - `off_x` : `Any`
  - `off_y` : `Any`
  - `off_z` : `Any`
  - `rot_x` : `Any`
  - `rot_y` : `Any`
  - `rot_z` : `Any`
  - `scale` : `Float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayCreateAdvanced`
- 示例：

    ```python
    lib.itemdisplay.create_advanced(0.0, 64.0, 0.0, 0, "minecraft:diamond", "...", "...", "...", "...", "...", "...", "...", "...")
    ```

### `create_full`

`lib.itemdisplay.create_full(x, y, z, dim, item, aux, off_x, off_y, off_z, rot_x, rot_y, rot_z, scale, nbt = '', glint = False)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `item` : `String`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `Int`<br>物品数据值（damage / aux）
  - `off_x` : `Any`
  - `off_y` : `Any`
  - `off_z` : `Any`
  - `rot_x` : `Any`
  - `rot_y` : `Any`
  - `rot_z` : `Any`
  - `scale` : `Float`<br>缩放倍率，1.0 = 原始大小
  - `nbt` : `String`<br>SNBT 字符串（可空）（默认 `''`）
  - `glint` : `Any`（默认 `False`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayCreateFull`
- 示例：

    ```python
    lib.itemdisplay.create_full(0.0, 64.0, 0.0, 0, "minecraft:diamond", "...", "...", "...", "...", "...", "...", "...", "...", "...", "...")
    ```

### `create_random`

`lib.itemdisplay.create_random(x, y, z, dim, item, aux = 0)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `item` : `String`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `Int`<br>物品数据值（damage / aux）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayCreateRandom`
- 示例：

    ```python
    lib.itemdisplay.create_random(0.0, 64.0, 0.0, 0, "minecraft:diamond", "...")
    ```

### `create_seamless`

`lib.itemdisplay.create_seamless(x, y, z, dim, item, aux, off_x, off_y, off_z, rot_x, rot_y, rot_z, scale, mode, view_distance, visible_player)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `item` : `String`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `Int`<br>物品数据值（damage / aux）
  - `off_x` : `Any`
  - `off_y` : `Any`
  - `off_z` : `Any`
  - `rot_x` : `Any`
  - `rot_y` : `Any`
  - `rot_z` : `Any`
  - `scale` : `Float`<br>缩放倍率，1.0 = 原始大小
  - `mode` : `Any`
  - `view_distance` : `Any`
  - `visible_player` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayCreateSeamless`
- 示例：

    ```python
    lib.itemdisplay.create_seamless(0.0, 64.0, 0.0, 0, "minecraft:diamond", "...", "...", "...", "...", "...", "...", "...", "...", "...", "...", "...")
    ```

### `create_with_id`

`lib.itemdisplay.create_with_id(x, y, z, dim, item, aux, desired_id)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `item` : `String`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `Int`<br>物品数据值（damage / aux）
  - `desired_id` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayCreateWithId`
- 示例：

    ```python
    lib.itemdisplay.create_with_id(0.0, 64.0, 0.0, 0, "minecraft:diamond", "...", "...")
    ```

### `destroy`

`lib.itemdisplay.destroy(display_id: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayDestroy`
- 示例：

    ```python
    lib.itemdisplay.destroy(1)
    ```

### `destroy_all`

`lib.itemdisplay.destroy_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayDestroyAll`
- 示例：

    ```python
    lib.itemdisplay.destroy_all()
    ```

### `emit_for_entity`

`lib.itemdisplay.emit_for_entity(player_name: str, unique_id: int, item: str, aux: int = 0, offset_y: float = 0.0, rot_x: float = 0.0, rot_y: float = 0.0, rot_z: float = 0.0, scale: float = 1.0, nbt: str = '', glint: bool = False)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `unique_id` : `int`<br>uniqueId
  - `item` : `str`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `int`<br>物品数据值（damage / aux）（默认 `0`）
  - `offset_y` : `float`（默认 `0.0`）
  - `rot_x` : `float`（默认 `0.0`）
  - `rot_y` : `float`（默认 `0.0`）
  - `rot_z` : `float`（默认 `0.0`）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小（默认 `1.0`）
  - `nbt` : `str`<br>SNBT 字符串（可空）（默认 `''`）
  - `glint` : `bool`（默认 `False`）
- 返回值：在指定实体位置给指定玩家生成一个展示（一次性）。
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayEmitForEntity`
- 示例：

    ```python
    lib.itemdisplay.emit_for_entity("Steve", 1, "minecraft:diamond", 1, 1, 1, 1, 1, 1, "...", "...")
    ```

### `exists`

`lib.itemdisplay.exists(display_id: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayExists`
- 示例：

    ```python
    lib.itemdisplay.exists(1)
    ```

### `find_nearest`

`lib.itemdisplay.find_nearest(x, y, z, dim, max_dist)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `max_dist` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayFindNearest`
- 示例：

    ```python
    lib.itemdisplay.find_nearest(0.0, 64.0, 0.0, 0, "...")
    ```

### `follow`

`lib.itemdisplay.follow(display_id: int, player_name: str, off_x = 0.0, off_y = 0.0, off_z = 0.0)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `off_x` : `Any`（默认 `0.0`）
  - `off_y` : `Any`（默认 `0.0`）
  - `off_z` : `Any`（默认 `0.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayFollow`
- 示例：

    ```python
    lib.itemdisplay.follow(1, "Steve", "...", "...", "...")
    ```

### `get_all_ids`

`lib.itemdisplay.get_all_ids()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`itemDisplayGetAllIds`
- 示例：

    ```python
    lib.itemdisplay.get_all_ids()
    ```

### `get_info`

`lib.itemdisplay.get_info(display_id: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`itemDisplayGetInfo`
- 示例：

    ```python
    lib.itemdisplay.get_info(1)
    ```

### `is_id_used`

`lib.itemdisplay.is_id_used(display_id: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayIsIdUsed`
- 示例：

    ```python
    lib.itemdisplay.is_id_used(1)
    ```

### `rotate_y`

`lib.itemdisplay.rotate_y(display_id: int, delta: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `delta` : `float`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayRotateY`
- 示例：

    ```python
    lib.itemdisplay.rotate_y(1, 1)
    ```

### `runtime_id`

`lib.itemdisplay.runtime_id(display_id: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDisplayRuntimeId`
- 示例：

    ```python
    lib.itemdisplay.runtime_id(1)
    ```

### `scale_by`

`lib.itemdisplay.scale_by(display_id: int, factor: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `factor` : `float`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayScaleBy`
- 示例：

    ```python
    lib.itemdisplay.scale_by(1, 1)
    ```

### `scale_to`

`lib.itemdisplay.scale_to(display_id: int, target: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `target` : `float`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayScaleTo`
- 示例：

    ```python
    lib.itemdisplay.scale_to(1, 1)
    ```

### `set_anim`

`lib.itemdisplay.set_anim(display_id: int, spin_speed: float = 0.0, spin_axis: int = 1, bob_amp: float = 0.0, bob_period: float = 0.0, pulse_amp: float = 0.0, pulse_period: float = 0.0, sway_amp: float = 0.0, sway_period: float = 0.0, phase: float = 0.0, billboard: bool = False)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `spin_speed` : `float`（默认 `0.0`）
  - `spin_axis` : `int`（默认 `1`）
  - `bob_amp` : `float`（默认 `0.0`）
  - `bob_period` : `float`（默认 `0.0`）
  - `pulse_amp` : `float`（默认 `0.0`）
  - `pulse_period` : `float`（默认 `0.0`）
  - `sway_amp` : `float`（默认 `0.0`）
  - `sway_period` : `float`（默认 `0.0`）
  - `phase` : `float`（默认 `0.0`）
  - `billboard` : `bool`（默认 `False`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetAnim`
- 示例：

    ```python
    lib.itemdisplay.set_anim(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, "...")
    ```

### `set_base_offset`

`lib.itemdisplay.set_base_offset(display_id: int, ox, oy, oz)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `ox` : `Any`
  - `oy` : `Any`
  - `oz` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetBaseOffset`
- 示例：

    ```python
    lib.itemdisplay.set_base_offset(1, "...", "...", "...")
    ```

### `set_enabled`

`lib.itemdisplay.set_enabled(display_id: int, enabled: bool)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `enabled` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetEnabled`
- 示例：

    ```python
    lib.itemdisplay.set_enabled(1, "...")
    ```

### `set_extend`

`lib.itemdisplay.set_extend(display_id: int, extend: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `extend` : `float`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetExtend`
- 示例：

    ```python
    lib.itemdisplay.set_extend(1, 1)
    ```

### `set_glint`

`lib.itemdisplay.set_glint(display_id: int, on: bool)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `on` : `bool`<br>开 / 关
- 返回值：记录附魔光效；线格式需要 NBT tag（本版未接）。
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetGlint`
- 示例：

    ```python
    lib.itemdisplay.set_glint(1, "...")
    ```

### `set_hitbox`

`lib.itemdisplay.set_hitbox(display_id: int, width: float, height: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `width` : `float`<br>宽
  - `height` : `float`<br>高
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetHitbox`
- 示例：

    ```python
    lib.itemdisplay.set_hitbox(1, 1, 1)
    ```

### `set_item`

`lib.itemdisplay.set_item(display_id: int, item: str, aux = 0)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `item` : `str`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `Int`<br>物品数据值（damage / aux）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetItem`
- 示例：

    ```python
    lib.itemdisplay.set_item(1, "minecraft:diamond", "...")
    ```

### `set_item_with_nbt`

`lib.itemdisplay.set_item_with_nbt(display_id: int, item: str, aux = 0, nbt: str = '')`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `item` : `str`<br>物品标识符，如 `minecraft:diamond`
  - `aux` : `Int`<br>物品数据值（damage / aux）（默认 `0`）
  - `nbt` : `str`<br>SNBT 字符串（可空）（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetItemWithNbt`
- 示例：

    ```python
    lib.itemdisplay.set_item_with_nbt(1, "minecraft:diamond", "...", "...")
    ```

### `set_mode`

`lib.itemdisplay.set_mode(display_id: int, mode: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `mode` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetMode`
- 示例：

    ```python
    lib.itemdisplay.set_mode(1, 1)
    ```

### `set_offset`

`lib.itemdisplay.set_offset(display_id: int, ox, oy, oz)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `ox` : `Any`
  - `oy` : `Any`
  - `oz` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetOffset`
- 示例：

    ```python
    lib.itemdisplay.set_offset(1, "...", "...", "...")
    ```

### `set_offset_live`

`lib.itemdisplay.set_offset_live(display_id: int, ox, oy, oz, persist: bool = True)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `ox` : `Any`
  - `oy` : `Any`
  - `oz` : `Any`
  - `persist` : `bool`（默认 `True`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetOffsetLive`
- 示例：

    ```python
    lib.itemdisplay.set_offset_live(1, "...", "...", "...", "...")
    ```

### `set_position`

`lib.itemdisplay.set_position(display_id: int, x, y, z, dim = None)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `None`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetPosition`
- 示例：

    ```python
    lib.itemdisplay.set_position(1, 0.0, 64.0, 0.0, 0)
    ```

### `set_preset`

`lib.itemdisplay.set_preset(display_id: int, preset: str, a: float = 0.0, b: float = 0.0, c: float = 0.0)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `preset` : `str`
  - `a` : `float`<br>透明度 0~1（默认 `0.0`）
  - `b` : `float`<br>蓝色分量 0~1（默认 `0.0`）
  - `c` : `float`（默认 `0.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetPreset`
- 示例：

    ```python
    lib.itemdisplay.set_preset(1, "...", 1, 1, 1)
    ```

### `set_rotation`

`lib.itemdisplay.set_rotation(display_id: int, rx, ry, rz)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `rx` : `Any`
  - `ry` : `Any`
  - `rz` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetRotation`
- 示例：

    ```python
    lib.itemdisplay.set_rotation(1, "...", "...", "...")
    ```

### `set_rotation_live`

`lib.itemdisplay.set_rotation_live(display_id: int, rx, ry, rz)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `rx` : `Any`
  - `ry` : `Any`
  - `rz` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetRotationLive`
- 示例：

    ```python
    lib.itemdisplay.set_rotation_live(1, "...", "...", "...")
    ```

### `set_scale`

`lib.itemdisplay.set_scale(display_id: int, scale: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetScale`
- 示例：

    ```python
    lib.itemdisplay.set_scale(1, 1)
    ```

### `set_scale_live`

`lib.itemdisplay.set_scale_live(display_id: int, scale: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：记录缩放值；客户端表现依赖 molang 动画（本版未接），因此不改包。
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetScaleLive`
- 示例：

    ```python
    lib.itemdisplay.set_scale_live(1, 1)
    ```

### `set_stretch`

`lib.itemdisplay.set_stretch(display_id: int, xz: float, y: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `xz` : `float`
  - `y` : `float`<br>Y 坐标
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetStretch`
- 示例：

    ```python
    lib.itemdisplay.set_stretch(1, 1, 64.0)
    ```

### `set_stretch_live`

`lib.itemdisplay.set_stretch_live(display_id: int, xz: float, y: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `xz` : `float`
  - `y` : `float`<br>Y 坐标
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetStretchLive`
- 示例：

    ```python
    lib.itemdisplay.set_stretch_live(1, 1, 64.0)
    ```

### `set_transform_live`

`lib.itemdisplay.set_transform_live(display_id: int, off_x: float, off_y: float, off_z: float, rot_x: float, rot_y: float, rot_z: float, scale: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `off_x` : `float`
  - `off_y` : `float`
  - `off_z` : `float`
  - `rot_x` : `float`
  - `rot_y` : `float`
  - `rot_z` : `float`
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetTransformLive`
- 示例：

    ```python
    lib.itemdisplay.set_transform_live(1, 1, 1, 1, 1, 1, 1, 1)
    ```

### `set_view_distance`

`lib.itemdisplay.set_view_distance(display_id: int, distance: float)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `distance` : `float`<br>距离（格）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetViewDistance`
- 示例：

    ```python
    lib.itemdisplay.set_view_distance(1, 1)
    ```

### `set_visible_player`

`lib.itemdisplay.set_visible_player(display_id: int, player_name: str)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetVisiblePlayer`
- 示例：

    ```python
    lib.itemdisplay.set_visible_player(1, "Steve")
    ```

### `set_visible_players`

`lib.itemdisplay.set_visible_players(display_id: int, names)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
  - `names` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplaySetVisiblePlayers`
- 示例：

    ```python
    lib.itemdisplay.set_visible_players(1, "...")
    ```

### `unfollow`

`lib.itemdisplay.unfollow(display_id: int)`

- 参数：
  - `display_id` : `int`<br>展示 id（`lib.itemdisplay.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDisplayUnfollow`
- 示例：

    ```python
    lib.itemdisplay.unfollow(1)
    ```


---

完整索引见 [Endstone API 索引](api.md)。
