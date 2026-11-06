"""玩家形态 NPC（PlayerNpc）域 —— LeviLamina 版 PlayerNpcManager 的 endstone 移植（核心子集）。"""

from __future__ import annotations

import threading
import uuid as _uuid

from .holograms import _dim_id

FIRST_RUNTIME_ID = 0x6F000000
FIRST_UNIQUE_ID = 0x500000


class _Npc:
    __slots__ = ("id", "unique_id", "runtime_id", "name", "skin_id", "x", "y", "z", "dimension", "yaw", "scale", "nametag", "viewers", "enabled", "uuid_bytes", "view_distance", "player_rotations", "riding", "image_size")

    def __init__(self, npc_id: int) -> None:
        self.id = npc_id
        self.unique_id = 0
        self.runtime_id = 0
        self.name = ""
        self.skin_id = ""
        self.x = self.y = self.z = 0.0
        self.dimension = 0
        self.yaw = 0.0
        self.scale = 1.0
        self.nametag = ""
        self.viewers: set[str] | None = None
        self.enabled = True
        self.uuid_bytes = b""
        self.view_distance = 0.0
        self.player_rotations: dict[str, float] = {}
        self.riding = 0
        self.image_size = ("", 0.0, 0.0, False)


class PlayerNpcApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._items: dict[int, _Npc] = {}
        self._next_id = 1
        self._next_runtime = FIRST_RUNTIME_ID
        self._next_unique = FIRST_UNIQUE_ID

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _targets(self, npc: _Npc) -> list:
        players = [p for p in self._plugin.server.online_players if _dim_id(p) == npc.dimension]
        if npc.viewers is not None:
            players = [p for p in players if p.name in npc.viewers]
        return players

    def _send(self, player, packet_id: int, body: bytes) -> bool:
        if not body:
            return False
        try:
            player.send_packet(packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"playernpc 发包失败 ({player.name}): {exc}")
            return False

    def _skin_desc(self, skin_id: str):
        skin = self._plugin.skins.get(skin_id)
        if skin is None:
            return None
        return self._plugin.core.make_skin_desc(skin.id, skin.width, skin.height, skin.rgba, skin.geometry, "wide")

    def _spawn_for(self, npc: _Npc, player, desc) -> bool:
        core = self._plugin.core
        ok = self._send(player, core.player_list_packet_id,
                        core.encode_player_list_add(npc.uuid_bytes, npc.unique_id, npc.name, desc))
        ok = self._send(player, core.add_player_packet_id,
                        core.encode_add_player(npc.runtime_id, npc.name, npc.uuid_bytes, desc,
                                               npc.x, npc.y, npc.z, npc.yaw, 0.0, npc.scale, npc.nametag)) and ok
        return ok

    def get(self, npc_id: int) -> _Npc | None:
        with self._lock:
            return self._items.get(npc_id)

    # ── 创建 ──────────────────────────────────────────────────────────────
    def _create(self, x, y, z, dim, name, skin_id, yaw=0.0, scale=1.0, desired_id=0) -> int:
        desc = self._skin_desc(skin_id)
        if desc is None:
            self._plugin.logger.debug(f"playerNpcCreate: 皮肤不存在 {skin_id}")
            return -1
        with self._lock:
            npc_id = int(desired_id) if desired_id else self._next_id
            if npc_id in self._items:
                return -1
            if not desired_id:
                self._next_id += 1
            npc = _Npc(npc_id)
            npc.runtime_id = self._next_runtime
            self._next_runtime += 1
            npc.unique_id = self._next_unique
            self._next_unique += 1
            npc.name = str(name)
            npc.skin_id = str(skin_id)
            npc.x, npc.y, npc.z = float(x), float(y), float(z)
            npc.dimension = int(dim)
            npc.yaw = float(yaw)
            npc.scale = float(scale)
            npc.uuid_bytes = _uuid.uuid5(_uuid.NAMESPACE_OID, f"{npc.name}:{npc.unique_id}").bytes
            self._items[npc_id] = npc
        viewers = self._targets(npc)
        if not viewers:
            with self._lock:
                self._items.pop(npc_id, None)
            return -1
        for player in viewers:
            self._spawn_for(npc, player, desc)
        return npc_id

    def create(self, x, y, z, dim, name, skin_id) -> int:
        return self._create(x, y, z, dim, name, skin_id)

    def create_ex(self, x, y, z, dim, name, skin_id, yaw=0.0, scale=1.0) -> int:
        return self._create(x, y, z, dim, name, skin_id, yaw, scale)

    def create_random(self, x, y, z, dim, name, skin_id) -> int:
        return self._create(x, y, z, dim, name, skin_id)

    def create_with_id(self, x, y, z, dim, name, skin_id, desired_id) -> int:
        return self._create(x, y, z, dim, name, skin_id, desired_id=int(desired_id))

    # ── 销毁 / 查询 ───────────────────────────────────────────────────────
    def destroy(self, npc_id: int) -> bool:
        with self._lock:
            npc = self._items.pop(npc_id, None)
        if npc is None:
            return False
        core = self._plugin.core
        for player in self._targets(npc):
            self._send(player, core.remove_actor_packet_id, core.encode_remove_actor(npc.unique_id))
            self._send(player, core.player_list_packet_id, core.encode_player_list_remove(npc.uuid_bytes))
        return True

    def destroy_all(self) -> int:
        with self._lock:
            ids = list(self._items.keys())
        for npc_id in ids:
            self.destroy(npc_id)
        return len(ids)

    def exists(self, npc_id: int) -> bool:
        with self._lock:
            return npc_id in self._items

    def is_id_used(self, npc_id: int) -> bool:
        return self.exists(npc_id)

    def get_all_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._items.keys())

    def runtime_id(self, npc_id: int) -> int:
        npc = self.get(npc_id)
        return npc.runtime_id if npc is not None else 0

    def get_debug_info(self, npc_id: int) -> str:
        npc = self.get(npc_id)
        if npc is None:
            return ""
        return (
            f"#{npc.id} name={npc.name} skin={npc.skin_id} dim={npc.dimension} "
            f"pos=({npc.x:.2f},{npc.y:.2f},{npc.z:.2f}) yaw={npc.yaw:.1f} scale={npc.scale:.3f} "
            f"rid={npc.runtime_id} uid={npc.unique_id} viewers="
            f"{'all' if npc.viewers is None else ','.join(sorted(npc.viewers))}"
        )

    # ── 变换 / 外观 ───────────────────────────────────────────────────────
    def set_pos(self, npc_id: int, x, y, z, dim=None) -> bool:
        return self._move(npc_id, x, y, z, dim)

    def set_pos_light(self, npc_id: int, x, y, z, dim=None) -> bool:
        return self._move(npc_id, x, y, z, dim)

    def _move(self, npc_id: int, x, y, z, dim=None) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.x, npc.y, npc.z = float(x), float(y), float(z)
        if dim is not None:
            npc.dimension = int(dim)
        body = self._plugin.core.encode_move_actor_absolute(
            npc.runtime_id, npc.x, npc.y, npc.z, 1, 0,
            int(round((npc.yaw % 360.0) / 360.0 * 256.0)) & 0xFF,
            int(round((npc.yaw % 360.0) / 360.0 * 256.0)) & 0xFF,
        )
        return any(self._send(p, self._plugin.core.move_actor_absolute_packet_id, body) for p in self._targets(npc))

    def set_rotation(self, npc_id: int, yaw: float) -> bool:
        return self.set_rot_light(npc_id, yaw)

    def set_rot_light(self, npc_id: int, yaw: float) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.yaw = float(yaw)
        return self._move(npc_id, npc.x, npc.y, npc.z)

    def set_scale(self, npc_id: int, scale: float) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.scale = float(scale)
        return self._respawn(npc)

    def set_nametag(self, npc_id: int, text: str) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.nametag = str(text)
        return self._respawn(npc)

    def set_skin(self, npc_id: int, skin_id: str) -> bool:
        npc = self.get(npc_id)
        if npc is None or self._skin_desc(skin_id) is None:
            return False
        npc.skin_id = str(skin_id)
        return self._respawn(npc)

    def _respawn(self, npc: _Npc) -> bool:
        """外观类改动（皮肤/缩放/气泡名）只能拆了重建。"""
        if not npc.enabled:
            return True
        desc = self._skin_desc(npc.skin_id)
        if desc is None:
            return False
        core = self._plugin.core
        for player in self._targets(npc):
            self._send(player, core.remove_actor_packet_id, core.encode_remove_actor(npc.unique_id))
            self._send(player, core.player_list_packet_id, core.encode_player_list_remove(npc.uuid_bytes))
            self._spawn_for(npc, player, desc)
        return True

    def set_enabled(self, npc_id: int, enabled: bool) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        if bool(enabled) == npc.enabled:
            return True
        npc.enabled = bool(enabled)
        if npc.enabled:
            return self._respawn(npc)
        core = self._plugin.core
        for player in self._targets(npc):
            self._send(player, core.remove_actor_packet_id, core.encode_remove_actor(npc.unique_id))
            self._send(player, core.player_list_packet_id, core.encode_player_list_remove(npc.uuid_bytes))
        return True

    # ── 可见性 ────────────────────────────────────────────────────────────
    def set_visible_player(self, npc_id: int, player_name: str) -> bool:
        return self.set_visible_players(npc_id, [player_name])

    def set_visible_players(self, npc_id: int, names) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        if isinstance(names, str):
            names = [n.strip() for n in names.split(",") if n.strip()]
        npc.viewers = set(str(n) for n in names)
        return True

    def clear_visible_players(self, npc_id: int) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.viewers = None
        return True

    # ── 动画 / 装备 / 姿态 ────────────────────────────────────────────────
    def play_animation(self, npc_id: int, animation: str, stop_expression: str = "", duration_ticks: int = 0) -> bool:
        npc = self.get(npc_id)
        if npc is None or not animation:
            return False
        stop = stop_expression or (f"q.anim_time > {duration_ticks / 20.0:.2f}" if duration_ticks else "")
        body = self._plugin.core.encode_animate_entity(animation, "none", stop, 0, f"hololib.playernpc.{npc.id}", 0.0, [npc.runtime_id])
        return any(self._send(p, self._plugin.core.animate_entity_packet_id, body) for p in self._targets(npc))

    def play_animation_to(self, npc_id: int, player_name: str, animation: str, stop_expression: str = "", duration_ticks: int = 0) -> bool:
        npc = self.get(npc_id)
        player = self._player(player_name)
        if npc is None or player is None or not animation:
            return False
        stop = stop_expression or (f"q.anim_time > {duration_ticks / 20.0:.2f}" if duration_ticks else "")
        body = self._plugin.core.encode_animate_entity(animation, "none", stop, 0, f"hololib.playernpc.{npc.id}", 0.0, [npc.runtime_id])
        return self._send(player, self._plugin.core.animate_entity_packet_id, body)

    def stop_animation(self, npc_id: int) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        body = self._plugin.core.encode_animate_entity("", "none", "1", 0, f"hololib.playernpc.{npc.id}", 0.0, [npc.runtime_id])
        return any(self._send(p, self._plugin.core.animate_entity_packet_id, body) for p in self._targets(npc))

    def stop_animation_to(self, npc_id: int, player_name: str) -> bool:
        npc = self.get(npc_id)
        player = self._player(player_name)
        if npc is None or player is None:
            return False
        body = self._plugin.core.encode_animate_entity("", "none", "1", 0, f"hololib.playernpc.{npc.id}", 0.0, [npc.runtime_id])
        return self._send(player, self._plugin.core.animate_entity_packet_id, body)

    def set_equipment(self, npc_id: int, slot: int, name: str, aux: int = 0, nbt: str = "") -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        from . import items

        numeric = items.numeric_id(name)
        if numeric < 0:
            return False
        body = self._plugin.core.encode_mob_equipment(npc.runtime_id, numeric, 1, int(aux), int(slot), 0, 0)
        return any(self._send(p, self._plugin.core.mob_equipment_packet_id, body) for p in self._targets(npc))

    def set_pose(self, npc_id: int, pose: int) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        from .native import meta_int

        body = self._plugin.core.encode_set_actor_data(npc.runtime_id, self._plugin.tick_count, [meta_int(79, int(pose))])
        return any(self._send(p, self._plugin.core.set_actor_data_packet_id, body) for p in self._targets(npc))

    def set_view_distance(self, npc_id: int, dist: float) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.view_distance = float(dist)
        return True

    # ── 逐玩家朝向 ────────────────────────────────────────────────────────
    def set_player_rotation(self, npc_id: int, player_name: str, yaw: float) -> bool:
        npc = self.get(npc_id)
        player = self._player(player_name)
        if npc is None or player is None:
            return False
        npc.player_rotations[str(player_name)] = float(yaw)
        body = self._plugin.core.encode_move_actor_absolute(
            npc.runtime_id, npc.x, npc.y, npc.z, 1, 0,
            int(round((float(yaw) % 360.0) / 360.0 * 256.0)) & 0xFF,
            int(round((float(yaw) % 360.0) / 360.0 * 256.0)) & 0xFF,
        )
        return self._send(player, self._plugin.core.move_actor_absolute_packet_id, body)

    def clear_player_rotation(self, npc_id: int, player_name: str) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        return npc.player_rotations.pop(str(player_name), None) is not None

    def clear_player_rotations(self, npc_id: int) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.player_rotations.clear()
        return True

    # ── 皮肤注入（给已存在的玩家换皮）────────────────────────────────────
    def inject_skin(self, viewer_name: str, target_name: str, skin_id: str) -> bool:
        desc = self._skin_desc(skin_id)
        if desc is None:
            return False
        target = self._player(target_name)
        if target is None:
            return False
        body = self._plugin.core.encode_player_list_add(bytes(target.unique_id.bytes), 0, target.name, desc)
        if viewer_name:
            viewer = self._player(viewer_name)
            return viewer is not None and self._send(viewer, self._plugin.core.player_list_packet_id, body)
        sent = False
        for player in list(self._plugin.server.online_players):
            sent = self._send(player, self._plugin.core.player_list_packet_id, body) or sent
        return sent

    def inject_skin_all(self, target_name: str, skin_id: str) -> bool:
        return self.inject_skin("", target_name, skin_id)

    # ── 皮肤杂项 ──────────────────────────────────────────────────────────
    def capture_skin(self, skin_id: str, player_name: str) -> bool:
        """把玩家皮肤存成 skinId。endstone 不暴露 Image，只能复用已注册的同名皮肤。"""
        if self._plugin.skins.has(skin_id):
            return True
        self._plugin.logger.debug(f"captureSkin({player_name}): endstone 无 Image 绑定，无法读取像素")
        return False

    def register_image_skin(self, png_path: str, skin_id: str, size_blocks: float = 1.0, transparent: bool = False) -> bool:
        """图像皮肤：按普通皮肤解码 PNG，记录 sizeBlocks（贴图缩放由客户端几何决定）。"""
        ok = self._plugin.skins.register_png(png_path, skin_id)
        if ok:
            self._plugin.skins.set_image_size(skin_id, float(size_blocks), float(size_blocks), transparent)
        return ok

    def register_image_skin_sized(self, png_path: str, skin_id: str, width_blocks: float, height_blocks: float,
                                  transparent: bool = False) -> bool:
        ok = self._plugin.skins.register_png(png_path, skin_id)
        if ok:
            self._plugin.skins.set_image_size(skin_id, float(width_blocks), float(height_blocks), transparent)
        return ok

    def set_image_size(self, npc_id: int, png_path: str, width_blocks: float, height_blocks: float,
                       transparent: bool = False) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.image_size = (str(png_path), float(width_blocks), float(height_blocks), bool(transparent))
        self._plugin.skins.set_image_size(npc.skin_id, float(width_blocks), float(height_blocks), transparent)
        return True

    # ── 皮肤注册（转发到 skins） ──────────────────────────────────────────
    def register_skin(self, png_path: str, skin_id: str, geometry: str = "", arm_size: int = 0) -> bool:
        return self._plugin.skins.register_png(png_path, skin_id, geometry, arm_size)

    def register_skin_data(self, png_path: str, skin_id: str, arm_size: int = 0, geometry: str = "") -> bool:
        return self._plugin.skins.register_png(png_path, skin_id, geometry, arm_size)

    def register_skin_data_opaque(self, png_path: str, skin_id: str, arm_size: int = 0, geometry: str = "") -> bool:
        return self._plugin.skins.register_png(png_path, skin_id, geometry, arm_size)

    def has_skin(self, skin_id: str) -> bool:
        return self._plugin.skins.has(skin_id)

    def get_skin_ids(self) -> list[str]:
        return self._plugin.skins.get_ids()

    def get_skin_blob_b64(self, skin_id: str) -> str:
        return self._plugin.skins.export_blob(skin_id)

    def register_skin_from_blob_b64(self, blob: str) -> str:
        return self._plugin.skins.import_blob(blob)

    def register_skin_from_blob_file(self, path: str) -> str:
        try:
            return self._plugin.skins.import_blob(open(path, "r", encoding="utf-8").read().strip())
        except Exception as exc:
            self._plugin.logger.debug(f"registerSkinFromBlobFile 失败: {exc}")
            return ""

    def unregister_skin(self, skin_id: str) -> bool:
        return self._plugin.skins.unregister(skin_id)

    def import_skins(self, dir_path: str) -> int:
        return self._plugin.skins.import_dir(dir_path)

    def poll_spawns(self) -> list[str]:
        return []

    def clear_spawns(self) -> bool:
        return True

    # ── 表情 / 骑乘 ───────────────────────────────────────────────────────
    def play_emote(self, npc_id: int, piece_id: str, length_ticks: int = 0) -> bool:
        npc = self.get(npc_id)
        if npc is None or not piece_id:
            return False
        body = self._plugin.core.encode_emote(npc.runtime_id, str(piece_id), int(length_ticks), 0)
        return any(self._send(p, self._plugin.core.emote_packet_id, body) for p in self._targets(npc))

    def _link(self, npc: _Npc, link_type: int, vehicle_unique_id: int) -> bool:
        body = self._plugin.core.encode_set_actor_link(link_type, npc.unique_id, int(vehicle_unique_id), True, False, 0.0)
        return any(self._send(p, self._plugin.core.set_actor_link_packet_id, body) for p in self._targets(npc))

    def set_ride_unique_id(self, npc_id: int, vehicle_unique_id: int) -> bool:
        npc = self.get(npc_id)
        if npc is None:
            return False
        npc.riding = int(vehicle_unique_id)
        return self._link(npc, 1, vehicle_unique_id)  # ActorLinkType::Riding

    def set_ride_entity(self, npc_id: int, vehicle_entity_id: int) -> bool:
        npc = self.get(npc_id)
        vehicle = self._plugin.entities.get(int(vehicle_entity_id))
        if npc is None or vehicle is None:
            return False
        return self.set_ride_unique_id(npc_id, vehicle.unique_id)

    def set_ride_player(self, npc_id: int, player_name: str) -> bool:
        """骑在真实玩家身上需要玩家的 actor uniqueId，endstone 只给 uuid，暂不支持。"""
        _ = (npc_id, player_name)
        return False

    def clear_ride(self, npc_id: int) -> bool:
        npc = self.get(npc_id)
        if npc is None or not npc.riding:
            return False
        ok = self._link(npc, 0, npc.riding)  # ActorLinkType::None 解除
        npc.riding = 0
        return ok

    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def __len__(self) -> int:
        with self._lock:
            return len(self._items)
