"""形状场景：形状表 + 编码 + 发送。"""

from __future__ import annotations

import threading

from .native import (
    SHAPE_ARROW,
    SHAPE_BOX,
    SHAPE_CIRCLE,
    SHAPE_CONE,
    SHAPE_CYLINDER,
    SHAPE_ELLIPSOID,
    SHAPE_LINE,
    SHAPE_PYRAMID,
    SHAPE_SPHERE,
    SHAPE_TEXT,
    NativeCore,
    ShapeDesc,
)

# 删除一个形状 = PrimitiveShapes 里只带 networkId 的条目（type < 0 表示不写形状体）
REMOVE_ONLY = -1


class ShapeScene:
    """线程安全的形状表。id 由插件分配，networkId 走与 LL 版相同的向下号段。"""

    FIRST_NETWORK_ID = (1 << 63) - 1

    def __init__(self, core: NativeCore) -> None:
        self._core = core
        self._lock = threading.RLock()
        self._next_network_id = self.FIRST_NETWORK_ID
        self._shapes: dict[int, ShapeDesc] = {}
        self._network_ids: dict[int, int] = {}

    # ── 基础 ──────────────────────────────────────────────────────────────
    def _alloc(self) -> tuple[int, int]:
        shape_id = len(self._shapes) + 1
        while shape_id in self._shapes:
            shape_id += 1
        network_id = self._next_network_id
        self._next_network_id -= 1
        return shape_id, network_id

    def add(self, desc: ShapeDesc) -> int:
        with self._lock:
            shape_id, network_id = self._alloc()
            desc.network_id = network_id
            self._shapes[shape_id] = desc
            self._network_ids[shape_id] = network_id
            return shape_id

    def get(self, shape_id: int) -> ShapeDesc | None:
        with self._lock:
            return self._shapes.get(shape_id)

    def remove(self, shape_id: int) -> ShapeDesc | None:
        with self._lock:
            desc = self._shapes.pop(shape_id, None)
            self._network_ids.pop(shape_id, None)
            return desc

    def clear(self) -> list[ShapeDesc]:
        with self._lock:
            descs = list(self._shapes.values())
            self._shapes.clear()
            self._network_ids.clear()
            return descs

    def snapshot(self) -> list[ShapeDesc]:
        with self._lock:
            return list(self._shapes.values())

    def items(self) -> list[tuple[int, ShapeDesc]]:
        with self._lock:
            return list(self._shapes.items())

    def __len__(self) -> int:
        with self._lock:
            return len(self._shapes)

    # ── 编码 ──────────────────────────────────────────────────────────────
    def encode(self, descs: list[ShapeDesc]) -> bytes:
        return self._core.encode_primitive_shapes(descs)

    def encode_removals(self, descs: list[ShapeDesc]) -> bytes:
        if not descs:
            return b""
        removals = []
        for desc in descs:
            only = ShapeDesc()
            only.network_id = desc.network_id
            only.type = REMOVE_ONLY
            removals.append(only)
        return self._core.encode_primitive_shapes(removals)


# ── 形状构造 ──────────────────────────────────────────────────────────────
def shape_desc(shape_type: int, x: float, y: float, z: float) -> ShapeDesc:
    desc = ShapeDesc()
    desc.type = shape_type
    desc.has_location = 1
    desc.x, desc.y, desc.z = float(x), float(y), float(z)
    desc.text_depth_test = 1
    desc.text_show_backface = 1
    desc.text_show_text_backface = 1
    return desc


def text_shape(x: float, y: float, z: float, text: str) -> ShapeDesc:
    desc = shape_desc(SHAPE_TEXT, x, y, z)
    desc.text = str(text).encode("utf-8")
    return desc


def line_shape(x1: float, y1: float, z1: float, x2: float, y2: float, z2: float) -> ShapeDesc:
    desc = shape_desc(SHAPE_LINE, x1, y1, z1)
    desc.has_end = 1
    desc.ex, desc.ey, desc.ez = float(x2), float(y2), float(z2)
    return desc


def box_shape(x: float, y: float, z: float, bx: float, by: float, bz: float) -> ShapeDesc:
    desc = shape_desc(SHAPE_BOX, x, y, z)
    desc.has_end = 1
    desc.ex, desc.ey, desc.ez = float(bx), float(by), float(bz)
    return desc


def arrow_shape(x1: float, y1: float, z1: float, x2: float, y2: float, z2: float) -> ShapeDesc:
    desc = shape_desc(SHAPE_ARROW, x1, y1, z1)
    desc.has_end = 1
    desc.ex, desc.ey, desc.ez = float(x2), float(y2), float(z2)
    return desc


def circle_shape(x: float, y: float, z: float, scale: float = 1.0) -> ShapeDesc:
    desc = shape_desc(SHAPE_CIRCLE, x, y, z)
    desc.has_scale = 1
    desc.scale = float(scale)
    return desc


def sphere_shape(x: float, y: float, z: float, scale: float = 1.0, segments: int = 16) -> ShapeDesc:
    desc = shape_desc(SHAPE_SPHERE, x, y, z)
    desc.has_scale = 1
    desc.scale = float(scale)
    desc.segments = int(segments)
    return desc


def cylinder_shape(x: float, y: float, z: float, radius_x: float, radius_z: float, height: float, segments: int = 16) -> ShapeDesc:
    desc = shape_desc(SHAPE_CYLINDER, x, y, z)
    desc.radius_x, desc.radius_z, desc.height = float(radius_x), float(radius_z), float(height)
    desc.segments = int(segments)
    return desc


def pyramid_shape(x: float, y: float, z: float, width: float, height: float, depth: float) -> ShapeDesc:
    desc = shape_desc(SHAPE_PYRAMID, x, y, z)
    desc.width, desc.height, desc.depth = float(width), float(height), float(depth)
    return desc


def ellipsoid_shape(x: float, y: float, z: float, rx: float, ry: float, rz: float, segments_per_axis: int = 12) -> ShapeDesc:
    desc = shape_desc(SHAPE_ELLIPSOID, x, y, z)
    desc.has_end = 1
    desc.ex, desc.ey, desc.ez = float(rx), float(ry), float(rz)
    desc.segments = int(segments_per_axis)
    return desc


def cone_shape(x: float, y: float, z: float, radius_x: float, radius_z: float, height: float, segments: int = 16) -> ShapeDesc:
    desc = shape_desc(SHAPE_CONE, x, y, z)
    desc.radius_x, desc.radius_z, desc.height = float(radius_x), float(radius_z), float(height)
    desc.segments = int(segments)
    return desc


def with_color(desc: ShapeDesc, argb: int) -> ShapeDesc:
    desc.has_color = 1
    desc.color = int(argb)
    return desc
