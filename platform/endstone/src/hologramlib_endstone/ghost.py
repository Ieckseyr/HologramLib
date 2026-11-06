"""幽灵交互（Ghost）域 —— 占位实现。"""

from __future__ import annotations


class GhostApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._cleared = 0

    def poll_interactions(self) -> list[str]:
        return []

    def clear_interactions(self) -> bool:
        self._cleared += 1
        return True

    # 兼容短名
    poll = poll_interactions
    clear = clear_interactions
