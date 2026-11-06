"""粒子形状（Particle）域 —— LeviLamina 版 ParticleShapeManager 的 endstone 移植。"""

from __future__ import annotations

import math
import threading

TOTAL_CAP = 2048

KIND_POINT = 0
KIND_LINE = 1
KIND_RECT = 2
KIND_PLANE = 3
KIND_BOX = 4
KIND_BOX_FACES = 5
KIND_POLY = 6

KIND_NAMES = {
    KIND_POINT: "point",
    KIND_LINE: "line",
    KIND_RECT: "rect",
    KIND_PLANE: "plane",
    KIND_BOX: "box",
    KIND_BOX_FACES: "boxFaces",
    KIND_POLY: "poly",
}

DEFAULT_EFFECT = "minecraft:endrod"


def _sample_segment(out: list[tuple[float, float, float]], a, b, step: float) -> None:
    dx, dy, dz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    length = math.sqrt(dx * dx + dy * dy + dz * dz)
    if length < 1e-6:
        out.append((a[0], a[1], a[2]))
        return
    count = max(1, int(math.ceil(length / step)))
    for i in range(count + 1):
        if len(out) >= TOTAL_CAP:
            return
        t = i / count
        out.append((a[0] + dx * t, a[1] + dy * t, a[2] + dz * t))


def _axis_to_xyz(axis: int, u: float, v: float):
    if axis == 0:
        return u, v, 0.0
    if axis == 1:
        return 0.0, v, u
    return u, 0.0, v


def _rect_corners(axis: int, w: float, h: float):
    hw, hh = w / 2.0, h / 2.0
    return [_axis_to_xyz(axis, u, v) for u, v in ((-hw, -hh), (hw, -hh), (hw, hh), (-hw, hh))]


def _euler_matrix(rx: float, ry: float, rz: float):
    cx, sx = math.cos(math.radians(rx)), math.sin(math.radians(rx))
    cy, sy = math.cos(math.radians(ry)), math.sin(math.radians(ry))
    cz, sz = math.cos(math.radians(rz)), math.sin(math.radians(rz))
    return (
        (cz * cy, cz * sy * sx - sz * cx, cz * sy * cx + sz * sx),
        (sz * cy, sz * sy * sx + cz * cx, sz * sy * cx - cz * sx),
        (-sy, cy * sx, cy * cx),
    )


