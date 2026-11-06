"""可见性（Visibility）域 —— 让某个真实玩家对其他人不可见。"""

from __future__ import annotations

import threading

MASK_SPAWN = 1
MASK_TAB = 2
MASK_MOTION = 4
MASK_STATE = 8
MASK_SKIN = 16

ALL_MASKS = MASK_SPAWN | MASK_TAB | MASK_MOTION | MASK_STATE | MASK_SKIN


class VisibilityApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._hidden: dict[str, int] = {}  # playerName -> mask

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _broadcast_add_player(self, target_name: str) -> bool:
        """给其他玩家补发 AddPlayer（恢复可见）。"""
        target = self._player(target_name)
        if target is None:
            return False
        skin = None
        for skin_id in self._plugin.skins.get_ids():
            skin = self._plugin.skins.get(skin_id)
            break
        if skin is None:
            return False
        desc = self._plugin.core.make_skin_desc(skin.id, skin.width, skin.height, skin.rgba, skin.geometry, "wide")
        body = self._plugin.core.encode_add_player(
            int(target.runtime_id), target.name, bytes(target.unique_id.bytes), desc,
            target.location.x, target.location.y, target.location.z, 0.0, 0.0, 1.0, "",
        )
        sent = False
        for player in list(self._plugin.server.online_players):
            if player.name == target_name:
                continue
            try:
                player.send_packet(self._plugin.core.add_player_packet_id, body)
                sent = True
            except Exception:
                pass
        return sent

    # ── 对外 ──────────────────────────────────────────────────────────────
    def set_hidden(self, player_name: str, hidden: bool) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        with self._lock:
            if hidden:
                runtime_id = int(player.runtime_id)
                self._hidden[player_name] = self._hidden.get(player_name, ALL_MASKS)
                view = self._plugin.view
                view._state.hidden_runtime_ids.setdefault(runtime_id, set())
                view._state.owners[runtime_id] = player_name
                view._plugin.ensure_view_filter()
            else:
                mask = self._hidden.pop(player_name, 0)
                view = self._plugin.view
                view._state.hidden_runtime_ids.pop(int(player.runtime_id), None)
                view._state.owners.pop(int(player.runtime_id), None)
                if mask == 0:
                    return True
        self._broadcast_add_player(player_name)
        return True

    def is_hidden(self, player_name: str) -> bool:
        with self._lock:
            return player_name in self._hidden

    def set_mask(self, player_name: str, spawn: bool, tab: bool, motion: bool, state: bool, skin: bool, also_self: bool = False) -> bool:
        _ = also_self
        mask = 0
        for bit, flag in ((MASK_SPAWN, spawn), (MASK_TAB, tab), (MASK_MOTION, motion), (MASK_STATE, state), (MASK_SKIN, skin)):
            if flag:
                mask |= bit
        with self._lock:
            if mask == 0:
                return self.set_hidden(player_name, False)
            self._hidden[player_name] = mask
        return self.set_hidden(player_name, True)

    def get_mask(self, player_name: str) -> int:
        with self._lock:
            return self._hidden.get(player_name, 0)

    def restore(self, player_name: str) -> bool:
        return self.set_hidden(player_name, False)
