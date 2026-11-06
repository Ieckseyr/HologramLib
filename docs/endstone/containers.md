# 虚拟容器 · Endstone

协议层容器界面：给玩家开一个「箱子/熔炉」界面，里面的内容完全由本库决定，不存在的方块、不落存档。适合做商店、背包扩展、展示柜。

关闭时容器即销毁；点击回传要接 `ItemStackRequest`（当前受限，见总览）。

## 快速开始

```python
cid = lib.container.open("Steve", 3, "§8商店")
lib.container.set_slot(cid, 0, "minecraft:diamond", 1, 0, "§b钻石", "§7售价 10")
lib.container.set_interactive(cid, True)
```

## 函数

### `block_event`

`lib.container.block_event(menu_id: int, b0: int = 0, b1: int = 0)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `b0` : `int`（默认 `0`）
  - `b1` : `int`（默认 `0`）
- 返回值：屏蔽某个菜单的容器事件：记录后由出站/入站过滤处理（当前只记录）。
- 返回值类型：`bool`
- 对应 LSE 导出：`containerBlockEvent`
- 示例：

    ```python
    lib.container.block_event(1, 1, 1)
    ```

### `clear_clicks`

`lib.container.clear_clicks()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`None`
- 对应 LSE 导出：`containerClearClicks`
- 示例：

    ```python
    lib.container.clear_clicks()
    ```

### `close`

`lib.container.close(menu_id: int)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`containerClose`
- 示例：

    ```python
    lib.container.close(1)
    ```

### `close_all`

`lib.container.close_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`containerCloseAll`
- 示例：

    ```python
    lib.container.close_all()
    ```

### `dropped_clicks`

`lib.container.dropped_clicks()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`containerDroppedClicks`
- 示例：

    ```python
    lib.container.dropped_clicks()
    ```

### `get_ids`

`lib.container.get_ids()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[int]`
- 对应 LSE 导出：`containerGetIds`
- 示例：

    ```python
    lib.container.get_ids()
    ```

### `is_open`

`lib.container.is_open(menu_id: int)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`containerIsOpen`
- 示例：

    ```python
    lib.container.is_open(1)
    ```

### `open`

`lib.container.open(player_name: str, title: str, rows: int = 3)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `title` : `str`<br>标题文本
  - `rows` : `int`（默认 `3`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`containerOpen`
- 示例：

    ```python
    lib.container.open("Steve", "hello", 1)
    ```

### `poll_clicks`

`lib.container.poll_clicks()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`list[str]`
- 对应 LSE 导出：`containerPollClicks`
- 示例：

    ```python
    lib.container.poll_clicks()
    ```

### `preview`

`lib.container.preview(player_name: str, dim: int, x: float, y: float, z: float, title: str = '')`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `title` : `str`<br>标题文本（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`containerPreview`
- 示例：

    ```python
    lib.container.preview("Steve", 0, 0.0, 64.0, 0.0, "hello")
    ```

### `preview_titled`

`lib.container.preview_titled(player_name: str, dim: int, x: float, y: float, z: float, title: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `dim` : `int`<br>维度 id（0 主世界 / 1 下界 / 2 末地）
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `title` : `str`<br>标题文本
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`containerPreviewTitled`
- 示例：

    ```python
    lib.container.preview_titled("Steve", 0, 0.0, 64.0, 0.0, "hello")
    ```

### `set_interactive`

`lib.container.set_interactive(menu_id: int, enabled: bool)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `enabled` : `bool`<br>开 / 关
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`containerSetInteractive`
- 示例：

    ```python
    lib.container.set_interactive(1, "...")
    ```

### `set_item`

`lib.container.set_item(menu_id: int, slot: int, item_type: str, count: int, damage: int = 0, name: str = '', lore_csv: str = '')`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `slot` : `int`<br>槽位号
  - `item_type` : `str`
  - `count` : `int`<br>数量
  - `damage` : `int`<br>耐久损伤值（默认 `0`）
  - `name` : `str`<br>名字 / 标识符（默认 `''`）
  - `lore_csv` : `str`（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`containerSetItem`
- 示例：

    ```python
    lib.container.set_item(1, 1, "...", 1, 1, "hello", "...")
    ```

### `set_item_enchanted`

`lib.container.set_item_enchanted(menu_id: int, slot: int, item_type: str, count: int, damage: int, name: str, lore_csv: str, ench_csv: str = '')`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `slot` : `int`<br>槽位号
  - `item_type` : `str`
  - `count` : `int`<br>数量
  - `damage` : `int`<br>耐久损伤值
  - `name` : `str`<br>名字 / 标识符
  - `lore_csv` : `str`
  - `ench_csv` : `str`（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`containerSetItemEnchanted`
- 示例：

    ```python
    lib.container.set_item_enchanted(1, 1, "...", 1, 1, "hello", "...", "...")
    ```

### `set_title`

`lib.container.set_title(menu_id: int, title: str)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `title` : `str`<br>标题文本
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`containerSetTitle`
- 示例：

    ```python
    lib.container.set_title(1, "hello")
    ```

### `update`

`lib.container.update(menu_id: int, title: str)`

- 参数：
  - `menu_id` : `int`<br>容器菜单 id
  - `title` : `str`<br>标题文本
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`containerUpdate`
- 示例：

    ```python
    lib.container.update(1, "hello")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
