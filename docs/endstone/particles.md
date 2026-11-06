# 粒子 · Endstone

用原版粒子效果拼形状：点、线、面、盒子、盒子六面、多边形。每个粒子组可以设间隔（多少 tick 喷一次）、生命周期、可见玩家。

粒子按玩家下发，所以 `create_*` 的第一个参数是玩家名；`set_visible_players` 会接管之后发给谁。

## 快速上手

```python
lib.particles.create_point("Steve", 0, 64, 0, "minecraft:end_rod", 4, 100)
lib.particles.create_box("Steve", 0, 0, 64, 0, 2, 2, 2, 0.5, "minecraft:flame", 4, 100)
```

## 函数（27 个）

#### `clear_visible_players`

`lib.particles.clear_visible_players(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `particleClearVisiblePlayers`

#### `create_box`

`lib.particles.create_box(player_name, dim_id, cx, cy, cz, hx, hy, hz, step = 1.0, effect = '', interval = 4, lifetime = 0)`

- 参数：
  - `player_name` : `String`<br>玩家名（在线玩家，大小写敏感）
  - `dim_id` : `Int`<br>维度 id
  - `cx` : `Any`
  - `cy` : `Any`
  - `cz` : `Any`
  - `hx` : `Any`
  - `hy` : `Any`
  - `hz` : `Any`
  - `step` : `Any`（默认 `1.0`）
  - `effect` : `String`<br>粒子效果标识符，如 `minecraft:flame`（默认 `''`）
  - `interval` : `Int`<br>下发间隔（tick）（默认 `4`）
  - `lifetime` : `Int`<br>生命周期（tick），0 = 永久（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `particleCreateBox`

#### `create_box_faces`

`lib.particles.create_box_faces(player_name, dim_id, cx, cy, cz, hx, hy, hz, step = 1.0, effect = '', interval = 4, lifetime = 0)`

- 参数：
  - `player_name` : `String`<br>玩家名（在线玩家，大小写敏感）
  - `dim_id` : `Int`<br>维度 id
  - `cx` : `Any`
  - `cy` : `Any`
  - `cz` : `Any`
  - `hx` : `Any`
  - `hy` : `Any`
  - `hz` : `Any`
  - `step` : `Any`（默认 `1.0`）
  - `effect` : `String`<br>粒子效果标识符，如 `minecraft:flame`（默认 `''`）
  - `interval` : `Int`<br>下发间隔（tick）（默认 `4`）
  - `lifetime` : `Int`<br>生命周期（tick），0 = 永久（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `particleCreateBoxFaces`

#### `create_line`

`lib.particles.create_line(player_name, dim_id, x1, y1, z1, x2, y2, z2, step = 1.0, effect = '', interval = 4, lifetime = 0)`

- 参数：
  - `player_name` : `String`<br>玩家名（在线玩家，大小写敏感）
  - `dim_id` : `Int`<br>维度 id
  - `x1` : `Float`<br>起点 X
  - `y1` : `Float`<br>起点 Y
  - `z1` : `Float`<br>起点 Z
  - `x2` : `Float`<br>终点 X
  - `y2` : `Float`<br>终点 Y
  - `z2` : `Float`<br>终点 Z
  - `step` : `Any`（默认 `1.0`）
  - `effect` : `String`<br>粒子效果标识符，如 `minecraft:flame`（默认 `''`）
  - `interval` : `Int`<br>下发间隔（tick）（默认 `4`）
  - `lifetime` : `Int`<br>生命周期（tick），0 = 永久（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `particleCreateLine`

#### `create_plane`

`lib.particles.create_plane(player_name, dim_id, cx, cy, cz, w, h, axis = 0, step = 1.0, effect = '', interval = 4, lifetime = 0)`

- 参数：
  - `player_name` : `String`<br>玩家名（在线玩家，大小写敏感）
  - `dim_id` : `Int`<br>维度 id
  - `cx` : `Any`
  - `cy` : `Any`
  - `cz` : `Any`
  - `w` : `Any`
  - `h` : `Any`
  - `axis` : `Any`（默认 `0`）
  - `step` : `Any`（默认 `1.0`）
  - `effect` : `String`<br>粒子效果标识符，如 `minecraft:flame`（默认 `''`）
  - `interval` : `Int`<br>下发间隔（tick）（默认 `4`）
  - `lifetime` : `Int`<br>生命周期（tick），0 = 永久（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `particleCreatePlane`

#### `create_point`

`lib.particles.create_point(player_name: str, dim_id: int, x, y, z, effect: str = '', interval: int = 4, lifetime: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `dim_id` : `int`<br>维度 id
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `effect` : `str`<br>粒子效果标识符，如 `minecraft:flame`（默认 `''`）
  - `interval` : `int`<br>下发间隔（tick）（默认 `4`）
  - `lifetime` : `int`<br>生命周期（tick），0 = 永久（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `particleCreatePoint`

#### `create_poly`

`lib.particles.create_poly(player_name, dim_id, verts, edges, step = 1.0, effect = '', interval = 4, lifetime = 0)`

- 参数：
  - `player_name` : `String`<br>玩家名（在线玩家，大小写敏感）
  - `dim_id` : `Int`<br>维度 id
  - `verts` : `Any`
  - `edges` : `Any`
  - `step` : `Any`（默认 `1.0`）
  - `effect` : `String`<br>粒子效果标识符，如 `minecraft:flame`（默认 `''`）
  - `interval` : `Int`<br>下发间隔（tick）（默认 `4`）
  - `lifetime` : `Int`<br>生命周期（tick），0 = 永久（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `particleCreatePoly`
- 说明：verts: "x,y,z;x,y,z;..." 或 [x,y,z,...]；edges: "0,1;1,2" 或 [0,1,1,2]。

#### `create_rect`

`lib.particles.create_rect(player_name, dim_id, cx, cy, cz, w, h, axis = 0, step = 1.0, effect = '', interval = 4, lifetime = 0)`

- 参数：
  - `player_name` : `String`<br>玩家名（在线玩家，大小写敏感）
  - `dim_id` : `Int`<br>维度 id
  - `cx` : `Any`
  - `cy` : `Any`
  - `cz` : `Any`
  - `w` : `Any`
  - `h` : `Any`
  - `axis` : `Any`（默认 `0`）
  - `step` : `Any`（默认 `1.0`）
  - `effect` : `String`<br>粒子效果标识符，如 `minecraft:flame`（默认 `''`）
  - `interval` : `Int`<br>下发间隔（tick）（默认 `4`）
  - `lifetime` : `Int`<br>生命周期（tick），0 = 永久（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `particleCreateRect`

#### `destroy`

`lib.particles.destroy(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `particleDestroy`

#### `destroy_all`

`lib.particles.destroy_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `particleDestroyAll`

#### `exists`

`lib.particles.exists(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `particleExists`

#### `follow`

`lib.particles.follow(shape_id: int, uuid: str, off_x = 0.0, off_y = 0.0, off_z = 0.0)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `uuid` : `str`
  - `off_x` : `Any`（默认 `0.0`）
  - `off_y` : `Any`（默认 `0.0`）
  - `off_z` : `Any`（默认 `0.0`）
- 返回值类型：`bool`
- 对应 LSE： `particleFollow`

#### `get_all_ids`

`lib.particles.get_all_ids()`

- 参数：无
- 返回值类型：`list[int]`
- 对应 LSE： `particleGetAllIds`

#### `get_info`

`lib.particles.get_info(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值类型：`str`
- 对应 LSE： `particleGetInfo`

#### `move_by`

`lib.particles.move_by(shape_id: int, dx, dy, dz)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `dx` : `Any`
  - `dy` : `Any`
  - `dz` : `Any`
- 返回值类型：`bool`
- 对应 LSE： `particleMoveBy`

#### `move_to`

`lib.particles.move_to(shape_id: int, x, y, z, duration_ticks: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `duration_ticks` : `int`<br>持续时间（tick）
- 返回值类型：`bool`
- 对应 LSE： `particleMoveTo`

#### `set_effect`

`lib.particles.set_effect(shape_id: int, effect: str)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `effect` : `str`<br>粒子效果标识符，如 `minecraft:flame`
- 返回值类型：`bool`
- 对应 LSE： `particleSetEffect`

#### `set_interval`

`lib.particles.set_interval(shape_id: int, ticks: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `ticks` : `int`<br>tick 数
- 返回值类型：`bool`
- 对应 LSE： `particleSetInterval`

#### `set_lifetime`

`lib.particles.set_lifetime(shape_id: int, ticks: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `ticks` : `int`<br>tick 数
- 返回值类型：`bool`
- 对应 LSE： `particleSetLifetime`

#### `set_pos`

`lib.particles.set_pos(shape_id: int, x, y, z)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
- 返回值类型：`bool`
- 对应 LSE： `particleSetPos`

#### `set_rot`

`lib.particles.set_rot(shape_id: int, rx, ry, rz)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `rx` : `Any`
  - `ry` : `Any`
  - `rz` : `Any`
- 返回值类型：`bool`
- 对应 LSE： `particleSetRot`

#### `set_scale`

`lib.particles.set_scale(shape_id: int, scale: float)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值类型：`bool`
- 对应 LSE： `particleSetScale`

#### `set_view_distance`

`lib.particles.set_view_distance(shape_id: int, blocks: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `blocks` : `int`
- 返回值类型：`bool`
- 对应 LSE： `particleSetViewDistance`

#### `set_visible`

`lib.particles.set_visible(shape_id: int, players_csv: str)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `players_csv` : `str`
- 返回值类型：`bool`
- 对应 LSE： `particleSetVisible`

#### `set_visible_players`

`lib.particles.set_visible_players(shape_id: int, player_uuids: list[str])`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `player_uuids` : `list[str]`
- 返回值类型：`bool`
- 对应 LSE： `particleSetVisiblePlayers`

#### `spin`

`lib.particles.spin(shape_id: int, sx, sy, sz)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
  - `sx` : `Any`
  - `sy` : `Any`
  - `sz` : `Any`
- 返回值类型：`bool`
- 对应 LSE： `particleSpin`

#### `unfollow`

`lib.particles.unfollow(shape_id: int)`

- 参数：
  - `shape_id` : `int`<br>形状 id（`create_*` 的返回值）
- 返回值类型：`bool`
- 对应 LSE： `particleUnfollow`


---

完整索引见 [Endstone API 索引](api.md)。
