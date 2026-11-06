"""物品展示（ItemDisplay）域 —— LeviLamina 版 ItemDisplayManager 的 endstone 移植（核心子集）。"""

from __future__ import annotations

import math
import threading

from . import items
from .holograms import _dim_id

# 虚拟号段（与 LL 版 core/VirtualActorIds.h 的 itemDisplay 段一致）
FIRST_RUNTIME_ID = 0x6D000000
FIRST_UNIQUE_ID = 0x200000


def _rot_to_byte(degrees: float) -> int:
    return int(round((float(degrees) % 360.0) / 360.0 * 256.0)) & 0xFF


class _Display:
    __slots__ = (
        "id",
        "unique_id",
        "runtime_id",
        "x",
        "y",
        "z",
        "dimension",
        "item",
        "aux",
        "count",
        "nbt",
        "glint",
        "offset",
        "rotation",
        "scale",
        "viewers",
        "view_distance",
        "follow",
        "enabled",
        "mode",
        "hitbox",
        "anim",
        "stretch",
        "preset",
        "extend",
        "egg",
    )

    def __init__(self, display_id: int) -> None:
        self.id = display_id
        self.unique_id = 0
        self.runtime_id = 0
        self.x = self.y = self.z = 0.0
        self.dimension = 0
        self.item = ""
        self.aux = 0
        self.count = 1
        self.nbt = ""
        self.glint = False
        self.offset = (0.0, 0.0, 0.0)
        self.rotation = (0.0, 0.0, 0.0)
        self.scale = 1.0
        self.viewers: set[str] | None = None
        self.view_distance = 0
        self.follow: tuple[str, float, float, float] | None = None
        self.enabled = True
        self.mode = 0
        self.hitbox = (0.0, 0.0)
        self.anim: dict = {}
        self.stretch = (1.0, 1.0)
        self.preset = ("", 0.0, 0.0, 0.0)
        self.extend = 0.0
        self.egg: dict | None = None

    def world_pos(self) -> tuple[float, float, float]:
        return (self.x + self.offset[0], self.y + self.offset[1], self.z + self.offset[2])


class ItemDisplayApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._items: dict[int, _Display] = {}
        self._next_id = 1
        self._next_runtime = FIRST_RUNTIME_ID
        self._next_unique = FIRST_UNIQUE_ID

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _targets(self, display: _Display) -> list:
        players = [p for p in self._plugin.server.online_players if _dim_id(p) == display.dimension]
        if display.viewers is not None:
            players = [p for p in players if p.name in display.viewers]
        return players

    def _send(self, players, packet_id: int, body: bytes) -> bool:
        if not body:
            return False
        sent = False
        for player in players:
            try:
                player.send_packet(packet_id, body)
                sent = True
            except Exception as exc:
                self._plugin.logger.debug(f"itemdisplay 发包失败 ({player.name}): {exc}")
        return sent

    def get(self, display_id: int) -> _Display | None:
        with self._lock:
            return self._items.get(display_id)

    def _spawn(self, display: _Display) -> bool:
        players = self._targets(display)
        if not players:
            return False
        x, y, z = display.world_pos()
        body = self._plugin.core.encode_add_actor_ex(
            "minecraft:fox", "", display.runtime_id, display.unique_id, x, y, z,
            float(display.rotation[1]), float(display.rotation[0]),
            display.hitbox[0], display.hitbox[1],
        )
        if not self._send(players, self._plugin.core.add_actor_packet_id, body):
            return False
        self._equip(display, players)
        return True

    def _equip(self, display: _Display, players=None) -> bool:
        numeric = items.numeric_id(display.item)
        if numeric < 0:
            return False
        body = self._plugin.core.encode_mob_equipment(
            display.runtime_id, numeric, max(1, int(display.count)), int(display.aux), 0, 0, 0
        )
        return self._send(players if players is not None else self._targets(display),
                          self._plugin.core.mob_equipment_packet_id, body)

    def _move(self, display: _Display) -> bool:
        x, y, z = display.world_pos()
        body = self._plugin.core.encode_move_actor_absolute(
            display.runtime_id, x, y, z, 1,
            _rot_to_byte(display.rotation[0]), _rot_to_byte(display.rotation[1]), _rot_to_byte(display.rotation[1]),
        )
        return self._send(self._targets(display), self._plugin.core.move_actor_absolute_packet_id, body)

    # ── 创建 ──────────────────────────────────────────────────────────────
    def _create(
        self,
        x,
        y,
        z,
        dim,
        item,
        aux=0,
        off=(0.0, 0.0, 0.0),
        rot=(0.0, 0.0, 0.0),
        scale=1.0,
        nbt="",
        glint=False,
        desired_id=0,
    ) -> int:
        if items.numeric_id(item) < 0:
            self._plugin.logger.debug(f"itemDisplayCreate: 未知物品 {item}")
            return -1
        with self._lock:
            display_id = int(desired_id) if desired_id else self._next_id
            if display_id in self._items:
                return -1
            if not desired_id:
                self._next_id += 1
            display = _Display(display_id)
            display.runtime_id = self._next_runtime
            self._next_runtime += 1
            display.unique_id = self._next_unique
            self._next_unique += 1
            display.x, display.y, display.z = float(x), float(y), float(z)
            display.dimension = int(dim)
            display.item = str(item)
            display.aux = int(aux)
            display.offset = (float(off[0]), float(off[1]), float(off[2]))
            display.rotation = (float(rot[0]), float(rot[1]), float(rot[2]))
            display.scale = float(scale)
            display.nbt = str(nbt)
            display.glint = bool(glint)
            self._items[display_id] = display
        if not self._spawn(display):
            with self._lock:
                self._items.pop(display_id, None)
            return -1
        return display_id

    def create(self, x, y, z, dim, item, aux=0) -> int:
        return self._create(x, y, z, dim, item, aux)

    def create_advanced(self, x, y, z, dim, item, aux, off_x, off_y, off_z, rot_x, rot_y, rot_z, scale) -> int:
        return self._create(x, y, z, dim, item, aux, (off_x, off_y, off_z), (rot_x, rot_y, rot_z), scale)

    def create_full(self, x, y, z, dim, item, aux, off_x, off_y, off_z, rot_x, rot_y, rot_z, scale, nbt="", glint=False) -> int:
        return self._create(x, y, z, dim, item, aux, (off_x, off_y, off_z), (rot_x, rot_y, rot_z), scale, nbt, glint)

    def create_with_id(self, x, y, z, dim, item, aux, desired_id) -> int:
        return self._create(x, y, z, dim, item, aux, desired_id=int(desired_id))

    def create_seamless(self, x, y, z, dim, item, aux, off_x, off_y, off_z, rot_x, rot_y, rot_z, scale, mode, view_distance, visible_player) -> int:
        display_id = self._create(x, y, z, dim, item, aux, (off_x, off_y, off_z), (rot_x, rot_y, rot_z), scale)
        if display_id < 0:
            return -1
        self.set_mode(display_id, mode)
        self.set_view_distance(display_id, view_distance)
        if visible_player:
            self.set_visible_player(display_id, visible_player)
        return display_id

    def create_random(self, x, y, z, dim, item, aux=0) -> int:
        return self.create(x, y, z, dim, item, aux)

    # ── 销毁 / 查询 ───────────────────────────────────────────────────────
    def destroy(self, display_id: int) -> bool:
        with self._lock:
            display = self._items.pop(display_id, None)
        if display is None:
            return False
        body = self._plugin.core.encode_remove_actor(display.unique_id)
        self._send(self._targets(display), self._plugin.core.remove_actor_packet_id, body)
        return True

    def destroy_all(self) -> int:
        with self._lock:
            ids = list(self._items.keys())
        for display_id in ids:
            self.destroy(display_id)
        return len(ids)

    def exists(self, display_id: int) -> bool:
        with self._lock:
            return display_id in self._items

    def is_id_used(self, display_id: int) -> bool:
        return self.exists(display_id)

    def get_all_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._items.keys())

    def runtime_id(self, display_id: int) -> int:
        display = self.get(display_id)
        return display.runtime_id if display is not None else 0

    def get_info(self, display_id: int) -> str:
        display = self.get(display_id)
        if display is None:
            return ""
        x, y, z = display.world_pos()
        return (
            f"#{display.id} item={display.item} aux={display.aux} dim={display.dimension} "
            f"pos=({x:.2f},{y:.2f},{z:.2f}) rot={display.rotation} scale={display.scale:.3f} "
            f"rid={display.runtime_id} enabled={display.enabled} viewers="
            f"{'all' if display.viewers is None else ','.join(sorted(display.viewers))}"
        )

    def find_nearest(self, x, y, z, dim, max_dist) -> int:
        best_id, best_d2 = -1, float(max_dist) ** 2
        for display_id, display in list(self._items.items()):
            if display.dimension != int(dim):
                continue
            wx, wy, wz = display.world_pos()
            d2 = (wx - x) ** 2 + (wy - y) ** 2 + (wz - z) ** 2
            if d2 <= best_d2:
                best_id, best_d2 = display_id, d2
        return best_id

    # ── 属性 ──────────────────────────────────────────────────────────────
    def set_item(self, display_id: int, item: str, aux=0) -> bool:
        return self.set_item_with_nbt(display_id, item, aux, "")

    def set_item_with_nbt(self, display_id: int, item: str, aux=0, nbt: str = "") -> bool:
        display = self.get(display_id)
        if display is None or items.numeric_id(item) < 0:
            return False
        display.item = str(item)
        display.aux = int(aux)
        display.nbt = str(nbt)
        return self._equip(display)

    def set_position(self, display_id: int, x, y, z, dim=None) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.x, display.y, display.z = float(x), float(y), float(z)
        if dim is not None:
            display.dimension = int(dim)
            return self._move(display)
        return self._move(display)

    def set_offset(self, display_id: int, ox, oy, oz) -> bool:
        return self.set_offset_live(display_id, ox, oy, oz, persist=True)

    def set_base_offset(self, display_id: int, ox, oy, oz) -> bool:
        return self.set_offset(display_id, ox, oy, oz)

    def set_offset_live(self, display_id: int, ox, oy, oz, persist: bool = True) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        if persist:
            display.offset = (float(ox), float(oy), float(oz))
        return self._move(display)

    def set_rotation(self, display_id: int, rx, ry, rz) -> bool:
        return self.set_rotation_live(display_id, rx, ry, rz)

    def set_rotation_live(self, display_id: int, rx, ry, rz) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.rotation = (float(rx), float(ry), float(rz))
        return self._move(display)

    def rotate_y(self, display_id: int, delta: float) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        rx, ry, rz = display.rotation
        return self.set_rotation_live(display_id, rx, ry + float(delta), rz)

    def set_scale(self, display_id: int, scale: float) -> bool:
        return self.set_scale_live(display_id, scale)

    def set_scale_live(self, display_id: int, scale: float) -> bool:
        """记录缩放值；客户端表现依赖 molang 动画（本版未接），因此不改包。"""
        display = self.get(display_id)
        if display is None:
            return False
        display.scale = float(scale)
        return True

    def scale_to(self, display_id: int, target: float) -> bool:
        return self.set_scale_live(display_id, target)

    def scale_by(self, display_id: int, factor: float) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        return self.set_scale_live(display_id, display.scale * float(factor))

    def set_mode(self, display_id: int, mode: int) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.mode = int(mode)
        return True

    # ── 彩蛋：朋友的⑨ ─────────────────────────────────────────────────────
    def easter_egg(self, display_id: int, duration_ticks: int = 60, rotate: bool = True) -> bool:
        """彩蛋「朋友的⑨」的展示版：到点自己停，自动还原。

        LL 版把膨胀写成客户端 molang 表达式（stretchXZ / stretchY / rotY），
        endstone 没有 molang 通道，所以这里做的是**可见的近似**：
        旋转是真的逐 tick 下发，缩放只在本地记值（客户端看到的仍是原大小）。
        """
        display = self.get(display_id)
        if display is None:
            return False
        ticks = int(duration_ticks) if int(duration_ticks) > 0 else 60
        display.egg = {
            "end": self._plugin.tick_count + ticks,
            "rotate": bool(rotate),
            "old_rot": tuple(display.rotation),
            "old_scale": float(display.scale),
            "start": self._plugin.tick_count,
        }
        return True

    def _tick_egg(self, display: "_Display") -> None:
        egg = display.egg
        if not egg:
            return
        now = self._plugin.tick_count
        if now >= egg["end"]:
            display.rotation = egg["old_rot"]
            display.scale = egg["old_scale"]
            if egg["rotate"]:
                self._move(display)
            display.egg = None
            return
        elapsed = (now - egg["start"]) / 20.0
        # 与 LL 版同一条曲线：q.life_time*720 度/秒，一次肿胀转一圈
        if egg["rotate"]:
            rx, ry, rz = egg["old_rot"]
            display.rotation = (rx, ry + elapsed * 720.0, rz)
            self._move(display)
        import math

        swelling = math.sin(elapsed * 720.0 * math.pi / 180.0)
        display.scale = egg["old_scale"] * (1.0 + 0.1 * max(swelling, 0.0) - 0.4 * max(-swelling, 0.0))

    def set_extend(self, display_id: int, extend: float) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.extend = float(extend)
        return True

    def set_glint(self, display_id: int, on: bool) -> bool:
        """记录附魔光效；线格式需要 NBT tag（本版未接）。"""
        display = self.get(display_id)
        if display is None:
            return False
        display.glint = bool(on)
        return True

    def set_hitbox(self, display_id: int, width: float, height: float) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.hitbox = (float(width), float(height))
        return self._move(display)

    def set_view_distance(self, display_id: int, distance: float) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.view_distance = float(distance)
        return True

    # ── 可见性 / 跟随 ─────────────────────────────────────────────────────
    def set_visible_player(self, display_id: int, player_name: str) -> bool:
        return self.set_visible_players(display_id, [player_name])

    def set_visible_players(self, display_id: int, names) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        if isinstance(names, str):
            names = [n.strip() for n in names.split(",") if n.strip()]
        display.viewers = set(str(n) for n in names)
        return True

    def clear_visible_players(self, display_id: int) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.viewers = None
        return True

    def follow(self, display_id: int, player_name: str, off_x=0.0, off_y=0.0, off_z=0.0) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.follow = (str(player_name), float(off_x), float(off_y), float(off_z))
        return True

    def unfollow(self, display_id: int) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.follow = None
        return True

    def set_enabled(self, display_id: int, enabled: bool) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        if bool(enabled) == display.enabled:
            return True
        display.enabled = bool(enabled)
        if display.enabled:
            return self._spawn(display)
        body = self._plugin.core.encode_remove_actor(display.unique_id)
        return self._send(self._targets(display), self._plugin.core.remove_actor_packet_id, body)

    # ── 每 tick（跟随） ───────────────────────────────────────────────────
    # ── 动画 / 拉伸（记录型；客户端表现依赖 molang，见模块说明）───────────
    def set_anim(
        self,
        display_id: int,
        spin_speed: float = 0.0,
        spin_axis: int = 1,
        bob_amp: float = 0.0,
        bob_period: float = 0.0,
        pulse_amp: float = 0.0,
        pulse_period: float = 0.0,
        sway_amp: float = 0.0,
        sway_period: float = 0.0,
        phase: float = 0.0,
        billboard: bool = False,
    ) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.anim = {
            "spin_speed": float(spin_speed), "spin_axis": int(spin_axis),
            "bob_amp": float(bob_amp), "bob_period": float(bob_period),
            "pulse_amp": float(pulse_amp), "pulse_period": float(pulse_period),
            "sway_amp": float(sway_amp), "sway_period": float(sway_period),
            "phase": float(phase), "billboard": bool(billboard),
        }
        return True

    def clear_anim(self, display_id: int) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.anim = {}
        return True

    def set_stretch(self, display_id: int, xz: float, y: float) -> bool:
        return self.set_stretch_live(display_id, xz, y)

    def set_stretch_live(self, display_id: int, xz: float, y: float) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.stretch = (float(xz), float(y))
        return True

    def set_transform_live(self, display_id: int, off_x: float, off_y: float, off_z: float,
                           rot_x: float, rot_y: float, rot_z: float, scale: float) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.offset = (float(off_x), float(off_y), float(off_z))
        display.rotation = (float(rot_x), float(rot_y), float(rot_z))
        display.scale = float(scale)
        return self._move(display)

    def set_preset(self, display_id: int, preset: str, a: float = 0.0, b: float = 0.0, c: float = 0.0) -> bool:
        display = self.get(display_id)
        if display is None:
            return False
        display.preset = (str(preset), float(a), float(b), float(c))
        return True

    def emit_for_entity(self, player_name: str, unique_id: int, item: str, aux: int = 0, offset_y: float = 0.0,
                        rot_x: float = 0.0, rot_y: float = 0.0, rot_z: float = 0.0, scale: float = 1.0,
                        nbt: str = "", glint: bool = False) -> int:
        """在指定实体位置给指定玩家生成一个展示（一次性）。"""
        target = None
        for entity_id in self._plugin.entities.get_all_ids():
            if self._plugin.entities.unique_id(entity_id) == int(unique_id):
                target = self._plugin.entities.get(entity_id)
                break
        if target is None:
            return -1
        display_id = self._create(
            target.x, target.y + float(offset_y), target.z, target.dimension, item, aux,
            (0.0, 0.0, 0.0), (rot_x, rot_y, rot_z), scale, nbt, glint,
        )
        if display_id < 0:
            return -1
        self.set_visible_player(display_id, player_name)
        return display_id

    def tick(self) -> None:
        for display in list(self._items.values()):
            anim = display.anim
            if anim and anim.get("spin_speed"):
                axis = anim.get("spin_axis", 1)
                step = anim["spin_speed"]
                rx, ry, rz = display.rotation
                if axis == 0:
                    rx += step
                elif axis == 2:
                    rz += step
                else:
                    ry += step
                display.rotation = (rx, ry, rz)
                self._move(display)
            if display.egg:
                self._tick_egg(display)
            if display.follow is None:
                continue
            name, ox, oy, oz = display.follow
            try:
                player = self._plugin.server.get_player(name)
            except Exception:
                player = None
            if player is None:
                continue
            loc = player.location
            display.x, display.y, display.z = loc.x + ox, loc.y + oy, loc.z + oz
            display.dimension = _dim_id(player)
            self._move(display)

    def __len__(self) -> int:
        with self._lock:
            return len(self._items)
