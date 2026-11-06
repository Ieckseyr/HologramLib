# 交易菜单 · Endstone

交易界面：用隐形的村民当载体，把原版交易 UI 拉起来，买卖项、等级、经验全由本库决定。适合做商店、兑换台、NPC 交易。

报价用 `add_offer` 追加；改完记得 `update`/`set_tier` 让客户端刷新。

## 快速上手

```python
tid = lib.trade.open("Steve", "Shop", 1, 0, 0, "")
lib.trade.add_offer(tid, "minecraft:emerald", 3, "", 0, "minecraft:diamond", 1, "§b钻石")
lib.trade.set_tier(tid, 2, 20)
lib.trade.close(tid)
```

## 函数（12 个）

#### `add_offer`

`lib.trade.add_offer(menu_id: int, buy_a_type: str, buy_a_count: int, buy_b_type: str, buy_b_count: int, sell_type: str, sell_count: int, sell_name: str = '', tier: int = 1, locked: bool = False)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `buy_a_type` : `str`
  - `buy_a_count` : `int`
  - `buy_b_type` : `str`
  - `buy_b_count` : `int`
  - `sell_type` : `str`
  - `sell_count` : `int`
  - `sell_name` : `str`（默认 `''`）
  - `tier` : `int`（默认 `1`）
  - `locked` : `bool`（默认 `False`）
- 返回值类型：`bool`
- 对应 LSE： `tradeAddOffer`

#### `close`

`lib.trade.close(menu_id: int)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
- 返回值类型：`bool`
- 对应 LSE： `tradeClose`

#### `close_all`

`lib.trade.close_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `tradeCloseAll`

#### `dropped_settlements`

`lib.trade.dropped_settlements()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `tradeDroppedSettlements`

#### `get_ids`

`lib.trade.get_ids()`

- 参数：无
- 返回值类型：`list[int]`
- 对应 LSE： `tradeGetIds`

#### `is_open`

`lib.trade.is_open(menu_id: int)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
- 返回值类型：`bool`
- 对应 LSE： `tradeIsOpen`

#### `is_settle`

`lib.trade.is_settle(menu_id: int)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
- 返回值类型：`bool`
- 对应 LSE： `tradeIsSettle`

#### `open`

`lib.trade.open(player_name: str, trade_type: str = 'Trade', tier: int = 1, experience: int = 0, real_trades: int = 0, carrier_identifier: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `trade_type` : `str`（默认 `'Trade'`）
  - `tier` : `int`（默认 `1`）
  - `experience` : `int`（默认 `0`）
  - `real_trades` : `int`（默认 `0`）
  - `carrier_identifier` : `str`（默认 `''`）
- 返回值类型：`int`
- 对应 LSE： `tradeOpen`

#### `poll_settlements`

`lib.trade.poll_settlements()`

- 参数：无
- 返回值类型：`list[str]`
- 对应 LSE： `tradePollSettlements`

#### `set_settle`

`lib.trade.set_settle(menu_id: int, on: bool)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `on` : `bool`<br>开 / 关
- 返回值类型：`bool`
- 对应 LSE： `tradeSetSettle`

#### `set_tier`

`lib.trade.set_tier(menu_id: int, tier: int, experience: int = 0)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `tier` : `int`
  - `experience` : `int`（默认 `0`）
- 返回值类型：`bool`
- 对应 LSE： `tradeSetTier`

#### `update`

`lib.trade.update(menu_id: int, trade_type: str = '', tier: int = 0, experience: int = 0)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `trade_type` : `str`（默认 `''`）
  - `tier` : `int`（默认 `0`）
  - `experience` : `int`（默认 `0`）
- 返回值类型：`bool`
- 对应 LSE： `tradeUpdate`


---

完整索引见 [Endstone API 索引](api.md)。
