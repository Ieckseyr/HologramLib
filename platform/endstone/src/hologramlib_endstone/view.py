"""视角覆盖（View）域 —— LeviLamina 版 ViewOverrideManager 的 endstone 移植。"""

from __future__ import annotations

import base64
import threading

from .holograms import _dim_id

# 出站包：携带 actor runtimeId，且可安全取消
_ACTOR_PACKETS = ("add_actor", "add_player", "move_actor_absolute", "set_actor_data")

# ── 方块实体 NBT（network 方言）──────────────────────────────────────────────
# BDS 的 network NBT 与磁盘 NBT 不同：string 长度是 varint，Int / Int64 是 zigzag varint，
# List/Array 的 count 也是 zigzag varint（见 core 侧的 bedrock-protocol nbt.hpp）。
_TAG_END = 0x00
_TAG_INT = 0x03
_TAG_STRING = 0x08
_TAG_LIST = 0x09
_TAG_COMPOUND = 0x0A

_HEX_DIGITS = frozenset("0123456789abcdefABCDEF")


def _read_varint(raw: bytes, pos: int, signed: bool = False) -> tuple[int, int]:
    value = 0
    shift = 0
    while True:
        byte = raw[pos]
        pos += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            break
        shift += 7
        if shift > 70:
            raise ValueError("varint 过长")
    if signed:
        return (value >> 1) ^ -(value & 1), pos
    return value, pos


