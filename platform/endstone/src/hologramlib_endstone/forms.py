"""表单（Form）域 —— LeviLamina 版 formClose / formCloseAll 的 endstone 移植。"""

from __future__ import annotations


class FormApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin

    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def close(self, player_name: str) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        try:
            player.close_form()
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"formClose 失败 ({player_name}): {exc}")
            return False

    def close_all(self) -> int:
        closed = 0
        for player in list(self._plugin.server.online_players):
            if self.close(player.name):
                closed += 1
        return closed
