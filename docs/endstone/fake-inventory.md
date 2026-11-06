# 假背包 · Endstone

给玩家伪造一整套背包内容：主背包、护甲、副手、末影箱都能改。底层是 `InventoryContent`，玩家看到的是假数据，真实背包不变。

## 快速开始

```python
lib.fakeinv.apply("Steve", [("minecraft:diamond", 64), ("minecraft:emerald", 16)])
lib.fakeinv.reset("Steve")
```

## 函数

### `apply`

`lib.fakeinv.apply(player_name: str, items_spec: str, refresh_interval_ticks: int = 20)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `items_spec` : `str`
  - `refresh_interval_ticks` : `int`（默认 `20`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`fakeInvApply`
- 示例：

    ```python
    lib.fakeinv.apply("Steve", "...", 1)
    ```

### `clear`

`lib.fakeinv.clear(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`fakeInvClear`
- 示例：

    ```python
    lib.fakeinv.clear("Steve")
    ```

### `clear_all`

`lib.fakeinv.clear_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`fakeInvClearAll`
- 示例：

    ```python
    lib.fakeinv.clear_all()
    ```

### `clear_clicks`

`lib.fakeinv.clear_clicks()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`None`
- 对应 LSE 导出：`fakeInvClearClicks`
- 示例：

    ```python
    lib.fakeinv.clear_clicks()
    ```

### `dropped_clicks`

`lib.fakeinv.dropped_clicks()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`fakeInvDroppedClicks`
- 示例：

    ```python
    lib.fakeinv.dropped_clicks()
    ```

### `get_players`

`lib.fakeinv.get_players()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[str]`
- 对应 LSE 导出：`fakeInvGetPlayers`
- 示例：

    ```python
    lib.fakeinv.get_players()
    ```

### `is_active`

`lib.fakeinv.is_active(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`fakeInvIsActive`
- 示例：

    ```python
    lib.fakeinv.is_active("Steve")
    ```

### `poll_clicks`

`lib.fakeinv.poll_clicks()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[str]`
- 对应 LSE 导出：`fakeInvPollClicks`
- 示例：

    ```python
    lib.fakeinv.poll_clicks()
    ```

### `refresh`

`lib.fakeinv.refresh(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`fakeInvRefresh`
- 示例：

    ```python
    lib.fakeinv.refresh("Steve")
    ```

### `set_slot`

`lib.fakeinv.set_slot(player_name: str, slot: int, item_type: str, count: int, damage: int = 0, name: str = '', lore_csv: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `slot` : `int`<br>槽位号
  - `item_type` : `str`
  - `count` : `int`<br>数量
  - `damage` : `int`<br>耐久损伤值（默认 `0`）
  - `name` : `str`<br>名字 / 标识符（默认 `''`）
  - `lore_csv` : `str`（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`fakeInvSetSlot`
- 示例：

    ```python
    lib.fakeinv.set_slot("Steve", 1, "...", 1, 1, "hello", "...")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
