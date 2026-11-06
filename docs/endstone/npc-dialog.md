# NPC 对话 · Endstone

原版 NPC 对话框：标题、台词、若干按钮，按钮可以带命令。载体是对该玩家单独可见的隐身实体，关掉即销毁。

按钮规格：`标签|actionId|mode|命令1,命令2;第二个按钮|...`。

## 快速上手

```python
did = lib.npcdialog.open("Steve", "§e商人", "shop", "要点什么？",
                        "买钻石|buy|0|shop buy diamond;离开|bye|0|")
lib.npcdialog.update(did, "还有别的吗？", "买绿宝石|buy2|0|shop buy emerald")
lib.npcdialog.close(did)
```

## 函数（10 个）

#### `clear_clicks`

`lib.npcdialog.clear_clicks()`

- 参数：无
- 返回值类型：`None`
- 对应 LSE： `npcDialogClearClicks`

#### `close`

`lib.npcdialog.close(dialog_id: int)`

- 参数：
  - `dialog_id` : `int`<br>对话 id
- 返回值类型：`bool`
- 对应 LSE： `npcDialogClose`

#### `close_all`

`lib.npcdialog.close_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `npcDialogCloseAll`

#### `dropped_clicks`

`lib.npcdialog.dropped_clicks()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `npcDialogDroppedClicks`

#### `get_ids`

`lib.npcdialog.get_ids()`

- 参数：无
- 返回值类型：`list[int]`
- 对应 LSE： `npcDialogGetIds`

#### `is_open`

`lib.npcdialog.is_open(dialog_id: int)`

- 参数：
  - `dialog_id` : `int`<br>对话 id
- 返回值类型：`bool`
- 对应 LSE： `npcDialogIsOpen`

#### `open`

`lib.npcdialog.open(player_name: str, npc_name: str, scene_name: str, dialogue: str, buttons_spec: str = '', carrier_identifier: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `npc_name` : `str`
  - `scene_name` : `str`
  - `dialogue` : `str`
  - `buttons_spec` : `str`（默认 `''`）
  - `carrier_identifier` : `str`（默认 `''`）
- 返回值类型：`int`
- 对应 LSE： `npcDialogOpen`

#### `poll_clicks`

`lib.npcdialog.poll_clicks()`

- 参数：无
- 返回值类型：`list[str]`
- 对应 LSE： `npcDialogPollClicks`

#### `set_avatar`

`lib.npcdialog.set_avatar(dialog_id: int, skin_variant: int, avatar_view_spec: str = '')`

- 参数：
  - `dialog_id` : `int`<br>对话 id
  - `skin_variant` : `int`
  - `avatar_view_spec` : `str`（默认 `''`）
- 返回值类型：`bool`
- 对应 LSE： `npcDialogSetAvatar`

#### `update`

`lib.npcdialog.update(dialog_id: int, dialogue: str, buttons_spec: str = '')`

- 参数：
  - `dialog_id` : `int`<br>对话 id
  - `dialogue` : `str`
  - `buttons_spec` : `str`（默认 `''`）
- 返回值类型：`bool`
- 对应 LSE： `npcDialogUpdate`


---

完整索引见 [Endstone API 索引](api.md)。
