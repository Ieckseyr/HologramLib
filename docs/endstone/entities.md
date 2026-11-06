# 自定义实体 · Endstone

协议层自定义实体：给客户端下发一个原版实体（村民、盔甲架、任何 `minecraft:` 实体），位置、朝向、缩放、名称牌、装备、变体、可见玩家都能改，服务端不占用实体系统。

视觉上等价于真实体，但**没有 AI、不会动、不进碰撞箱**，也不会被其他插件当成实体处理。

## 快速上手

```python
eid = lib.entities.create_advanced("minecraft:villager_v2", 0, 64, 0, 0,
                                  yaw=90.0, scale=1.0, nametag="§e商人")
lib.entities.set_equipment_slot(eid, 0, "minecraft:emerald")
lib.entities.set_variant(eid, 3)
lib.entities.set_pose(eid, 1)          # 下蹲
```

## 函数（47 个）

#### `clear_mob_properties`

`lib.entities.clear_mob_properties(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `entityClearMobProperties`

#### `clear_player_appearance`

`lib.entities.clear_player_appearance(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `entityClearPlayerAppearance`

#### `clear_player_rotation`

`lib.entities.clear_player_rotation(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `entityClearPlayerRotation`

#### `clear_player_rotations`

`lib.entities.clear_player_rotations(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `entityClearPlayerRotations`

#### `clear_ride`

`lib.entities.clear_ride(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `entityClearRide`

#### `clear_spawns`

`lib.entities.clear_spawns()`

- 参数：无
- 返回值类型：`bool`
- 对应 LSE： `entityClearSpawns`

#### `clear_visible_players`

`lib.entities.clear_visible_players(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `entityClearVisiblePlayers`

#### `create`

`lib.entities.create(identifier, x, y, z, dim)`

- 参数：
  - `identifier` : `String`<br>实体标识符，如 `minecraft:villager_v2`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
- 返回值类型：`int`
- 对应 LSE： `entityCreate`

#### `create_advanced`

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
- 返回值类型：`int`
- 对应 LSE： `entityCreateAdvanced`

#### `create_random`

`lib.entities.create_random(identifier, x, y, z, dim)`

- 参数：
  - `identifier` : `String`<br>实体标识符，如 `minecraft:villager_v2`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
- 返回值类型：`int`
- 对应 LSE： `entityCreateRandom`

#### `create_with_id`

`lib.entities.create_with_id(identifier, x, y, z, dim, desired_id)`

- 参数：
  - `identifier` : `String`<br>实体标识符，如 `minecraft:villager_v2`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `desired_id` : `Any`
- 返回值类型：`int`
- 对应 LSE： `entityCreateWithId`

#### `destroy`

`lib.entities.destroy(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `entityDestroy`

#### `destroy_all`

`lib.entities.destroy_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `entityDestroyAll`

#### `exists`

`lib.entities.exists(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `entityExists`

#### `find_nearest`

`lib.entities.find_nearest(x, y, z, dim, max_dist)`

- 参数：
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `max_dist` : `Any`
- 返回值类型：`int`
- 对应 LSE： `entityFindNearest`

#### `get_all_ids`

`lib.entities.get_all_ids()`

- 参数：无
- 返回值类型：`list[int]`
- 对应 LSE： `entityGetAllIds`

#### `get_info`

`lib.entities.get_info(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`str`
- 对应 LSE： `entityGetInfo`

#### `is_id_used`

`lib.entities.is_id_used(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `entityIsIdUsed`

#### `play_animation`

`lib.entities.play_animation(entity_id: int, animation: str, stop_expression: str = '', duration_ticks: int = 0)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `animation` : `str`<br>原版动画名，如 `animation.player.attack.positions`
  - `stop_expression` : `str`<br>停止表达式（molang）；空 = 由 duration 决定（默认 `''`）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `0`）
- 返回值类型：`bool`
- 对应 LSE： `entityPlayAnimation`

#### `play_animation_to`

`lib.entities.play_animation_to(entity_id: int, player_name: str, animation: str, stop_expression: str = '', duration_ticks: int = 0)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `animation` : `str`<br>原版动画名，如 `animation.player.attack.positions`
  - `stop_expression` : `str`<br>停止表达式（molang）；空 = 由 duration 决定（默认 `''`）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `0`）
- 返回值类型：`bool`
- 对应 LSE： `entityPlayAnimationTo`

#### `poll_spawns`

`lib.entities.poll_spawns()`

- 参数：无
- 返回值类型：`list[str]`
- 对应 LSE： `entityPollSpawns`
- 说明：实体对玩家的 spawn 事件：需要入站包解析（未实现）。

#### `scale_by`

`lib.entities.scale_by(entity_id: int, factor: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `factor` : `float`
- 返回值类型：`bool`
- 对应 LSE： `entityScaleBy`

#### `set_color_index`

`lib.entities.set_color_index(entity_id: int, color_index: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `color_index` : `int`
- 返回值类型：`bool`
- 对应 LSE： `entitySetColorIndex`

#### `set_enabled`

`lib.entities.set_enabled(entity_id: int, enabled: bool)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `enabled` : `bool`<br>开 / 关
- 返回值类型：`bool`
- 对应 LSE： `entitySetEnabled`

#### `set_equipment_slot`

`lib.entities.set_equipment_slot(entity_id: int, slot: int, name: str, aux: int = 0, nbt: str = '')`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `slot` : `int`<br>槽位号
  - `name` : `str`<br>名字 / 标识符
  - `aux` : `int`<br>物品数据值（damage / aux）（默认 `0`）
  - `nbt` : `str`<br>SNBT 字符串（可空）（默认 `''`）
- 返回值类型：`bool`
- 对应 LSE： `entitySetEquipmentSlot`

#### `set_flags`

`lib.entities.set_flags(entity_id: int, flags: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `flags` : `int`
- 返回值类型：`bool`
- 对应 LSE： `entitySetFlags`

#### `set_identifier`

`lib.entities.set_identifier(entity_id: int, identifier: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `identifier` : `str`<br>实体标识符，如 `minecraft:villager_v2`
- 返回值类型：`bool`
- 对应 LSE： `entitySetIdentifier`

#### `set_invisible`

`lib.entities.set_invisible(entity_id: int, on: bool)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `on` : `bool`<br>开 / 关
- 返回值类型：`bool`
- 对应 LSE： `entitySetInvisible`

#### `set_mark_variant`

`lib.entities.set_mark_variant(entity_id: int, mark_variant: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `mark_variant` : `int`
- 返回值类型：`bool`
- 对应 LSE： `entitySetMarkVariant`

#### `set_mob_property`

`lib.entities.set_mob_property(entity_id: int, name: str, value: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `name` : `str`<br>名字 / 标识符
  - `value` : `str`
- 返回值类型：`bool`
- 对应 LSE： `entitySetMobProperty`

#### `set_nametag`

`lib.entities.set_nametag(entity_id: int, text: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值类型：`bool`
- 对应 LSE： `entitySetNametag`

#### `set_player_equipment_slot`

`lib.entities.set_player_equipment_slot(entity_id: int, player_name: str, slot: int, name: str, aux: int = 0, nbt: str = '')`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `slot` : `int`<br>槽位号
  - `name` : `str`<br>名字 / 标识符
  - `aux` : `int`<br>物品数据值（damage / aux）（默认 `0`）
  - `nbt` : `str`<br>SNBT 字符串（可空）（默认 `''`）
- 返回值类型：`bool`
- 对应 LSE： `entitySetPlayerEquipmentSlot`

#### `set_player_nametag`

`lib.entities.set_player_nametag(entity_id: int, player_name: str, text: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值类型：`bool`
- 对应 LSE： `entitySetPlayerNametag`

#### `set_player_rotation`

`lib.entities.set_player_rotation(entity_id: int, player_name: str, yaw: float, pitch: float = 0.0)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `yaw` : `float`<br>水平朝向（度）
  - `pitch` : `float`<br>俯仰角（度）（默认 `0.0`）
- 返回值类型：`bool`
- 对应 LSE： `entitySetPlayerRotation`

#### `set_player_scale`

`lib.entities.set_player_scale(entity_id: int, player_name: str, scale: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值类型：`bool`
- 对应 LSE： `entitySetPlayerScale`

#### `set_pose`

`lib.entities.set_pose(entity_id: int, pose: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `pose` : `int`
- 返回值类型：`bool`
- 对应 LSE： `entitySetPose`

#### `set_position`

`lib.entities.set_position(entity_id: int, x, y, z, dim = None)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `Int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `None`）
- 返回值类型：`bool`
- 对应 LSE： `entitySetPosition`

#### `set_ride_entity`

`lib.entities.set_ride_entity(entity_id: int, vehicle_entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `vehicle_entity_id` : `int`<br>载具的库内实体 id
- 返回值类型：`bool`
- 对应 LSE： `entitySetRideEntity`

#### `set_ride_player`

`lib.entities.set_ride_player(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `entitySetRidePlayer`
- 说明：骑真实玩家需要 actor uniqueId，endstone 只给 uuid，暂不支持。

#### `set_rotation`

`lib.entities.set_rotation(entity_id: int, yaw: float, pitch: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `yaw` : `float`<br>水平朝向（度）
  - `pitch` : `float`<br>俯仰角（度）
- 返回值类型：`bool`
- 对应 LSE： `entitySetRotation`

#### `set_scale`

`lib.entities.set_scale(entity_id: int, scale: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值类型：`bool`
- 对应 LSE： `entitySetScale`

#### `set_variant`

`lib.entities.set_variant(entity_id: int, variant: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `variant` : `int`
- 返回值类型：`bool`
- 对应 LSE： `entitySetVariant`

#### `set_view_distance`

`lib.entities.set_view_distance(entity_id: int, dist: float)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `dist` : `float`<br>距离（格）
- 返回值类型：`bool`
- 对应 LSE： `entitySetViewDistance`

#### `set_visible_player`

`lib.entities.set_visible_player(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `entitySetVisiblePlayer`

#### `set_visible_players`

`lib.entities.set_visible_players(entity_id: int, names)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `names` : `Any`
- 返回值类型：`bool`
- 对应 LSE： `entitySetVisiblePlayers`

#### `stop_animation`

`lib.entities.stop_animation(entity_id: int)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `entityStopAnimation`

#### `stop_animation_to`

`lib.entities.stop_animation_to(entity_id: int, player_name: str)`

- 参数：
  - `entity_id` : `int`<br>实体 id（`lib.entities.create(...)` 的返回值）
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `entityStopAnimationTo`


---

完整索引见 [Endstone API 索引](api.md)。
