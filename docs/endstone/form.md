# 表单 · Endstone

关掉玩家当前打开的表单。endstone 自带 `send_form`，本库只补「强制关闭」这一半，因为很多插件关不干净。

## 快速上手

```python
lib.forms.close("Steve")
```

## 函数（2 个）

#### `close`

`lib.forms.close(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `formClose`

#### `close_all`

`lib.forms.close_all()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `formCloseAll`


---

完整索引见 [Endstone API 索引](api.md)。
