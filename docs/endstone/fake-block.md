# 假方块 · Endstone

只对某个玩家显示成别的方块：把石头「变成」钻石矿，服务端世界里还是石头。底层是 `UpdateBlock`（包 id 21），不写世界、不进存档。

包体 = 坐标（x/y/z 各 zigzag varint）+ 方块网络 id（varint）+ flag(3) + layer(0)。26.60 开着 `block-network-ids-are-hashes`，网络 id 是哈希值（`minecraft:stone` → `2150698529`），由 `create_block_data` 解析，不要自己拼数字。

方块名接受 `minecraft:` 前缀，也接受带状态的字面量（如 `minecraft:oak_stairs[upside_down_bit=true]`）。

批量替换（`blockBatch*`，含靠近切换成硫磺立方体）目前只在 LeviLamina 端提供，endstone 端待接。

## 快速开始

```python
lib.fakeblock.set("Steve", 10, 64, 10, "minecraft:diamond_ore")
lib.fakeblock.clear("Steve", 10, 64, 10)
```

## 函数

### `clear`

`lib.fakeblock.clear(player_name: str, x: int, y: int, z: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`blockFakeClear`
- 示例：

    ```python
    lib.fakeblock.clear("Steve", 0.0, 64.0, 0.0)
    ```

### `clear_all`

`lib.fakeblock.clear_all(x: int, y: int, z: int, dim: int)`

- 参数：
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`blockFakeClearAll`
- 示例：

    ```python
    lib.fakeblock.clear_all(0.0, 64.0, 0.0, 0)
    ```

### `resolve`

`lib.fakeblock.resolve(state: str)`

- 参数：
  - `state` : `str`
- 返回值：状态串 → 网络 id；解析不了返回 -1。
- 返回值类型：`int`
- 对应 LSE 导出：`blockFakeResolve`
- 示例：

    ```python
    lib.fakeblock.resolve("minecraft:stone")
    ```

### `set`

`lib.fakeblock.set(player_name: str, x: int, y: int, z: int, state: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
  - `state` : `str`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`blockFakeSet`
- 示例：

    ```python
    lib.fakeblock.set("Steve", 0.0, 64.0, 0.0, "minecraft:stone")
    ```

### `set_all`

`lib.fakeblock.set_all(x: int, y: int, z: int, dim: int, state: str)`

- 参数：
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `state` : `str`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`blockFakeSetAll`
- 示例：

    ```python
    lib.fakeblock.set_all(0.0, 64.0, 0.0, 0, "minecraft:stone")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
