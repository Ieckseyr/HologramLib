# 线路诊断 · Endstone

抓出站包看「到底发了什么」：开始录制后按包 id、长度、收件人记一条，可以打标记、导出最近 64 条。排查客户端没反应、包被谁吃掉时最有用。

只在 `start()` 之后才挂监听，不抓包时没有额外开销。

## 快速开始

```python
lib.wire.start("debug")
lib.wire.mark("开菜单前")
lib.bossbar.add("Steve", "test", 1.0)
print(lib.wire.dump())
lib.wire.stop()
```

## 函数

### `clear`

`lib.wire.clear()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`wireResetStats`
- 示例：

    ```python
    lib.wire.clear()
    ```

### `stats`

`lib.wire.stats()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`wireStats`
- 示例：

    ```python
    lib.wire.stats()
    ```

### `is_active`

`lib.wire.is_active()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`wireTraceActive`
- 示例：

    ```python
    lib.wire.is_active()
    ```

### `clear`

`lib.wire.clear()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`wireTraceClear`
- 示例：

    ```python
    lib.wire.clear()
    ```

### `count`

`lib.wire.count()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`wireTraceCount`
- 示例：

    ```python
    lib.wire.count()
    ```

### `dump`

`lib.wire.dump()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`wireTraceDump`
- 示例：

    ```python
    lib.wire.dump()
    ```

### `mark`

`lib.wire.mark(tag: str)`

- 参数：
  - `tag` : `str`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`wireTraceMark`
- 示例：

    ```python
    lib.wire.mark("...")
    ```

### `start`

`lib.wire.start(tag: str = 'default')`

- 参数：
  - `tag` : `str`（默认 `'default'`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`wireTraceStart`
- 示例：

    ```python
    lib.wire.start("...")
    ```

### `stop`

`lib.wire.stop()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`wireTraceStop`
- 示例：

    ```python
    lib.wire.stop()
    ```


---

完整索引见 [Endstone API 索引](api.md)。
