# 复活 · Endstone

死亡后强制把玩家复活到指定位置，或拦住本次重生。自己做复活点/竞技场重生逻辑时用得上。

## 快速上手

```python
lib.playerrespawn.respawn("Steve", 0, 64, 0, 0)
```

## 函数（3 个）

#### `respawn`

`lib.playerrespawn.respawn(player_name: str, state: int = 2)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `state` : `int`（默认 `2`）
- 返回值类型：`bool`
- 对应 LSE： `playerRespawn`

#### `respawn_at`

`lib.playerrespawn.respawn_at(player_name: str, x: float, y: float, z: float, state: int = 2)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `state` : `int`（默认 `2`）
- 返回值类型：`bool`
- 对应 LSE： `playerRespawnAt`

#### `respawn_state`

`lib.playerrespawn.respawn_state(player_name: str, state: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `state` : `int`
- 返回值类型：`bool`
- 对应 LSE： `playerRespawnState`


---

完整索引见 [Endstone API 索引](api.md)。
