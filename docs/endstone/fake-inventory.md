# 假背包 · Endstone

给玩家伪造一整套背包内容：主背包、护甲、副手、末影箱都能改。底层是 `InventoryContent`，玩家看到的是假数据，真实背包不变。

## 快速上手

```python
lib.fakeinv.apply("Steve", [("minecraft:diamond", 64), ("minecraft:emerald", 16)])
lib.fakeinv.reset("Steve")
```

## 函数（10 个）

#### `apply`

`lib.fakeinv.apply(player_name: str, items_spec: str, refresh_interval_ticks: int = 20)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `items_spec` : `str`
  - `refresh_interval_ticks` : `int`（默认 `20`）
- 返回值类型：`bool`
- 对应 LSE： `fakeInvApply`

#### `clear`

`lib.fakeinv.clear(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `fakeInvClear`

#### `clear_all`

`lib.fakeinv.clear_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `fakeInvClearAll`

#### `clear_clicks`

`lib.fakeinv.clear_clicks()`

- 参数：无
- 返回值类型：`None`
- 对应 LSE： `fakeInvClearClicks`

#### `dropped_clicks`

`lib.fakeinv.dropped_clicks()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `fakeInvDroppedClicks`

#### `get_players`

`lib.fakeinv.get_players()`

- 参数：无
- 返回值类型：`list[str]`
- 对应 LSE： `fakeInvGetPlayers`

#### `is_active`

`lib.fakeinv.is_active(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `fakeInvIsActive`

#### `poll_clicks`

`lib.fakeinv.poll_clicks()`

- 参数：无
- 返回值类型：`list[str]`
- 对应 LSE： `fakeInvPollClicks`

#### `refresh`

`lib.fakeinv.refresh(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `fakeInvRefresh`

#### `set_slot`

`lib.fakeinv.set_slot(player_name: str, slot: int, item_type: str, count: int, damage: int = 0, name: str = '', lore_csv: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `slot` : `int`<br>槽位号
  - `item_type` : `str`
  - `count` : `int`<br>数量
  - `damage` : `int`<br>耐久损伤值（默认 `0`）
  - `name` : `str`<br>名字 / 标识符（默认 `''`）
  - `lore_csv` : `str`（默认 `''`）
- 返回值类型：`bool`
- 对应 LSE： `fakeInvSetSlot`


---

完整索引见 [Endstone API 索引](api.md)。
