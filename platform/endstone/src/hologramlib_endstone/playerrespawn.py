"""玩家复活（PlayerRespawn）域 —— LeviLamina 版 PlayerRespawnManager 的 endstone 移植。"""

from __future__ import annotations

import threading

STATE_CLIENT_READY = 2


class PlayerRespawnApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()

    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _send(self, player, x: float, y: float, z: float, state: int) -> bool:
        try:
            body = self._plugin.core.encode_respawn(float(x), float(y), float(z), int(state), int(player.runtime_id))
            player.send_packet(self._plugin.core.respawn_packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"respawn 发包失败 ({player.name}): {exc}")
            return False

    def respawn(self, player_name: str, state: int = STATE_CLIENT_READY) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        loc = player.location
        return self._send(player, loc.x, loc.y, loc.z, state)

    def respawn_at(self, player_name: str, x: float, y: float, z: float, state: int = STATE_CLIENT_READY) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        return self._send(player, x, y, z, state)

    def respawn_state(self, player_name: str, state: int) -> bool:
        return self.respawn(player_name, max(0, min(2, int(state))))
