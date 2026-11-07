"""holo_core.dll 的 ctypes 绑定。"""

from __future__ import annotations

import ctypes
from pathlib import Path

# 形状类型编号：与 core/ShapeIdMap.h 的 shape_id::kLse* 一致
SHAPE_TEXT = 0
SHAPE_LINE = 1
SHAPE_BOX = 2
SHAPE_CIRCLE = 3
SHAPE_SPHERE = 4
SHAPE_ARROW = 5
SHAPE_HOLE = 6
SHAPE_CYLINDER = 7
SHAPE_PYRAMID = 8
SHAPE_ELLIPSOID = 9
SHAPE_CONE = 10


class ShapeDesc(ctypes.Structure):
    """与 core/c_api.h 的 holo_shape_desc 逐字段对齐。"""

    _fields_ = [
        ("network_id", ctypes.c_uint64),
        ("type", ctypes.c_int32),
        ("has_location", ctypes.c_int32),
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("z", ctypes.c_float),
        ("has_end", ctypes.c_int32),
        ("ex", ctypes.c_float),
        ("ey", ctypes.c_float),
        ("ez", ctypes.c_float),
        ("has_scale", ctypes.c_int32),
        ("scale", ctypes.c_float),
        ("has_rotation", ctypes.c_int32),
        ("rx", ctypes.c_float),
        ("ry", ctypes.c_float),
        ("rz", ctypes.c_float),
        ("has_total_time_left", ctypes.c_int32),
        ("total_time_left", ctypes.c_float),
        ("has_max_render_distance", ctypes.c_int32),
        ("max_render_distance", ctypes.c_float),
        ("has_color", ctypes.c_int32),
        ("color", ctypes.c_int32),
        ("has_attached_entity", ctypes.c_int32),
        ("attached_entity_id", ctypes.c_int64),
        ("has_dimension", ctypes.c_int32),
        ("dimension_id", ctypes.c_int32),
        ("width", ctypes.c_float),
        ("height", ctypes.c_float),
        ("depth", ctypes.c_float),
        ("radius_x", ctypes.c_float),
        ("radius_z", ctypes.c_float),
        ("segments", ctypes.c_int32),
        ("text", ctypes.c_char_p),
        ("text_use_rotation", ctypes.c_int32),
        ("has_text_background", ctypes.c_int32),
        ("text_background_color", ctypes.c_int32),
        ("text_line_gap_height", ctypes.c_float),
        ("text_depth_test", ctypes.c_int32),
        ("text_show_backface", ctypes.c_int32),
        ("text_show_text_backface", ctypes.c_int32),
    ]


class ScoreEntry(ctypes.Structure):
    """与 core/c_api.h 的 holo_score_entry 对齐（计分板条目）。"""

    _fields_ = [
        ("scoreboard_id", ctypes.c_int64),
        ("score_value", ctypes.c_int32),
        ("identity_type", ctypes.c_int32),
        ("actor_unique_id", ctypes.c_int64),
        ("objective", ctypes.c_char_p),
        ("fake_player_name", ctypes.c_char_p),
    ]


class MetaEntry(ctypes.Structure):
    """与 core/c_api.h 的 holo_meta_entry 对齐（Actor 元数据条目）。"""

    _fields_ = [
        ("id", ctypes.c_int32),
        ("kind", ctypes.c_int32),
        ("ival", ctypes.c_int64),
        ("fval", ctypes.c_float),
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("z", ctypes.c_float),
    ]


KIND_BYTE = 0
KIND_SHORT = 1
KIND_INT = 2
KIND_FLOAT = 3
KIND_LONG = 4
KIND_VEC3 = 5


def meta_int(entry_id: int, value: int) -> "MetaEntry":
    entry = MetaEntry()
    entry.id = int(entry_id)
    entry.kind = KIND_INT
    entry.ival = int(value)
    return entry


def meta_float(entry_id: int, value: float) -> "MetaEntry":
    entry = MetaEntry()
    entry.id = int(entry_id)
    entry.kind = KIND_FLOAT
    entry.fval = float(value)
    return entry


class ItemSlot(ctypes.Structure):
    """与 core/c_api.h 的 holo_item_slot 对齐（容器槽位）。"""

    _fields_ = [
        ("item_id", ctypes.c_int16),
        ("stack_size", ctypes.c_uint16),
        ("aux", ctypes.c_uint32),
        ("user_data", ctypes.c_char_p),
        ("user_data_len", ctypes.c_size_t),
    ]


