"""视角覆盖（View）域 —— LeviLamina 版 ViewOverrideManager 的 endstone 移植。"""

from __future__ import annotations

import threading

from .holograms import _dim_id

# 出站包：携带 actor runtimeId，且可安全取消
_ACTOR_PACKETS = ("add_actor", "add_player", "move_actor_absolute", "set_actor_data")


class _ViewState:
    __slots__ = ("hidden_runtime_ids", "blocks", "notes", "owners")

    def __init__(self) -> None:
        self.hidden_runtime_ids: dict[int, set[str]] = {}  # runtimeId -> 观看者（空集合=所有人）
        self.blocks: dict[str, dict[tuple[int, int, int], str]] = {}
        self.notes: dict[str, list[str]] = {}
        self.owners: dict[int, str] = {}  # runtimeId -> 本人的玩家名，永远不取消发给本人的包


class ViewApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._state = _ViewState()
        self._registered = False

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
            self._plugin.logger.debug(f"view 发包失败 ({player.name}): {exc}")
            return False

    def mark_registered(self) -> None:
        self._registered = True

    def _note(self, who: str, text: str) -> None:
        with self._lock:
            bucket = self._state.notes.setdefault(who or "*", [])
            if text not in bucket:
                bucket.append(text)

    # ── 方块覆盖（真实生效） ──────────────────────────────────────────────
    def view_block(self, player_name: str, x: int, y: int, z: int, block_type: str) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        runtime_id = self._plugin.fakeblock.resolve(block_type)
        if runtime_id < 0:
            return False
        body = self._plugin.core.encode_update_block(int(x), int(y), int(z), runtime_id, 3, 0)
        ok = self._send(player, self._plugin.core.update_block_packet_id, body)
        if ok:
            with self._lock:
                self._state.blocks.setdefault(player_name, {})[(int(x), int(y), int(z))] = str(block_type)
        return ok

    def view_clear_block(self, player_name: str, x: int, y: int, z: int) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        runtime_id = self._plugin.fakeblock._real_runtime_id(player, int(x), int(y), int(z))
        if runtime_id < 0:
            return False
        body = self._plugin.core.encode_update_block(int(x), int(y), int(z), runtime_id, 3, 0)
        ok = self._send(player, self._plugin.core.update_block_packet_id, body)
        if ok:
            with self._lock:
                self._state.blocks.get(player_name, {}).pop((int(x), int(y), int(z)), None)
        return ok

    # ── 实体可见性（真实生效：取消出站包） ────────────────────────────────
    def _resolve_runtime_id(self, unique_id: int) -> int:
        for entity_id in self._plugin.entities.get_all_ids():
            if self._plugin.entities.unique_id(entity_id) == int(unique_id):
                return self._plugin.entities.runtime_id(entity_id)
        return -1

    def view_entity(self, player_name: str, unique_id: int, spec: str = "") -> bool:
        """spec 里只有 hidden 能真实生效；类型/皮肤/名字改写需要出站解码器（未实现）。"""
        runtime_id = self._resolve_runtime_id(unique_id)
        if runtime_id < 0:
            return False
        if "hidden" in str(spec).lower():
            hidden = "hidden=false" not in str(spec).lower() and "hidden=0" not in str(spec).lower()
            with self._lock:
                self._state.hidden_runtime_ids.setdefault(runtime_id, set()).add(player_name)
            self._plugin.ensure_view_filter()
            return hidden
        return False

    def view_clear_entity(self, player_name: str, unique_id: int) -> bool:
        runtime_id = self._resolve_runtime_id(unique_id)
        if runtime_id < 0:
            return False
        with self._lock:
            bucket = self._state.hidden_runtime_ids.get(runtime_id)
            if bucket is None:
                return False
            bucket.discard(player_name)
            if not bucket:
                self._state.hidden_runtime_ids.pop(runtime_id, None)
        return True

    def view_clear_id(self, unique_id: int) -> bool:
        runtime_id = self._resolve_runtime_id(unique_id)
        if runtime_id < 0:
            return False
        with self._lock:
            return self._state.hidden_runtime_ids.pop(runtime_id, None) is not None

    def view_clear_all(self, player_name: str) -> None:
        with self._lock:
            for runtime_id, viewers in list(self._state.hidden_runtime_ids.items()):
                viewers.discard(player_name)
                if not viewers:
                    self._state.hidden_runtime_ids.pop(runtime_id, None)
            squares = list(self._state.blocks.pop(player_name, {}).keys())
        player = self._player(player_name)
        if player is not None:
            for (x, y, z) in squares:
                self.view_clear_block(player_name, x, y, z)

    # ── 自我覆盖 ──────────────────────────────────────────────────────────
    def view_self_hide(self, player_name: str, hidden: bool) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        runtime_id = int(player.runtime_id)
        with self._lock:
            if hidden:
                self._state.hidden_runtime_ids.setdefault(runtime_id, set())
                self._state.owners[runtime_id] = player_name
            else:
                self._state.hidden_runtime_ids.pop(runtime_id, None)
                self._state.owners.pop(runtime_id, None)
        self._plugin.ensure_view_filter()
        return True

    def view_self(self, player_name: str, spec: str) -> bool:
        return self.view_self_hide(player_name, "hidden" in str(spec).lower() and "false" not in str(spec).lower())

    def view_self_clear(self, player_name: str) -> bool:
        return self.view_self_hide(player_name, False)

    def view_self_type(self, player_name: str, type_: str) -> bool:
        self._note(player_name, f"selfType {type_} 需要出站解码器")
        return False

    def view_self_skin(self, player_name: str, skin_of_player: str) -> bool:
        self._note(player_name, f"selfSkin {skin_of_player} 需要出站解码器")
        return False

    def view_self_name(self, player_name: str, nametag: str, always_show: bool = True) -> bool:
        self._note(player_name, f"selfName {nametag} 需要出站解码器")
        return False

    def view_target_type(self, unique_id: int, type_: str) -> bool:
        self._note("*", f"targetType {unique_id}->{type_} 需要出站解码器")
        return False

    # ── 查询 / 诊断 ───────────────────────────────────────────────────────
    def view_unique_id_of(self, player_name: str) -> int:
        player = self._player(player_name)
        return int(player.runtime_id) if player is not None else -1  # endstone 无 actor uniqueId

    def view_echo(self, text: str) -> str:
        data = str(text).encode("utf-8", "replace")
        hex_part = "".join(f"{b:02x}" for b in data[:16])
        return f"len={len(data)} hex={hex_part} text=[{text}]"

    def view_describe(self, player_name: str) -> str:
        with self._lock:
            hidden = {rid: sorted(v) for rid, v in self._state.hidden_runtime_ids.items()}
            blocks = dict(self._state.blocks.get(player_name, {}))
            notes = list(self._state.notes.get(player_name, [])) + list(self._state.notes.get("*", []))
        return f"player={player_name} hidden={hidden} blocks={len(blocks)} notes={notes}"

    def view_meta_probe(self, spec: str) -> str:
        parts = [p.strip() for p in str(spec).split(",") if p.strip()]
        pairs = []
        for part in parts:
            if "=" in part:
                key, value = part.split("=", 1)
                pairs.append(f"{key.strip()}={value.strip()}")
            else:
                pairs.append(f"{part}=<none>")
        return "meta[" + ", ".join(pairs) + "]"

    def view_poll_spawn_events(self) -> list[str]:
        return []

    def view_item_spawn_hooked(self) -> bool:
        return True

    # ── 出站过滤：给 plugin 的事件监听用 ──────────────────────────────────
    def should_cancel(self, packet_id: int, payload: bytes, viewer_name: str) -> bool:
        with self._lock:
            if not self._state.hidden_runtime_ids:
                return False
            targets = dict(self._state.hidden_runtime_ids)
        if not targets:
            return False
        core = self._plugin.core
        if packet_id not in (core.add_actor_packet_id, core.add_player_packet_id,
                             core.move_actor_absolute_packet_id, core.set_actor_data_packet_id):
            return False
        runtime_id, _ = _read_varint64(payload)
        bucket = targets.get(runtime_id)
        if bucket is None:
            return False
        with self._lock:
            owner = self._state.owners.get(runtime_id)
        if owner is not None and owner == viewer_name:
            return False  # 本人永远能看见自己，否则客户端会卡死/掉线
        if bucket and viewer_name not in bucket:
            return False
        return True


def _read_varint64(data: bytes) -> tuple[int, int]:
    value = 0
    shift = 0
    i = 0
    while i < len(data) and shift < 70:
        byte = data[i]
        i += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            break
        shift += 7
    return value, i
