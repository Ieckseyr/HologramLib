# Boss 条 · Endstone

屏幕顶部的 Boss 血条，纯协议下发：标题、血量百分比、颜色、样式都能改。适合做「Boss 血量」「任务进度」「倒计时」。

一个玩家可以同时有多条（`add_on` 会叠上去），也可以一次清空。

## 快速上手

```python
bid = lib.bossbar.add("Steve", "§c世界 Boss", 1.0)
lib.bossbar.set_health(bid, 0.5)
lib.bossbar.set_style(bid, 0, 1)   # 颜色 红, 样式 分段
```

## 函数（10 个）

#### `add`

`lib.bossbar.add(player_name: str, title: str, health_percent: float = 1.0, color: int = 0, overlay: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `title` : `str`<br>标题文本
  - `health_percent` : `float`（默认 `1.0`）
  - `color` : `int`<br>颜色（RGB 整数或枚举，见各域说明）（默认 `0`）
  - `overlay` : `int`（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `bossBarAdd`

#### `add_on`

`lib.bossbar.add_on(player_name: str, target_id: int, title: str, percent: float = 1.0, color: int = 0, overlay: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `target_id` : `int`
  - `title` : `str`<br>标题文本
  - `percent` : `float`<br>百分比 0~1（默认 `1.0`）
  - `color` : `int`<br>颜色（RGB 整数或枚举，见各域说明）（默认 `0`）
  - `overlay` : `int`（默认 `0`）
- 返回值类型：`int`
- 对应 LSE： `bossBarAddOn`

#### `count`

`lib.bossbar.count(player_name: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）（默认 `''`）
- 返回值类型：`int`
- 对应 LSE： `bossBarCount`

#### `get_info`

`lib.bossbar.get_info(bar_id: int)`

- 参数：
  - `bar_id` : `int`
- 返回值类型：`str`
- 对应 LSE： `bossBarGetInfo`

#### `probe`

`lib.bossbar.probe()`

- 参数：无
- 返回值类型：`list[int]`
- 对应 LSE： `bossBarProbe`

#### `remove`

`lib.bossbar.remove(bar_id: int)`

- 参数：
  - `bar_id` : `int`
- 返回值类型：`bool`
- 对应 LSE： `bossBarRemove`

#### `remove_all`

`lib.bossbar.remove_all(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`int`
- 对应 LSE： `bossBarRemoveAll`

#### `set_health`

`lib.bossbar.set_health(bar_id: int, percent: float)`

- 参数：
  - `bar_id` : `int`
  - `percent` : `float`<br>百分比 0~1
- 返回值类型：`bool`
- 对应 LSE： `bossBarSetHealth`

#### `set_style`

`lib.bossbar.set_style(bar_id: int, color: int, overlay: int)`

- 参数：
  - `bar_id` : `int`
  - `color` : `int`<br>颜色（RGB 整数或枚举，见各域说明）
  - `overlay` : `int`
- 返回值类型：`bool`
- 对应 LSE： `bossBarSetStyle`

#### `set_title`

`lib.bossbar.set_title(bar_id: int, title: str)`

- 参数：
  - `bar_id` : `int`
  - `title` : `str`<br>标题文本
- 返回值类型：`bool`
- 对应 LSE： `bossBarSetTitle`


---

完整索引见 [Endstone API 索引](api.md)。
