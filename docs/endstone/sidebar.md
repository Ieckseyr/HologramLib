# 侧边栏 · Endstone

右侧计分板：标题 + 若干行，逐玩家下发。适合做在线列表、统计面板、任务进度。

行的先后顺序由调用顺序决定；`set_line` 只改一行，`set` 整体替换。

## 快速上手

```python
lib.sidebar.set("Steve", "§e服务器状态", ["§7在线: §f{online}", "§7TPS: §a20.0"])
lib.sidebar.set_line("Steve", 0, "§7在线: §f3")
```

## 函数（7 个）

#### `is_shown`

`lib.sidebar.is_shown(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `sidebarIsShown`

#### `line_count`

`lib.sidebar.line_count(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`int`
- 对应 LSE： `sidebarLineCount`

#### `remove`

`lib.sidebar.remove(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `sidebarRemove`

#### `remove_all`

`lib.sidebar.remove_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `sidebarRemoveAll`

#### `set`

`lib.sidebar.set(player_name: str, title: str, lines: list[str])`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `title` : `str`<br>标题文本
  - `lines` : `list[str]`<br>多行文本
- 返回值类型：`bool`
- 对应 LSE： `sidebarSet`

#### `set_line`

`lib.sidebar.set_line(player_name: str, index: int, text: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `index` : `int`<br>行号 / 槽位号，从 0 开始
  - `text` : `str`<br>文本，支持 `§` 颜色码与 `{player}` 之类变量
- 返回值类型：`bool`
- 对应 LSE： `sidebarSetLine`

#### `set_title`

`lib.sidebar.set_title(player_name: str, title: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `title` : `str`<br>标题文本
- 返回值类型：`bool`
- 对应 LSE： `sidebarSetTitle`


---

完整索引见 [Endstone API 索引](api.md)。
