"""Boss 条（BossBar）域 —— LeviLamina 版 BossBarManager 的 endstone 移植。"""

from __future__ import annotations

import threading

# BossEventPacket::EventType
ADD = 0
PLAYER_ADDED = 1
REMOVE = 2
PLAYER_REMOVED = 3
UPDATE_PERCENT = 4
UPDATE_NAME = 5
UPDATE_STYLE = 7

# 虚拟号段（与 LL 版一致）
_ANCHOR_FIRST = 0x71000000
_ANCHOR_LAST = 0x72000000


class BossBar:
    def __init__(self, bar_id: int, target_id: int, player_name: str, title: str, percent: float, color: int, overlay: int):
        self.id = bar_id
        self.target_id = target_id
        self.player_name = player_name
        self.title = title
        self.percent = percent
        self.color = color
        self.overlay = overlay


class BossBarApi:
    """Boss 条对外 API，方法名对齐 LSE 的 bossBar*。"""

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._bars: dict[int, BossBar] = {}
        self._next_id = 1
        self._next_anchor = _ANCHOR_FIRST

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _alloc_anchor(self) -> int:
        with self._lock:
            anchor = self._next_anchor
            self._next_anchor += 1
            if self._next_anchor >= _ANCHOR_LAST:
                self._next_anchor = _ANCHOR_FIRST
            return anchor

    def _send(self, bar: BossBar, event_type: int) -> bool:
        player = self._player(bar.player_name)
        if player is None:
            return False
        try:
            body = self._plugin.core.encode_boss_event(
                bar.target_id, 0, event_type, bar.title, bar.title, bar.percent, bar.color, bar.overlay
            )
            player.send_packet(self._plugin.core.boss_event_packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"bossbar 发包失败 ({bar.player_name}): {exc}")
            return False

    def get(self, bar_id: int) -> BossBar | None:
        with self._lock:
            return self._bars.get(bar_id)

    # ── 创建 ──────────────────────────────────────────────────────────────
    def add(self, player_name: str, title: str, health_percent: float = 1.0, color: int = 0, overlay: int = 0) -> int:
        return self.add_on(player_name, 0, title, health_percent, color, overlay)

    def add_on(
        self, player_name: str, target_id: int, title: str, percent: float = 1.0, color: int = 0, overlay: int = 0
    ) -> int:
        if self._player(player_name) is None:
            return -1
        target = int(target_id) if target_id else self._alloc_anchor()
        with self._lock:
            bar_id = self._next_id
            self._next_id += 1
            bar = BossBar(bar_id, target, player_name, str(title), float(percent), int(color), int(overlay))
            self._bars[bar_id] = bar
        self._send(bar, ADD)
        self._send(bar, PLAYER_ADDED)
        return bar_id

    # ── 修改 ──────────────────────────────────────────────────────────────
    def set_title(self, bar_id: int, title: str) -> bool:
        bar = self.get(bar_id)
        if bar is None:
            return False
        bar.title = str(title)
        return self._send(bar, UPDATE_NAME)

    def set_health(self, bar_id: int, percent: float) -> bool:
        bar = self.get(bar_id)
        if bar is None:
            return False
        bar.percent = max(0.0, min(1.0, float(percent)))
        return self._send(bar, UPDATE_PERCENT)

    def set_style(self, bar_id: int, color: int, overlay: int) -> bool:
        bar = self.get(bar_id)
        if bar is None:
            return False
        bar.color = int(color)
        bar.overlay = int(overlay)
        return self._send(bar, UPDATE_STYLE)

    # ── 删除 / 查询 ───────────────────────────────────────────────────────
    def remove(self, bar_id: int) -> bool:
        with self._lock:
            bar = self._bars.pop(bar_id, None)
        if bar is None:
            return False
        self._send(bar, PLAYER_REMOVED)
        self._send(bar, REMOVE)
        return True

    def remove_all(self, player_name: str) -> int:
        with self._lock:
            ids = [b.id for b in self._bars.values() if b.player_name == player_name]
        for bar_id in ids:
            self.remove(bar_id)
        return len(ids)

    def count(self, player_name: str = "") -> int:
        with self._lock:
            if not player_name:
                return len(self._bars)
            return sum(1 for b in self._bars.values() if b.player_name == player_name)

    def get_info(self, bar_id: int) -> str:
        bar = self.get(bar_id)
        if bar is None:
            return ""
        return (
            f"#{bar.id} player={bar.player_name} target=0x{bar.target_id:X} "
            f"title={bar.title!r} health={bar.percent:.2f} color={bar.color} overlay={bar.overlay}"
        )

    def probe(self) -> list[int]:
        with self._lock:
            return sorted(self._bars.keys())

    def destroy_all(self) -> int:
        with self._lock:
            ids = list(self._bars.keys())
        for bar_id in ids:
            self.remove(bar_id)
        return len(ids)

    def forget(self, player_name: str) -> None:
        """玩家退服：Boss 条是客户端侧的，直接丢掉记录即可（不发包）。"""
        with self._lock:
            for bar_id in [b.id for b in self._bars.values() if b.player_name == player_name]:
                self._bars.pop(bar_id, None)

    def __len__(self) -> int:
        with self._lock:
            return len(self._bars)
