"""定位条（LocatorBar）域 —— LeviLamina 版 LocatorBarManager 的 endstone 移植。"""

from __future__ import annotations

import threading

from .native import Waypoint

# LocatorBarAction
ACTION_ADD = 1
ACTION_REMOVE = 2
ACTION_UPDATE = 3

# UpdateFlags
FLAG_POSITION = 1
FLAG_VISIBILITY = 2
FLAG_TEXTURE = 4
FLAG_ICON_SIZE = 8
FLAG_COLOR = 16
FLAG_AUTHORITY = 32

_MASK64 = (1 << 64) - 1
_FNV_MSB_SEED = 1469598103934665603
_FNV_LSB_SEED = 1099511628211
_FNV_PRIME = 1099511628211


def _fnv1a(text: str, seed: int) -> int:
    h = seed & _MASK64
    for byte in text.encode("utf-8"):
        h ^= byte
        h = (h * _FNV_PRIME) & _MASK64
    # 转成有符号 int64 传给 C
    return h - (1 << 64) if h >= (1 << 63) else h


def _uuid_of(key: str) -> tuple[int, int]:
    return _fnv1a(key, _FNV_MSB_SEED), _fnv1a(key, _FNV_LSB_SEED)


class _Waypoint:
    __slots__ = (
        "msb",
        "lsb",
        "x",
        "y",
        "z",
        "dimension",
        "color",
        "visible",
        "texture",
        "icon_w",
        "icon_h",
        "authority",
        "actor_uid",
        "group_key",
    )

    def __init__(self, key: str) -> None:
        self.msb, self.lsb = _uuid_of(key)
        self.x = self.y = self.z = 0.0
        self.dimension = 0
        self.color = -1
        self.visible = True
        self.texture = ""
        self.icon_w = self.icon_h = 0.0
        self.authority = False
        self.actor_uid = 0
        self.group_key = ""

    def to_payload(self, action: int) -> Waypoint:
        payload = Waypoint()
        payload.msb = self.msb
        payload.lsb = self.lsb
        payload.action = action
        if action == ACTION_REMOVE:
            return payload

        flags = FLAG_POSITION | FLAG_VISIBILITY | FLAG_AUTHORITY
        payload.has_position = 1
        payload.x, payload.y, payload.z = self.x, self.y, self.z
        payload.dimension = self.dimension
        payload.has_visible = 1
        payload.visible = 1 if self.visible else 0
        payload.has_authority = 1
        payload.authority = 1 if self.authority else 0
        if self.color >= 0:
            flags |= FLAG_COLOR
            payload.has_color = 1
            payload.color = self.color
        if self.texture:
            flags |= FLAG_TEXTURE
            payload.has_texture = 1
            payload.texture = self.texture.encode("utf-8")
        if self.icon_w > 0.0 and self.icon_h > 0.0:
            flags |= FLAG_ICON_SIZE
            payload.has_icon_size = 1
            payload.icon_w, payload.icon_h = self.icon_w, self.icon_h
        if self.actor_uid:
            payload.has_actor = 1
            payload.actor_uid = self.actor_uid
        payload.flags = flags
        return payload


class LocatorBarApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._by_player: dict[str, dict[str, _Waypoint]] = {}
        self._hidden_from_others: dict[str, bool] = {}

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _send(self, player, payloads: list[Waypoint]) -> bool:
        if not payloads:
            return False
        try:
            body = self._plugin.core.encode_locator_bar(payloads)
            player.send_packet(self._plugin.core.locator_bar_packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"locatorbar 发包失败 ({player.name}): {exc}")
            return False

    def _send_one(self, player, waypoint: _Waypoint, is_update: bool) -> bool:
        return self._send(player, [waypoint.to_payload(ACTION_UPDATE if is_update else ACTION_ADD)])

    def _lookup(self, player_name: str, key: str) -> _Waypoint | None:
        with self._lock:
            return self._by_player.get(player_name, {}).get(key)

    # ── 对外 ──────────────────────────────────────────────────────────────
    def set(self, player_name: str, key: str, x, y, z, dim: int = 0, color: int = -1, visible: bool = True) -> bool:
        return self.set_ex(player_name, key, x, y, z, dim, color, visible, "", 0.0, 0.0, False)

    def set_ex(
        self,
        player_name: str,
        key: str,
        x,
        y,
        z,
        dim: int = 0,
        color: int = -1,
        visible: bool = True,
        texture: str = "",
        icon_w: float = 0.0,
        icon_h: float = 0.0,
        authority: bool = False,
    ) -> bool:
        if not key:
            return False
        player = self._player(player_name)
        if player is None:
            return False
        with self._lock:
            per_player = self._by_player.setdefault(player_name, {})
            waypoint = per_player.get(key)
            existed = waypoint is not None
            if waypoint is None:
                waypoint = _Waypoint(key)
                per_player[key] = waypoint
            waypoint.x, waypoint.y, waypoint.z = float(x), float(y), float(z)
            waypoint.dimension = int(dim)
            waypoint.color = int(color)
            waypoint.visible = bool(visible)
            waypoint.texture = str(texture)
            waypoint.icon_w, waypoint.icon_h = float(icon_w), float(icon_h)
            waypoint.authority = bool(authority)
        return self._send_one(player, waypoint, existed)

    def remove(self, player_name: str, key: str) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        with self._lock:
            per_player = self._by_player.get(player_name)
            if not per_player or key not in per_player:
                return False
            waypoint = per_player.pop(key)
        return self._send(player, [waypoint.to_payload(ACTION_REMOVE)])

    def clear(self, player_name: str) -> int:
        player = self._player(player_name)
        if player is None:
            return 0
        with self._lock:
            per_player = self._by_player.pop(player_name, {})
        if not per_player:
            return 0
        payloads = [w.to_payload(ACTION_REMOVE) for w in per_player.values()]
        return len(payloads) if self._send(player, payloads) else 0

    def count(self, player_name: str) -> int:
        with self._lock:
            return len(self._by_player.get(player_name, {}))

    def set_group(self, player_name: str, key: str, group_key: str) -> bool:
        waypoint = self._lookup(player_name, key)
        if waypoint is None:
            return False
        waypoint.group_key = str(group_key)
        return True

    def sync_group(self, player_name: str, group_key: str) -> int:
        if not group_key:
            return 0
        player = self._player(player_name)
        if player is None:
            return 0
        with self._lock:
            batch = [
                w for w in self._by_player.get(player_name, {}).values() if w.group_key == str(group_key)
            ]
        if not batch:
            return 0
        payloads = [w.to_payload(ACTION_UPDATE) for w in batch]
        return len(payloads) if self._send(player, payloads) else 0

    def set_icon(self, player_name: str, key: str, texture: str) -> bool:
        waypoint = self._lookup(player_name, key)
        player = self._player(player_name)
        if waypoint is None or player is None:
            return False
        waypoint.texture = str(texture)
        return self._send_one(player, waypoint, True)

    def set_icon_size(self, player_name: str, key: str, width: float, height: float) -> bool:
        waypoint = self._lookup(player_name, key)
        player = self._player(player_name)
        if waypoint is None or player is None:
            return False
        waypoint.icon_w, waypoint.icon_h = float(width), float(height)
        return self._send_one(player, waypoint, True)

    def set_client_authority(self, player_name: str, key: str, enabled: bool) -> bool:
        waypoint = self._lookup(player_name, key)
        player = self._player(player_name)
        if waypoint is None or player is None:
            return False
        waypoint.authority = bool(enabled)
        return self._send_one(player, waypoint, True)

    def attach_entity(self, player_name: str, key: str, entity_unique_id: int) -> bool:
        waypoint = self._lookup(player_name, key)
        player = self._player(player_name)
        if waypoint is None or player is None:
            return False
        waypoint.actor_uid = int(entity_unique_id)
        return self._send_one(player, waypoint, True)

    def clear_attach(self, player_name: str, key: str) -> bool:
        waypoint = self._lookup(player_name, key)
        player = self._player(player_name)
        if waypoint is None or player is None:
            return False
        waypoint.actor_uid = 0
        return self._send_one(player, waypoint, True)

    def hide_from_others(self, player_name: str, hidden: bool) -> bool:
        with self._lock:
            self._hidden_from_others[player_name] = bool(hidden)
        return True

    def is_hidden_from_others(self, player_name: str) -> bool:
        with self._lock:
            return bool(self._hidden_from_others.get(player_name, False))

    def forget(self, player_name: str) -> None:
        with self._lock:
            self._by_player.pop(player_name, None)
            self._hidden_from_others.pop(player_name, None)