class ParticleShape:
    def __init__(self, shape_id: int, kind: int) -> None:
        self.id = shape_id
        self.kind = kind
        self.player_name = ""
        self.dim_id = 0

        # 几何
        self.ax = self.ay = self.az = 0.0
        self.bx = self.by = self.bz = 0.0
        self.w = self.h = 0.0
        self.axis = 0
        self.hx = self.hy = self.hz = 0.0
        self.poly_verts: list[float] = []
        self.poly_edges: list[int] = []
        self.step = 1.0

        # 变换
        self.px = self.py = self.pz = 0.0
        self.rot_x = self.rot_y = self.rot_z = 0.0
        self.scale = 1.0
        self.spin_x = self.spin_y = self.spin_z = 0.0

        # 跟随 / 动画
        self.follow_uuid: str = ""
        self.off_x = self.off_y = self.off_z = 0.0
        self.anim_active = False
        self.anim_from = (0.0, 0.0, 0.0)
        self.anim_to = (0.0, 0.0, 0.0)
        self.anim_start = 0
        self.anim_duration = 0

        # 表现
        self.effect = DEFAULT_EFFECT
        self.visible_all = True
        self.visible_players: set[str] = set()
        self.view_distance = 96
        self.interval = 4

        self.end_tick = 0
        self.next_emit = 0

        self._cache: list[tuple[float, float, float]] | None = None

    def invalidate(self) -> None:
        self._cache = None

    def local_points(self) -> list[tuple[float, float, float]]:
        if self._cache is not None:
            return self._cache
        pts: list[tuple[float, float, float]] = []
        k = self.kind
        if k == KIND_POINT:
            pts.append((0.0, 0.0, 0.0))
        elif k == KIND_LINE:
            _sample_segment(pts, (self.ax, self.ay, self.az), (self.bx, self.by, self.bz), self.step)
        elif k == KIND_RECT:
            corners = _rect_corners(self.axis, self.w, self.h)
            for i in range(4):
                _sample_segment(pts, corners[i], corners[(i + 1) % 4], self.step)
        elif k == KIND_PLANE:
            hw, hh = self.w / 2.0, self.h / 2.0
            u = -hw
            while u <= hw + 1e-3 and len(pts) < TOTAL_CAP:
                v = -hh
                while v <= hh + 1e-3 and len(pts) < TOTAL_CAP:
                    pts.append(_axis_to_xyz(self.axis, u, v))
                    v += self.step
                u += self.step
        elif k == KIND_BOX:
            corners = [
                (-self.hx, -self.hy, -self.hz),
                (self.hx, -self.hy, -self.hz),
                (self.hx, -self.hy, self.hz),
                (-self.hx, -self.hy, self.hz),
                (-self.hx, self.hy, -self.hz),
                (self.hx, self.hy, -self.hz),
                (self.hx, self.hy, self.hz),
                (-self.hx, self.hy, self.hz),
            ]
            edges = [
                (0, 1), (1, 2), (2, 3), (3, 0),
                (4, 5), (5, 6), (6, 7), (7, 4),
                (0, 4), (1, 5), (2, 6), (3, 7),
            ]
            for i, j in edges:
                _sample_segment(pts, corners[i], corners[j], self.step)
        elif k == KIND_BOX_FACES:
            for face in range(6):
                if len(pts) >= TOTAL_CAP:
                    break
                u = -1.0
                while u <= 1.0 + 1e-3 and len(pts) < TOTAL_CAP:
                    v = -1.0
                    while v <= 1.0 + 1e-3 and len(pts) < TOTAL_CAP:
                        if face == 0:
                            pts.append((-self.hx, u * self.hy, v * self.hz))
                        elif face == 1:
                            pts.append((self.hx, u * self.hy, v * self.hz))
                        elif face == 2:
                            pts.append((u * self.hx, -self.hy, v * self.hz))
                        elif face == 3:
                            pts.append((u * self.hx, self.hy, v * self.hz))
                        elif face == 4:
                            pts.append((u * self.hx, v * self.hy, -self.hz))
                        else:
                            pts.append((u * self.hx, v * self.hy, self.hz))
                        v += self.step
                    u += self.step
        elif k == KIND_POLY:
            n = len(self.poly_verts) // 3
            for e in range(0, len(self.poly_edges) - 1, 2):
                i, j = self.poly_edges[e], self.poly_edges[e + 1]
                if i >= n or j >= n:
                    continue
                a = (self.poly_verts[i * 3], self.poly_verts[i * 3 + 1], self.poly_verts[i * 3 + 2])
                b = (self.poly_verts[j * 3], self.poly_verts[j * 3 + 1], self.poly_verts[j * 3 + 2])
                _sample_segment(pts, a, b, self.step)
        self._cache = pts
        return pts


