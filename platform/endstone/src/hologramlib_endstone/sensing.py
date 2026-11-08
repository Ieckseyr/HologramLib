"""感知/输入（Sensing）域。

两块内容：

- **输入设备**（`device_*` / `is_touch`）：endstone 的 Python 面拿不到 `PlayerAuthInput`，
  仍是占位实现，返回值是空/False。
- **攻击 / 挥动**（`swing*`）：真的能跑。数据源是入站 `AnimatePacket`（包 id 44，`Action = Swing`），
  目标是靠 endstone 自己的 `PlayerInteractEvent` 补的 —— 它会把「左键点方块」和「左键点空气」
  分成 `LEFT_CLICK_BLOCK` / `LEFT_CLICK_AIR` 两种动作。

!!! 性能

    `swing*` 第一次被调用时才会注册 `PacketReceiveEvent` 监听（每个入站包进一次 Python）。
    不用这套接口的插件不会付这份开销。注册之后就到进程结束为止，`reset()` 只清数据不摘监听。
"""

from __future__ import annotations

import time
from dataclasses import dataclass, field

from endstone.event import PacketReceiveEvent, PlayerInteractEvent, event_handler

# AnimatePacket（id 44）的 Action 枚举里 Swing = 1；payload 第一个字节就是 Action（uint8）
_ANIMATE_PACKET_ID = 44
_ACTION_SWING = 1

# 客户端来源（ActorSwingSource）数值 → 文本；None 表示包里没带这个字段
_SOURCE_NAMES = {
    0: "none",
    1: "build",
    2: "mine",
    3: "interact",
    4: "attack",
    5: "useItem",
    6: "throwItem",
    7: "dropItem",
    8: "event",
}

TARGET_AIR = "air"
TARGET_BLOCK = "block"
TARGET_ENTITY = "entity"

# 挥手与点击事件在时间上可能差几毫秒（两个包），窗口给 0.5 秒足够
_CORRELATE_WINDOW = 0.5

_MAX_QUEUE = 1024
_MAX_LAST = 4096


