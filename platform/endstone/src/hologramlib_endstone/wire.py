"""线路诊断（Wire）—— LeviLamina 版 wire* 导出的 endstone 移植。"""

from __future__ import annotations

import threading

MAX_RECORDS = 512


class WireApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._active = False
        self._tag = ""
        self._records: list[str] = []
        self._marks: list[str] = []
        self._total = 0
        self._registered = False

    # ── 由 PacketSendEvent 调用 ────────────────────────────────────────────
    def on_packet(self, packet_id: int, payload_len: int, player_name: str) -> None:
        with self._lock:
            if not self._active:
                return
            self._total += 1
            if len(self._records) < MAX_RECORDS:
                self._records.append(f"{len(self._records):04d} id={packet_id} len={payload_len} -> {player_name}")

    def mark_registered(self) -> None:
        self._registered = True

    @property
    def active(self) -> bool:
        return self._active

    # ── 对外 ──────────────────────────────────────────────────────────────
    def start(self, tag: str = "default") -> bool:
        with self._lock:
            self._active = True
            self._tag = str(tag)
            self._records.clear()
            self._marks.clear()
        self._plugin.ensure_wire_tap()
        return True

    def stop(self) -> bool:
        with self._lock:
            self._active = False
        return True

    def is_active(self) -> bool:
        with self._lock:
            return self._active

    def mark(self, tag: str) -> bool:
        with self._lock:
            if not self._active:
                return False
            self._marks.append(f"-- {tag} @{len(self._records)} --")
        return True

    def count(self) -> int:
        with self._lock:
            return len(self._records)

    def clear(self) -> bool:
        with self._lock:
            self._records.clear()
            self._marks.clear()
            self._total = 0
        return True

    def dump(self) -> str:
        with self._lock:
            lines = [f"trace tag={self._tag} active={self._active} records={len(self._records)}"]
            lines.extend(self._marks)
            lines.extend(self._records[-64:])
            return "\n".join(lines)

    def stats(self) -> str:
        with self._lock:
            return (
                f"wire stats: active={self._active} tag={self._tag} "
                f"captured={self._total} kept={len(self._records)} registered={self._registered}"
            )
