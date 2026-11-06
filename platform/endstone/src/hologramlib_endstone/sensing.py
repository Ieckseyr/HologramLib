"""感知/输入（Sensing）域 —— 占位实现。"""

from __future__ import annotations


class SensingApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._notes: list[str] = []

    def _note(self, what: str) -> None:
        if what not in self._notes:
            self._notes.append(what)
            self._plugin.logger.debug(f"sensing: {what} 需要客户端 AuthInput 解码，端侧无入口")

    def device_code(self, player_name: str) -> str:
        self._note(f"deviceCode({player_name})")
        return ""

    def device_of(self, player_name: str) -> str:
        self._note(f"deviceOf({player_name})")
        return ""

    def is_touch(self, player_name: str) -> bool:
        self._note(f"isTouch({player_name})")
        return False

    def clear(self) -> None:
        self._notes.clear()
