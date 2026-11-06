# 硫磺方块 · Endstone

`minecraft:sulfur_cube` 展示：一个方块变成的立方体生物，可以隐形、缩放、主手换成任意方块。适合做装饰、任务目标、活动道具。

## 快速上手

```python
sid = lib.sulfur.create(0, 64, 0, 0, "minecraft:diamond_block")
lib.sulfur.set_invisible(sid, False)
lib.sulfur.set_scale(sid, 2.0)
```

## 函数（10 个）

#### `create`

`lib.sulfur.create(x: float, y: float, z: float, dim: int = 0, block_type: str = 'minecraft:stone', block_count: int = 1, block_damage: int = 0, block_name: str = '', archetype: str = '', invisible: bool = False)`

- 参数：
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）（默认 `0`）
  - `block_type` : `str`（默认 `'minecraft:stone'`）
  - `block_count` : `int`（默认 `1`）
  - `block_damage` : `int`（默认 `0`）
  - `block_name` : `str`（默认 `''`）
  - `archetype` : `str`（默认 `''`）
  - `invisible` : `bool`（默认 `False`）
- 返回值类型：`int`
- 对应 LSE： `sulfurCreate`

#### `destroy`

`lib.sulfur.destroy(sulfur_id: int)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
- 返回值类型：`bool`
- 对应 LSE： `sulfurDestroy`

#### `destroy_all`

`lib.sulfur.destroy_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `sulfurDestroyAll`

#### `exists`

`lib.sulfur.exists(sulfur_id: int)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
- 返回值类型：`bool`
- 对应 LSE： `sulfurExists`

#### `get_entity_id`

`lib.sulfur.get_entity_id(sulfur_id: int)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
- 返回值类型：`int`
- 对应 LSE： `sulfurGetEntityId`

#### `get_ids`

`lib.sulfur.get_ids()`

- 参数：无
- 返回值类型：`list[int]`
- 对应 LSE： `sulfurGetIds`

#### `set_archetype`

`lib.sulfur.set_archetype(sulfur_id: int, archetype: str)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
  - `archetype` : `str`
- 返回值类型：`bool`
- 对应 LSE： `sulfurSetArchetype`

#### `set_block`

`lib.sulfur.set_block(sulfur_id: int, block_type: str, count: int = 1, damage: int = 0, name: str = '')`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
  - `block_type` : `str`
  - `count` : `int`<br>数量（默认 `1`）
  - `damage` : `int`<br>耐久损伤值（默认 `0`）
  - `name` : `str`<br>名字 / 标识符（默认 `''`）
- 返回值类型：`bool`
- 对应 LSE： `sulfurSetBlock`

#### `set_invisible`

`lib.sulfur.set_invisible(sulfur_id: int, on: bool)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
  - `on` : `bool`<br>开 / 关
- 返回值类型：`bool`
- 对应 LSE： `sulfurSetInvisible`

#### `set_scale`

`lib.sulfur.set_scale(sulfur_id: int, scale: float)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值类型：`bool`
- 对应 LSE： `sulfurSetScale`


---

完整索引见 [Endstone API 索引](api.md)。
