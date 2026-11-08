# 自定义实体 · Endstone

协议层自定义实体：给客户端下发一个原版实体（村民、盔甲架、任何 `minecraft:` 实体），位置、朝向、缩放、名称牌、装备、变体、可见玩家都能改，服务端不占用实体系统。

视觉上等价于真实体，但**没有 AI、不会动、不进碰撞箱**，也不会被其他插件当成实体处理。

## 快速开始

```python
eid = lib.entities.create_advanced("minecraft:villager_v2", 0, 64, 0, 0,
                                  yaw=90.0, scale=1.0, nametag="§e商人")
lib.entities.set_equipment_slot(eid, 0, "minecraft:emerald")
lib.entities.set_variant(eid, 3)
lib.entities.set_pose(eid, 1)          # 下蹲
```

## 函数

### `clear_mob_properties`

`lib.entities.clear_mob_properties(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityClearMobProperties`
- 示例：

    ```python
    lib.entities.clear_mob_properties(1)
    ```

### `clear_player_appearance`

`lib.entities.clear_player_appearance(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityClearPlayerAppearance`
- 示例：

    ```python
    lib.entities.clear_player_appearance(1, "Steve")
    ```

### `clear_player_rotation`

`lib.entities.clear_player_rotation(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityClearPlayerRotation`
- 示例：

    ```python
    lib.entities.clear_player_rotation(1, "Steve")
    ```

### `clear_player_rotations`

`lib.entities.clear_player_rotations(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityClearPlayerRotations`
- 示例：

    ```python
    lib.entities.clear_player_rotations(1)
    ```

### `clear_ride`

`lib.entities.clear_ride(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityClearRide`
- 示例：

    ```python
    lib.entities.clear_ride(1)
    ```

### `clear_spawns`

`lib.entities.clear_spawns()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityClearSpawns`
- 示例：

    ```python
    lib.entities.clear_spawns()
    ```

### `clear_visible_players`

`lib.entities.clear_visible_players(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityClearVisiblePlayers`
- 示例：

    ```python
    lib.entities.clear_visible_players(1)
    ```

### `create`

`lib.entities.create(identifier, x, y, z, dim)`

- 参数：
  - `identifier` : `String`<br>实体标识符，如 `minecraft:villager_v2`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`entityCreate`
- 示例：

    ```python
    lib.entities.create("minecraft:cow", 0.0, 64.0, 0.0, 0)
    ```

### `create_advanced`

`lib.entities.create_advanced(identifier, x, y, z, dim, yaw = 0.0, pitch = 0.0, scale = 1.0, nametag = '', viewers = None)`

- 参数：
  - `identifier` : `String`<br>实体标识符，如 `minecraft:villager_v2`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `yaw` : `Float`<br>水平朝向（度）（默认 `0.0`）
  - `pitch` : `Float`<br>俯仰角（度）（默认 `0.0`）
  - `scale` : `Float`<br>缩放倍率，1.0 = 原始大小（默认 `1.0`）
  - `nametag` : `Any`（默认 `''`）
  - `viewers` : `Any`（默认 `None`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`entityCreateAdvanced`
- 示例：

    ```python
    lib.entities.create_advanced("minecraft:cow", 0.0, 64.0, 0.0, 0, "...", "...", "...", "...", "...")
    ```

### `create_random`

`lib.entities.create_random(identifier, x, y, z, dim)`

- 参数：
  - `identifier` : `String`<br>实体标识符，如 `minecraft:villager_v2`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`entityCreateRandom`
- 示例：

    ```python
    lib.entities.create_random("minecraft:cow", 0.0, 64.0, 0.0, 0)
    ```

### `create_with_id`

`lib.entities.create_with_id(identifier, x, y, z, dim, desired_id)`

- 参数：
  - `identifier` : `String`<br>实体标识符，如 `minecraft:villager_v2`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `desired_id` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`entityCreateWithId`
- 示例：

    ```python
    lib.entities.create_with_id("minecraft:cow", 0.0, 64.0, 0.0, 0, "...")
    ```

### `destroy`

`lib.entities.destroy(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityDestroy`
- 示例：

    ```python
    lib.entities.destroy(1)
    ```

### `destroy_all`

