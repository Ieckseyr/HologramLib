"""渐变线（GradientLine）域 —— LeviLamina 版 GradientLineManager 的 endstone 移植。"""

from __future__ import annotations

import colorsys
import threading

from .shapes import line_shape


def _pack(r: float, g: float, b: float, a: float = 1.0) -> int:
    def c(v: float) -> int:
        return max(0, min(255, int(round(float(v) * 255.0))))

    return (c(a) << 24) | (c(r) << 16) | (c(g) << 8) | c(b)


def _lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def _lerp_color(c1: tuple[float, ...], c2: tuple[float, ...], t: float) -> tuple[float, float, float, float]:
    return (
        _lerp(c1[0], c2[0], t),
        _lerp(c1[1], c2[1], t),
        _lerp(c1[2], c2[2], t),
        _lerp(c1[3], c2[3], t),
    )


class GradientLine:
    def __init__(self, line_id: int, p1, p2, segments: int) -> None:
        self.id = line_id
        self.p1 = p1
        self.p2 = p2
        self.segments = max(2, min(100, int(segments)))
        self.color1 = (1.0, 0.0, 0.0, 1.0)
        self.color2 = (0.0, 0.0, 1.0, 1.0)
        self.rainbow = False
        self.rainbow_speed = 1.0
        self.anim_time = 0.0
        self.dim_id = -1  # -1 = 全部维度

        self.segment_ids: list[int] = []
        self.drawn = False
        self.last_colors: list[int] = []


class GradientApi:
    """渐变线对外 API，方法名对齐 LSE 的 gradient*。"""

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._items: dict[int, GradientLine] = {}
        self._next_id = 1

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _viewers(self, item: GradientLine) -> list:
        players = list(self._plugin.server.online_players)
        if item.dim_id >= 0:
            from .holograms import _dim_id

            players = [p for p in players if _dim_id(p) == item.dim_id]
        return players

    def _segment_color(self, item: GradientLine, t: float) -> tuple[float, float, float, float]:
        if item.rainbow:
            hue = (item.anim_time * item.rainbow_speed * 60.0 + t * 360.0) % 360.0
            r, g, b = colorsys.hsv_to_rgb(hue / 360.0, 1.0, 1.0)
            return (r, g, b, 1.0)
        return _lerp_color(item.color1, item.color2, t)

    def _build_segments(self, item: GradientLine) -> list:
        from .shapes import shape_desc  # 局部导入避免循环
        from .native import SHAPE_LINE

        shapes = []
        n = item.segments
        for i in range(n):
            t0 = i / n
            t1 = (i + 1) / n
            p0 = (
                _lerp(item.p1[0], item.p2[0], t0),
                _lerp(item.p1[1], item.p2[1], t0),
                _lerp(item.p1[2], item.p2[2], t0),
            )
            p1 = (
                _lerp(item.p1[0], item.p2[0], t1),
                _lerp(item.p1[1], item.p2[1], t1),
                _lerp(item.p1[2], item.p2[2], t1),
            )
            desc = line_shape(p0[0], p0[1], p0[2], p1[0], p1[1], p1[2])
            desc.has_color = 1
            desc.color = _pack(*self._segment_color(item, (t0 + t1) * 0.5))
            shapes.append(desc)
        _ = shape_desc
        _ = SHAPE_LINE
        return shapes

    # ── 创建 / 销毁 ───────────────────────────────────────────────────────
    def create(self, x1, y1, z1, x2, y2, z2, segments: int = 20) -> int:
        with self._lock:
            line_id = self._next_id
            self._next_id += 1
            self._items[line_id] = GradientLine(line_id, (float(x1), float(y1), float(z1)), (float(x2), float(y2), float(z2)), segments)
            return line_id

    def get(self, line_id: int) -> GradientLine | None:
        with self._lock:
            return self._items.get(line_id)

    def _drop_segments(self, item: GradientLine, notify: bool = True) -> None:
        scene = self._plugin.scene
        removals = []
        for shape_id in item.segment_ids:
            desc = scene.remove(shape_id)
            if desc is not None:
                removals.append(desc)
        item.segment_ids.clear()
        item.drawn = False
        item.last_colors.clear()
        if notify and removals:
            self._plugin.api._send(list(self._plugin.server.online_players), scene.encode_removals(removals))

    def destroy(self, line_id: int) -> bool:
        with self._lock:
            item = self._items.pop(line_id, None)
        if item is None:
            return False
        self._drop_segments(item)
        return True

    def destroy_all(self) -> int:
        with self._lock:
            items = list(self._items.values())
            self._items.clear()
        for item in items:
            self._drop_segments(item)
        return len(items)

    # ── 设置 ──────────────────────────────────────────────────────────────
    def set_color(self, line_id: int, r, g, b, a: float = 1.0) -> bool:
        item = self.get(line_id)
        if item is None:
            return False
        item.color1 = (r, g, b, a)
        item.color2 = (r, g, b, a)
        item.rainbow = False
        return True

    def set_gradient(self, line_id: int, r1, g1, b1, r2, g2, b2) -> bool:
        item = self.get(line_id)
        if item is None:
            return False
        item.color1 = (r1, g1, b1, 1.0)
        item.color2 = (r2, g2, b2, 1.0)
        item.rainbow = False
        return True

    def set_rainbow(self, line_id: int, speed: float = 1.0) -> bool:
        item = self.get(line_id)
        if item is None:
            return False
        item.rainbow = True
        item.rainbow_speed = float(speed)
        return True

    def set_endpoints(self, line_id: int, x1, y1, z1, x2, y2, z2) -> bool:
        item = self.get(line_id)
        if item is None:
            return False
        item.p1 = (x1, y1, z1)
        item.p2 = (x2, y2, z2)
        if item.drawn:  # 端点变了要重建线段
            self._drop_segments(item)
            self.draw(line_id)
        return True

    # ── 绘制 ──────────────────────────────────────────────────────────────
    def draw(self, line_id: int) -> bool:
        item = self.get(line_id)
        if item is None:
            return False
        scene = self._plugin.scene
        shapes = self._build_segments(item)
        for desc in shapes:
            item.segment_ids.append(scene.add(desc))
        item.last_colors = [d.color for d in shapes]
        item.drawn = True
        self._plugin.api._send(self._viewers(item), scene.encode(shapes))
        return True

    def draw_to_dimension(self, line_id: int, dim_id: int) -> bool:
        item = self.get(line_id)
        if item is None:
            return False
        item.dim_id = int(dim_id)
        return self.draw(line_id)

    def remove(self, line_id: int) -> bool:
        item = self.get(line_id)
        if item is None:
            return False
        self._drop_segments(item)
        return True

    # ── 每帧 ──────────────────────────────────────────────────────────────
    def tick(self, delta_seconds: float = 0.05) -> None:
        with self._lock:
            items = [it for it in self._items.values() if it.drawn]
        scene = self._plugin.scene
        for item in items:
            if not item.rainbow:
                continue
            item.anim_time += delta_seconds
            shapes = self._build_segments(item)
            changed = []
            for desc, old in zip(shapes, item.last_colors):
                if desc.color != old:
                    changed.append(desc)
            if not changed:
                continue
            # 把新颜色写回场景里的对应形状
            for shape_id, desc in zip(item.segment_ids, shapes):
                existing = scene.get(shape_id)
                if existing is not None and existing.color != desc.color:
                    existing.color = desc.color
                    existing.has_color = 1
            item.last_colors = [d.color for d in shapes]
            self._plugin.api._send(self._viewers(item), scene.encode(changed))

    def __len__(self) -> int:
        with self._lock:
            return len(self._items)