class TradeItem(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_char_p),
        ("count", ctypes.c_int32),
        ("damage", ctypes.c_int32),
        ("name", ctypes.c_char_p),
        ("lore_csv", ctypes.c_char_p),
        ("ench_csv", ctypes.c_char_p),
        ("is_block", ctypes.c_int32),
        ("block_version", ctypes.c_int32),
    ]


class TradeOffer(ctypes.Structure):
    _fields_ = [
        ("buy_a", TradeItem),
        ("buy_b", TradeItem),
        ("sell", TradeItem),
        ("tier", ctypes.c_int32),
        ("max_uses", ctypes.c_int32),
        ("uses", ctypes.c_int32),
        ("net_id", ctypes.c_int32),
        ("trader_exp", ctypes.c_int32),
        ("demand", ctypes.c_int32),
        ("reward_exp", ctypes.c_int32),
        ("price_multiplier_a", ctypes.c_float),
        ("price_multiplier_b", ctypes.c_float),
    ]


class SkinDesc(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_char_p),
        ("geometry", ctypes.c_char_p),
        ("arm_size", ctypes.c_char_p),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("rgba", ctypes.c_char_p),
        ("rgba_len", ctypes.c_size_t),
    ]


class Waypoint(ctypes.Structure):
    """与 core/c_api.h 的 holo_waypoint 对齐（定位条路点）。"""

    _fields_ = [
        ("msb", ctypes.c_int64),
        ("lsb", ctypes.c_int64),
        ("action", ctypes.c_uint8),
        ("flags", ctypes.c_uint32),
        ("has_visible", ctypes.c_int32),
        ("visible", ctypes.c_uint8),
        ("has_position", ctypes.c_int32),
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("z", ctypes.c_float),
        ("dimension", ctypes.c_int32),
        ("has_texture", ctypes.c_int32),
        ("texture", ctypes.c_char_p),
        ("has_icon_size", ctypes.c_int32),
        ("icon_w", ctypes.c_float),
        ("icon_h", ctypes.c_float),
        ("has_color", ctypes.c_int32),
        ("color", ctypes.c_int32),
        ("has_authority", ctypes.c_int32),
        ("authority", ctypes.c_uint8),
        ("has_actor", ctypes.c_int32),
        ("actor_uid", ctypes.c_int64),
    ]


class NativeCoreError(RuntimeError):
    """holo_core.dll 调用失败。"""


