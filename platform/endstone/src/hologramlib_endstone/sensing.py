"""感知/输入（Sensing）域。

- **空手挥动计数**（`count` / `total` / `reset`）：监听入站 `AnimatePacket`（包 id 44），
  `Action = Swing` 就记一次。空手挥、挥剑、打空气、点方块、打实体都算，**不区分目标** ——
  协议里只有 `Swing` 一个动作，客户端不会告诉服务端"对着什么挥"。
- **输入设备**（`device_*` / `is_touch`）：endstone 的 Python 面拿不到 `PlayerAuthInput`，
  仍是占位实现，返回值是空 / False。

!!! 性能

    计数第一次被调用时才注册 `PacketReceiveEvent`（每个入站包进一次 Python）；
    不用这套接口就不会有这份开销。注册之后一直到进程结束，`reset()` 只清计数不摘监听。
"""

from __future__ import annotations

import time
from dataclasses import dataclass, field

from endstone.event import PacketReceiveEvent, event_handler

# AnimatePacket（id 44）payload 的第一个字节是 Action（uint8），Swing = 1
_ANIMATE_PACKET_ID = 44
_ACTION_SWING = 1

_MAX_PLAYERS = 4096


def is_swing(payload: bytes) -> bool:
    """入站 AnimatePacket 是不是挥动。只看第一个字节，不依赖后续字段布局。"""
    return len(payload) >= 1 and payload[0] == _ACTION_SWING


@dataclass
class _SwingStore:
    counts: dict[str, int] = field(default_factory=dict)
    total: int = 0

    def note(self, player_name: str) -> None:
        if len(self.counts) >= _MAX_PLAYERS and player_name not in self.counts:
            self.counts.clear()
        self.counts[player_name] = self.counts.get(player_name, 0) + 1
        self.total += 1

    def reset(self) -> None:
        self.counts.clear()
        self.total = 0


class _SwingListener:
    """惰性注册的包监听。异常一律吞掉，不能把异常抛进 endstone 的事件分发。"""

    def __init__(self, api: "SensingApi") -> None:
        self._api = api

    @event_handler
    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        try:
            if event.packet_id != _ANIMATE_PACKET_ID:
                return
            if not is_swing(bytes(event.payload)):
                return
            player = event.player
            if player is not None:
                self._api._note(player.name)
        except Exception:  # noqa: BLE001 - 库的旁路功能，绝不影响收包
            pass


class SensingApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._store = _SwingStore()
        self._listener: _SwingListener | None = None
        self._active = False
        self._notes: list[str] = []

    # ── 输入设备（占位）──────────────────────────────────────────────────
    def _note_device(self, what: str) -> None:
        if what not in self._notes:
            self._notes.append(what)
            self._plugin.logger.debug(f"sensing: {what} 需要客户端 AuthInput 解码，端侧无入口")

    def device_code(self, player_name: str) -> str:
        self._note_device(f"deviceCode({player_name})")
        return ""

    def device_of(self, player_name: str) -> str:
        self._note_device(f"deviceOf({player_name})")
        return ""

    def is_touch(self, player_name: str) -> bool:
        self._note_device(f"isTouch({player_name})")
        return False

    # ── 空手挥动计数 ─────────────────────────────────────────────────────
    def _ensure_listener(self) -> None:
        if self._listener is not None:
            return
        self._listener = _SwingListener(self)
        self._active = True
        self._plugin.register_events(self._listener)
        self._plugin.logger.debug("sensing: 挥动计数监听已启用（PacketReceiveEvent id=44）")

    def _note(self, player_name: str) -> None:
        if not self._active:
            return
        self._store.note(player_name)

    def count(self, player_name: str) -> int:
        """该玩家自上次 reset 以来的挥动次数；没挥过或不在线返回 0。"""
        self._ensure_listener()
        return int(self._store.counts.get(player_name, 0))

    def total(self) -> int:
        """全服累计挥动次数（含已退出玩家，直到 reset）。"""
        self._ensure_listener()
        return int(self._store.total)

    def reset(self) -> None:
        """清空全部玩家计数与总数。"""
        self._store.reset()

    def clear(self) -> None:
        """插件禁用时清状态（监听已由 endstone 随插件注销）。"""
        self._active = False
        self._store.reset()
        self._notes.clear()
