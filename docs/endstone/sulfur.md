# 硫磺方块 · Endstone

`minecraft:sulfur_cube` 展示：一个方块变成的立方体生物，可以隐形、缩放、主手换成任意方块。适合做装饰、任务目标、活动道具。

## 快速开始

```python
sid = lib.sulfur.create(0, 64, 0, 0, "minecraft:diamond_block")
lib.sulfur.set_invisible(sid, False)
lib.sulfur.set_scale(sid, 2.0)
```

## 函数

### `create`

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
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`sulfurCreate`
- 示例：

    ```python
    lib.sulfur.create(0.0, 64.0, 0.0, 0, "minecraft:stone", 1, 1, "...", "...", "...")
    ```

### `destroy`

`lib.sulfur.destroy(sulfur_id: int)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`sulfurDestroy`
- 示例：

    ```python
    lib.sulfur.destroy(1)
    ```

### `destroy_all`

`lib.sulfur.destroy_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`sulfurDestroyAll`
- 示例：

    ```python
    lib.sulfur.destroy_all()
    ```

### `exists`

`lib.sulfur.exists(sulfur_id: int)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`sulfurExists`
- 示例：

    ```python
    lib.sulfur.exists(1)
    ```

### `get_entity_id`

`lib.sulfur.get_entity_id(sulfur_id: int)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`sulfurGetEntityId`
- 示例：

    ```python
    lib.sulfur.get_entity_id(1)
    ```

### `get_ids`

`lib.sulfur.get_ids()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`sulfurGetIds`
- 示例：

    ```python
    lib.sulfur.get_ids()
    ```

### `set_archetype`

`lib.sulfur.set_archetype(sulfur_id: int, archetype: str)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
  - `archetype` : `str`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`sulfurSetArchetype`
- 示例：

    ```python
    lib.sulfur.set_archetype(1, "...")
    ```

### `set_block`

`lib.sulfur.set_block(sulfur_id: int, block_type: str, count: int = 1, damage: int = 0, name: str = '')`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
  - `block_type` : `str`
  - `count` : `int`<br>数量（默认 `1`）
  - `damage` : `int`<br>耐久损伤值（默认 `0`）
  - `name` : `str`<br>名字 / 标识符（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`sulfurSetBlock`
- 示例：

    ```python
    lib.sulfur.set_block(1, "minecraft:stone", 1, 1, "hello")
    ```

### `set_invisible`

`lib.sulfur.set_invisible(sulfur_id: int, on: bool)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
  - `on` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`sulfurSetInvisible`
- 示例：

    ```python
    lib.sulfur.set_invisible(1, "...")
    ```

### `set_scale`

`lib.sulfur.set_scale(sulfur_id: int, scale: float)`

- 参数：
  - `sulfur_id` : `int`<br>硫磺方块 id
  - `scale` : `float`<br>缩放倍率，1.0 = 原始大小
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`sulfurSetScale`
- 示例：

    ```python
    lib.sulfur.set_scale(1, 1)
    ```


---

完整索引见 [Endstone API 索引](api.md)。
