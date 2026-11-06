# 客户端感知 · Endstone

读客户端输入设备（键鼠/触屏/手柄）。endstone 的 Python 面没有 AuthInput 入口，当前三个函数返回空值，接口保留以便后续补齐。

## 快速上手

```python
is_touch = lib.sensing.is_touch("Steve")   # 目前恒为 False
```

## 函数（3 个）

#### `device_code`

`lib.sensing.device_code(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`str`
- 对应 LSE： `sensingDeviceCode`

#### `device_of`

`lib.sensing.device_of(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`str`
- 对应 LSE： `sensingDeviceOf`

#### `is_touch`

`lib.sensing.is_touch(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值类型：`bool`
- 对应 LSE： `sensingIsTouch`


---

完整索引见 [Endstone API 索引](api.md)。
