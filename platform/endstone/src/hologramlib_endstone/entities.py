"""自定义实体（Entity）域 —— LeviLamina 版 CustomEntityManager 的 endstone 移植（核心子集）。"""

from __future__ import annotations

import threading

from . import items
from .holograms import _dim_id

FIRST_RUNTIME_ID = 0x6E000000
FIRST_UNIQUE_ID = 0x400000

# ActorDataIDs
ID_VARIANT = 2
ID_COLOR_INDEX = 3
ID_NAME = 4
ID_MARK_VARIANT = 43
ID_POSE_INDEX = 79
ID_NAMETAG_ALWAYS_SHOW = 81
ID_FLAGS = 0
ID_HITBOX_W = 53
ID_HITBOX_H = 54

FLAG_INVISIBLE = 1 << 5


class _Entity:
    __slots__ = (
        "id",
        "unique_id",
        "runtime_id",
        "identifier",
        "x",
        "y",
        "z",
        "dimension",
        "yaw",
        "pitch",
        "scale",
        "nametag",
        "variant",
        "color_index",
        "mark_variant",
        "pose",
        "invisible",
        "flags",
        "viewers",
        "view_distance",
        "enabled",
        "riding",
        "mob_properties",
        "player_overrides",
    )

    def __init__(self, entity_id: int) -> None:
        self.id = entity_id
        self.unique_id = 0
        self.runtime_id = 0
        self.identifier = ""
        self.x = self.y = self.z = 0.0
        self.dimension = 0
        self.yaw = self.pitch = 0.0
        self.scale = 1.0
        self.nametag = ""
        self.variant = -1
        self.color_index = -1
        self.mark_variant = -1
        self.pose = -1
        self.invisible = False
        self.flags = FLAG_INVISIBLE
        self.viewers: set[str] | None = None
        self.view_distance = 0.0
        self.enabled = True
        self.riding = 0
        self.mob_properties: dict[str, str] = {}
        self.player_overrides: dict[str, dict] = {}


class EntityApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._items: dict[int, _Entity] = {}
        self._next_id = 1
        self._next_runtime = FIRST_RUNTIME_ID
        self._next_unique = FIRST_UNIQUE_ID

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _targets(self, entity: _Entity) -> list:
        players = [p for p in self._plugin.server.online_players if _dim_id(p) == entity.dimension]
        if entity.viewers is not None:
            players = [p for p in players if p.name in entity.viewers]
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
                self._plugin.logger.debug(f"entity 发包失败 ({player.name}): {exc}")
        return sent

    def get(self, entity_id: int) -> _Entity | None:
        with self._lock:
            return self._items.get(entity_id)

    def _spawn(self, entity: _Entity) -> bool:
        players = self._targets(entity)
        if not players:
            return False
        body = self._plugin.core.encode_add_actor_ex(
            entity.identifier, entity.nametag, entity.runtime_id, entity.unique_id,
            entity.x, entity.y, entity.z, entity.yaw, entity.pitch, 0.0, 0.0,
        )
        if not self._send(players, self._plugin.core.add_actor_packet_id, body):
            return False
        self._sync_metadata(entity, players)
        return True

    def _sync_metadata(self, entity: _Entity, players=None) -> bool:
        from .native import meta_int

        entries = []
        if entity.flags or entity.invisible:
            flags = entity.flags | (FLAG_INVISIBLE if entity.invisible else 0)
            entries.append(meta_int(ID_FLAGS, flags))
        if entity.variant >= 0:
            entries.append(meta_int(ID_VARIANT, entity.variant))
        if entity.color_index >= 0:
            entries.append(meta_int(ID_COLOR_INDEX, entity.color_index))
        if entity.mark_variant >= 0:
            entries.append(meta_int(ID_MARK_VARIANT, entity.mark_variant))
        if entity.pose >= 0:
            entries.append(meta_int(ID_POSE_INDEX, entity.pose))
        if entity.nametag:
            entries.append(meta_int(ID_NAMETAG_ALWAYS_SHOW, 1))
        if not entries:
            return True
        body = self._plugin.core.encode_set_actor_data(entity.runtime_id, self._plugin.tick_count, entries)
        return self._send(players if players is not None else self._targets(entity),
                          self._plugin.core.set_actor_data_packet_id, body)

    def _move(self, entity: _Entity) -> bool:
        body = self._plugin.core.encode_move_actor_absolute(
            entity.runtime_id, entity.x, entity.y, entity.z, 1,
            int(round((entity.pitch % 360.0) / 360.0 * 256.0)) & 0xFF,
            int(round((entity.yaw % 360.0) / 360.0 * 256.0)) & 0xFF,
            int(round((entity.yaw % 360.0) / 360.0 * 256.0)) & 0xFF,
        )
        return self._send(self._targets(entity), self._plugin.core.move_actor_absolute_packet_id, body)

    # ── 创建 ──────────────────────────────────────────────────────────────
    def _create(self, identifier, x, y, z, dim, yaw=0.0, pitch=0.0, scale=1.0, nametag="", desired_id=0, viewers=None) -> int:
        if not identifier:
            return -1
        with self._lock:
            entity_id = int(desired_id) if desired_id else self._next_id
            if entity_id in self._items:
                return -1
            if not desired_id:
                self._next_id += 1
            entity = _Entity(entity_id)
            entity.runtime_id = self._next_runtime
            self._next_runtime += 1
            entity.unique_id = self._next_unique
            self._next_unique += 1
            entity.identifier = str(identifier)
            entity.x, entity.y, entity.z = float(x), float(y), float(z)
            entity.dimension = int(dim)
            entity.yaw, entity.pitch = float(yaw), float(pitch)
            entity.scale = float(scale)
            entity.nametag = str(nametag)
            entity.viewers = set(viewers) if viewers is not None else None
            self._items[entity_id] = entity
        if not self._spawn(entity):
            with self._lock:
                self._items.pop(entity_id, None)
            return -1
        return entity_id

    def create(self, identifier, x, y, z, dim) -> int:
        return self._create(identifier, x, y, z, dim)

    def create_advanced(self, identifier, x, y, z, dim, yaw=0.0, pitch=0.0, scale=1.0, nametag="", viewers=None) -> int:
        return self._create(identifier, x, y, z, dim, yaw, pitch, scale, nametag, viewers=viewers)

    def create_random(self, identifier, x, y, z, dim) -> int:
        return self._create(identifier, x, y, z, dim)

    def create_with_id(self, identifier, x, y, z, dim, desired_id) -> int:
        return self._create(identifier, x, y, z, dim, desired_id=int(desired_id))

    # ── 生命周期 ──────────────────────────────────────────────────────────
    def destroy(self, entity_id: int) -> bool:
        with self._lock:
            entity = self._items.pop(entity_id, None)
        if entity is None:
            return False
        body = self._plugin.core.encode_remove_actor(entity.unique_id)
        self._send(self._targets(entity), self._plugin.core.remove_actor_packet_id, body)
        return True

    def destroy_all(self) -> int:
        with self._lock:
            ids = list(self._items.keys())
        for entity_id in ids:
            self.destroy(entity_id)
        return len(ids)

    def exists(self, entity_id: int) -> bool:
        with self._lock:
            return entity_id in self._items

    def is_id_used(self, entity_id: int) -> bool:
        return self.exists(entity_id)

    def get_all_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._items.keys())

    def get_info(self, entity_id: int) -> str:
        entity = self.get(entity_id)
        if entity is None:
            return ""
        return (
            f"#{entity.id} {entity.identifier} dim={entity.dimension} "
            f"pos=({entity.x:.2f},{entity.y:.2f},{entity.z:.2f}) yaw={entity.yaw:.1f} "
            f"variant={entity.variant} color={entity.color_index} rid={entity.runtime_id} "
            f"enabled={entity.enabled}"
        )

    def find_nearest(self, x, y, z, dim, max_dist) -> int:
        best, best_d2 = -1, float(max_dist) ** 2
        for entity_id, entity in list(self._items.items()):
            if entity.dimension != int(dim):
                continue
            d2 = (entity.x - x) ** 2 + (entity.y - y) ** 2 + (entity.z - z) ** 2
            if d2 <= best_d2:
                best, best_d2 = entity_id, d2
        return best

    def set_enabled(self, entity_id: int, enabled: bool) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        if bool(enabled) == entity.enabled:
            return True
        entity.enabled = bool(enabled)
        if entity.enabled:
            return self._spawn(entity)
        body = self._plugin.core.encode_remove_actor(entity.unique_id)
        return self._send(self._targets(entity), self._plugin.core.remove_actor_packet_id, body)

    # ── 外观 / 状态 ───────────────────────────────────────────────────────
    def set_position(self, entity_id: int, x, y, z, dim=None) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.x, entity.y, entity.z = float(x), float(y), float(z)
        if dim is not None:
            entity.dimension = int(dim)
        return self._move(entity)

    def set_rotation(self, entity_id: int, yaw: float, pitch: float) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.yaw, entity.pitch = float(yaw), float(pitch)
        return self._move(entity)

    def set_scale(self, entity_id: int, scale: float) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.scale = float(scale)
        return True

    def scale_by(self, entity_id: int, factor: float) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        return self.set_scale(entity_id, entity.scale * float(factor))

    def set_nametag(self, entity_id: int, text: str) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.nametag = str(text)
        return self._sync_metadata(entity) and self._respawn_nametag(entity)

    def _respawn_nametag(self, entity: _Entity) -> bool:
        # 名字是 AddActor 的元数据，改名字最稳的是重发一遍 AddActor
        body = self._plugin.core.encode_add_actor_ex(
            entity.identifier, entity.nametag, entity.runtime_id, entity.unique_id,
            entity.x, entity.y, entity.z, entity.yaw, entity.pitch, 0.0, 0.0,
        )
        return self._send(self._targets(entity), self._plugin.core.add_actor_packet_id, body)

    def set_variant(self, entity_id: int, variant: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.variant = int(variant)
        return self._sync_metadata(entity)

    def set_color_index(self, entity_id: int, color_index: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.color_index = int(color_index)
        return self._sync_metadata(entity)

    def set_mark_variant(self, entity_id: int, mark_variant: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.mark_variant = int(mark_variant)
        return self._sync_metadata(entity)

    def set_pose(self, entity_id: int, pose: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.pose = int(pose)
        return self._sync_metadata(entity)

    def set_flags(self, entity_id: int, flags: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.flags = int(flags)
        return self._sync_metadata(entity)

    def set_invisible(self, entity_id: int, on: bool) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.invisible = bool(on)
        return self._sync_metadata(entity)

    def set_identifier(self, entity_id: int, identifier: str) -> bool:
        entity = self.get(entity_id)
        if entity is None or not identifier:
            return False
        old = entity.identifier
        if str(identifier) == old:
            return True
        # 换 identifier 只能拆了重建：先删旧实体，再用新标识生成
        self._send(self._targets(entity), self._plugin.core.remove_actor_packet_id,
                   self._plugin.core.encode_remove_actor(entity.unique_id))
        entity.identifier = str(identifier)
        if self._spawn(entity):
            return True
        entity.identifier = old
        self._spawn(entity)
        return False

    # ── 动画 / 装备 ───────────────────────────────────────────────────────
    def play_animation(self, entity_id: int, animation: str, stop_expression: str = "", duration_ticks: int = 0) -> bool:
        entity = self.get(entity_id)
        if entity is None or not animation:
            return False
        stop = stop_expression or (f"q.anim_time > {duration_ticks / 20.0:.2f}" if duration_ticks else "")
        body = self._plugin.core.encode_animate_entity(animation, "none", stop, 0, "", 0.0, [entity.runtime_id])
        return self._send(self._targets(entity), self._plugin.core.animate_entity_packet_id, body)

    def play_animation_to(self, entity_id: int, player_name: str, animation: str, stop_expression: str = "", duration_ticks: int = 0) -> bool:
        entity = self.get(entity_id)
        if entity is None or not animation:
            return False
        player = self._player(player_name)
        if player is None:
            return False
        stop = stop_expression or (f"q.anim_time > {duration_ticks / 20.0:.2f}" if duration_ticks else "")
        body = self._plugin.core.encode_animate_entity(animation, "none", stop, 0, "", 0.0, [entity.runtime_id])
        return self._send([player], self._plugin.core.animate_entity_packet_id, body)

    def stop_animation(self, entity_id: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        body = self._plugin.core.encode_animate_entity("", "none", "q.anim_time > 0", 0, "", 0.0, [entity.runtime_id])
        return self._send(self._targets(entity), self._plugin.core.animate_entity_packet_id, body)

    def stop_animation_to(self, entity_id: int, player_name: str) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        player = self._player(player_name)
        if player is None:
            return False
        body = self._plugin.core.encode_animate_entity("", "none", "q.anim_time > 0", 0, "", 0.0, [entity.runtime_id])
        return self._send([player], self._plugin.core.animate_entity_packet_id, body)

    def set_equipment_slot(self, entity_id: int, slot: int, name: str, aux: int = 0, nbt: str = "") -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        numeric = items.numeric_id(name)
        if numeric < 0:
            return False
        body = self._plugin.core.encode_mob_equipment(entity.runtime_id, numeric, 1, int(aux), int(slot), 0, 0)
        return self._send(self._targets(entity), self._plugin.core.mob_equipment_packet_id, body)

    # ── 可见性 ────────────────────────────────────────────────────────────
    # ── 逐玩家外观覆盖 ────────────────────────────────────────────────────
    def _viewer(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def set_player_nametag(self, entity_id: int, player_name: str, text: str) -> bool:
        entity = self.get(entity_id)
        viewer = self._viewer(player_name)
        if entity is None or viewer is None:
            return False
        from .native import meta_int

        entity.player_overrides.setdefault(str(player_name), {})["nametag"] = str(text)
        body = self._plugin.core.encode_set_actor_data(entity.runtime_id, self._plugin.tick_count, [meta_int(81, 1)])
        return self._send([viewer], self._plugin.core.set_actor_data_packet_id, body)

    def set_player_rotation(self, entity_id: int, player_name: str, yaw: float, pitch: float = 0.0) -> bool:
        entity = self.get(entity_id)
        viewer = self._viewer(player_name)
        if entity is None or viewer is None:
            return False
        entity.player_overrides.setdefault(str(player_name), {})["rotation"] = (float(yaw), float(pitch))
        body = self._plugin.core.encode_move_actor_absolute(
            entity.runtime_id, entity.x, entity.y, entity.z, 1,
            int(round((float(pitch) % 360.0) / 360.0 * 256.0)) & 0xFF,
            int(round((float(yaw) % 360.0) / 360.0 * 256.0)) & 0xFF,
            int(round((float(yaw) % 360.0) / 360.0 * 256.0)) & 0xFF,
        )
        return self._send([viewer], self._plugin.core.move_actor_absolute_packet_id, body)

    def set_player_scale(self, entity_id: int, player_name: str, scale: float) -> bool:
        entity = self.get(entity_id)
        viewer = self._viewer(player_name)
        if entity is None or viewer is None:
            return False
        entity.player_overrides.setdefault(str(player_name), {})["scale"] = float(scale)
        from .native import meta_float

        body = self._plugin.core.encode_set_actor_data(
            entity.runtime_id, self._plugin.tick_count, [meta_float(54, 1.8 * float(scale))]
        )
        return self._send([viewer], self._plugin.core.set_actor_data_packet_id, body)

    def set_player_equipment_slot(self, entity_id: int, player_name: str, slot: int, name: str, aux: int = 0, nbt: str = "") -> bool:
        entity = self.get(entity_id)
        viewer = self._viewer(player_name)
        if entity is None or viewer is None:
            return False
        numeric = items.numeric_id(name)
        if numeric < 0:
            return False
        entity.player_overrides.setdefault(str(player_name), {})[f"equip{int(slot)}"] = str(name)
        body = self._plugin.core.encode_mob_equipment(entity.runtime_id, numeric, 1, int(aux), int(slot), 0, 0)
        return self._send([viewer], self._plugin.core.mob_equipment_packet_id, body)

    def clear_player_rotation(self, entity_id: int, player_name: str) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        overrides = entity.player_overrides.get(str(player_name)) or {}
        return overrides.pop("rotation", None) is not None

    def clear_player_rotations(self, entity_id: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        for overrides in entity.player_overrides.values():
            overrides.pop("rotation", None)
        return True

    def poll_spawns(self) -> list[str]:
        """实体对玩家的 spawn 事件：需要入站包解析（未实现）。"""
        return []

    def clear_spawns(self) -> bool:
        return True

    def clear_player_appearance(self, entity_id: int, player_name: str) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        return entity.player_overrides.pop(str(player_name), None) is not None

    # ── 骑乘 / mob 属性 ───────────────────────────────────────────────────
    def _link(self, entity: _Entity, link_type: int, vehicle_unique_id: int) -> bool:
        body = self._plugin.core.encode_set_actor_link(link_type, entity.unique_id, int(vehicle_unique_id), True, False, 0.0)
        sent = False
        for player in self._targets(entity):
            sent = self._send([player], self._plugin.core.set_actor_link_packet_id, body) or sent
        return sent

    def set_ride_entity(self, entity_id: int, vehicle_entity_id: int) -> bool:
        entity = self.get(entity_id)
        vehicle = self.get(int(vehicle_entity_id))
        if entity is None or vehicle is None:
            return False
        entity.riding = vehicle.unique_id
        return self._link(entity, 1, vehicle.unique_id)

    def set_ride_player(self, entity_id: int, player_name: str) -> bool:
        """骑真实玩家需要 actor uniqueId，endstone 只给 uuid，暂不支持。"""
        _ = (entity_id, player_name)
        return False

    def clear_ride(self, entity_id: int) -> bool:
        entity = self.get(entity_id)
        if entity is None or not entity.riding:
            return False
        ok = self._link(entity, 0, entity.riding)
        entity.riding = 0
        return ok

    def set_mob_property(self, entity_id: int, name: str, value: str) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.mob_properties[str(name)] = str(value)
        return True  # PropertySyncData 同步未实现，只记录

    def clear_mob_properties(self, entity_id: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.mob_properties.clear()
        return True

    def set_view_distance(self, entity_id: int, dist: float) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.view_distance = float(dist)
        return True

    def set_visible_player(self, entity_id: int, player_name: str) -> bool:
        return self.set_visible_players(entity_id, [player_name])

    def set_visible_players(self, entity_id: int, names) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        if isinstance(names, str):
            names = [n.strip() for n in names.split(",") if n.strip()]
        entity.viewers = set(str(n) for n in names)
        return True

    def clear_visible_players(self, entity_id: int) -> bool:
        entity = self.get(entity_id)
        if entity is None:
            return False
        entity.viewers = None
        return True

    # ── 辅助 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def runtime_id(self, entity_id: int) -> int:
        entity = self.get(entity_id)
        return entity.runtime_id if entity is not None else 0

    def unique_id(self, entity_id: int) -> int:
        entity = self.get(entity_id)
        return entity.unique_id if entity is not None else 0

    def __len__(self) -> int:
        with self._lock:
            return len(self._items)
