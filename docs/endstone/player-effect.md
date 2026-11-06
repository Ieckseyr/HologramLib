# 玩家表现 · Endstone

给玩家播原版表现：村庄英雄、不祥之兆、庆祝（烟花）、不祥药水。这些不是药水效果，而是客户端动画/音效事件，不需要真的给效果。

## 快速开始

```python
lib.playereffect.show_village_hero("Steve", 5)
lib.playereffect.play_celebrate("Steve")
```

## 函数

### `play_celebrate`

`lib.playereffect.play_celebrate(player_name: str, seconds: float = 3.0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `seconds` : `float`<br>持续秒数（默认 `3.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerPlayCelebrate`
- 示例：

    ```python
    lib.playereffect.play_celebrate("Steve", 1)
    ```

### `play_ominous_drink`

`lib.playereffect.play_ominous_drink(player_name: str, hold_ticks: int = 40)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `hold_ticks` : `int`（默认 `40`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerPlayOminousDrink`
- 示例：

    ```python
    lib.playereffect.play_ominous_drink("Steve", 1)
    ```

### `show_bad_omen`

`lib.playereffect.show_bad_omen(player_name: str, duration_ticks: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerShowBadOmen`
- 示例：

    ```python
    lib.playereffect.show_bad_omen("Steve", 1)
    ```

### `show_effect`

`lib.playereffect.show_effect(player_name: str, effect_id: int, duration_ticks: int = 200, amplifier: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `effect_id` : `int`
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `200`）
  - `amplifier` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerShowEffect`
- 示例：

    ```python
    lib.playereffect.show_effect("Steve", 1, 1, 1)
    ```

### `show_village_hero`

`lib.playereffect.show_village_hero(player_name: str, duration_ticks: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `duration_ticks` : `int`<br>持续时间（tick）（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`playerShowVillageHero`
- 示例：

    ```python
    lib.playereffect.show_village_hero("Steve", 1)
    ```


---

完整索引见 [Endstone API 索引](api.md)。
