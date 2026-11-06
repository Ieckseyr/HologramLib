"""形状域对外 API —— 与 LSE 的 46 个 shape* 导出逐一对应。"""

from __future__ import annotations

import threading

from endstone import Player

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
    ShapeDesc,
)
from .shapes import (
    arrow_shape,
    box_shape,
    circle_shape,
    cone_shape,
    cylinder_shape,
    ellipsoid_shape,
    line_shape,
    pyramid_shape,
    shape_desc,
    sphere_shape,
    text_shape,
)

_ALL = ("all", None)


class ShapeApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        # 每个形状的下发目标：("all", None) | ("player", name) | ("dim", dimId)
        self._targets: dict[int, tuple[str, object]] = {}

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _players(self) -> list[Player]:
        return list(self._plugin.server.online_players)

    def _send(self, players, payload: bytes) -> None:
        if not payload:
            return
        packet_id = self._plugin.core.primitive_shapes_packet_id
        for player in players:
            try:
                player.send_packet(packet_id, payload)
            except Exception as exc:  # 玩家刚好掉线 / 未进世界
                self._plugin.logger.debug(f"send_packet 失败 ({player.name}): {exc}")

    def _resolve(self, target: tuple[str, object]) -> list[Player]:
        kind, value = target
        players = self._players()
        if kind == "player":
            return [p for p in players if p.name == value]
        if kind == "dim":
            from .holograms import _dim_id

            return [p for p in players if _dim_id(p) == value]
        return players

    def _broadcast(self, descs: list[ShapeDesc], target: tuple[str, object] = _ALL) -> None:
        if not descs:
            return
        self._send(self._resolve(target), self._plugin.scene.encode(descs))

    def _desc(self, shape_id: int) -> ShapeDesc | None:
        return self._plugin.scene.get(shape_id)

    def _add(self, desc: ShapeDesc, target: tuple[str, object] = _ALL) -> int:
        shape_id = self._plugin.scene.add(desc)
        with self._lock:
            self._targets[shape_id] = target
        if desc.has_dimension:
            target = ("dim", desc.dimension_id)
            with self._lock:
                self._targets[shape_id] = target
        self._broadcast([desc], target)
        return shape_id

    def _remove_ids(self, ids: list[int]) -> bool:
        scene = self._plugin.scene
        descs = []
        targets = []
        for shape_id in ids:
            desc = scene.remove(shape_id)
            if desc is None:
                return False
            with self._lock:
                targets.append(self._targets.pop(shape_id, _ALL))
            descs.append(desc)
        scene_ids = [id(d) for d in descs]
        _ = scene_ids
        for target in set(targets):
            subset = [d for d, t in zip(descs, targets) if t == target]
            self._send(self._resolve(target), scene.encode_removals(subset))
        return True

    # ── 创建（10 种形状） ─────────────────────────────────────────────────
    def create_text(self, x: float, y: float, z: float, text: str) -> int:
        return self._add(text_shape(x, y, z, text))

    def create_line(self, x1: float, y1: float, z1: float, x2: float, y2: float, z2: float) -> int:
        return self._add(line_shape(x1, y1, z1, x2, y2, z2))

    def create_box(self, x1: float, y1: float, z1: float, x2: float, y2: float, z2: float) -> int:
        return self._add(box_shape(x1, y1, z1, x2, y2, z2))

    def create_arrow(self, x1: float, y1: float, z1: float, x2: float, y2: float, z2: float) -> int:
        return self._add(arrow_shape(x1, y1, z1, x2, y2, z2))

    def create_circle(self, x: float, y: float, z: float, scale: float = 1.0) -> int:
        return self._add(circle_shape(x, y, z, scale))

    def create_sphere(self, x: float, y: float, z: float, scale: float = 1.0) -> int:
        return self._add(sphere_shape(x, y, z, scale))

    def create_cylinder(
        self, x: float, y: float, z: float, radius_x: float, radius_z: float, height: float, segments: int = 16
    ) -> int:
        return self._add(cylinder_shape(x, y, z, radius_x, radius_z, height, segments))

    def create_pyramid(self, x: float, y: float, z: float, width: float, height: float, depth: float) -> int:
        return self._add(pyramid_shape(x, y, z, width, height, depth))

    def create_ellipsoid(
        self, x: float, y: float, z: float, radius_x: float, radius_y: float, radius_z: float, segments: int = 12
    ) -> int:
        return self._add(ellipsoid_shape(x, y, z, radius_x, radius_y, radius_z, segments))

    def create_cone(
        self, x: float, y: float, z: float, radius_x: float, radius_z: float, height: float, segments: int = 16
    ) -> int:
        return self._add(cone_shape(x, y, z, radius_x, radius_z, height, segments))

    def create(self, shape_type: int, x: float, y: float, z: float) -> int:
        return self._add(shape_desc(shape_type, x, y, z))

    # ── 绘制 / 更新 / 删除 ────────────────────────────────────────────────
    def draw(self, shape_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        self._broadcast([desc], self._targets.get(shape_id, _ALL))
        return True

    def draw_to_player(self, shape_id: int, player_name: str) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        with self._lock:
            self._targets[shape_id] = ("player", player_name)
        self._broadcast([desc], ("player", player_name))
        return True

    def draw_to_dimension(self, shape_id: int, dim_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        with self._lock:
            self._targets[shape_id] = ("dim", int(dim_id))
        self._broadcast([desc], ("dim", int(dim_id)))
        return True

    def draw_batch(self, ids: list[int]) -> bool:
        descs = [d for d in (self._desc(i) for i in ids) if d is not None]
        if not descs:
            return False
        self._broadcast(descs)
        return True

    def update(self, shape_id: int) -> bool:
        return self.draw(shape_id)

    def update_to_player(self, shape_id: int, player_name: str) -> bool:
        return self.draw_to_player(shape_id, player_name)

    def update_to_dimension(self, shape_id: int, dim_id: int) -> bool:
        return self.draw_to_dimension(shape_id, dim_id)

    def remove(self, shape_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        target = self._targets.get(shape_id, _ALL)
        self._plugin.scene.remove(shape_id)
        with self._lock:
            self._targets.pop(shape_id, None)
        self._send(self._resolve(target), self._plugin.scene.encode_removals([desc]))
        return True

    def remove_to_player(self, shape_id: int, player_name: str) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        player = self._plugin.server.get_player(player_name)
        players = [player] if player is not None else []
        self._send(players, self._plugin.scene.encode_removals([desc]))
        return True

    def remove_to_dimension(self, shape_id: int, dim_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        self._send(self._resolve(("dim", int(dim_id))), self._plugin.scene.encode_removals([desc]))
        return True

    def destroy(self, shape_id: int) -> bool:
        return self.remove(shape_id)

    def destroy_batch(self, ids: list[int]) -> bool:
        return self._remove_ids([int(i) for i in ids])

    def destroy_all(self) -> int:
        """清空所有形状（一次批量发删除包）。返回清理数量。"""
        return len(self.destroy_all_remaining())

    def destroy_all_remaining(self) -> list[int]:
        scene = self._plugin.scene
        remaining = [sid for sid, _ in scene.items()]
        if not remaining:
            return []
        descs = []
        for sid in remaining:
            desc = scene.remove(sid)
            if desc is not None:
                descs.append(desc)
        with self._lock:
            self._targets.clear()
        self._send(self._players(), scene.encode_removals(descs))
        return remaining

    def clear(self) -> int:
        return len(self.destroy_all_remaining())

    # ── 进服补发 ──────────────────────────────────────────────────────────
    def sync_to(self, player) -> None:
        """给单个玩家补发当前全部形状（进服时调用）。"""
        descs = self._plugin.scene.snapshot()
        if descs:
            self._send([player], self._plugin.scene.encode(descs))

    def forget(self, player) -> None:
        """玩家退服（形状在客户端侧，退服无需清理）。"""
        _ = player

    # ── 属性设置 ──────────────────────────────────────────────────────────
    def set_text(self, shape_id: int, text: str) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.text = str(text).encode("utf-8")
        return self.update(shape_id)

    def set_color(self, shape_id: int, r: float, g: float, b: float, a: float = 1.0) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_color = 1
        desc.color = _pack(r, g, b, a)
        return self.update(shape_id)

    def set_background_color(self, shape_id: int, r: float, g: float, b: float, a: float = 1.0) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_text_background = 1
        desc.text_background_color = _pack(r, g, b, a)
        return self.update(shape_id)

    def clear_background_color(self, shape_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_text_background = 0
        return self.update(shape_id)

    def set_rotation(self, shape_id: int, pitch: float, yaw: float, roll: float = 0.0) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_rotation = 1
        desc.rx, desc.ry, desc.rz = float(pitch), float(yaw), float(roll)
        return self.update(shape_id)

    def clear_rotation(self, shape_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_rotation = 0
        return self.update(shape_id)

    def set_scale(self, shape_id: int, scale: float) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_scale = 1
        desc.scale = float(scale)
        return self.update(shape_id)

    def set_location(self, shape_id: int, x: float, y: float, z: float) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_location = 1
        desc.x, desc.y, desc.z = float(x), float(y), float(z)
        return self.update(shape_id)

    def set_depth_test(self, shape_id: int, enabled: bool) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.text_depth_test = 1 if enabled else 0
        return self.update(shape_id)

    def set_duration(self, shape_id: int, seconds: float) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_total_time_left = 1
        desc.total_time_left = float(seconds)
        return self.update(shape_id)

    def set_max_render_distance(self, shape_id: int, distance: float) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_max_render_distance = 1
        desc.max_render_distance = float(distance)
        return self.update(shape_id)

    def set_dimension(self, shape_id: int, dim_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_dimension = 1
        desc.dimension_id = int(dim_id)
        with self._lock:
            self._targets[shape_id] = ("dim", int(dim_id))
        return self.update(shape_id)

    def attach_entity(self, shape_id: int, entity_unique_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_attached_entity = 1
        desc.attached_entity_id = int(entity_unique_id)
        return self.update(shape_id)

    def clear_attached_entity(self, shape_id: int) -> bool:
        desc = self._desc(shape_id)
        if desc is None:
            return False
        desc.has_attached_entity = 0
        return self.update(shape_id)

    # ── 查询 ──────────────────────────────────────────────────────────────
    def exists(self, shape_id: int) -> bool:
        return self._desc(shape_id) is not None

    def get_all_shape_ids(self) -> list[int]:
        return [sid for sid, _ in self._plugin.scene.items()]

    def get_shape_type(self, shape_id: int) -> int:
        desc = self._desc(shape_id)
        return int(desc.type) if desc is not None else -1

    def get_text(self, shape_id: int) -> str:
        desc = self._desc(shape_id)
        if desc is None or not desc.text:
            return ""
        return bytes(desc.text).decode("utf-8", "replace")

    def get_location(self, shape_id: int) -> list[float]:
        desc = self._desc(shape_id)
        if desc is None:
            return []
        return [desc.x, desc.y, desc.z]

    def get_rotation(self, shape_id: int) -> list[float]:
        desc = self._desc(shape_id)
        if desc is None or not desc.has_rotation:
            return []
        return [desc.rx, desc.ry, desc.rz]

    def get_color(self, shape_id: int) -> list[float]:
        desc = self._desc(shape_id)
        if desc is None or not desc.has_color:
            return []
        argb = int(desc.color) & 0xFFFFFFFF
        a = ((argb >> 24) & 0xFF) / 255.0
        r = ((argb >> 16) & 0xFF) / 255.0
        g = ((argb >> 8) & 0xFF) / 255.0
        b = (argb & 0xFF) / 255.0
        return [r, g, b, a]

    def _ids(self) -> list[int]:
        return [sid for sid, _ in self._plugin.scene.items()]

    def find_text_by_location(self, x: float, y: float, z: float, radius: float = 1.0) -> list[int]:
        out = []
        r2 = float(radius) ** 2
        for sid, desc in self._plugin.scene.items():
            if int(desc.type) != SHAPE_TEXT:
                continue
            dx, dy, dz = desc.x - x, desc.y - y, desc.z - z
            if dx * dx + dy * dy + dz * dz <= r2:
                out.append(sid)
        return out

    def find_text_by_location_and_content(self, x: float, y: float, z: float, radius: float, text: str) -> int:
        for sid in self.find_text_by_location(x, y, z, radius):
            if self.get_text(sid) == text:
                return sid
        return -1

    @property
    def count(self) -> int:
        return len(self._plugin.scene)


def _pack(r: float, g: float, b: float, a: float = 1.0) -> int:
    def c(v: float) -> int:
        return max(0, min(255, int(round(float(v) * 255.0))))

    return (c(a) << 24) | (c(r) << 16) | (c(g) << 8) | c(b)


__all__ = ["ShapeApi", "SHAPE_ARROW", "SHAPE_BOX", "SHAPE_CIRCLE", "SHAPE_CONE", "SHAPE_CYLINDER", "SHAPE_ELLIPSOID", "SHAPE_LINE", "SHAPE_PYRAMID", "SHAPE_SPHERE", "SHAPE_TEXT"]
