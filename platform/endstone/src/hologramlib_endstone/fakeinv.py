"""假背包（FakeInventory）域 —— LeviLamina 版 FakeInventoryManager 的 endstone 移植（非交互子集）。"""

from __future__ import annotations

import threading

from . import items
from .native import ItemSlot

INVENTORY_CONTAINER_ID = 0
FAKE_SLOTS = 36


class _Entry:
    __slots__ = ("type", "count", "damage", "name", "lore")

    def __init__(self, item_type: str = "", count: int = 0, damage: int = 0, name: str = "", lore: str = "") -> None:
        self.type = item_type
        self.count = count
        self.damage = damage
        self.name = name
        self.lore = lore


class _FakeInv:
    __slots__ = ("player", "slots", "interval", "next_refresh")

    def __init__(self, player: str, interval: int) -> None:
        self.player = player
        self.slots: list[_Entry] = [_Entry() for _ in range(FAKE_SLOTS)]
        self.interval = max(1, int(interval))
        self.next_refresh = 0


def parse_items(spec: str) -> list[_Entry]:
    slots = [_Entry() for _ in range(FAKE_SLOTS)]
    text = str(spec).replace("\n", ";")
    for raw in text.split(";"):
        entry = raw.strip()
        if not entry:
            continue
        fields = [f.strip() for f in entry.split("|")]
        if len(fields) < 2:
            continue
        try:
            slot = int(fields[0])
        except ValueError:
            continue
        if slot < 0 or slot >= FAKE_SLOTS:
            continue
        count = 1
        if len(fields) > 2 and fields[2]:
            try:
                count = int(fields[2])
            except ValueError:
                count = 1
        slots[slot] = _Entry(fields[1], count, 0, fields[3] if len(fields) > 3 else "", fields[4] if len(fields) > 4 else "")
    return slots


class FakeInventoryApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._inv: dict[str, _FakeInv] = {}

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _content(self, inv: _FakeInv) -> bytes:
        slots: list[ItemSlot] = []
        for entry in inv.slots:
            slot = ItemSlot()
            numeric = items.numeric_id(entry.type) if entry.type else -1
            if numeric > 0 and entry.count > 0:
                slot.item_id = numeric
                slot.stack_size = int(entry.count)
                slot.aux = int(entry.damage)
                if entry.name or entry.lore:
                    lore = [line for line in str(entry.lore).split(";") if line] if entry.lore else []
                    nbt = self._plugin.core.build_item_nbt(entry.type, int(entry.count), int(entry.damage), entry.name, lore)
                    slot.user_data = nbt
                    slot.user_data_len = len(nbt)
            slots.append(slot)
        return self._plugin.core.encode_inventory_content(INVENTORY_CONTAINER_ID, slots)

    def _send(self, player, body: bytes) -> bool:
        if not body:
            return False
        try:
            player.send_packet(self._plugin.core.inventory_content_packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"fakeinv 发包失败 ({player.name}): {exc}")
            return False

    # ── 对外 ──────────────────────────────────────────────────────────────
    def apply(self, player_name: str, items_spec: str, refresh_interval_ticks: int = 20) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        with self._lock:
            inv = _FakeInv(player_name, refresh_interval_ticks)
            inv.slots = parse_items(items_spec)
            inv.next_refresh = self._plugin.tick_count + inv.interval
            self._inv[player_name] = inv
        return self._send(player, self._content(inv))

    def set_slot(self, player_name: str, slot: int, item_type: str, count: int, damage: int = 0, name: str = "", lore_csv: str = "") -> bool:
        with self._lock:
            inv = self._inv.get(player_name)
            if inv is None or not (0 <= int(slot) < FAKE_SLOTS):
                return False
            inv.slots[int(slot)] = _Entry(str(item_type), max(1, int(count)), int(damage), str(name), str(lore_csv))
            player = self._player(player_name)
            content = self._content(inv)
        if player is None:
            return False
        return self._send(player, content)

    def refresh(self, player_name: str) -> bool:
        with self._lock:
            inv = self._inv.get(player_name)
            player = self._player(player_name)
            if inv is None or player is None:
                return False
            content = self._content(inv)
            inv.next_refresh = self._plugin.tick_count + inv.interval
        return self._send(player, content)

    def clear(self, player_name: str) -> bool:
        with self._lock:
            inv = self._inv.pop(player_name, None)
        if inv is None:
            return False
        player = self._player(player_name)
        if player is None:
            return True
        # 用空内容覆盖回去（客户端会按真实背包重画）
        empty = _FakeInv(player_name, inv.interval)
        return self._send(player, self._content(empty))

    def clear_all(self) -> int:
        with self._lock:
            names = list(self._inv.keys())
        for name in names:
            self.clear(name)
        return len(names)

    def get_players(self) -> list[str]:
        with self._lock:
            return sorted(self._inv.keys())

    def is_active(self, player_name: str) -> bool:
        with self._lock:
            return player_name in self._inv

    # 需要拦入站点击（未实现）
    def poll_clicks(self) -> list[str]:
        return []

    def clear_clicks(self) -> None:
        return None

    def dropped_clicks(self) -> int:
        return 0

    # ── 每 tick ───────────────────────────────────────────────────────────
    def tick(self) -> None:
        now = self._plugin.tick_count
        with self._lock:
            due = [inv for inv in self._inv.values() if now >= inv.next_refresh]
        for inv in due:
            try:
                self.refresh(inv.player)
            except Exception as exc:
                self._plugin.logger.debug(f"fakeinv refresh 失败 ({inv.player}): {exc}")
