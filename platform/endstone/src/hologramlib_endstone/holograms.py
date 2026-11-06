"""悬浮字（Hologram）域 —— LeviLamina 版 FloatingTextManager 的 endstone 移植。"""

from __future__ import annotations

import threading
import time
from typing import Callable

from .native import ShapeDesc
from .shapes import text_shape

VariableProvider = Callable[[str], str]

# 与 BDS 的 Dimension 数值一致（0 主世界 / 1 下界 / 2 末地）
_DIM_IDS = {"OVERWORLD": 0, "NETHER": 1, "THE_END": 2}


def _dim_id(player) -> int:
    try:
        return _DIM_IDS.get(str(player.dimension.type).upper(), 0)
    except Exception:
        return 0


def _pack_rgba(r: float, g: float, b: float, a: float = 1.0) -> int:
    def c(v: float) -> int:
        return max(0, min(255, int(round(float(v) * 255.0))))

    return (c(a) << 24) | (c(r) << 16) | (c(g) << 8) | c(b)


class HologramLine:
    __slots__ = ("text", "pool", "interval_ms", "parse_variables", "_pool_index", "_pool_at")

    def __init__(self, text: str = "") -> None:
        self.text = text
        self.pool: list[str] = []
        self.interval_ms = 0
        self.parse_variables = True
        self._pool_index = 0
        self._pool_at = 0.0

    def current(self, now_ms: float) -> str:
        if not self.pool:
            return self.text
        if self.interval_ms > 0 and now_ms - self._pool_at >= self.interval_ms:
            steps = int((now_ms - self._pool_at) // self.interval_ms)
            self._pool_index = (self._pool_index + steps) % len(self.pool)
            self._pool_at += steps * self.interval_ms
        return self.pool[self._pool_index]


class FloatingText:
    def __init__(self, text_id: int, x: float, y: float, z: float) -> None:
        self.id = text_id
        self.x, self.y, self.z = float(x), float(y), float(z)
        self.dim_id = 0

        self.lines: list[HologramLine] = []
        self.color = (1.0, 1.0, 1.0, 1.0)
        self.scale = 1.0
        self.background: tuple[float, float, float, float] | None = None
        self.depth_test = False
        self.rotation: tuple[float, float, float] | None = None
        self.follow: tuple[str, float] | None = None

        self.drawn = False
        self.viewers: dict[str, int] = {}  # player_name -> shape id
        self.last_text: dict[str, str] = {}


class HologramApi:
    """悬浮字对外 API。方法名对齐 LSE 的 holo*（去掉前缀、改 snake_case）。"""

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._items: dict[int, FloatingText] = {}
        self._next_id = 1
        self._variables: dict[str, VariableProvider] = {}
        self._register_builtin_variables()

    # ── 变量 ──────────────────────────────────────────────────────────────
    def _register_builtin_variables(self) -> None:
        self.register_variable("time", lambda _n: time.strftime("%H:%M:%S"))
        self.register_variable("online", lambda _n: str(len(self._plugin.server.online_players)))
        self.register_variable("player", lambda n: n if n else "Unknown")
        self.register_variable("tps", lambda _n: f"{getattr(self._plugin.server, 'current_tps', 20.0):.1f}")
        self.register_variable("dimension", lambda n: str(self._player_dim_id(n)))
        self.register_variable("x", lambda n: self._coord(n, 0))
        self.register_variable("y", lambda n: self._coord(n, 1))
        self.register_variable("z", lambda n: self._coord(n, 2))

    def register_variable(self, name: str, provider: VariableProvider) -> None:
        with self._lock:
            self._variables[name] = provider

    def unregister_variable(self, name: str) -> bool:
        with self._lock:
            return self._variables.pop(name, None) is not None

    def _player(self, name: str):
        if not name:
            return None
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _player_dim_id(self, name: str) -> int:
        player = self._player(name)
        return _dim_id(player) if player is not None else -1

    def _coord(self, name: str, axis: int) -> str:
        player = self._player(name)
        if player is None:
            return "?"
        loc = player.location
        return f"{[loc.x, loc.y, loc.z][axis]:.1f}"

    def _apply_variables(self, text: str, player_name: str) -> str:
        if not text or "{" not in text:
            return text
        with self._lock:
            providers = list(self._variables.items())
        for name, provider in providers:
            token = "{" + name + "}"
            if token not in text:
                continue
            try:
                value = provider(player_name)
            except Exception:
                value = "?"
            text = text.replace(token, value)
        return text

    # ── 创建 / 销毁 ───────────────────────────────────────────────────────
    def create(self, x: float, y: float, z: float) -> int:
        with self._lock:
            text_id = self._next_id
            self._next_id += 1
            self._items[text_id] = FloatingText(text_id, x, y, z)
            return text_id

    def get(self, text_id: int) -> FloatingText | None:
        with self._lock:
            return self._items.get(text_id)

    def destroy(self, text_id: int) -> bool:
        with self._lock:
            item = self._items.pop(text_id, None)
        if item is None:
            return False
        self._drop_viewers(item)
        return True

    def destroy_all(self) -> int:
        with self._lock:
            items = list(self._items.values())
            self._items.clear()
        for item in items:
            self._drop_viewers(item)
        return len(items)

    def info(self) -> str:
        with self._lock:
            return f"HologramLib(holograms={len(self._items)}, api={self._plugin.core.api_version})"

    # ── 行 ────────────────────────────────────────────────────────────────
    def add_line(self, text_id: int, text: str) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.lines.append(HologramLine(str(text)))
        return True

    def set_line_text(self, text_id: int, index: int, text: str) -> bool:
        item = self.get(text_id)
        if item is None or not (0 <= index < len(item.lines)):
            return False
        item.lines[index].text = str(text)
        return True

    def set_line_pool(self, text_id: int, index: int, content: list[str], interval_ms: int) -> bool:
        item = self.get(text_id)
        if item is None or not (0 <= index < len(item.lines)):
            return False
        line = item.lines[index]
        line.pool = [str(c) for c in content]
        line.interval_ms = max(0, int(interval_ms))
        line._pool_index = 0
        line._pool_at = time.monotonic() * 1000.0
        return True

    def set_line_parse_variables(self, text_id: int, index: int, enabled: bool) -> bool:
        item = self.get(text_id)
        if item is None or not (0 <= index < len(item.lines)):
            return False
        item.lines[index].parse_variables = bool(enabled)
        return True

    def remove_line(self, text_id: int, index: int) -> bool:
        item = self.get(text_id)
        if item is None or not (0 <= index < len(item.lines)):
            return False
        del item.lines[index]
        return True

    def clear_lines(self, text_id: int) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.lines.clear()
        return True

    def get_line_count(self, text_id: int) -> int:
        item = self.get(text_id)
        return len(item.lines) if item is not None else 0

    # ── 样式 ──────────────────────────────────────────────────────────────
    def set_color(self, text_id: int, r: float, g: float, b: float, a: float = 1.0) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.color = (float(r), float(g), float(b), float(a))
        return True

    def set_background_color(self, text_id: int, r: float, g: float, b: float, a: float = 1.0) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.background = (float(r), float(g), float(b), float(a))
        return True

    def clear_background_color(self, text_id: int) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.background = None
        return True

    def set_scale(self, text_id: int, scale: float) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.scale = float(scale)
        return True

    def set_depth_test(self, text_id: int, enabled: bool) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.depth_test = bool(enabled)
        return True

    def set_rotation(self, text_id: int, pitch: float, yaw: float, roll: float = 0.0) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.rotation = (float(pitch), float(yaw), float(roll))
        return True

    def clear_rotation(self, text_id: int) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.rotation = None
        return True

    def set_location(self, text_id: int, x: float, y: float, z: float) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.x, item.y, item.z = float(x), float(y), float(z)
        return True

    def set_dimension(self, text_id: int, dim_id: int) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.dim_id = int(dim_id)
        return True

    def set_follow_player(self, text_id: int, player_name: str, offset_y: float = 0.0) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.follow = (str(player_name), float(offset_y))
        return True

    def clear_follow_player(self, text_id: int) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.follow = None
        return True

    # ── 绘制 ──────────────────────────────────────────────────────────────
    def _viewers(self, item: FloatingText) -> list:
        players = list(self._plugin.server.online_players)
        if item.dim_id >= 0:
            players = [p for p in players if _dim_id(p) == item.dim_id]
        return players

    def _position(self, item: FloatingText) -> tuple[float, float, float]:
        if item.follow is None:
            return item.x, item.y, item.z
        player = self._player(item.follow[0])
        if player is None:
            return item.x, item.y, item.z
        loc = player.location
        return loc.x, loc.y + item.follow[1], loc.z

    def _compose(self, item: FloatingText, player_name: str, now_ms: float) -> str:
        parts = []
        for line in item.lines:
            text = line.current(now_ms)
            if line.parse_variables:
                text = self._apply_variables(text, player_name)
            parts.append(text)
        return "\n".join(parts)

    def _descriptor(self, item: FloatingText, text: str) -> ShapeDesc:
        x, y, z = self._position(item)
        desc = text_shape(x, y, z, text)
        desc.has_color = 1
        desc.color = _pack_rgba(*item.color)
        desc.has_scale = 1
        desc.scale = item.scale
        desc.text_depth_test = 1 if item.depth_test else 0
        desc.text_show_backface = 1
        desc.text_show_text_backface = 1
        if item.background is not None:
            desc.has_text_background = 1
            desc.text_background_color = _pack_rgba(*item.background)
        if item.rotation is not None:
            desc.has_rotation = 1
            desc.rx, desc.ry, desc.rz = item.rotation
        return desc

    def draw(self, text_id: int) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.drawn = True
        self.refresh(text_id)
        return True

    def draw_to_player(self, text_id: int, player_name: str) -> bool:
        item = self.get(text_id)
        player = self._player(player_name)
        if item is None or player is None:
            return False
        item.drawn = True
        self._render_viewer(item, player)
        return True

    def draw_to_dimension(self, text_id: int, dim_id: int) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        item.dim_id = int(dim_id)
        item.drawn = True
        self.refresh(text_id)
        return True

    def refresh(self, text_id: int) -> bool:
        item = self.get(text_id)
        if item is None:
            return False
        for player in self._viewers(item):
            self._render_viewer(item, player)
        return True

    def _render_viewer(self, item: FloatingText, player) -> None:
        scene = self._plugin.scene
        now_ms = time.monotonic() * 1000.0
        text = self._compose(item, player.name, now_ms)
        desc = self._descriptor(item, text)

        shape_id = item.viewers.get(player.name)
        if shape_id is None or scene.get(shape_id) is None:
            shape_id = scene.add(desc)
            item.viewers[player.name] = shape_id
            item.last_text[player.name] = text
            self._plugin.api._send([player], scene.encode([desc]))
            return

        # 内容没变就不重发（LL 版同样按 mLastSentText 去重）
        if item.last_text.get(player.name) == text:
            return
        existing = scene.get(shape_id)
        if existing is not None:
            existing.text = text.encode("utf-8")
            existing.x, existing.y, existing.z = desc.x, desc.y, desc.z
            existing.color = desc.color
            existing.has_color = 1
        item.last_text[player.name] = text
        self._plugin.api._send([player], scene.encode([existing] if existing is not None else [desc]))

    def _drop_viewers(self, item: FloatingText) -> None:
        scene = self._plugin.scene
        if not item.viewers:
            return
        removals = []
        for shape_id in item.viewers.values():
            desc = scene.remove(shape_id)
            if desc is not None:
                removals.append(desc)
        if removals:
            self._plugin.api._send(list(self._plugin.server.online_players), scene.encode_removals(removals))
        item.viewers.clear()
        item.last_text.clear()

    def forget(self, player_name: str) -> None:
        """玩家退服：丢掉他的观看者形状（形状是客户端侧的，不需要发删除包）。"""
        with self._lock:
            items = list(self._items.values())
        for item in items:
            item.viewers.pop(player_name, None)
            item.last_text.pop(player_name, None)

    def tick(self) -> None:
        """刷新节拍：重绘所有已绘制的悬浮字。"""
        with self._lock:
            items = [it for it in self._items.values() if it.drawn]
        for item in items:
            try:
                self.refresh(item.id)
            except Exception as exc:  # 单个悬浮字出错不能拖垮整个 tick
                self._plugin.logger.debug(f"refresh {item.id} 失败: {exc}")

    def __len__(self) -> int:
        with self._lock:
            return len(self._items)