class NativeCore:
    """holo_core.dll 的薄封装。"""

    def __init__(self, dll_path: Path | str | None = None) -> None:
        path = Path(dll_path) if dll_path is not None else _default_dll_path()
        if not path.is_file():
            raise NativeCoreError(
                f"找不到 holo_core.dll: {path}\n"
                "先在仓库根目录跑 `xmake build holo_core`（或 build.cmd），它会把 DLL 部署到 "
                "platform/endstone/src/hologramlib_endstone/_native/"
            )
        self._lib = ctypes.CDLL(str(path))

        self._lib.holo_core_api_version.restype = ctypes.c_uint32
        self._lib.holo_core_api_version.argtypes = []
        self._lib.holo_core_packet_id_primitive_shapes.restype = ctypes.c_uint32
        self._lib.holo_core_packet_id_primitive_shapes.argtypes = []
        self._lib.holo_core_version.restype = ctypes.c_char_p
        self._lib.holo_core_version.argtypes = []
        self._lib.holo_core_encode_primitive_shapes.restype = ctypes.c_int32
        self._lib.holo_core_encode_primitive_shapes.argtypes = [
            ctypes.POINTER(ShapeDesc),
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)),
            ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_packet_id_spawn_particle.restype = ctypes.c_uint32
        self._lib.holo_core_packet_id_spawn_particle.argtypes = []
        self._lib.holo_core_encode_spawn_particle.restype = ctypes.c_int32
        self._lib.holo_core_encode_spawn_particle.argtypes = [
            ctypes.c_uint8,
            ctypes.c_int64,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)),
            ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_packet_id_boss_event.restype = ctypes.c_uint32
        self._lib.holo_core_packet_id_boss_event.argtypes = []
        self._lib.holo_core_encode_boss_event.restype = ctypes.c_int32
        self._lib.holo_core_encode_boss_event.argtypes = [
            ctypes.c_int64,
            ctypes.c_int64,
            ctypes.c_uint8,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_float,
            ctypes.c_uint32,
            ctypes.c_uint32,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)),
            ctypes.POINTER(ctypes.c_size_t),
        ]
        for name in (
            "holo_core_packet_id_add_actor",
            "holo_core_packet_id_container_open",
            "holo_core_packet_id_container_close",
            "holo_core_packet_id_inventory_content",
            "holo_core_packet_id_update_trade",
            "holo_core_packet_id_npc_dialogue",
            "holo_core_packet_id_emote",
            "holo_core_packet_id_set_actor_link",
            "holo_core_packet_id_player_list",
            "holo_core_packet_id_add_player",
            "holo_core_packet_id_mob_equipment",
            "holo_core_packet_id_add_item_actor",
            "holo_core_packet_id_remove_actor",
            "holo_core_packet_id_move_actor_absolute",
            "holo_core_packet_id_set_actor_motion",
            "holo_core_packet_id_take_item_actor",
            "holo_core_packet_id_respawn",
            "holo_core_packet_id_set_actor_data",
            "holo_core_packet_id_update_block",
            "holo_core_packet_id_mob_effect",
            "holo_core_packet_id_actor_event",
            "holo_core_packet_id_animate_entity",
            "holo_core_packet_id_set_score",
            "holo_core_packet_id_set_display_objective",
            "holo_core_packet_id_remove_objective",
            "holo_core_packet_id_set_hud",
        ):
            fn = getattr(self._lib, name)
            fn.restype = ctypes.c_uint32
            fn.argtypes = []

        self._lib.holo_core_encode_set_display_objective.restype = ctypes.c_int32
        self._lib.holo_core_encode_set_display_objective.argtypes = [
            ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint8,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_remove_objective.restype = ctypes.c_int32
        self._lib.holo_core_encode_remove_objective.argtypes = [
            ctypes.c_char_p, ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_set_score.restype = ctypes.c_int32
        self._lib.holo_core_encode_set_score.argtypes = [
            ctypes.c_uint8, ctypes.POINTER(ScoreEntry), ctypes.c_size_t,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_set_hud.restype = ctypes.c_int32
        self._lib.holo_core_encode_set_hud.argtypes = [
            ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.c_uint8,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_add_actor_ex.restype = ctypes.c_int32
        self._lib.holo_core_encode_add_actor_ex.argtypes = [
            ctypes.c_char_p, ctypes.c_char_p,
            ctypes.c_uint64, ctypes.c_int64,
            ctypes.c_float, ctypes.c_float, ctypes.c_float,
            ctypes.c_float, ctypes.c_float,
            ctypes.c_float, ctypes.c_float,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_mob_equipment.restype = ctypes.c_int32
        self._lib.holo_core_encode_mob_equipment.argtypes = [
            ctypes.c_uint64, ctypes.c_int16, ctypes.c_uint16, ctypes.c_uint32,
            ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_add_item_actor.restype = ctypes.c_int32
        self._lib.holo_core_encode_add_item_actor.argtypes = [
            ctypes.c_uint64, ctypes.c_int64, ctypes.c_int16, ctypes.c_uint16, ctypes.c_uint32,
            ctypes.c_float, ctypes.c_float, ctypes.c_float,
            ctypes.c_float, ctypes.c_float, ctypes.c_float,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_remove_actor.restype = ctypes.c_int32
        self._lib.holo_core_encode_remove_actor.argtypes = [
            ctypes.c_int64, ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_move_actor_absolute.restype = ctypes.c_int32
        self._lib.holo_core_encode_move_actor_absolute.argtypes = [
            ctypes.c_uint64, ctypes.c_uint8,
            ctypes.c_float, ctypes.c_float, ctypes.c_float,
            ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_set_actor_motion.restype = ctypes.c_int32
        self._lib.holo_core_encode_set_actor_motion.argtypes = [
            ctypes.c_uint64, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_uint64,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_take_item_actor.restype = ctypes.c_int32
        self._lib.holo_core_encode_take_item_actor.argtypes = [
            ctypes.c_uint64, ctypes.c_uint64,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_respawn.restype = ctypes.c_int32
        self._lib.holo_core_encode_respawn.argtypes = [
            ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_uint8, ctypes.c_uint64,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_set_actor_data.restype = ctypes.c_int32
        self._lib.holo_core_encode_set_actor_data.argtypes = [
            ctypes.c_uint64, ctypes.c_uint64, ctypes.POINTER(MetaEntry), ctypes.c_size_t,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_update_block.restype = ctypes.c_int32
        self._lib.holo_core_encode_update_block.argtypes = [
            ctypes.c_int32, ctypes.c_int32, ctypes.c_int32,
            ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_mob_effect.restype = ctypes.c_int32
        self._lib.holo_core_encode_mob_effect.argtypes = [
            ctypes.c_uint64, ctypes.c_uint8, ctypes.c_int32, ctypes.c_int32, ctypes.c_uint8,
            ctypes.c_int32, ctypes.c_uint64, ctypes.c_uint8,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_actor_event.restype = ctypes.c_int32
        self._lib.holo_core_encode_actor_event.argtypes = [
            ctypes.c_uint64, ctypes.c_int32, ctypes.c_int32,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_core_encode_animate_entity.restype = ctypes.c_int32
        self._lib.holo_core_encode_animate_entity.argtypes = [
            ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int32, ctypes.c_char_p, ctypes.c_float,
            ctypes.POINTER(ctypes.c_uint64), ctypes.c_size_t,
            ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
        ]
        self._lib.holo_free.restype = None
        self._lib.holo_free.argtypes = [ctypes.c_void_p]

        self.api_version = int(self._lib.holo_core_api_version())
        self.version = self._lib.holo_core_version().decode("utf-8", "replace")
        self.primitive_shapes_packet_id = int(self._lib.holo_core_packet_id_primitive_shapes())
        self.spawn_particle_packet_id = int(self._lib.holo_core_packet_id_spawn_particle())
        self.boss_event_packet_id = int(self._lib.holo_core_packet_id_boss_event())
        self.set_score_packet_id = int(self._lib.holo_core_packet_id_set_score())
        self.set_display_objective_packet_id = int(self._lib.holo_core_packet_id_set_display_objective())
        self.remove_objective_packet_id = int(self._lib.holo_core_packet_id_remove_objective())
        self.set_hud_packet_id = int(self._lib.holo_core_packet_id_set_hud())
        self.mob_effect_packet_id = int(self._lib.holo_core_packet_id_mob_effect())
        self.actor_event_packet_id = int(self._lib.holo_core_packet_id_actor_event())
        self.animate_entity_packet_id = int(self._lib.holo_core_packet_id_animate_entity())
        self.update_block_packet_id = int(self._lib.holo_core_packet_id_update_block())
        self.set_actor_data_packet_id = int(self._lib.holo_core_packet_id_set_actor_data())
        self.respawn_packet_id = int(self._lib.holo_core_packet_id_respawn())
        self.add_item_actor_packet_id = int(self._lib.holo_core_packet_id_add_item_actor())
        self.add_actor_packet_id = int(self._lib.holo_core_packet_id_add_actor())
        self.container_open_packet_id = int(self._lib.holo_core_packet_id_container_open())
        self.container_close_packet_id = int(self._lib.holo_core_packet_id_container_close())
        self.inventory_content_packet_id = int(self._lib.holo_core_packet_id_inventory_content())
        self.update_trade_packet_id = int(self._lib.holo_core_packet_id_update_trade())
        self.npc_dialogue_packet_id = int(self._lib.holo_core_packet_id_npc_dialogue())
        self.player_list_packet_id = int(self._lib.holo_core_packet_id_player_list())
        self.emote_packet_id = int(self._lib.holo_core_packet_id_emote())
        self.set_actor_link_packet_id = int(self._lib.holo_core_packet_id_set_actor_link())
        self.add_player_packet_id = int(self._lib.holo_core_packet_id_add_player())
        self.mob_equipment_packet_id = int(self._lib.holo_core_packet_id_mob_equipment())
        self.remove_actor_packet_id = int(self._lib.holo_core_packet_id_remove_actor())
        self.move_actor_absolute_packet_id = int(self._lib.holo_core_packet_id_move_actor_absolute())
        self.set_actor_motion_packet_id = int(self._lib.holo_core_packet_id_set_actor_motion())
        self.take_item_actor_packet_id = int(self._lib.holo_core_packet_id_take_item_actor())

    def encode_primitive_shapes(self, shapes: list[ShapeDesc]) -> bytes:
        """把形状列表编码成 PrimitiveShapes 的 body（不含包头）。"""
        if not shapes:
            return b""
        array = (ShapeDesc * len(shapes))(*shapes)
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_primitive_shapes(array, len(shapes), ctypes.byref(out_ptr), ctypes.byref(out_len))
        if rc != 0:
            raise NativeCoreError(f"holo_core_encode_primitive_shapes 返回 {rc}")
        try:
            return ctypes.string_at(out_ptr, out_len.value)
        finally:
            self._lib.holo_free(out_ptr)


    def encode_spawn_particle(self, dim_id: int, x: float, y: float, z: float, effect: str) -> bytes:
        """SpawnParticleEffect 的 body（不含包头）。"""
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_spawn_particle(
            ctypes.c_uint8(dim_id & 0xFF),
            ctypes.c_int64(-1),
            ctypes.c_float(x),
            ctypes.c_float(y),
            ctypes.c_float(z),
            effect.encode("utf-8"),
            ctypes.byref(out_ptr),
            ctypes.byref(out_len),
        )
        if rc != 0:
            raise NativeCoreError(f"holo_core_encode_spawn_particle 返回 {rc}")
        try:
            return ctypes.string_at(out_ptr, out_len.value)
        finally:
            self._lib.holo_free(out_ptr)


    def encode_boss_event(
        self,
        target_actor_id: int,
        player_id: int,
        event_type: int,
        name: str,
        filtered_name: str,
        percentage: float,
        color: int,
        overlay: int,
    ) -> bytes:
        """BossEvent 的 body（不含包头）。"""
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_boss_event(
            ctypes.c_int64(target_actor_id),
            ctypes.c_int64(player_id),
            ctypes.c_uint8(event_type & 0xFF),
            name.encode("utf-8"),
            filtered_name.encode("utf-8"),
            ctypes.c_float(percentage),
            ctypes.c_uint32(color & 0xFFFFFFFF),
            ctypes.c_uint32(overlay & 0xFFFFFFFF),
            ctypes.byref(out_ptr),
            ctypes.byref(out_len),
        )
        if rc != 0:
            raise NativeCoreError(f"holo_core_encode_boss_event 返回 {rc}")
        try:
            return ctypes.string_at(out_ptr, out_len.value)
        finally:
            self._lib.holo_free(out_ptr)


    # ── 物品展示（fox 载体） ──────────────────────────────────────────────
    def encode_add_actor_ex(
        self,
        identifier: str,
        name: str,
        runtime_id: int,
        unique_id: int,
        x: float,
        y: float,
        z: float,
        yaw: float = 0.0,
        pitch: float = 0.0,
        hitbox_width: float = 0.0,
        hitbox_height: float = 0.0,
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_add_actor_ex(
            identifier.encode("utf-8"), name.encode("utf-8") if name else None,
            ctypes.c_uint64(runtime_id), ctypes.c_int64(unique_id),
            ctypes.c_float(x), ctypes.c_float(y), ctypes.c_float(z),
            ctypes.c_float(yaw), ctypes.c_float(pitch),
            ctypes.c_float(hitbox_width), ctypes.c_float(hitbox_height),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_add_actor_ex")

    def encode_mob_equipment(
        self,
        runtime_id: int,
        item_id: int,
        stack_size: int,
        aux: int,
        slot: int = 0,
        selected_slot: int = 0,
        container_id: int = 0,
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_mob_equipment(
            ctypes.c_uint64(runtime_id), ctypes.c_int16(item_id), ctypes.c_uint16(stack_size), ctypes.c_uint32(aux),
            ctypes.c_uint8(slot), ctypes.c_uint8(selected_slot), ctypes.c_uint8(container_id),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_mob_equipment")

    def build_item_nbt(
        self,
        item_type: str,
        count: int = 1,
        damage: int = 0,
        name: str = "",
        lore: list[str] | None = None,
        enchants: list[tuple[int, int]] | None = None,
        is_block: bool = False,
        block_version: int = 18168865,
    ) -> bytes:
        lore_csv = ";".join(lore or []).encode("utf-8") or None
        ench_csv = (";".join(f"{i}:{l}" for i, l in (enchants or []))).encode("utf-8") or None
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_build_item_nbt(
            item_type.encode("utf-8"), ctypes.c_int32(count), ctypes.c_int32(damage),
            name.encode("utf-8") if name else None, lore_csv, ench_csv,
            ctypes.c_int32(1 if is_block else 0), ctypes.c_int32(block_version),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_build_item_nbt")

    # ── 表情 / 骑乘 ───────────────────────────────────────────────────────
    def encode_emote(self, runtime_id: int, emote_id: str, length_ticks: int = 0, flags: int = 0) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_emote(
            ctypes.c_uint64(runtime_id), emote_id.encode("utf-8"), ctypes.c_uint32(length_ticks),
            ctypes.c_uint8(flags), ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_emote")

    def encode_set_actor_link(
        self,
        link_type: int,
        target_a_unique_id: int,
        target_b_unique_id: int,
        immediate: bool = True,
        passenger_initiated: bool = False,
        angular_velocity: float = 0.0,
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_set_actor_link(
            ctypes.c_uint8(link_type), ctypes.c_int64(target_a_unique_id), ctypes.c_int64(target_b_unique_id),
            ctypes.c_uint8(1 if immediate else 0), ctypes.c_uint8(1 if passenger_initiated else 0),
            ctypes.c_float(angular_velocity),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_set_actor_link")

    # ── 玩家形态 NPC ──────────────────────────────────────────────────────
    @staticmethod
    def make_skin_desc(skin_id: str, width: int, height: int, rgba: bytes, geometry: str = "", arm_size: str = "wide") -> "SkinDesc":
        desc = SkinDesc()
        desc.id = skin_id.encode("utf-8")
        desc.geometry = geometry.encode("utf-8") if geometry else None
        desc.arm_size = arm_size.encode("utf-8")
        desc.width = int(width)
        desc.height = int(height)
        desc.rgba = bytes(rgba)
        desc.rgba_len = len(rgba)
        return desc

    def encode_player_list_add(self, uuid16: bytes, actor_unique_id: int, name: str, skin: "SkinDesc") -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_player_list_add(
            bytes(uuid16), ctypes.c_int64(actor_unique_id), name.encode("utf-8"), ctypes.byref(skin),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_player_list_add")

    def encode_player_list_remove(self, uuid16: bytes) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_player_list_remove(bytes(uuid16), ctypes.byref(out_ptr), ctypes.byref(out_len))
        return self._take(rc, out_ptr, out_len, "holo_core_encode_player_list_remove")

    def encode_add_player(
        self,
        runtime_id: int,
        name: str,
        uuid16: bytes,
        skin: "SkinDesc",
        x: float,
        y: float,
        z: float,
        yaw: float = 0.0,
        pitch: float = 0.0,
        scale: float = 1.0,
        nametag: str = "",
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_add_player(
            ctypes.c_uint64(runtime_id), name.encode("utf-8"), bytes(uuid16), ctypes.byref(skin),
            ctypes.c_float(x), ctypes.c_float(y), ctypes.c_float(z),
            ctypes.c_float(yaw), ctypes.c_float(pitch), ctypes.c_float(scale),
            nametag.encode("utf-8") if nametag else None,
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_add_player")

    # ── NPC 对话 ──────────────────────────────────────────────────────────
    def encode_npc_dialogue(
        self, npc_id: int, action: int, dialogue: str, scene_name: str, npc_name: str, action_json: str
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_npc_dialogue(
            ctypes.c_uint64(npc_id), ctypes.c_uint8(action),
            dialogue.encode("utf-8"), scene_name.encode("utf-8"),
            npc_name.encode("utf-8"), action_json.encode("utf-8"),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_npc_dialogue")

    # ── 交易 ──────────────────────────────────────────────────────────────
    def build_offers(self, offers: list[TradeOffer], tier_exp: list[int] | None = None, base_net_id: int = 3676) -> bytes:
        array = (TradeOffer * len(offers))(*offers) if offers else None
        tiers = (ctypes.c_int32 * len(tier_exp))(*tier_exp) if tier_exp else None
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_build_offers(
            array, len(offers), tiers, len(tier_exp or []), ctypes.c_int32(base_net_id),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_build_offers")

    def encode_update_trade(
        self,
        container_id: int,
        container_type: int,
        size: int,
        tier: int,
        entity_unique_id: int,
        last_trading_player: int,
        display_name: str,
        use_new_screen: bool,
        use_economy_trade: bool,
        offers_nbt: bytes,
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_update_trade(
            ctypes.c_uint8(container_id), ctypes.c_uint8(container_type), ctypes.c_int32(size), ctypes.c_int32(tier),
            ctypes.c_int64(entity_unique_id), ctypes.c_int64(last_trading_player),
            display_name.encode("utf-8"), ctypes.c_uint8(1 if use_new_screen else 0),
            ctypes.c_uint8(1 if use_economy_trade else 0),
            offers_nbt, len(offers_nbt),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_update_trade")

    # ── 虚拟容器 ──────────────────────────────────────────────────────────
    def encode_container_open(self, container_id: int, container_type: int, x: int, y: int, z: int, target_actor_id: int = -1) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_container_open(
            ctypes.c_uint8(container_id), ctypes.c_uint8(container_type),
            ctypes.c_int32(x), ctypes.c_int32(y), ctypes.c_int32(z), ctypes.c_int64(target_actor_id),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_container_open")

    def encode_container_close(self, container_id: int, container_type: int, server_initiated: bool = True) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_container_close(
            ctypes.c_uint8(container_id), ctypes.c_uint8(container_type), ctypes.c_uint8(1 if server_initiated else 0),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_container_close")

    def encode_inventory_content(self, container_id: int, slots: list["ItemSlot"]) -> bytes:
        array = (ItemSlot * len(slots))(*slots) if slots else None
        _keep = [slot.user_data for slot in slots]  # ctypes 只存指针，调用期间必须保活
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_inventory_content(
            ctypes.c_uint32(container_id), array, len(slots), ctypes.byref(out_ptr), ctypes.byref(out_len)
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_inventory_content")

    # ── 物品实体 ──────────────────────────────────────────────────────────
    def encode_add_item_actor(
        self,
        runtime_id: int,
        unique_id: int,
        item_id: int,
        stack_size: int,
        aux: int,
        x: float,
        y: float,
        z: float,
        vx: float = 0.0,
        vy: float = 0.0,
        vz: float = 0.0,
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_add_item_actor(
            ctypes.c_uint64(runtime_id), ctypes.c_int64(unique_id), ctypes.c_int16(item_id),
            ctypes.c_uint16(stack_size), ctypes.c_uint32(aux),
            ctypes.c_float(x), ctypes.c_float(y), ctypes.c_float(z),
            ctypes.c_float(vx), ctypes.c_float(vy), ctypes.c_float(vz),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_add_item_actor")

    def encode_remove_actor(self, unique_id: int) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_remove_actor(
            ctypes.c_int64(unique_id), ctypes.byref(out_ptr), ctypes.byref(out_len)
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_remove_actor")

    def encode_move_actor_absolute(
        self,
        runtime_id: int,
        x: float,
        y: float,
        z: float,
        flags: int = 1,
        rotation_x: int = 0,
        rotation_y: int = 0,
        rotation_y_head: int = 0,
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_move_actor_absolute(
            ctypes.c_uint64(runtime_id), ctypes.c_uint8(flags),
            ctypes.c_float(x), ctypes.c_float(y), ctypes.c_float(z),
            ctypes.c_uint8(rotation_x), ctypes.c_uint8(rotation_y), ctypes.c_uint8(rotation_y_head),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_move_actor_absolute")

    def encode_set_actor_motion(self, runtime_id: int, vx: float, vy: float, vz: float, tick: int = 0) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_set_actor_motion(
            ctypes.c_uint64(runtime_id), ctypes.c_float(vx), ctypes.c_float(vy), ctypes.c_float(vz),
            ctypes.c_uint64(tick), ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_set_actor_motion")

    def encode_take_item_actor(self, item_runtime_id: int, actor_runtime_id: int) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_take_item_actor(
            ctypes.c_uint64(item_runtime_id), ctypes.c_uint64(actor_runtime_id),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_take_item_actor")

    # ── 复活 ──────────────────────────────────────────────────────────────
    def encode_respawn(self, x: float, y: float, z: float, state: int, runtime_id: int) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_respawn(
            ctypes.c_float(x), ctypes.c_float(y), ctypes.c_float(z),
            ctypes.c_uint8(state), ctypes.c_uint64(runtime_id),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_respawn")

    # ── 定位条 ────────────────────────────────────────────────────────────
    def encode_set_actor_data(self, runtime_id: int, tick: int, entries: list[MetaEntry]) -> bytes:
        if not entries:
            return b""
        array = (MetaEntry * len(entries))(*entries)
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_set_actor_data(
            ctypes.c_uint64(runtime_id), ctypes.c_uint64(tick), array, len(entries),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_set_actor_data")

    # ── 假方块 ────────────────────────────────────────────────────────────
    def encode_update_block(self, x: int, y: int, z: int, runtime_id: int, flag: int = 3, layer: int = 0) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_update_block(
            ctypes.c_int32(x), ctypes.c_int32(y), ctypes.c_int32(z),
            ctypes.c_uint32(runtime_id), ctypes.c_uint32(flag), ctypes.c_uint32(layer),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_update_block")

    # ── 玩家表现 ──────────────────────────────────────────────────────────
    def encode_mob_effect(
        self,
        runtime_id: int,
        event_type: int = 0,
        effect_id: int = 1,
        amplifier: int = 0,
        show_particles: bool = True,
        duration_ticks: int = 200,
        tick: int = 0,
        ambient: bool = False,
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_mob_effect(
            ctypes.c_uint64(runtime_id),
            ctypes.c_uint8(event_type),
            ctypes.c_int32(effect_id),
            ctypes.c_int32(amplifier),
            ctypes.c_uint8(1 if show_particles else 0),
            ctypes.c_int32(duration_ticks),
            ctypes.c_uint64(tick),
            ctypes.c_uint8(1 if ambient else 0),
            ctypes.byref(out_ptr),
            ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_mob_effect")

    def encode_actor_event(self, runtime_id: int, event_id: int, data: int = 0) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_actor_event(
            ctypes.c_uint64(runtime_id), ctypes.c_int32(event_id), ctypes.c_int32(data),
            ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_actor_event")

    def encode_animate_entity(
        self,
        animation: str,
        next_state: str = "none",
        stop_expression: str = "",
        molang_version: int = 0,
        controller: str = "",
        blend_out_time: float = 0.0,
        runtime_ids: list[int] | None = None,
    ) -> bytes:
        ids = runtime_ids or []
        array = (ctypes.c_uint64 * len(ids))(*ids) if ids else None
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_animate_entity(
            animation.encode("utf-8"),
            next_state.encode("utf-8"),
            stop_expression.encode("utf-8"),
            ctypes.c_int32(molang_version),
            controller.encode("utf-8"),
            ctypes.c_float(blend_out_time),
            array,
            len(ids),
            ctypes.byref(out_ptr),
            ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_animate_entity")

    # ── 计分板 / HUD ──────────────────────────────────────────────────────
    def _take(self, rc: int, out_ptr, out_len, what: str) -> bytes:
        if rc != 0:
            raise NativeCoreError(f"{what} 返回 {rc}")
        try:
            return ctypes.string_at(out_ptr, out_len.value)
        finally:
            self._lib.holo_free(out_ptr)

    def encode_set_display_objective(
        self, slot: str, objective: str, display_name: str, criteria: str = "dummy", sort_order: int = 0
    ) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_set_display_objective(
            slot.encode("utf-8"), objective.encode("utf-8"), display_name.encode("utf-8"),
            criteria.encode("utf-8"), ctypes.c_uint8(sort_order), ctypes.byref(out_ptr), ctypes.byref(out_len),
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_set_display_objective")

    def encode_remove_objective(self, objective: str) -> bytes:
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_remove_objective(
            objective.encode("utf-8"), ctypes.byref(out_ptr), ctypes.byref(out_len)
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_remove_objective")

    def encode_set_score(self, packet_type: int, entries: list[ScoreEntry]) -> bytes:
        if not entries:
            return b""
        array = (ScoreEntry * len(entries))(*entries)
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_set_score(
            ctypes.c_uint8(packet_type), array, len(entries), ctypes.byref(out_ptr), ctypes.byref(out_len)
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_set_score")

    def encode_set_hud(self, elements: list[int], visibility: int) -> bytes:
        array = (ctypes.c_uint8 * len(elements))(*[e & 0xFF for e in elements]) if elements else None
        out_ptr = ctypes.POINTER(ctypes.c_uint8)()
        out_len = ctypes.c_size_t(0)
        rc = self._lib.holo_core_encode_set_hud(
            array, len(elements), ctypes.c_uint8(visibility), ctypes.byref(out_ptr), ctypes.byref(out_len)
        )
        return self._take(rc, out_ptr, out_len, "holo_core_encode_set_hud")


def _default_dll_path() -> Path:
    return Path(__file__).resolve().parent / "_native" / "holo_core.dll"
