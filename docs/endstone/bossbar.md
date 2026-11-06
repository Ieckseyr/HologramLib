# Boss 条 · Endstone

屏幕顶部的 Boss 血条，纯协议下发：标题、血量百分比、颜色、样式都能改。适合做「Boss 血量」「任务进度」「倒计时」。

一个玩家可以同时有多条（`add_on` 会叠上去），也可以一次清空。

## 快速开始

```python
bid = lib.bossbar.add("Steve", "§c世界 Boss", 1.0)
lib.bossbar.set_health(bid, 0.5)
lib.bossbar.set_style(bid, 0, 1)   # 颜色 红, 样式 分段
```

## 函数

### `add`

`lib.bossbar.add(player_name: str, title: str, health_percent: float = 1.0, color: int = 0, overlay: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `title` : `str`<br>标题文本
  - `health_percent` : `float`（默认 `1.0`）
  - `color` : `int`<br>颜色（RGB 整数或枚举，见各域说明）（默认 `0`）
  - `overlay` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`bossBarAdd`
- 示例：

    ```python
    lib.bossbar.add("Steve", "hello", 1, 1, 1)
    ```

### `add_on`

`lib.bossbar.add_on(player_name: str, target_id: int, title: str, percent: float = 1.0, color: int = 0, overlay: int = 0)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `target_id` : `int`
  - `title` : `str`<br>标题文本
  - `percent` : `float`<br>百分比 0~1（默认 `1.0`）
  - `color` : `int`<br>颜色（RGB 整数或枚举，见各域说明）（默认 `0`）
  - `overlay` : `int`（默认 `0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`bossBarAddOn`
- 示例：

    ```python
    lib.bossbar.add_on("Steve", 1, "hello", 1, 1, 1)
    ```

### `count`

`lib.bossbar.count(player_name: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`bossBarCount`
- 示例：

    ```python
    lib.bossbar.count("Steve")
    ```

### `get_info`

`lib.bossbar.get_info(bar_id: int)`

- 参数：
  - `bar_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`str`
- 对应 LSE 导出：`bossBarGetInfo`
- 示例：

    ```python
    lib.bossbar.get_info(1)
    ```

### `probe`

`lib.bossbar.probe()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`bossBarProbe`
- 示例：

    ```python
    lib.bossbar.probe()
    ```

### `remove`

`lib.bossbar.remove(bar_id: int)`

- 参数：
  - `bar_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`bossBarRemove`
- 示例：

    ```python
    lib.bossbar.remove(1)
    ```

### `remove_all`

`lib.bossbar.remove_all(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`bossBarRemoveAll`
- 示例：

    ```python
    lib.bossbar.remove_all("Steve")
    ```

### `set_health`

`lib.bossbar.set_health(bar_id: int, percent: float)`

- 参数：
  - `bar_id` : `int`
  - `percent` : `float`<br>百分比 0~1
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`bossBarSetHealth`
- 示例：

    ```python
    lib.bossbar.set_health(1, 1)
    ```

### `set_style`

`lib.bossbar.set_style(bar_id: int, color: int, overlay: int)`

- 参数：
  - `bar_id` : `int`
  - `color` : `int`<br>颜色（RGB 整数或枚举，见各域说明）
  - `overlay` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`bossBarSetStyle`
- 示例：

    ```python
    lib.bossbar.set_style(1, 1, 1)
    ```

### `set_title`

`lib.bossbar.set_title(bar_id: int, title: str)`

- 参数：
  - `bar_id` : `int`
  - `title` : `str`<br>标题文本
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`bossBarSetTitle`
- 示例：

    ```python
    lib.bossbar.set_title(1, "hello")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
