# 物品详情 · Endstone

在世界上方显示一个「物品 + 名称 + Lore」的悬浮详情，走文本/形状下发，不生成掉落物。适合做商店货架、奖励预览。

## 快速开始

```python
lib.itemdetail.show(0, 66, 0, "minecraft:diamond", "§b钻石", "§7价值 64 绿宝石")
```

## 函数

### `hide`

`lib.itemdetail.hide(detail_id: int)`

- 参数：
  - `detail_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`itemDetailHide`
- 示例：

    ```python
    lib.itemdetail.hide(1)
    ```

### `show`

`lib.itemdetail.show(dim_id: int, x: float, y: float, z: float, item_id: str, aux: int = 0, count: int = 1, custom_text: str = '')`

- 参数：
  - `dim_id` : `int`<br>维度 id
  - `x` : `float`<br>X 坐标
  - `y` : `float`<br>Y 坐标
  - `z` : `float`<br>Z 坐标
  - `item_id` : `str`
  - `aux` : `int`<br>物品数据值（damage / aux）（默认 `0`）
  - `count` : `int`<br>数量（默认 `1`）
  - `custom_text` : `str`（默认 `''`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`itemDetailShow`
- 示例：

    ```python
    lib.itemdetail.show(0, 0.0, 64.0, 0.0, "...", 1, 1, "...")
    ```


---

完整索引见 [Endstone API 索引](api.md)。