`lib.entities.destroy_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`entityDestroyAll`
- 示例：

    ```python
    lib.entities.destroy_all()
    ```

### `exists`

`lib.entities.exists(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityExists`
- 示例：

    ```python
    lib.entities.exists(1)
    ```

### `find_nearest`

`lib.entities.find_nearest(x, y, z, dim, max_dist)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `max_dist` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`entityFindNearest`
- 示例：

    ```python
    lib.entities.find_nearest(0.0, 64.0, 0.0, 0, "...")
    ```

### `get_all_ids`

`lib.entities.get_all_ids()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`entityGetAllIds`
- 示例：

    ```python
    lib.entities.get_all_ids()
    ```

### `get_info`

`lib.entities.get_info(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`entityGetInfo`
- 示例：

    ```python
    lib.entities.get_info(1)
    ```

### `is_id_used`

`lib.entities.is_id_used(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityIsIdUsed`
- 示例：

    ```python
    lib.entities.is_id_used(1)
    ```

### `play_animation`

`lib.entities.play_animation(entity_id: int, animation: str, stop_expression: str = '', duration_ticks: int = 0)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `animation` : `str`<br>原版动画名，如 `animation.player.attack.positions`
  - `stop_expression` : `str`<br>停止表达式（molang）；空 = 由 duration 决定（默认 `''`）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityPlayAnimation`
- 示例：

    ```python
    lib.entities.play_animation(1, "...", "...", 1)
    ```

### `play_animation_to`

`lib.entities.play_animation_to(entity_id: int, player_name: str, animation: str, stop_expression: str = '', duration_ticks: int = 0)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `animation` : `str`<br>原版动画名，如 `animation.player.attack.positions`
  - `stop_expression` : `str`<br>停止表达式（molang）；空 = 由 duration 决定（默认 `''`）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityPlayAnimationTo`
- 示例：

    ```python
    lib.entities.play_animation_to(1, "Steve", "...", "...", 1)
    ```

### `poll_spawns`

`lib.entities.poll_spawns()`

- 参数：无
- 返回值：实体对玩家的 spawn 事件：需要入站包解析（未实现）。
- 返回值类型：`list[str]`
- 对应 LSE 导出：`entityPollSpawns`
- 示例：

    ```python
    lib.entities.poll_spawns()
    ```

### `scale_by`

`lib.entities.scale_by(entity_id: int, factor: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `factor` : `float`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityScaleBy`
- 示例：

    ```python
    lib.entities.scale_by(1, 1)
    ```

### `set_color_index`

`lib.entities.set_color_index(entity_id: int, color_index: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `color_index` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetColorIndex`
- 示例：

    ```python
    lib.entities.set_color_index(1, 1)
    ```

### `set_enabled`

`lib.entities.set_enabled(entity_id: int, enabled: bool)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `enabled` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetEnabled`
- 示例：

    ```python
    lib.entities.set_enabled(1, "...")
    ```

### `set_equipment_slot`

`lib.entities.set_equipment_slot(entity_id: int, slot: int, name: str, aux: int = 0, nbt: str = '')`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `slot` : `int`<br>槽位号
  - `name` : `str`<br>名字 / 标识符
  - `aux` : `int`<br>物品数据值（damage / aux）（默认 `0`）
  - `nbt` : `str`<br>SNBT 字符串（可空）（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetEquipmentSlot`
- 示例：

    ```python
    lib.entities.set_equipment_slot(1, 1, "hello", 1, "...")
    ```

### `set_flags`

`lib.entities.set_flags(entity_id: int, flags: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `flags` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetFlags`
- 示例：

    ```python
    lib.entities.set_flags(1, 1)
    ```

### `set_identifier`

`lib.entities.set_identifier(entity_id: int, identifier: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `identifier` : `str`<br>实体标识符，如 `minecraft:villager_v2`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetIdentifier`
- 示例：

    ```python
    lib.entities.set_identifier(1, "minecraft:cow")
    ```

### `set_invisible`

`lib.entities.set_invisible(entity_id: int, on: bool)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `on` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetInvisible`
- 示例：

    ```python
    lib.entities.set_invisible(1, "...")
    ```

### `set_mark_variant`

`lib.entities.set_mark_variant(entity_id: int, mark_variant: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `mark_variant` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetMarkVariant`
- 示例：

    ```python
    lib.entities.set_mark_variant(1, 1)
    ```

### `set_mob_property`

`lib.entities.set_mob_property(entity_id: int, name: str, value: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `name` : `str`<br>名字 / 标识符
  - `value` : `str`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetMobProperty`
- 示例：

    ```python
    lib.entities.set_mob_property(1, "hello", "...")
    ```

### `set_nametag`

`lib.entities.set_nametag(entity_id: int, text: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetNametag`
- 示例：

    ```python
    lib.entities.set_nametag(1, "hello")
    ```

### `set_player_equipment_slot`

`lib.entities.set_player_equipment_slot(entity_id: int, player_name: str, slot: int, name: str, aux: int = 0, nbt: str = '')`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `slot` : `int`<br>槽位号
  - `name` : `str`<br>名字 / 标识符
  - `aux` : `int`<br>物品数据值（damage / aux）（默认 `0`）
  - `nbt` : `str`<br>SNBT 字符串（可空）（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetPlayerEquipmentSlot`
- 示例：

    ```python
    lib.entities.set_player_equipment_slot(1, "Steve", 1, "hello", 1, "...")
    ```

### `set_player_nametag`

`lib.entities.set_player_nametag(entity_id: int, player_name: str, text: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetPlayerNametag`
- 示例：

    ```python
    lib.entities.set_player_nametag(1, "Steve", "hello")
    ```

### `set_player_rotation`

`lib.entities.set_player_rotation(entity_id: int, player_name: str, yaw: float, pitch: float = 0.0)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `yaw` : `float`<br>水平朝向（度）
  - `pitch` : `float`<br>俯仰角（度）（默认 `0.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetPlayerRotation`
- 示例：

    ```python
    lib.entities.set_player_rotation(1, "Steve", 1, 1)
    ```

### `set_player_scale`

`lib.entities.set_player_scale(entity_id: int, player_name: str, scale: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetPlayerScale`
- 示例：

    ```python
    lib.entities.set_player_scale(1, "Steve", 1)
    ```

### `set_pose`

`lib.entities.set_pose(entity_id: int, pose: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `pose` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetPose`
- 示例：

    ```python
    lib.entities.set_pose(1, 1)
    ```

### `set_position`

`lib.entities.set_position(entity_id: int, x, y, z, dim = None)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `None`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetPosition`
- 示例：

    ```python
    lib.entities.set_position(1, 0.0, 64.0, 0.0, 0)
    ```

### `set_ride_entity`

`lib.entities.set_ride_entity(entity_id: int, vehicle_entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `vehicle_entity_id` : `int`<br>载具的库内实体 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetRideEntity`
- 示例：

    ```python
    lib.entities.set_ride_entity(1, 1)
    ```

### `set_ride_player`

`lib.entities.set_ride_player(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：骑真实玩家需要 actor uniqueId，endstone 只给 uuid，暂不支持。
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetRidePlayer`
- 示例：

    ```python
    lib.entities.set_ride_player(1, "Steve")
    ```

### `set_rotation`

`lib.entities.set_rotation(entity_id: int, yaw: float, pitch: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `yaw` : `float`<br>水平朝向（度）
  - `pitch` : `float`<br>俯仰角（度）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetRotation`
- 示例：

    ```python
    lib.entities.set_rotation(1, 1, 1)
    ```

### `set_scale`

`lib.entities.set_scale(entity_id: int, scale: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetScale`
- 示例：

    ```python
    lib.entities.set_scale(1, 1)
    ```

### `set_variant`

`lib.entities.set_variant(entity_id: int, variant: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `variant` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetVariant`
- 示例：

    ```python
    lib.entities.set_variant(1, 1)
    ```

### `set_view_distance`

`lib.entities.set_view_distance(entity_id: int, dist: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `dist` : `float`<br>距离（格）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetViewDistance`
- 示例：

    ```python
    lib.entities.set_view_distance(1, 1)
    ```

### `set_visible_player`

`lib.entities.set_visible_player(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetVisiblePlayer`
- 示例：

    ```python
    lib.entities.set_visible_player(1, "Steve")
    ```

### `set_visible_players`

`lib.entities.set_visible_players(entity_id: int, names)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `names` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entitySetVisiblePlayers`
- 示例：

    ```python
    lib.entities.set_visible_players(1, "...")
    ```

### `stop_animation`

`lib.entities.stop_animation(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityStopAnimation`
- 示例：

    ```python
    lib.entities.stop_animation(1)
    ```

### `stop_animation_to`

`lib.entities.stop_animation_to(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`entityStopAnimationTo`
- 示例：

    ```python
    lib.entities.stop_animation_to(1, "Steve")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
