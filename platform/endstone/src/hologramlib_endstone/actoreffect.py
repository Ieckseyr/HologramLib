"""Actor 表现域 —— LeviLamina 版 ActorEffectManager 的 endstone 移植（协议部分）。"""

from __future__ import annotations

import threading

from .native import meta_float, meta_int

ACTOR_EVENT_HURT = 2
ACTOR_EVENT_DEATH = 3
ACTOR_EVENT_INSTANT_DEATH = 61

ACTOR_DATA_POSE_INDEX = 79
ACTOR_DATA_FREEZING = 120


def _read_unsigned_varint64(data: bytes, offset: int = 0) -> tuple[int, int]:
    value = 0
    shift = 0
    i = offset
    while i < len(data) and shift < 70:
        byte = data[i]
        i += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            break
        shift += 7
    return value, i


class BlockFilter:
    """按 ActorEvent 事件号拦出站包；只对 ActorEvent 包做一次头部解析。"""

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._blocked_all: set[int] = set()
        self._blocked_by_player: dict[str, set[int]] = {}
        self._blocked_count = 0
        self._registered = False
        self._blocked_entities: dict[int, set[int]] = {}

    @property
    def registered(self) -> bool:
        return self._registered

    def mark_registered(self) -> None:
        self._registered = True

    def block(self, event_id: int, player_name: str = "") -> None:
        with self._lock:
            if player_name:
                self._blocked_by_player.setdefault(player_name, set()).add(int(event_id))
            else:
                self._blocked_all.add(int(event_id))

    def unblock(self, event_id: int, player_name: str = "") -> None:
        with self._lock:
            if player_name:
                bucket = self._blocked_by_player.get(player_name)
                if bucket is not None:
                    bucket.discard(int(event_id))
                    if not bucket:
                        self._blocked_by_player.pop(player_name, None)
            else:
                self._blocked_all.discard(int(event_id))

    def clear(self) -> int:
        with self._lock:
            count = len(self._blocked_all) + sum(len(v) for v in self._blocked_by_player.values())
            self._blocked_all.clear()
            self._blocked_by_player.clear()
            return count

    def stats(self) -> str:
        with self._lock:
            per_player = ";".join(f"{k}={len(v)}" for k, v in sorted(self._blocked_by_player.items()))
            return (
                f"blocked_all={sorted(self._blocked_all)} per_player=[{per_player}] "
                f"dropped={self._blocked_count} registered={self._registered}"
            )

    def block_entity(self, runtime_id: int, event_id: int) -> None:
        with self._lock:
            self._blocked_entities.setdefault(int(runtime_id), set()).add(int(event_id))

    def unblock_entity(self, runtime_id: int, event_id: int) -> None:
        with self._lock:
            bucket = self._blocked_entities.get(int(runtime_id))
            if bucket is not None:
                bucket.discard(int(event_id))
                if not bucket:
                    self._blocked_entities.pop(int(runtime_id), None)

    def should_drop(self, payload: bytes, player_name: str) -> bool:
        """payload 是 ActorEvent 的 body：varint64 runtimeId + byte eventId + varint data。"""
        if not self._blocked_all and not self._blocked_by_player and not self._blocked_entities:
            return False
        if len(payload) < 2:
            return False
        try:
            _, offset = _read_unsigned_varint64(payload, 0)
            if offset >= len(payload):
                return False
            event_id = payload[offset]
        except Exception:
            return False
        runtime_id, _ = _read_unsigned_varint64(payload, 0)
        with self._lock:
            if event_id in self._blocked_all:
                self._blocked_count += 1
                return True
            entity_bucket = self._blocked_entities.get(runtime_id)
            if entity_bucket and event_id in entity_bucket:
                self._blocked_count += 1
                return True
            bucket = self._blocked_by_player.get(player_name)
            if bucket and event_id in bucket:
                self._blocked_count += 1
                return True
        return False


class ActorEffectApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self.blocks = BlockFilter(plugin)

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _send_to(self, player, packet_id: int, body: bytes) -> bool:
        if not body:
            return False
        try:
            player.send_packet(packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"actor effect 发包失败 ({player.name}): {exc}")
            return False

    def _broadcast(self, packet_id: int, body: bytes) -> bool:
        if not body:
            return False
        sent = False
        for player in list(self._plugin.server.online_players):
            sent = self._send_to(player, packet_id, body) or sent
        return sent

    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    # ── 事件 ──────────────────────────────────────────────────────────────
    def play_event(self, runtime_id: int, event_id: int, data: int = 0) -> bool:
        if int(runtime_id) <= 0:
            return False
        body = self._plugin.core.encode_actor_event(int(runtime_id), int(event_id), int(data))
        return self._broadcast(self._plugin.core.actor_event_packet_id, body)

    def play_event_for(self, player_name: str, runtime_id: int, event_id: int, data: int = 0) -> bool:
        player = self._player(player_name)
        if player is None or int(runtime_id) <= 0:
            return False
        body = self._plugin.core.encode_actor_event(int(runtime_id), int(event_id), int(data))
        return self._send_to(player, self._plugin.core.actor_event_packet_id, body)

    def play_hurt(self, runtime_id: int) -> bool:
        return self.play_event(runtime_id, ACTOR_EVENT_HURT, 0)

    def play_death(self, runtime_id: int) -> bool:
        return self.play_event(runtime_id, ACTOR_EVENT_DEATH, 0)

    def play_instant_death(self, runtime_id: int) -> bool:
        return self.play_event(runtime_id, ACTOR_EVENT_INSTANT_DEATH, 0)

    # ── 动画 ──────────────────────────────────────────────────────────────
    def play_animation(
        self, runtime_id: int, animation: str, stop_expression: str = "", blend_out_time: float = 0.0
    ) -> bool:
        return self.play_animation_full(runtime_id, animation, "none", stop_expression, blend_out_time, "", 0)

    def play_animation_full(
        self,
        runtime_id: int,
        animation: str,
        next_state: str = "none",
        stop_expression: str = "",
        blend_out_time: float = 0.0,
        controller: str = "",
        molang_version: int = 0,
    ) -> bool:
        if int(runtime_id) <= 0 or not animation:
            return False
        body = self._plugin.core.encode_animate_entity(
            str(animation),
            str(next_state),
            str(stop_expression),
            int(molang_version),
            str(controller),
            float(blend_out_time),
            [int(runtime_id)],
        )
        return self._broadcast(self._plugin.core.animate_entity_packet_id, body)

    # ── 元数据 ────────────────────────────────────────────────────────────
    def _send_meta(self, runtime_id: int, entries) -> bool:
        if int(runtime_id) <= 0:
            return False
        body = self._plugin.core.encode_set_actor_data(int(runtime_id), self._plugin.tick_count, entries)
        return self._broadcast(self._plugin.core.set_actor_data_packet_id, body)

    def set_pose(self, runtime_id: int, pose_index: int) -> bool:
        if int(pose_index) < 0:
            return False
        return self._send_meta(runtime_id, [meta_int(ACTOR_DATA_POSE_INDEX, int(pose_index))])

    def freeze(self, runtime_id: int, strength: float) -> bool:
        value = min(1.0, max(0.0, float(strength)))
        return self._send_meta(runtime_id, [meta_float(ACTOR_DATA_FREEZING, value)])

    def unfreeze(self, runtime_id: int) -> bool:
        return self._send_meta(runtime_id, [meta_float(ACTOR_DATA_FREEZING, 0.0)])

    def play_totem(self, runtime_id: int, hold_ticks: int = 40) -> bool:
        """图腾激活动画（ActorEvent 35）：不换副手（endstone 无手持槽 API），只播事件。"""
        _ = hold_ticks
        return self.play_event(int(runtime_id), 35, 0)

    def creeper_display(self, runtime_id: int) -> bool:
        """苦力怕膨胀彩蛋：AnimateEntity + LL 同款 molang 表达式。"""
        expression = (
            "v.swelling_sin=math.sin(q.life_time*720);"
            "v.swelling_scale1=1.0-0.1*math.max(v.swelling_sin,0)+0.15*math.max(-v.swelling_sin,0);"
            "v.swelling_scale2=1.0+0.1*math.max(v.swelling_sin,0)-0.4*math.max(-v.swelling_sin,0);"
        )
        body = self._plugin.core.encode_animate_entity(
            "animation.creeper.swelling", "none", expression, 0, "", 0.0, [int(runtime_id)]
        )
        return self._broadcast(self._plugin.core.animate_entity_packet_id, body)

    def easter_egg_creeper_display(self, runtime_id: int) -> bool:
        return self.creeper_display(runtime_id)

    def poll_interactions(self) -> list[str]:
        return []

    def clear_interactions(self) -> bool:
        return True

    # ── 查询 ──────────────────────────────────────────────────────────────
    def runtime_id_of_player(self, player_name: str) -> int:
        player = self._player(player_name)
        return int(player.runtime_id) if player is not None else -1

    def runtime_id_of_entity(self, lib_id: int) -> int:
        """库内实体 id -> runtimeId（走自定义实体注册表）。"""
        try:
            return int(self._plugin.entities.runtime_id(int(lib_id)))
        except Exception:
            return -1

    # ── 出站事件过滤 ──────────────────────────────────────────────────────
    def play_event_for_entity(self, runtime_id: int, event_id: int, data: int = 0) -> bool:
        return self.play_event(int(runtime_id), int(event_id), int(data))

    def block_event_for_entity(self, runtime_id: int, event_id: int) -> bool:
        """按实体号过滤：登记成"该 runtimeId 的事件丢弃"，由 plugin 的出站过滤执行。"""
        self.blocks.block_entity(int(runtime_id), int(event_id))
        self._plugin.ensure_packet_filter()
        return True

    def unblock_event_for_entity(self, runtime_id: int, event_id: int) -> bool:
        self.blocks.unblock_entity(int(runtime_id), int(event_id))
        return True

    def block_event(self, event_id: int) -> bool:
        self.blocks.block(event_id)
        self._plugin.ensure_packet_filter()
        return True

    def unblock_event(self, event_id: int) -> bool:
        self.blocks.unblock(event_id)
        return True

    def block_event_for(self, player_name: str, event_id: int) -> bool:
        self.blocks.block(event_id, str(player_name))
        self._plugin.ensure_packet_filter()
        return True

    def unblock_event_for(self, player_name: str, event_id: int) -> bool:
        self.blocks.unblock(event_id, str(player_name))
        return True

    def block_clear(self) -> bool:
        self.blocks.clear()
        return True

    def block_stats(self) -> str:
        return self.blocks.stats()
