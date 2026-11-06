# 线路诊断 · Endstone

抓出站包看「到底发了什么」：开始录制后按包 id、长度、收件人记一条，可以打标记、导出最近 64 条。排查客户端没反应、包被谁吃掉时最有用。

只在 `start()` 之后才挂监听，不抓包时没有额外开销。

## 快速上手

```python
lib.wire.start("debug")
lib.wire.mark("开菜单前")
lib.bossbar.add("Steve", "test", 1.0)
print(lib.wire.dump())
lib.wire.stop()
```

## 函数（9 个）

#### `clear`

`lib.wire.clear()`

- 参数：无
- 返回值类型：`bool`
- 对应 LSE： `wireResetStats`

#### `stats`

`lib.wire.stats()`

- 参数：无
- 返回值类型：`str`
- 对应 LSE： `wireStats`

#### `is_active`

`lib.wire.is_active()`

- 参数：无
- 返回值类型：`bool`
- 对应 LSE： `wireTraceActive`

#### `clear`

`lib.wire.clear()`

- 参数：无
- 返回值类型：`bool`
- 对应 LSE： `wireTraceClear`

#### `count`

`lib.wire.count()`

- 参数：无
- 返回值类型：`int`
- 对应 LSE： `wireTraceCount`

#### `dump`

`lib.wire.dump()`

- 参数：无
- 返回值类型：`str`
- 对应 LSE： `wireTraceDump`

#### `mark`

`lib.wire.mark(tag: str)`

- 参数：
  - `tag` : `str`
- 返回值类型：`bool`
- 对应 LSE： `wireTraceMark`

#### `start`

`lib.wire.start(tag: str = 'default')`

- 参数：
  - `tag` : `str`（默认 `'default'`）
- 返回值类型：`bool`
- 对应 LSE： `wireTraceStart`

#### `stop`

`lib.wire.stop()`

- 参数：无
- 返回值类型：`bool`
- 对应 LSE： `wireTraceStop`


---

完整索引见 [Endstone API 索引](api.md)。
