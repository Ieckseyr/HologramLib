"""物品实体（ItemActor）域 —— LeviLamina 版 ItemActorManager 的 endstone 移植。"""

from __future__ import annotations

import threading

from . import items
from .holograms import _dim_id  # 维度判定复用悬浮字那套


class _ItemActor:
    __slots__ = ("id", "unique_id", "runtime_id", "item_name", "aux", "count", "x", "y", "z", "dimension")

    def __init__(self, actor_id: int, unique_id: int, runtime_id: int, item_name: str, aux: int, count: int, x, y, z, dim: int):
        self.id = actor_id
        self.unique_id = unique_id
        self.runtime_id = runtime_id
        self.item_name = item_name
        self.aux = aux
        self.count = count
        self.x, self.y, self.z = float(x), float(y), float(z)
        self.dimension = int(dim)


class ItemActorApi:
    # 虚拟号段：与 LL 版 core/VirtualActorIds.h 的 itemActor 段一致（0x70000000 起）
    FIRST_RUNTIME_ID = 0x70000000
    FIRST_UNIQUE_ID = 0x100000

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._actors: dict[int, _ItemActor] = {}
        self._next_id = 1
        self._next_runtime = self.FIRST_RUNTIME_ID
        self._next_unique = self.FIRST_UNIQUE_ID

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _viewers(self, dimension: int) -> list:
        return [p for p in self._plugin.server.online_players if _dim_id(p) == int(dimension)]

    def _send(self, players, packet_id: int, body: bytes) -> bool:
        if not body:
            return False
        sent = False
        for player in players:
            try:
                player.send_packet(packet_id, body)
                sent = True
            except Exception as exc:
                self._plugin.logger.debug(f"itemactor 发包失败 ({player.name}): {exc}")
        return sent

    def get(self, actor_id: int) -> _ItemActor | None:
        with self._lock:
            return self._actors.get(actor_id)

    # ── 对外 ──────────────────────────────────────────────────────────────
    def spawn(
        self,
        item_id: str,
        aux: int,
        count: int,
        x: float,
        y: float,
        z: float,
        dim: int = 0,
        vx: float = 0.0,
        vy: float = 0.0,
        vz: float = 0.0,
    ) -> int:
        numeric = items.numeric_id(item_id)
        if numeric < 0:
            self._plugin.logger.debug(f"itemActorSpawn: 未知物品 {item_id}")
            return -1
        stack = max(1, int(count))
        viewers = self._viewers(dim)
        if not viewers:
            return -1

        with self._lock:
            actor_id = self._next_id
            self._next_id += 1
            runtime_id = self._next_runtime
            self._next_runtime += 1
            unique_id = self._next_unique
            self._next_unique += 1
            actor = _ItemActor(actor_id, unique_id, runtime_id, item_id, int(aux), stack, x, y, z, dim)
            self._actors[actor_id] = actor

        body = self._plugin.core.encode_add_item_actor(
            runtime_id, unique_id, numeric, stack, int(aux), float(x), float(y), float(z),
            float(vx), float(vy), float(vz),
        )
        if not self._send(viewers, self._plugin.core.add_item_actor_packet_id, body):
            with self._lock:
                self._actors.pop(actor_id, None)
            return -1
        return actor_id

    def destroy(self, actor_id: int) -> bool:
        with self._lock:
            actor = self._actors.pop(actor_id, None)
        if actor is None:
            return False
        body = self._plugin.core.encode_remove_actor(actor.unique_id)
        self._send(self._viewers(actor.dimension), self._plugin.core.remove_actor_packet_id, body)
        return True

    def destroy_all(self) -> int:
        with self._lock:
            ids = list(self._actors.keys())
        for actor_id in ids:
            self.destroy(actor_id)
        return len(ids)

    def exists(self, actor_id: int) -> bool:
        with self._lock:
            return actor_id in self._actors

    def get_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._actors.keys())

    def move_to(self, actor_id: int, x: float, y: float, z: float) -> bool:
        actor = self.get(actor_id)
        if actor is None:
            return False
        actor.x, actor.y, actor.z = float(x), float(y), float(z)
        body = self._plugin.core.encode_move_actor_absolute(actor.runtime_id, actor.x, actor.y, actor.z, 1, 0, 0, 0)
        return self._send(self._viewers(actor.dimension), self._plugin.core.move_actor_absolute_packet_id, body)

    def set_motion(self, actor_id: int, vx: float, vy: float, vz: float) -> bool:
        actor = self.get(actor_id)
        if actor is None:
            return False
        body = self._plugin.core.encode_set_actor_motion(actor.runtime_id, float(vx), float(vy), float(vz), self._plugin.tick_count)
        return self._send(self._viewers(actor.dimension), self._plugin.core.set_actor_motion_packet_id, body)

    def runtime_id(self, actor_id: int) -> int:
        actor = self.get(actor_id)
        return actor.runtime_id if actor is not None else 0

    def take(self, actor_id: int, player_name: str) -> bool:
        with self._lock:
            actor = self._actors.pop(actor_id, None)
        if actor is None:
            return False
        player = None
        try:
            player = self._plugin.server.get_player(player_name)
        except Exception:
            player = None
        if player is None:
            return False
        body = self._plugin.core.encode_take_item_actor(actor.runtime_id, int(player.runtime_id))
        self._send([player], self._plugin.core.take_item_actor_packet_id, body)
        body = self._plugin.core.encode_remove_actor(actor.unique_id)
        self._send(self._viewers(actor.dimension), self._plugin.core.remove_actor_packet_id, body)
        return True

    def __len__(self) -> int:
        with self._lock:
            return len(self._actors)
