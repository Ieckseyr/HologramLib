"""侧边栏（Sidebar / 计分板）域 —— LeviLamina 版 SidebarManager 的 endstone 移植。"""

from __future__ import annotations

import threading

from .native import ScoreEntry

SLOT = "sidebar"
CRITERIA = "dummy"

# SetScorePacket::PacketType
SCORE_CHANGE = 0
SCORE_REMOVE = 1

# SetScorePacket::IdentityType
IDENTITY_FAKE_PLAYER = 3

_MASK64 = (1 << 64) - 1
_FNV_OFFSET = 1469598103934665603
_FNV_OTHER = 1099511628211
_FNV_PRIME = 1099511628211


def _fnv1a(text: str, seed: int) -> int:
    h = seed & _MASK64
    for byte in text.encode("utf-8"):
        h ^= byte
        h = (h * _FNV_PRIME) & _MASK64
    return h


def objective_of(player_name: str) -> str:
    return f"hlib_sb_{_fnv1a(player_name, _FNV_OFFSET) & 0xFFFFFFFF:08x}"


def id_base_of(player_name: str) -> int:
    return 100_000 + (_fnv1a(player_name, _FNV_OTHER) % 4096) * 4096


class SidebarApi:
    """侧边栏对外 API，方法名对齐 LSE 的 sidebar*。"""

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._bars: dict[str, tuple[str, list[str]]] = {}  # player -> (title, lines)

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _send_raw(self, player, packet_id: int, body: bytes) -> bool:
        if not body:
            return False
        try:
            player.send_packet(packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"sidebar 发包失败 ({player.name}): {exc}")
            return False

    def _send_display(self, player, objective: str, title: str) -> bool:
        body = self._plugin.core.encode_set_display_objective(SLOT, objective, title, CRITERIA, 0)
        return self._send_raw(player, self._plugin.core.set_display_objective_packet_id, body)

    def _entries(self, objective: str, lines: list[str], id_base: int):
        entries = []
        for i, text in enumerate(lines):
            entries.append(
                ScoreEntry(
                    id_base + i,
                    i + 1,
                    IDENTITY_FAKE_PLAYER,
                    0,
                    objective.encode("utf-8"),
                    text.encode("utf-8"),
                )
            )
        return entries

    def _send_scores(self, player, objective: str, lines: list[str], id_base: int) -> bool:
        entries = self._entries(objective, lines, id_base)
        body = self._plugin.core.encode_set_score(SCORE_CHANGE, entries)
        return self._send_raw(player, self._plugin.core.set_score_packet_id, body)

    def _send_remove_scores(self, player, objective: str, count: int, id_base: int) -> bool:
        if count <= 0:
            return True
        entries = [
            ScoreEntry(id_base + i, 0, IDENTITY_FAKE_PLAYER, 0, objective.encode("utf-8"), b"")
            for i in range(count)
        ]
        body = self._plugin.core.encode_set_score(SCORE_REMOVE, entries)
        return self._send_raw(player, self._plugin.core.set_score_packet_id, body)

    def _send_remove_objective(self, player, objective: str) -> bool:
        body = self._plugin.core.encode_remove_objective(objective)
        return self._send_raw(player, self._plugin.core.remove_objective_packet_id, body)

    # ── 对外 ──────────────────────────────────────────────────────────────
    def set(self, player_name: str, title: str, lines: list[str]) -> bool:
        lines = [str(line) for line in lines]
        if not lines:
            return self.remove(player_name)

        player = self._player(player_name)
        if player is None:
            return False

        objective = objective_of(player_name)
        id_base = id_base_of(player_name)

        with self._lock:
            previous = self._bars.get(player_name)
            self._bars[player_name] = (str(title), lines)

        if previous is not None:
            self._send_remove_scores(player, objective, len(previous[1]), id_base)
            if previous[0] != title:
                self._send_remove_objective(player, objective)
                if not self._send_display(player, objective, title):
                    return False
        else:
            if not self._send_display(player, objective, title):
                return False
        return self._send_scores(player, objective, lines, id_base)

    def set_title(self, player_name: str, title: str) -> bool:
        with self._lock:
            bar = self._bars.get(player_name)
            if bar is None:
                return False
            lines = list(bar[1])
        return self.set(player_name, title, lines)

    def set_line(self, player_name: str, index: int, text: str) -> bool:
        with self._lock:
            bar = self._bars.get(player_name)
            if bar is None:
                return False
            lines = list(bar[1])
            if not (0 <= index < len(lines)):
                return False
            lines[index] = str(text)
            title = bar[0]
        return self.set(player_name, title, lines)

    def remove(self, player_name: str) -> bool:
        with self._lock:
            bar = self._bars.pop(player_name, None)
        if bar is None:
            return False
        player = self._player(player_name)
        if player is None:
            return True
        objective = objective_of(player_name)
        id_base = id_base_of(player_name)
        self._send_remove_scores(player, objective, len(bar[1]), id_base)
        self._send_remove_objective(player, objective)
        return True

    def remove_all(self) -> int:
        with self._lock:
            names = list(self._bars.keys())
        for name in names:
            self.remove(name)
        return len(names)

    def is_shown(self, player_name: str) -> bool:
        with self._lock:
            return player_name in self._bars

    def line_count(self, player_name: str) -> int:
        with self._lock:
            bar = self._bars.get(player_name)
            return len(bar[1]) if bar is not None else 0

    def forget(self, player_name: str) -> None:
        with self._lock:
            self._bars.pop(player_name, None)

    def __len__(self) -> int:
        with self._lock:
            return len(self._bars)