def _read_uvarint(buf: bytes, offset: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while True:
        if offset >= len(buf):
            raise ValueError("truncated varint")
        byte = buf[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return value, offset
        shift += 7
        if shift > 63:
            raise ValueError("varint too long")


def _decode_swing_source(buf: bytes, offset: int) -> int | None:
    """Swing Source 是 optional 枚举，线上长度随客户端版本变；按剩余字节数反推。

    已知两种形态：只有枚举值 1 字节；或 bool 存在位 + 枚举值 2 字节。
    对不上就当「没带」——只影响 source 字段，不影响挥动本身。
    """
    rest = len(buf) - offset
    if rest == 1:
        value = buf[offset]
        return value if value in _SOURCE_NAMES else None
    if rest == 2:
        present, value = buf[offset], buf[offset + 1]
        if present == 0:
            return None
        if present == 1 and value in _SOURCE_NAMES:
            return value
    return None


def decode_animate(payload: bytes) -> tuple[int, int | None] | None:
    """解析入站 AnimatePacket 的 body；不是 Swing 就返回 None。

    布局（protocol-docs r26_u5/r26_u6）：Action(u8) · Target Actor Runtime ID(uvarint64)
    · Data(float) · [Swing Source(optional)]。
    """
    if len(payload) < 6 or payload[0] != _ACTION_SWING:
        return None
    try:
        runtime_id, offset = _read_uvarint(payload, 1)
    except ValueError:
        return None
    offset += 4  # Data(float)，后面才是可选的 Swing Source
    if offset > len(payload):
        return None
    return runtime_id, _decode_swing_source(payload, offset)


@dataclass
class _SwingStore:
    counts: dict[str, int] = field(default_factory=dict)
    total: int = 0
    dropped: int = 0
    queue: list[dict] = field(default_factory=list)
    last: dict[str, dict] = field(default_factory=dict)
    # 玩家名 → (monotonic 时间, 目标信息)
    pending_click: dict[str, tuple[float, dict]] = field(default_factory=dict)
    # 玩家名 → (monotonic 时间, 事件字典)；点击后到时回溯修正 target
    pending_swing: dict[str, tuple[float, dict]] = field(default_factory=dict)

    def reset(self) -> None:
        self.counts.clear()
        self.queue.clear()
        self.last.clear()
        self.pending_click.clear()
        self.pending_swing.clear()
        self.total = 0
        self.dropped = 0


class _SwingListener:
    """惰性注册的包/交互监听。异常一律吞掉，不能把异常抛进 endstone 的事件分发。"""

    def __init__(self, api: "SensingApi") -> None:
        self._api = api

    @event_handler
    def on_packet_receive(self, event: PacketReceiveEvent) -> None:
        try:
            if event.packet_id != _ANIMATE_PACKET_ID:
                return
            self._api._on_animate(bytes(event.payload), event.player)
        except Exception:  # noqa: BLE001 - 库的旁路功能，绝不影响收包
            pass

    @event_handler
    def on_player_interact(self, event: PlayerInteractEvent) -> None:
        try:
            self._api._on_interact(event)
        except Exception:  # noqa: BLE001
            pass


class SensingApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._store = _SwingStore()
        self._listener: _SwingListener | None = None
        self._active = False
        self._notes: list[str] = []

    # ── 输入设备（占位）──────────────────────────────────────────────────
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

    # ── 攻击 / 挥动 ─────────────────────────────────────────────────────
    def _ensure_listener(self) -> None:
        if self._listener is not None:
            return
        self._listener = _SwingListener(self)
        self._active = True
        self._plugin.register_events(self._listener)
        self._plugin.logger.debug("sensing: 攻击/挥动监听已启用（PacketReceiveEvent id=44 + PlayerInteractEvent）")

    def _level_tick(self, player) -> int:
        try:
            return int(player.level.time)
        except Exception:  # noqa: BLE001
            return 0

    @staticmethod
    def _unique_of(player, runtime_id: int) -> int:
        if not runtime_id:
            return 0
        try:
            for actor in player.level.actors:
                if actor.runtime_id == runtime_id:
                    return int(actor.id)
        except Exception:  # noqa: BLE001
            pass
        return 0

    def _item_of(self, player) -> tuple[str, int]:
        try:
            stack = player.inventory.item_in_main_hand
            if stack is None:
                return "", 0
            identifier = getattr(stack.type, "id", None) or str(stack.type)
            return str(identifier), int(stack.amount)
        except Exception:  # noqa: BLE001
            return "", 0

    def _record(self, ev: dict) -> None:
        st = self._store
        name = ev["player"]
        st.counts[name] = st.counts.get(name, 0) + 1
        st.total += 1
        if len(st.queue) >= _MAX_QUEUE:
            st.queue.pop(0)
            st.dropped += 1
        st.queue.append(ev)
        if len(st.last) >= _MAX_LAST:
            st.last.clear()
        st.last[name] = ev

    def _on_animate(self, payload: bytes, player) -> None:
        if player is None or not self._active:
            return
        decoded = decode_animate(payload)
        if decoded is None:
            return
        runtime_id, source = decoded
        name = player.name
        now = time.monotonic()

        item_name, item_count = self._item_of(player)
        ev = {
            "player": name,
            "source": source,
            "target": TARGET_AIR,
            "x": 0,
            "y": 0,
            "z": 0,
            "face": -1,
            "runtimeId": runtime_id,
            "uniqueId": 0,
            "item": item_name,
            "count": item_count,
            "tick": self._level_tick(player),
        }

        if source == 4 or runtime_id:
            ev["target"] = TARGET_ENTITY
            ev["uniqueId"] = self._unique_of(player, runtime_id)
            self._record(ev)
            return

        click = self._store.pending_click.get(name)
        if click is not None and now - click[0] <= _CORRELATE_WINDOW:
            self._apply_click(ev, click[1])
            self._record(ev)
            return

        # 目标暂时记成 air；同一次点击的事件可能晚到几毫秒，先挂起，等来了一键回填
        self._store.pending_swing[name] = (now, ev)
        self._record(ev)

    @staticmethod
    def _apply_click(ev: dict, info: dict) -> None:
        ev["target"] = info["target"]
        if info["target"] == TARGET_BLOCK:
            ev["x"], ev["y"], ev["z"] = info["x"], info["y"], info["z"]
            ev["face"] = info["face"]

    def _on_interact(self, event) -> None:
        if not self._active:
            return
        action = getattr(event, "action", None)
        if action is None:
            return
        left_air = getattr(type(event), "LEFT_CLICK_AIR", None)
        left_block = getattr(type(event), "LEFT_CLICK_BLOCK", None)
        if action not in (left_air, left_block):
            return

        player = event.player
        if player is None:
            return
        name = player.name
        now = time.monotonic()

        info = {"target": TARGET_AIR, "x": 0, "y": 0, "z": 0, "face": -1}
        if action == left_block and getattr(event, "has_block", False):
            loc = event.block.location
            info["target"] = TARGET_BLOCK
            info["x"], info["y"], info["z"] = int(loc.x), int(loc.y), int(loc.z)
            face = getattr(event, "block_face", None)
            info["face"] = int(face.value) if face is not None else -1

        self._store.pending_click[name] = (now, info)

        pending = self._store.pending_swing.pop(name, None)
        if pending is not None and now - pending[0] <= _CORRELATE_WINDOW:
            self._apply_click(pending[1], info)

    # ── 对外 API ────────────────────────────────────────────────────────
    @staticmethod
    def _format(ev: dict) -> str:
        source = "unset" if ev["source"] is None else _SOURCE_NAMES.get(ev["source"], "unknown")
        item = ev["item"] or "none"
        return (
            f"player={ev['player']} source={source} target={ev['target']} "
            f"pos={ev['x']},{ev['y']},{ev['z']} face={ev['face']} "
            f"item={item} count={ev['count']} runtimeId={ev['runtimeId']} "
            f"uniqueId={ev['uniqueId']} tick={ev['tick']}"
        )

    def poll_events(self) -> list[str]:
        """取走自上次轮询以来攒下的挥动事件（取走即清空）。"""
        self._ensure_listener()
        events = [self._format(ev) for ev in self._store.queue]
        self._store.queue.clear()
        return events

    def clear_events(self) -> None:
        """只清事件队列，不动计数与「最近一次」。"""
        self._ensure_listener()
        self._store.queue.clear()

    def last_event(self, player_name: str) -> str:
        """该玩家最近一次挥动的事件行；没挥过返回空串。"""
        self._ensure_listener()
        ev = self._store.last.get(player_name)
        return self._format(ev) if ev is not None else ""

    def last_target(self, player_name: str) -> str:
        """该玩家最近一次挥动的目标：air / block / entity；没挥过返回 none。"""
        self._ensure_listener()
        ev = self._store.last.get(player_name)
        return ev["target"] if ev is not None else "none"

    def last_block(self, player_name: str) -> str:
        """最近一次「对着方块挥」的坐标 x,y,z；不是方块则返回空串。"""
        self._ensure_listener()
        ev = self._store.last.get(player_name)
        if ev is None or ev["target"] != TARGET_BLOCK:
            return ""
        return f"{ev['x']},{ev['y']},{ev['z']}"

    def count(self, player_name: str) -> int:
        """该玩家自上次 reset 以来的挥动次数。"""
        self._ensure_listener()
        return int(self._store.counts.get(player_name, 0))

    def total(self) -> int:
        """全服累计挥动次数（含已退出玩家，直到 reset）。"""
        self._ensure_listener()
        return int(self._store.total)

    def dropped(self) -> int:
        """事件队列满被丢弃的条数（诊断用）。"""
        self._ensure_listener()
        return int(self._store.dropped)

    def reset(self) -> None:
        """清空计数、事件队列、最近一次与丢弃计数。"""
        self._store.reset()

    def clear(self) -> None:
        """插件禁用时清状态（监听已由 endstone 随插件注销）。"""
        self._active = False
        self._store.reset()
        self._notes.clear()
