"""玩家表现域 —— LeviLamina 版 ActorEffectManager 里 player* 那几个导出的 endstone 移植。"""

from __future__ import annotations

import threading

# MobEffectPacket::Event
EFFECT_ADD = 0
EFFECT_UPDATE = 1
EFFECT_REMOVE = 2

# ActorEvent::DrinkPotion
ACTOR_EVENT_DRINK_POTION = 29


class PlayerEffectApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _send(self, player, packet_id: int, body: bytes) -> bool:
        if not body:
            return False
        try:
            player.send_packet(packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"player effect 发包失败 ({player.name}): {exc}")
            return False

    def _broadcast(self, packet_id: int, body: bytes) -> bool:
        if not body:
            return False
        sent = False
        for player in list(self._plugin.server.online_players):
            sent = self._send(player, packet_id, body) or sent
        return sent

    # ── 对外 ──────────────────────────────────────────────────────────────
    def show_effect(self, player_name: str, effect_id: int, duration_ticks: int = 200, amplifier: int = 0) -> bool:
        player = self._player(player_name)
        if player is None or int(effect_id) < 0:
            return False
        body = self._plugin.core.encode_mob_effect(
            int(player.runtime_id),
            EFFECT_ADD,
            int(effect_id),
            max(0, int(amplifier)),
            True,
            int(duration_ticks) if int(duration_ticks) > 0 else 200,
            self._plugin.tick_count,
            False,
        )
        return self._send(player, self._plugin.core.mob_effect_packet_id, body)

    def play_celebrate(self, player_name: str, seconds: float = 3.0) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        stop_at = float(seconds) if float(seconds) > 0 else 3.0
        body = self._plugin.core.encode_animate_entity(
            "animation.humanoid.celebrating",
            "none",
            f"q.anim_time > {stop_at:.2f}",
            13,
            "",
            0.0,
            [int(player.runtime_id)],
        )
        return self._broadcast(self._plugin.core.animate_entity_packet_id, body)

    def show_village_hero(self, player_name: str, duration_ticks: int = 0) -> bool:
        _ = duration_ticks
        return self.play_celebrate(player_name, 3.0)

    def play_ominous_drink(self, player_name: str, hold_ticks: int = 40) -> bool:
        _ = hold_ticks
        player = self._player(player_name)
        if player is None:
            return False
        body = self._plugin.core.encode_actor_event(int(player.runtime_id), ACTOR_EVENT_DRINK_POTION, 0)
        return self._broadcast(self._plugin.core.actor_event_packet_id, body)

    def show_bad_omen(self, player_name: str, duration_ticks: int = 0) -> bool:
        _ = duration_ticks
        return self.play_ominous_drink(player_name, 40)
