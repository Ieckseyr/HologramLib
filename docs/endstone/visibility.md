# 可见性 · Endstone

让某个真实玩家对其他人不可见（隐身），可控制作用范围（生成/Tab/动作/状态/皮肤），恢复时补发 `AddPlayer` 重建实体。

底层与视角覆盖共用一套出站过滤，所以两者会叠加。

## 快速上手

```python
lib.visibility.set_hidden("Steve", True)
lib.visibility.set_mask("Steve", spawn=True, tab=True, motion=True, state=True, skin=True)
lib.visibility.restore("Steve")
```

## 函数（5 个）

#### `get_mask`

`lib.visibility.get_mask(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`int`
- 对应 LSE： `playerHideGetMask`

#### `is_hidden`

`lib.visibility.is_hidden(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `playerHideIsHidden`

#### `restore`

`lib.visibility.restore(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `playerHideRestore`

#### `set_hidden`

`lib.visibility.set_hidden(player_name: str, hidden: bool)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `hidden` : `bool`<br>是否隐藏
- 返回值类型：`bool`
- 对应 LSE： `playerHideSetHidden`

#### `set_mask`

`lib.visibility.set_mask(player_name: str, spawn: bool, tab: bool, motion: bool, state: bool, skin: bool, also_self: bool = False)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `spawn` : `bool`
  - `tab` : `bool`
  - `motion` : `bool`
  - `state` : `bool`
  - `skin` : `bool`
  - `also_self` : `bool`（默认 `False`）
- 返回值类型：`bool`
- 对应 LSE： `playerHideSetMask`


---

完整索引见 [Endstone API 索引](api.md)。
