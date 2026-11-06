# 视角覆盖 · Endstone

「一个人眼里的世界」：让某个方块只对某人显示成别的方块、让某个实体对某人不可见。方块直接发 `UpdateBlock`；实体隐藏靠取消出站包（按 runtimeId 命中）。

改类型/皮肤/名字这类**改写**需要出站解码器，当前只做了隐藏与方块。

## 快速上手

```python
lib.view.view_block("Steve", 10, 64, 10, "minecraft:gold_block")
lib.view.view_self_hide("Steve", True)        # Steve 对别人隐身
lib.view.view_clear_all("Steve")
```

## 函数（19 个）

#### `view_block`

`lib.view.view_block(player_name: str, x: int, y: int, z: int, block_type: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
  - `block_type` : `str`
- 返回值类型：`bool`
- 对应 LSE： `viewBlock`

#### `view_clear_all`

`lib.view.view_clear_all(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`None`
- 对应 LSE： `viewClearAll`

#### `view_clear_block`

`lib.view.view_clear_block(player_name: str, x: int, y: int, z: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
- 返回值类型：`bool`
- 对应 LSE： `viewClearBlock`

#### `view_clear_entity`

`lib.view.view_clear_entity(player_name: str, unique_id: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `unique_id` : `int`<br>uniqueId
- 返回值类型：`bool`
- 对应 LSE： `viewClearEntity`

#### `view_clear_id`

`lib.view.view_clear_id(unique_id: int)`

- 参数：
  - `unique_id` : `int`<br>uniqueId
- 返回值类型：`bool`
- 对应 LSE： `viewClearId`

#### `view_describe`

`lib.view.view_describe(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`str`
- 对应 LSE： `viewDescribe`

#### `view_echo`

`lib.view.view_echo(text: str)`

- 参数：
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值类型：`str`
- 对应 LSE： `viewEcho`

#### `view_entity`

`lib.view.view_entity(player_name: str, unique_id: int, spec: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `unique_id` : `int`<br>uniqueId
  - `spec` : `str`（默认 `''`）
- 返回值类型：`bool`
- 对应 LSE： `viewEntity`
- 说明：spec 里只有 hidden 能真实生效；类型/皮肤/名字改写需要出站解码器（未实现）。

#### `view_item_spawn_hooked`

`lib.view.view_item_spawn_hooked()`

- 参数：无
- 返回值类型：`bool`
- 对应 LSE： `viewItemSpawnHooked`

#### `view_meta_probe`

`lib.view.view_meta_probe(spec: str)`

- 参数：
  - `spec` : `str`
- 返回值类型：`str`
- 对应 LSE： `viewMetaProbe`

#### `view_poll_spawn_events`

`lib.view.view_poll_spawn_events()`

- 参数：无
- 返回值类型：`list[str]`
- 对应 LSE： `viewPollSpawnEvents`

#### `view_self`

`lib.view.view_self(player_name: str, spec: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `spec` : `str`
- 返回值类型：`bool`
- 对应 LSE： `viewSelf`

#### `view_self_clear`

`lib.view.view_self_clear(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `viewSelfClear`

#### `view_self_hide`

`lib.view.view_self_hide(player_name: str, hidden: bool)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `hidden` : `bool`<br>是否隐藏
- 返回值类型：`bool`
- 对应 LSE： `viewSelfHide`

#### `view_self_name`

`lib.view.view_self_name(player_name: str, nametag: str, always_show: bool = True)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `nametag` : `str`
  - `always_show` : `bool`（默认 `True`）
- 返回值类型：`bool`
- 对应 LSE： `viewSelfName`

#### `view_self_skin`

`lib.view.view_self_skin(player_name: str, skin_of_player: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `skin_of_player` : `str`<br>要复制的那个玩家的名字
- 返回值类型：`bool`
- 对应 LSE： `viewSelfSkin`

#### `view_self_type`

`lib.view.view_self_type(player_name: str, type_: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `type_` : `str`<br>类型标识符
- 返回值类型：`bool`
- 对应 LSE： `viewSelfType`

#### `view_target_type`

`lib.view.view_target_type(unique_id: int, type_: str)`

- 参数：
  - `unique_id` : `int`<br>uniqueId
  - `type_` : `str`<br>类型标识符
- 返回值类型：`bool`
- 对应 LSE： `viewTargetType`

#### `view_unique_id_of`

`lib.view.view_unique_id_of(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`int`
- 对应 LSE： `viewUniqueIdOf`


---

完整索引见 [Endstone API 索引](api.md)。
