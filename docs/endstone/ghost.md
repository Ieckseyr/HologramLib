# 幽灵交互 · Endstone

轮询「玩家点了幽灵方块/幽灵实体」的交互事件。需要解析入站交互包，当前恒为空列表，接口保留。

## 快速开始

```python
for item in lib.ghost.poll_interactions():
    self.logger.info(item)
```

## 函数

### `clear_interactions`

`lib.ghost.clear_interactions()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`ghostClearInteractions`
- 示例：

    ```python
    lib.ghost.clear_interactions()
    ```

### `poll_interactions`

`lib.ghost.poll_interactions()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[str]`
- 对应 LSE 导出：`ghostPollInteractions`
- 示例：

    ```python
    lib.ghost.poll_interactions()
    ```


---

完整索引见 [Endstone API 索引](api.md)。
