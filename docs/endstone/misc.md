# 杂项 · Endstone

跨域的小工具：锚点（拿一个唯一的实体号）、假方块批量、Boss 条别名、线路诊断（出站包抓取）。

## 快速开始

```python
uid = lib.anchor.acquire("Steve")      # 拿一个唯一实体号当锚点
lib.anchor.release(uid)

lib.wire.start("debug")                # 开始抓出站包
lib.wire.mark("阶段 A")
print(lib.wire.dump())
lib.wire.stop()
```

## 函数

### `acquire`

`lib.anchor.acquire(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`anchorAcquire`
- 示例：

    ```python
    lib.anchor.acquire("Steve")
    ```

### `release`

`lib.anchor.release(unique_id: int)`

- 参数：
  - `unique_id` : `int`<br>uniqueId
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`anchorRelease`
- 示例：

    ```python
    lib.anchor.release(1)
    ```


---

完整索引见 [Endstone API 索引](api.md)。
