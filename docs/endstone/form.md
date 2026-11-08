# 表单 · Endstone

关掉玩家当前打开的表单。endstone 自带 `send_form`，本库只补「强制关闭」这一半，因为很多插件关不干净。

## 快速开始

```python
lib.forms.close("Steve")
```

## 函数

### `close`

`lib.forms.close(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`formClose`
- 示例：

    ```python
    lib.forms.close("Steve")
    ```

### `close_all`

`lib.forms.close_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`formCloseAll`
- 示例：

    ```python
    lib.forms.close_all()
    ```


---

完整索引见 [Endstone API 索引](api.md)。
