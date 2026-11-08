# 交易菜单 · Endstone

交易界面：用隐形的村民当载体，把原版交易 UI 拉起来，买卖项、等级、经验全由本库决定。适合做商店、兑换台、NPC 交易。

报价用 `add_offer` 追加；改完记得 `update`/`set_tier` 让客户端刷新。

## 快速开始

```python
tid = lib.trade.open("Steve", "Shop", 1, 0, 0, "")
lib.trade.add_offer(tid, "minecraft:emerald", 3, "", 0, "minecraft:diamond", 1, "§b钻石")
lib.trade.set_tier(tid, 2, 20)
lib.trade.close(tid)
```

## 函数

### `add_offer`

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
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`tradeAddOffer`
- 示例：

    ```python
    lib.trade.add_offer(1, "...", 1, "...", 1, "...", 1, "...", 1, "...")
    ```

### `close`

`lib.trade.close(menu_id: int)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`tradeClose`
- 示例：

    ```python
    lib.trade.close(1)
    ```

### `close_all`

`lib.trade.close_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`tradeCloseAll`
- 示例：

    ```python
    lib.trade.close_all()
    ```

### `dropped_settlements`

`lib.trade.dropped_settlements()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`tradeDroppedSettlements`
- 示例：

    ```python
    lib.trade.dropped_settlements()
    ```

### `get_ids`

`lib.trade.get_ids()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`tradeGetIds`
- 示例：

    ```python
    lib.trade.get_ids()
    ```

### `is_open`

`lib.trade.is_open(menu_id: int)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`tradeIsOpen`
- 示例：

    ```python
    lib.trade.is_open(1)
    ```

### `is_settle`

`lib.trade.is_settle(menu_id: int)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`tradeIsSettle`
- 示例：

    ```python
    lib.trade.is_settle(1)
    ```

### `open`

`lib.trade.open(player_name: str, trade_type: str = 'Trade', tier: int = 1, experience: int = 0, real_trades: int = 0, carrier_identifier: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `trade_type` : `str`（默认 `'Trade'`）
  - `tier` : `int`（默认 `1`）
  - `experience` : `int`（默认 `0`）
  - `real_trades` : `int`（默认 `0`）
  - `carrier_identifier` : `str`（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`tradeOpen`
- 示例：

    ```python
    lib.trade.open("Steve", "...", 1, 1, 1, "...")
    ```

### `poll_settlements`

`lib.trade.poll_settlements()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[str]`
- 对应 LSE 导出：`tradePollSettlements`
- 示例：

    ```python
    lib.trade.poll_settlements()
    ```

### `set_settle`

`lib.trade.set_settle(menu_id: int, on: bool)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `on` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`tradeSetSettle`
- 示例：

    ```python
    lib.trade.set_settle(1, "...")
    ```

### `set_tier`

`lib.trade.set_tier(menu_id: int, tier: int, experience: int = 0)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `tier` : `int`
  - `experience` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`tradeSetTier`
- 示例：

    ```python
    lib.trade.set_tier(1, 1, 1)
    ```

### `update`

`lib.trade.update(menu_id: int, trade_type: str = '', tier: int = 0, experience: int = 0)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `trade_type` : `str`（默认 `''`）
  - `tier` : `int`（默认 `0`）
  - `experience` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`tradeUpdate`
- 示例：

    ```python
    lib.trade.update(1, "...", 1, 1)
    ```


---

完整索引见 [Endstone API 索引](api.md)。
