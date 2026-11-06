# 定位条 · Endstone

屏幕上的方位条：给某个玩家在指定坐标放一个图标，可以分组、改图标、控制别人能不能看到。适合做路径点、队友标记、目标指引。

## 快速上手

```python
lib.locatorbar.set("Steve", "home", 0, 64, 0, 0, 0x00FF00)
lib.locatorbar.set_icon("Steve", "home", "textures/items/compass")
lib.locatorbar.hide_from_others("Steve", True)
```

## 函数（14 个）

#### `attach_entity`

`lib.locatorbar.attach_entity(player_name: str, key: str, entity_unique_id: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
  - `entity_unique_id` : `int`<br>实体的 uniqueId（网络层 id，不是库内 id）
- 返回值类型：`bool`
- 对应 LSE： `locatorBarAttachEntity`

#### `clear`

`lib.locatorbar.clear(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`int`
- 对应 LSE： `locatorBarClear`

#### `clear_attach`

`lib.locatorbar.clear_attach(player_name: str, key: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
- 返回值类型：`bool`
- 对应 LSE： `locatorBarClearAttach`

#### `count`

`lib.locatorbar.count(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`int`
- 对应 LSE： `locatorBarCount`

#### `hide_from_others`

`lib.locatorbar.hide_from_others(player_name: str, hidden: bool)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `hidden` : `bool`<br>是否隐藏
- 返回值类型：`bool`
- 对应 LSE： `locatorBarHideFromOthers`

#### `is_hidden_from_others`

`lib.locatorbar.is_hidden_from_others(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `locatorBarIsHiddenFromOthers`

#### `remove`

`lib.locatorbar.remove(player_name: str, key: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
- 返回值类型：`bool`
- 对应 LSE： `locatorBarRemove`

#### `set`

`lib.locatorbar.set(player_name: str, key: str, x, y, z, dim: int = 0, color: int = -1, visible: bool = True)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `0`）
  - `color` : `int`<br>颜色（RGB 整数或枚举，见各域说明）（默认 `-1`）
  - `visible` : `bool`<br>是否可见（默认 `True`）
- 返回值类型：`bool`
- 对应 LSE： `locatorBarSet`

#### `set_client_authority`

`lib.locatorbar.set_client_authority(player_name: str, key: str, enabled: bool)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
  - `enabled` : `bool`<br>开 / 关
- 返回值类型：`bool`
- 对应 LSE： `locatorBarSetClientAuthority`

#### `set_ex`

`lib.locatorbar.set_ex(player_name: str, key: str, x, y, z, dim: int = 0, color: int = -1, visible: bool = True, texture: str = '', icon_w: float = 0.0, icon_h: float = 0.0, authority: bool = False)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
  - `x` : `Float`<br>X 坐标
  - `y` : `Float`<br>Y 坐标
  - `z` : `Float`<br>Z 坐标
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `0`）
  - `color` : `int`<br>颜色（RGB 整数或枚举，见各域说明）（默认 `-1`）
  - `visible` : `bool`<br>是否可见（默认 `True`）
  - `texture` : `str`（默认 `''`）
  - `icon_w` : `float`（默认 `0.0`）
  - `icon_h` : `float`（默认 `0.0`）
  - `authority` : `bool`（默认 `False`）
- 返回值类型：`bool`
- 对应 LSE： `locatorBarSetEx`

#### `set_group`

`lib.locatorbar.set_group(player_name: str, key: str, group_key: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
  - `group_key` : `str`
- 返回值类型：`bool`
- 对应 LSE： `locatorBarSetGroup`

#### `set_icon`

`lib.locatorbar.set_icon(player_name: str, key: str, texture: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
  - `texture` : `str`
- 返回值类型：`bool`
- 对应 LSE： `locatorBarSetIcon`

#### `set_icon_size`

`lib.locatorbar.set_icon_size(player_name: str, key: str, width: float, height: float)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `key` : `str`
  - `width` : `float`<br>宽
  - `height` : `float`<br>高
- 返回值类型：`bool`
- 对应 LSE： `locatorBarSetIconSize`

#### `sync_group`

`lib.locatorbar.sync_group(player_name: str, group_key: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `group_key` : `str`
- 返回值类型：`int`
- 对应 LSE： `locatorBarSyncGroup`


---

完整索引见 [Endstone API 索引](api.md)。
