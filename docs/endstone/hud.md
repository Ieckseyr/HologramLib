# HUD 显隐 · Endstone

HUD 元素显隐：血量、饥饿、坐标、十字准星等，按玩家单独控制，可以只隐藏某几项。

`set_elements` 传要**保留**的元素列表，返回真正被隐藏的条目数；`set_raw` 直接把整张位图写下去，用来做实验或兼容新版本。

## 快速开始

```python
lib.hud.set_elements("Steve", ["hotbar", "crosshair"])   # 只留快捷栏和准星
lib.hud.reset_all("Steve")
```

## 函数

### `probe`

`lib.hud.probe()`

- 参数：无
- 返回值：把「Hide + 保留 12 项」的 body 打出来，便于和抓包对照。
- 返回值类型：`str`
- 对应 LSE 导出：`hudProbe`
- 示例：

    ```python
    lib.hud.probe()
    ```

### `reset_all`

`lib.hud.reset_all(player_name: str)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`hudResetAll`
- 示例：

    ```python
    lib.hud.reset_all("Steve")
    ```

### `set_element`

`lib.hud.set_element(player_name: str, element: int, visible: bool)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `element` : `int`
  - `visible` : `bool`<br>是否可见
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`hudSetElement`
- 示例：

    ```python
    lib.hud.set_element("Steve", 1, "...")
    ```

### `set_elements`

`lib.hud.set_elements(player_name: str, mask: int, visible: bool)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `mask` : `int`
  - `visible` : `bool`<br>是否可见
- 返回值：按位掩码设置，语义与 LL 版一致：
- 返回值类型：`int`
- 对应 LSE 导出：`hudSetElements`
- 示例：

    ```python
    lib.hud.set_elements("Steve", 1, "...")
    ```

### `set_raw`

`lib.hud.set_raw(player_name: str, mask: int, visibility: int)`

- 参数：
  - `player_name` : `str`<br>玩家名（在线玩家，大小写敏感）
  - `mask` : `int`
  - `visibility` : `int`
- 返回值：原始写法：mask 命中什么就发什么，visibility 原样下发（排障用）。
- 返回值类型：`bool`
- 对应 LSE 导出：`hudSetRaw`
- 示例：

    ```python
    lib.hud.set_raw("Steve", 1, 1)
    ```


---

完整索引见 [Endstone API 索引](api.md)。
