"""假方块（FakeBlock）域 —— LeviLamina 版 FakeBlockExporter 的 endstone 移植。"""

from __future__ import annotations

import threading

# UpdateBlockPacket 的 flag：3 = 邻居 + 网络（LL 版同值）
FLAG_NEIGHBORS_AND_NETWORK = 3


def parse_block_state(text: str) -> tuple[str, dict] | None:
    """把 "minecraft:oak_stairs[waterlogged=true,upside_down_bit=false]" 拆成 (name, states)。"""
    raw = text.strip()
    if not raw:
        return None
    open_at = raw.find("[")
    name = (raw if open_at < 0 else raw[:open_at]).strip()
    if not name:
        return None
    states: dict = {}
    if open_at < 0:
        return name, states

    close_at = raw.rfind("]")
    if close_at < 0 or close_at <= open_at:
        return None
    body = raw[open_at + 1 : close_at]

    parts: list[str] = []
    current = ""
    quote = ""
    for ch in body:
        if quote:
            current += ch
            if ch == quote:
                quote = ""
            continue
        if ch in ("'", '"'):
            quote = ch
            current += ch
            continue
        if ch == ",":
            parts.append(current)
            current = ""
            continue
        current += ch
    if current.strip():
        parts.append(current)

    for part in parts:
        part = part.strip()
        if not part:
            continue
        if "=" not in part:
            return None
        key, value = part.split("=", 1)
        key = key.strip()
        value = value.strip()
        if not key:
            return None
        if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
            states[key] = value[1:-1]
        elif value in ("true", "false"):
            states[key] = value == "true"
        else:
            try:
                states[key] = int(value)
                continue
            except ValueError:
                pass
            try:
                states[key] = float(value)
                continue
            except ValueError:
                pass
            states[key] = value
    return name, states


class FakeBlockApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._cache: dict[str, int] = {}

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def resolve(self, state: str) -> int:
        """状态串 → 网络 id；解析不了返回 -1。"""
        key = str(state).strip()
        with self._lock:
            if key in self._cache:
                return self._cache[key]
        parsed = parse_block_state(key)
        if parsed is None:
            return -1
        name, states = parsed
        try:
            data = self._plugin.server.create_block_data(name, states or None)
            runtime_id = int(data.runtime_id)
        except Exception as exc:
            self._plugin.logger.debug(f"blockFakeResolve 失败 ({key}): {exc}")
            return -1
        with self._lock:
            self._cache[key] = runtime_id
        return runtime_id

    def _send(self, player, x: int, y: int, z: int, runtime_id: int) -> bool:
        try:
            body = self._plugin.core.encode_update_block(x, y, z, runtime_id, FLAG_NEIGHBORS_AND_NETWORK, 0)
            player.send_packet(self._plugin.core.update_block_packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"blockFake 发包失败 ({player.name}): {exc}")
            return False

    def _real_runtime_id(self, player, x: int, y: int, z: int) -> int:
        try:
            block = player.dimension.get_block_at(x, y, z)
            return int(block.data.runtime_id)
        except Exception as exc:
            self._plugin.logger.debug(f"读真实方块失败 ({x},{y},{z}): {exc}")
            return -1

    # ── 对外 ──────────────────────────────────────────────────────────────
    def set(self, player_name: str, x: int, y: int, z: int, state: str) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        runtime_id = self.resolve(state)
        if runtime_id < 0:
            return False
        return self._send(player, int(x), int(y), int(z), runtime_id)

    def clear(self, player_name: str, x: int, y: int, z: int) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        runtime_id = self._real_runtime_id(player, int(x), int(y), int(z))
        if runtime_id < 0:
            return False
        return self._send(player, int(x), int(y), int(z), runtime_id)

    def set_all(self, x: int, y: int, z: int, dim: int, state: str) -> int:
        runtime_id = self.resolve(state)
        if runtime_id < 0:
            return 0
        from .holograms import _dim_id

        sent = 0
        for player in list(self._plugin.server.online_players):
            if _dim_id(player) == int(dim) and self._send(player, int(x), int(y), int(z), runtime_id):
                sent += 1
        return sent

    def clear_all(self, x: int, y: int, z: int, dim: int) -> int:
        from .holograms import _dim_id

        sent = 0
        for player in list(self._plugin.server.online_players):
            if _dim_id(player) != int(dim):
                continue
            runtime_id = self._real_runtime_id(player, int(x), int(y), int(z))
            if runtime_id >= 0 and self._send(player, int(x), int(y), int(z), runtime_id):
                sent += 1
        return sent
