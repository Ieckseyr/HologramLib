# 假方块 · Endstone

只对某个玩家显示成别的方块：把石头「变成」钻石矿，服务端世界里还是石头。底层是 `UpdateBlock`，不写世界、不进存档。

方块名接受 `minecraft:` 前缀，也接受带状态的字面量（如 `minecraft:stone`）。

## 快速上手

```python
lib.fakeblock.set("Steve", 10, 64, 10, "minecraft:diamond_ore")
lib.fakeblock.clear("Steve", 10, 64, 10)
```

## 函数（5 个）

#### `clear`

`lib.fakeblock.clear(player_name: str, x: int, y: int, z: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
- 返回值类型：`bool`
- 对应 LSE： `blockFakeClear`

#### `clear_all`

`lib.fakeblock.clear_all(x: int, y: int, z: int, dim: int)`

- 参数：
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
- 返回值类型：`int`
- 对应 LSE： `blockFakeClearAll`

#### `resolve`

`lib.fakeblock.resolve(state: str)`

- 参数：
  - `state` : `str`
- 返回值类型：`int`
- 对应 LSE： `blockFakeResolve`
- 说明：状态串 → 网络 id；解析不了返回 -1。

#### `set`

`lib.fakeblock.set(player_name: str, x: int, y: int, z: int, state: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
  - `state` : `str`
- 返回值类型：`bool`
- 对应 LSE： `blockFakeSet`

#### `set_all`

`lib.fakeblock.set_all(x: int, y: int, z: int, dim: int, state: str)`

- 参数：
  - `x` : `int`<br>X 坐标
  - `y` : `int`<br>Y 坐标
  - `z` : `int`<br>Z 坐标
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `state` : `str`
- 返回值类型：`int`
- 对应 LSE： `blockFakeSetAll`


---

完整索引见 [Endstone API 索引](api.md)。
