"""锚点（Anchor）域 —— 用一个看不见的物品实体占住 uniqueId。"""

from __future__ import annotations

import threading


class AnchorApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._anchors: dict[int, int] = {}  # uniqueId -> itemActor id

    def acquire(self, player_name: str) -> int:
        player = None
        try:
            player = self._plugin.server.get_player(player_name)
        except Exception:
            player = None
        if player is None:
            return -1
        from .holograms import _dim_id

        actor_id = self._plugin.itemactor.spawn(
            "minecraft:structure_void", 0, 1, 0.0, -120.0, 0.0, _dim_id(player)
        )
        if actor_id < 0:
            return -1
        unique_id = self._plugin.itemactor._next_unique - 1
        with self._lock:
            self._anchors[unique_id] = actor_id
        return unique_id

    def release(self, unique_id: int) -> bool:
        with self._lock:
            actor_id = self._anchors.pop(int(unique_id), None)
        if actor_id is None:
            return False
        return self._plugin.itemactor.destroy(actor_id)