class ParticleApi:
    """粒子域对外 API，方法名对齐 LSE 的 particle*。"""

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._items: dict[int, ParticleShape] = {}
        self._next_id = 1
        self._tick = 0

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _now(self) -> int:
        return self._tick

    def _add(self, shape: ParticleShape, lifetime: int) -> int:
        with self._lock:
            shape.id = self._next_id
            self._next_id += 1
            shape.end_tick = self._now() + lifetime if lifetime > 0 else 0
            shape.next_emit = self._now() + 1
            self._items[shape.id] = shape
            return shape.id

    def get(self, shape_id: int) -> ParticleShape | None:
        with self._lock:
            return self._items.get(shape_id)

    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    # ── 创建 ──────────────────────────────────────────────────────────────
    def create_point(self, player_name: str, dim_id: int, x, y, z, effect: str = "", interval: int = 4, lifetime: int = 0) -> int:
        s = ParticleShape(0, KIND_POINT)
        s.player_name, s.dim_id = player_name, int(dim_id)
        s.px, s.py, s.pz = float(x), float(y), float(z)
        s.effect = effect or DEFAULT_EFFECT
        s.interval = max(1, int(interval))
        s.view_distance = 0
        return self._add(s, lifetime)

    def create_line(self, player_name, dim_id, x1, y1, z1, x2, y2, z2, step=1.0, effect="", interval=4, lifetime=0) -> int:
        s = ParticleShape(0, KIND_LINE)
        s.player_name, s.dim_id = player_name, int(dim_id)
        s.ax, s.ay, s.az = float(x1), float(y1), float(z1)
        s.bx, s.by, s.bz = float(x2), float(y2), float(z2)
        s.step = max(0.25, float(step))
        s.effect = effect or DEFAULT_EFFECT
        s.interval = max(1, int(interval))
        return self._add(s, lifetime)

    def create_rect(self, player_name, dim_id, cx, cy, cz, w, h, axis=0, step=1.0, effect="", interval=4, lifetime=0) -> int:
        s = ParticleShape(0, KIND_RECT)
        s.player_name, s.dim_id = player_name, int(dim_id)
        s.px, s.py, s.pz = float(cx), float(cy), float(cz)
        s.w, s.h, s.axis = float(w), float(h), int(axis)
        s.step = max(0.25, float(step))
        s.effect = effect or DEFAULT_EFFECT
        s.interval = max(1, int(interval))
        return self._add(s, lifetime)

    def create_plane(self, player_name, dim_id, cx, cy, cz, w, h, axis=0, step=1.0, effect="", interval=4, lifetime=0) -> int:
        s = ParticleShape(0, KIND_PLANE)
        s.player_name, s.dim_id = player_name, int(dim_id)
        s.px, s.py, s.pz = float(cx), float(cy), float(cz)
        s.w, s.h, s.axis = float(w), float(h), int(axis)
        s.step = max(0.25, float(step))
        s.effect = effect or DEFAULT_EFFECT
        s.interval = max(1, int(interval))
        return self._add(s, lifetime)

    def create_box(self, player_name, dim_id, cx, cy, cz, hx, hy, hz, step=1.0, effect="", interval=4, lifetime=0) -> int:
        s = ParticleShape(0, KIND_BOX)
        s.player_name, s.dim_id = player_name, int(dim_id)
        s.px, s.py, s.pz = float(cx), float(cy), float(cz)
        s.hx, s.hy, s.hz = float(hx), float(hy), float(hz)
        s.step = max(0.25, float(step))
        s.effect = effect or DEFAULT_EFFECT
        s.interval = max(1, int(interval))
        return self._add(s, lifetime)

    def create_box_faces(self, player_name, dim_id, cx, cy, cz, hx, hy, hz, step=1.0, effect="", interval=4, lifetime=0) -> int:
        s = ParticleShape(0, KIND_BOX_FACES)
        s.player_name, s.dim_id = player_name, int(dim_id)
        s.px, s.py, s.pz = float(cx), float(cy), float(cz)
        s.hx, s.hy, s.hz = float(hx), float(hy), float(hz)
        s.step = max(0.25, float(step))
        s.effect = effect or DEFAULT_EFFECT
        s.interval = max(1, int(interval))
        return self._add(s, lifetime)

    def create_poly(self, player_name, dim_id, verts, edges, step=1.0, effect="", interval=4, lifetime=0) -> int:
        """verts: "x,y,z;x,y,z;..." 或 [x,y,z,...]；edges: "0,1;1,2" 或 [0,1,1,2]。"""
        s = ParticleShape(0, KIND_POLY)
        s.player_name, s.dim_id = player_name, int(dim_id)
        s.poly_verts = _parse_verts(verts)
        s.poly_edges = _parse_edges(edges)
        s.step = max(0.25, float(step))
        s.effect = effect or DEFAULT_EFFECT
        s.interval = max(1, int(interval))
        return self._add(s, lifetime)

    # ── 修改 ──────────────────────────────────────────────────────────────
    def set_pos(self, shape_id: int, x, y, z) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.px, s.py, s.pz = float(x), float(y), float(z)
        return True

    def move_by(self, shape_id: int, dx, dy, dz) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.px += float(dx)
        s.py += float(dy)
        s.pz += float(dz)
        return True

    def move_to(self, shape_id: int, x, y, z, duration_ticks: int) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.anim_from = (s.px, s.py, s.pz)
        s.anim_to = (float(x), float(y), float(z))
        s.anim_start = self._now()
        s.anim_duration = max(1, int(duration_ticks))
        s.anim_active = True
        return True

    def set_rot(self, shape_id: int, rx, ry, rz) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.rot_x, s.rot_y, s.rot_z = float(rx), float(ry), float(rz)
        return True

    def spin(self, shape_id: int, sx, sy, sz) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.spin_x, s.spin_y, s.spin_z = float(sx), float(sy), float(sz)
        return True

    def set_scale(self, shape_id: int, scale: float) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.scale = float(scale)
        return True

    def follow(self, shape_id: int, uuid: str, off_x=0.0, off_y=0.0, off_z=0.0) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.follow_uuid = str(uuid)
        s.off_x, s.off_y, s.off_z = float(off_x), float(off_y), float(off_z)
        return True

    def unfollow(self, shape_id: int) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.follow_uuid = ""
        return True

    def set_effect(self, shape_id: int, effect: str) -> bool:
        s = self.get(shape_id)
        if s is None or not effect:
            return False
        s.effect = str(effect)
        return True

    def set_visible(self, shape_id: int, players_csv: str) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        names = [n.strip() for n in str(players_csv).split(",") if n.strip()]
        s.visible_players = set(names)
        s.visible_all = not names
        return True

    def set_visible_players(self, shape_id: int, player_uuids: list[str]) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        names = set()
        for uid in player_uuids:
            try:
                player = self._plugin.server.get_player(uid)
            except Exception:
                player = None
            if player is not None:
                names.add(player.name)
        s.visible_players = names
        s.visible_all = not names
        return True

    def clear_visible_players(self, shape_id: int) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.visible_players.clear()
        s.visible_all = True
        return True

    def set_interval(self, shape_id: int, ticks: int) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.interval = max(1, int(ticks))
        return True

    def set_view_distance(self, shape_id: int, blocks: int) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.view_distance = int(blocks)
        return True

    def set_lifetime(self, shape_id: int, ticks: int) -> bool:
        s = self.get(shape_id)
        if s is None:
            return False
        s.end_tick = self._now() + int(ticks) if ticks > 0 else 0
        return True

    # ── 查询 / 销毁 ───────────────────────────────────────────────────────
    def exists(self, shape_id: int) -> bool:
        return self.get(shape_id) is not None

    def get_all_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._items.keys())

    def get_info(self, shape_id: int) -> str:
        s = self.get(shape_id)
        if s is None:
            return ""
        return (
            f"#{s.id} {KIND_NAMES.get(s.kind, '?')} dim={s.dim_id} pos=({s.px:.2f},{s.py:.2f},{s.pz:.2f}) "
            f"rot=({s.rot_x:.1f},{s.rot_y:.1f},{s.rot_z:.1f}) scale={s.scale:.3f} "
            f"pts={len(s.local_points())} step={s.step} effect={s.effect} "
            f"visible={'all' if s.visible_all else ','.join(sorted(s.visible_players))} "
            f"view={s.view_distance} interval={s.interval}t end={s.end_tick}"
        )

    def destroy(self, shape_id: int) -> bool:
        with self._lock:
            return self._items.pop(shape_id, None) is not None

    def destroy_all(self) -> int:
        with self._lock:
            count = len(self._items)
            self._items.clear()
            return count

    # ── 每 tick ───────────────────────────────────────────────────────────
    def tick(self) -> None:
        self._tick += 1
        now = self._tick
        due: list[ParticleShape] = []
        expired: list[int] = []
        with self._lock:
            items = list(self._items.values())
        for s in items:
            if s.end_tick and now >= s.end_tick:
                expired.append(s.id)
                continue
            if s.spin_x or s.spin_y or s.spin_z:
                s.rot_x += s.spin_x
                s.rot_y += s.spin_y
                s.rot_z += s.spin_z
            if s.anim_active:
                elapsed = now - s.anim_start
                if elapsed >= s.anim_duration:
                    s.px, s.py, s.pz = s.anim_to
                    s.anim_active = False
                else:
                    t = elapsed / s.anim_duration
                    t = 1.0 - (1.0 - t) ** 3
                    s.px = s.anim_from[0] + (s.anim_to[0] - s.anim_from[0]) * t
                    s.py = s.anim_from[1] + (s.anim_to[1] - s.anim_from[1]) * t
                    s.pz = s.anim_from[2] + (s.anim_to[2] - s.anim_from[2]) * t
            if now >= s.next_emit:
                due.append(s)
        for shape_id in expired:
            self.destroy(shape_id)
        for s in due:
            s.next_emit = now + s.interval
            try:
                self._emit(s)
            except Exception as exc:  # 单个粒子形状出错不能拖垮整个 tick
                self._plugin.logger.debug(f"particle {s.id} emit 失败: {exc}")

    def _emit(self, s: ParticleShape) -> None:
        players = list(self._plugin.server.online_players)
        from .holograms import _dim_id

        anchor = (s.px, s.py, s.pz)
        if s.follow_uuid:
            follower = None
            try:
                follower = self._plugin.server.get_player(s.follow_uuid)
            except Exception:
                follower = None
            if follower is None:
                return
            loc = follower.location
            anchor = (loc.x + s.off_x, loc.y + s.off_y, loc.z + s.off_z)
            s.dim_id = _dim_id(follower)

        targets = [p for p in players if _dim_id(p) == s.dim_id]
        if not s.visible_all:
            targets = [p for p in targets if p.name in s.visible_players]
        if not targets:
            return

        pts = s.local_points()
        if not pts:
            return

        m = _euler_matrix(s.rot_x, s.rot_y, s.rot_z)
        world = []
        for (lx, ly, lz) in pts:
            x, y, z = lx * s.scale, ly * s.scale, lz * s.scale
            wx = m[0][0] * x + m[0][1] * y + m[0][2] * z + anchor[0]
            wy = m[1][0] * x + m[1][1] * y + m[1][2] * z + anchor[1]
            wz = m[2][0] * x + m[2][1] * y + m[2][2] * z + anchor[2]
            world.append((wx, wy, wz))

        core = self._plugin.core
        packet_id = core.spawn_particle_packet_id
        for player in targets:
            loc = player.location
            r2 = s.view_distance * s.view_distance
            for (wx, wy, wz) in world:
                if s.view_distance > 0:
                    dx, dy, dz = wx - loc.x, wy - loc.y, wz - loc.z
                    if dx * dx + dy * dy + dz * dz > r2:
                        continue
                try:
                    player.send_packet(packet_id, core.encode_spawn_particle(s.dim_id, wx, wy, wz, s.effect))
                except Exception:
                    break  # 这个玩家发不出去就跳过，下个 tick 再试

    def __len__(self) -> int:
        with self._lock:
            return len(self._items)


def _parse_verts(verts) -> list[float]:
    if isinstance(verts, (list, tuple)):
        return [float(v) for v in verts]
    out: list[float] = []
    for triple in str(verts).split(";"):
        parts = [p for p in triple.split(",") if p.strip()]
        if len(parts) == 3:
            out.extend(float(p) for p in parts)
    return out


def _parse_edges(edges) -> list[int]:
    if isinstance(edges, (list, tuple)):
        return [int(v) for v in edges]
    out: list[int] = []
    for pair in str(edges).split(";"):
        parts = [p for p in pair.split(",") if p.strip()]
        if len(parts) == 2:
            out.extend(int(p) for p in parts)
    return out
