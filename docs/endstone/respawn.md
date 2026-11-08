# 复活 · Endstone

死亡后强制把玩家复活到指定位置，或拦住本次重生。自己做复活点/竞技场重生逻辑时用得上。

## 快速开始

```python
lib.playerrespawn.respawn("Steve", 0, 64, 0, 0)
```

## 函数

### `respawn`

`lib.playerrespawn.respawn(player_name: str, state: int = 2)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `state` : `int`（默认 `2`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerRespawn`
- 示例：

    ```python
    lib.playerrespawn.respawn("Steve", "minecraft:stone")
    ```

### `respawn_at`

`lib.playerrespawn.respawn_at(player_name: str, x: float, y: float, z: float, state: int = 2)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `state` : `int`（默认 `2`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerRespawnAt`
- 示例：

    ```python
    lib.playerrespawn.respawn_at("Steve", 0.0, 64.0, 0.0, "minecraft:stone")
    ```

### `respawn_state`

`lib.playerrespawn.respawn_state(player_name: str, state: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `state` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerRespawnState`
- 示例：

    ```python
    lib.playerrespawn.respawn_state("Steve", "minecraft:stone")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
