# 客户端感知 · Endstone

两块内容：

**攻击 / 挥动**（`swing*`，真正可用）：玩家每次挥手（空手挥、挥剑、打空气、点方块、打实体）都走入站 `AnimatePacket`（包 id 44，`Action = Swing`），库按玩家记成事件行；目标用 endstone 的 `PlayerInteractEvent` 补齐 —— 它把左键分成 `LEFT_CLICK_BLOCK` / `LEFT_CLICK_AIR`，所以能区分「对着方块挥」和「对着空气挥」，方块还带坐标与命中面。

**输入设备**（`device_*` / `is_touch`）：endstone 的 Python 面没有 AuthInput 入口，当前返回空值，接口保留。

!!! 注意

    `swing*` 第一次被调用时才注册包监听（每个入站包进一次 Python）；不用这套接口就没有开销。

## 快速开始

```python
for line in lib.sensing.poll_events():
    fields = dict(kv.split("=", 1) for kv in line.split(" "))
    if fields["target"] == "air":
        self.logger.info(f"{fields['player']} 空挥了一下")
    elif fields["target"] == "block":
        self.logger.info(f"{fields['player']} 对着方块 {fields['pos']} 挥动")

hit = lib.sensing.last_target("Steve")      # air / block / entity / none
```

## 函数

### `device_code`

`lib.sensing.device_code(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`sensingDeviceCode`
- 示例：

    ```python
    lib.sensing.device_code("Steve")
    ```

### `device_of`

`lib.sensing.device_of(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`sensingDeviceOf`
- 示例：

    ```python
    lib.sensing.device_of("Steve")
    ```

### `is_touch`

`lib.sensing.is_touch(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`sensingIsTouch`
- 示例：

    ```python
    lib.sensing.is_touch("Steve")
    ```

### `clear_events`

`lib.sensing.clear_events()`

- 参数：无
- 返回值：只清事件队列，不动计数与「最近一次」。
- 返回值类型：`None`
- 对应 LSE 导出：`swingClearEvents`
- 示例：

    ```python
    lib.sensing.clear_events()
    ```

### `count`

`lib.sensing.count(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：该玩家自上次 reset 以来的挥动次数。
- 返回值类型：`int`
- 对应 LSE 导出：`swingCount`
- 示例：

    ```python
    lib.sensing.count("Steve")
    ```

### `dropped`

`lib.sensing.dropped()`

- 参数：无
- 返回值：事件队列满被丢弃的条数（诊断用）。
- 返回值类型：`int`
- 对应 LSE 导出：`swingDropped`
- 示例：

    ```python
    lib.sensing.dropped()
    ```

### `last_block`

`lib.sensing.last_block(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：最近一次「对着方块挥」的坐标 x,y,z；不是方块则返回空串。
- 返回值类型：`str`
- 对应 LSE 导出：`swingLastBlock`
- 示例：

    ```python
    lib.sensing.last_block("Steve")
    ```

### `last_event`

`lib.sensing.last_event(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：该玩家最近一次挥动的事件行；没挥过返回空串。
- 返回值类型：`str`
- 对应 LSE 导出：`swingLastEvent`
- 示例：

    ```python
    lib.sensing.last_event("Steve")
    ```

### `last_target`

`lib.sensing.last_target(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：该玩家最近一次挥动的目标：air / block / entity；没挥过返回 none。
- 返回值类型：`str`
- 对应 LSE 导出：`swingLastTarget`
- 示例：

    ```python
    lib.sensing.last_target("Steve")
    ```

### `poll_events`

`lib.sensing.poll_events()`

- 参数：无
- 返回值：取走自上次轮询以来攒下的挥动事件（取走即清空）。
- 返回值类型：`list[str]`
- 对应 LSE 导出：`swingPollEvents`
- 示例：

    ```python
    lib.sensing.poll_events()
    ```

### `reset`

`lib.sensing.reset()`

- 参数：无
- 返回值：清空计数、事件队列、最近一次与丢弃计数。
- 返回值类型：`None`
- 对应 LSE 导出：`swingReset`
- 示例：

    ```python
    lib.sensing.reset()
    ```

### `total`

`lib.sensing.total()`

- 参数：无
- 返回值：全服累计挥动次数（含已退出玩家，直到 reset）。
- 返回值类型：`int`
- 对应 LSE 导出：`swingTotal`
- 示例：

    ```python
    lib.sensing.total()
    ```


---

完整索引见 [Endstone API 索引](api.md)。