def _write_varint(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        out.append(byte | 0x80 if value else byte)
        if not value:
            return bytes(out)


def _write_string(text: str) -> bytes:
    data = text.encode("utf-8")
    return _write_varint(len(data)) + data


def _skip_tag(raw: bytes, pos: int, tag: int) -> int:
    """跳过一个 network NBT 的 payload（type / name 已读）。"""
    if tag == 1:
        return pos + 1
    if tag == 2:
        return pos + 2
    if tag in (3, 4):
        return _read_varint(raw, pos, True)[1]
    if tag == 5:
        return pos + 4
    if tag == 6:
        return pos + 8
    if tag == 7:
        count, pos = _read_varint(raw, pos, True)
        return pos + max(count, 0)
    if tag == 8:
        count, pos = _read_varint(raw, pos)
        return pos + count
    if tag == 9:
        element = raw[pos]
        count, pos = _read_varint(raw, pos + 1, True)
        for _ in range(max(count, 0)):
            pos = _skip_tag(raw, pos, element)
        return pos
    if tag == 10:
        while True:
            tag = raw[pos]
            pos += 1
            if tag == _TAG_END:
                return pos
            length, pos = _read_varint(raw, pos)
            pos += length
            pos = _skip_tag(raw, pos, tag)
    if tag == 11:
        count, pos = _read_varint(raw, pos, True)
        for _ in range(max(count, 0)):
            _, pos = _read_varint(raw, pos, True)
        return pos
    if tag == 12:
        count, pos = _read_varint(raw, pos, True)
        for _ in range(max(count, 0)):
            _, pos = _read_varint(raw, pos, True)
        return pos
    raise ValueError(f"未知 NBT tag {tag}")


def decode_nbt_text(text: str) -> bytes | None:
    """hex 或 base64 → 字节；不是合法文本返回 None。

    纯 hex 字符且长度为偶数按 hex 解，其余按 base64（容忍空白与缺省 padding）。
    解出来的字节是否真的是 NBT 由 C ABI（holo_core_encode_block_actor_data）判断。
    """
    compact = "".join(str(text or "").split())
    if not compact:
        return None
    if len(compact) % 2 == 0 and all(c in _HEX_DIGITS for c in compact):
        try:
            return bytes.fromhex(compact)
        except ValueError:
            return None
    try:
        return base64.b64decode(compact + "=" * (-len(compact) % 4), validate=True)
    except Exception:
        return None


def nbt_with_string_field(raw: bytes, key: str, value: str) -> bytes | None:
    """在 network NBT 根 compound 里覆盖/追加一个字符串字段（同名旧字段会被丢掉）。

    根不是 compound / 结构不完整返回 None。用来把方块实体类型（id 字段）补进调用方给的 NBT，
    与 LSE 版 `viewBlockEntity` 的 `tag["id"] = type` 等价。
    """
    if not raw or raw[0] != _TAG_COMPOUND:
        return None
    target = str(key).encode("utf-8")
    try:
        name_len, pos = _read_varint(raw, 1)
        header_end = pos + name_len
        if header_end > len(raw):
            return None
        pos = header_end
        entries: list[bytes] = []
        while True:
            tag = raw[pos]
            start = pos
            pos += 1
            if tag == _TAG_END:
                break
            name_len, pos = _read_varint(raw, pos)
            name = raw[pos:pos + name_len]
            pos += name_len
            pos = _skip_tag(raw, pos, tag)
            if pos > len(raw):
                return None
            if name != target:
                entries.append(raw[start:pos])
        entry = bytes([_TAG_STRING]) + _write_string(str(key)) + _write_string(str(value))
        return raw[:header_end] + b"".join(entries) + entry + bytes([_TAG_END])
    except (IndexError, ValueError):
        return None



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

    def view_block_by_id(self, player_name: str, x: int, y: int, z: int, network_id: int) -> bool:
        """按 network id 直接改一个方块（状态由调用方自己算，对应 LSE 的 viewBlockById）。

        `network_id <= 0`（0 = 空气，也当作非法）或玩家不在线返回 False；清方块用 view_clear_block。
        """
        try:
            network_id = int(network_id)
        except (TypeError, ValueError):
            return False
        if network_id <= 0:
            return False
        player = self._player(player_name)
        if player is None:
            return False
        body = self._plugin.core.encode_update_block(int(x), int(y), int(z), network_id, 3, 0)
        ok = self._send(player, self._plugin.core.update_block_packet_id, body)
        if ok:
            with self._lock:
                self._state.blocks.setdefault(player_name, {})[(int(x), int(y), int(z))] = f"#{network_id}"
        return ok

    def view_block_entity(
        self, player_name: str, x: int, y: int, z: int, block_entity_type: str, nbt: str
    ) -> bool:
        """给一个坐标发 BlockActorData（id 56），只在该玩家客户端摆一个方块实体。

        对应 LSE 的 viewBlockEntity：告示牌 / 横幅 / 箱子这类"没有实体、数据在方块实体里"的方块。
        `nbt` 是已序列化的 network NBT（小端），接受 hex 或 base64；`block_entity_type`
        非空时覆盖/补上 NBT 里的 `id` 字段（如 "Banner"）。玩家不在线、nbt 解码失败、
        NBT 解析失败都返回 False。
        """
        player = self._player(player_name)
        if player is None:
            return False
        raw = decode_nbt_text(nbt)
        if not raw:
            return False
        if block_entity_type:
            with_id = nbt_with_string_field(raw, "id", str(block_entity_type))
            if not with_id:
                return False
            raw = with_id
        try:
            body = self._plugin.core.encode_block_actor_data(int(x), int(y), int(z), raw)
        except Exception as exc:
            self._plugin.logger.debug(f"view_block_entity 编码失败 ({player_name}): {exc}")
            return False
        return self._send(player, self._plugin.core.block_actor_data_packet_id, body)

    def _restore_block(self, player_name: str, x: int, y: int, z: int) -> bool:
        """把一个坐标恢复成服务端真实方块（只看在线，不看覆盖表）。"""
        player = self._player(player_name)
        if player is None:
            return False
        runtime_id = self._plugin.fakeblock._real_runtime_id(player, int(x), int(y), int(z))
        if runtime_id < 0:
            return False
        body = self._plugin.core.encode_update_block(int(x), int(y), int(z), runtime_id, 3, 0)
        return self._send(player, self._plugin.core.update_block_packet_id, body)

    def view_clear_block(self, player_name: str, x: int, y: int, z: int) -> bool:
        """撤掉该玩家的一个方块覆盖；没有覆盖（或玩家不在线）返回 False（与 LSE 版一致）。"""
        key = (int(x), int(y), int(z))
        with self._lock:
            if key not in self._state.blocks.get(player_name, {}):
                return False
        ok = self._restore_block(player_name, x, y, z)
        if ok:
            with self._lock:
                self._state.blocks.get(player_name, {}).pop(key, None)
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
                # 覆盖表已经 pop 掉了，这里直接走"恢复真实方块"，不能再调 view_clear_block
                self._restore_block(player_name, x, y, z)

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
