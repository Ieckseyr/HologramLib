# 渐变线 · Endstone

一条线按两端颜色插值出 N 段，用粒子或形状画出来，适合做光柱、路径指示、传送门边框这类「有方向感」的提示。

颜色是 RGB 0~1 的三元组，支持首尾不同色；密度决定每段的采样点数。

## 快速开始

```python
gid = lib.gradients.create(0, 64, 0, 0, 70, 0, 24, (1.0, 0.3, 0.1), (0.2, 0.6, 1.0))
lib.gradients.draw(gid)
```

## 函数

### `create`

`lib.gradients.create(x1, y1, z1, x2, y2, z2, segments: int = 20)`

- 参数：
  - `x1` : `Float`<br>起点 X
  - `y1` : `Float`<br>起点 Y
  - `z1` : `Float`<br>起点 Z
  - `x2` : `Float`<br>终点 X
  - `y2` : `Float`<br>终点 Y
  - `z2` : `Float`<br>终点 Z
  - `segments` : `int`（默认 `20`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`gradientCreate`
- 示例：

    ```python
    lib.gradients.create(0.0, 64.0, 0.0, 1.0, 65.0, 1.0, 1)
    ```

### `destroy`

`lib.gradients.destroy(line_id: int)`

- 参数：
  - `line_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`gradientDestroy`
- 示例：

    ```python
    lib.gradients.destroy(1)
    ```

### `destroy_all`

`lib.gradients.destroy_all()`

- 参数：无
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`int`
- 对应 LSE 导出：`gradientDestroyAll`
- 示例：

    ```python
    lib.gradients.destroy_all()
    ```

### `draw`

`lib.gradients.draw(line_id: int)`

- 参数：
  - `line_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`gradientDraw`
- 示例：

    ```python
    lib.gradients.draw(1)
    ```

### `draw_to_dimension`

`lib.gradients.draw_to_dimension(line_id: int, dim_id: int)`

- 参数：
  - `line_id` : `int`
  - `dim_id` : `int`<br>维度 id
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`gradientDrawToDimension`
- 示例：

    ```python
    lib.gradients.draw_to_dimension(1, 0)
    ```

### `remove`

`lib.gradients.remove(line_id: int)`

- 参数：
  - `line_id` : `int`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`gradientRemove`
- 示例：

    ```python
    lib.gradients.remove(1)
    ```

### `set_color`

`lib.gradients.set_color(line_id: int, r, g, b, a: float = 1.0)`

- 参数：
  - `line_id` : `int`
  - `r` : `Float`<br>红色分量 0~1
  - `g` : `Float`<br>绿色分量 0~1
  - `b` : `Float`<br>蓝色分量 0~1
  - `a` : `float`<br>透明度 0~1（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`gradientSetColor`
- 示例：

    ```python
    lib.gradients.set_color(1, "...", "...", "...", 1)
    ```

### `set_endpoints`

`lib.gradients.set_endpoints(line_id: int, x1, y1, z1, x2, y2, z2)`

- 参数：
  - `line_id` : `int`
  - `x1` : `Float`<br>起点 X
  - `y1` : `Float`<br>起点 Y
  - `z1` : `Float`<br>起点 Z
  - `x2` : `Float`<br>终点 X
  - `y2` : `Float`<br>终点 Y
  - `z2` : `Float`<br>终点 Z
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`gradientSetEndpoints`
- 示例：

    ```python
    lib.gradients.set_endpoints(1, 0.0, 64.0, 0.0, 1.0, 65.0, 1.0)
    ```

### `set_gradient`

`lib.gradients.set_gradient(line_id: int, r1, g1, b1, r2, g2, b2)`

- 参数：
  - `line_id` : `int`
  - `r1` : `Any`
  - `g1` : `Any`
  - `b1` : `Any`
  - `r2` : `Any`
  - `g2` : `Any`
  - `b2` : `Any`
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`gradientSetGradient`
- 示例：

    ```python
    lib.gradients.set_gradient(1, "...", "...", "...", "...", "...", "...")
    ```

### `set_rainbow`

`lib.gradients.set_rainbow(line_id: int, speed: float = 1.0)`

- 参数：
  - `line_id` : `int`
  - `speed` : `float`（默认 `1.0`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`bool`
- 对应 LSE 导出：`gradientSetRainbow`
- 示例：

    ```python
    lib.gradients.set_rainbow(1, 1)
    ```

### `tick`

`lib.gradients.tick(delta_seconds: float = 0.05)`

- 参数：
  - `delta_seconds` : `float`（默认 `0.05`）
- 返回值：成功返回对应结果，失败返回 `-1` / `false` / 空串（见对应 LSE 导出）
- 返回值类型：`None`
- 对应 LSE 导出：`gradientTick`
- 示例：

    ```python
    lib.gradients.tick(1)
    ```


---

完整索引见 [Endstone API 索引](api.md)。
