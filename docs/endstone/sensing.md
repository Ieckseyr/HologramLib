# 客户端感知 · Endstone

**空手挥动计数**（`count` / `total` / `reset`）：玩家每次挥手（空手挥、挥剑、打空气、点方块、打实体）都走入站 `AnimatePacket`（包 id 44，`Action = Swing`），库按玩家记数。

协议里只有 `Swing` 一个动作，客户端不会告诉服务端「对着什么挥」，所以**不区分目标**。

**输入设备**（`device_*` / `is_touch`）：endstone 的 Python 面没有 AuthInput 入口，当前返回空值。

!!! 注意

    计数第一次被调用时才注册包监听（每个入站包进一次 Python）；不用这套接口就没有开销。

## 快速开始

```python
logger.info(f"Steve 挥了 {lib.sensing.count('Steve')} 次，全服 {lib.sensing.total()} 次")
lib.sensing.reset()
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

### `count`

`lib.sensing.count(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：该玩家自上次 reset 以来的挥动次数；没挥过或不在线返回 0。
- 返回值类型：`int`
- 对应 LSE 导出：`swingCount`
- 示例：

    ```python
    lib.sensing.count("Steve")
    ```

### `reset`

`lib.sensing.reset()`

- 参数：无
- 返回值：清空全部玩家计数与总数。
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
