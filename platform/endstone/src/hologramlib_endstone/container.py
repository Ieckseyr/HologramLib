"""虚拟容器（Container）域 —— LeviLamina 版 ContainerMenuManager 的 endstone 移植（非交互子集）。"""

from __future__ import annotations

import threading

from . import items
from .holograms import _dim_id
from .native import ItemSlot

CONTAINER_TYPE = 0  # ContainerType::Container

FIRST_CONTAINER_ID = 2
_MAX_CONTAINER_SLOTS = 54


class _Menu:
    __slots__ = ("id", "player", "title", "rows", "x", "y", "z", "carrier_uid", "carrier_rid", "slots", "interactive", "blocked")

    def __init__(self, menu_id: int, player: str, title: str, rows: int, x: int, y: int, z: int,
                 uid: int, rid: int) -> None:
        self.id = menu_id
        self.player = player
        self.title = title
        self.rows = rows
        self.x, self.y, self.z = x, y, z
        self.carrier_uid = uid
        self.carrier_rid = rid
        self.slots: list[tuple[str, int, int, str, str]] = []  # (type, count, damage, name, lore)
        self.interactive = False
        self.blocked = (0, 0)

    @property
    def size(self) -> int:
        return min(_MAX_CONTAINER_SLOTS, max(9, self.rows * 9))


class ContainerApi:
    FIRST_RUNTIME_ID = 0x71000000 + 0x10000  # 避开 bossBar 段
    FIRST_UNIQUE_ID = 0x300000

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._menus: dict[int, _Menu] = {}
        self._next_menu = 1
        self._next_container_id = FIRST_CONTAINER_ID
        self._next_runtime = self.FIRST_RUNTIME_ID
        self._next_unique = self.FIRST_UNIQUE_ID
        self._clicks: list[str] = []

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
            self._plugin.logger.debug(f"container 发包失败 ({player.name}): {exc}")
            return False

    def _content(self, menu: _Menu) -> bytes:
        slots: list[ItemSlot] = []
        for index in range(menu.size):
            slot = ItemSlot()
            if index < len(menu.slots):
                item_type, count, damage, name, lore = menu.slots[index]
                numeric = items.numeric_id(item_type)
                slot.item_id = numeric if numeric and numeric > 0 else 0
                slot.stack_size = max(0, int(count))
                slot.aux = int(damage) if damage else 0
                if name or lore:
                    lore_list = [line for line in str(lore).split(";") if line] if lore else []
                    nbt = self._plugin.core.build_item_nbt(item_type, int(count), int(damage), str(name), lore_list)
                    slot.user_data = nbt
                    slot.user_data_len = len(nbt)
            else:
                slot.item_id = 0
                slot.stack_size = 0
                slot.aux = 0
            slots.append(slot)
        return self._plugin.core.encode_inventory_content(menu.id, slots)

    def _send_open(self, menu: _Menu, player) -> bool:
        body = self._plugin.core.encode_add_actor_ex(
            "minecraft:chest_minecart", menu.title, menu.carrier_rid, menu.carrier_uid,
            float(menu.x), float(menu.y), float(menu.z), 0.0, 0.0, 0.0, 0.0,
        )
        self._send(player, self._plugin.core.add_actor_packet_id, body)
        body = self._plugin.core.encode_container_open(menu.id, CONTAINER_TYPE, menu.x, menu.y, menu.z, -1)
        if not self._send(player, self._plugin.core.container_open_packet_id, body):
            return False
        self._send(player, self._plugin.core.inventory_content_packet_id, self._content(menu))
        return True

    def get(self, menu_id: int) -> _Menu | None:
        with self._lock:
            return self._menus.get(menu_id)

    # ── 对外 ──────────────────────────────────────────────────────────────
    def open(self, player_name: str, title: str, rows: int = 3) -> int:
        player = self._player(player_name)
        if player is None:
            return -1
        with self._lock:
            menu_id = self._next_menu
            self._next_menu += 1
            container_id = self._next_container_id
            self._next_container_id = FIRST_CONTAINER_ID + ((container_id - FIRST_CONTAINER_ID + 1) % 90)
            runtime_id = self._next_runtime
            self._next_runtime += 1
            unique_id = self._next_unique
            self._next_unique += 1
            # 载体放在玩家脚下一格，y 用 -66 这种远离世界的位置（与 LL 一致）
            menu = _Menu(menu_id, player_name, str(title), int(rows), 0, -66, 0, unique_id, runtime_id)
            menu.id = container_id
            self._menus[menu_id] = menu
        menu.id = container_id
        if not self._send_open(menu, player):
            with self._lock:
                self._menus.pop(menu_id, None)
            return -1
        return menu_id

    def close(self, menu_id: int) -> bool:
        with self._lock:
            menu = self._menus.pop(menu_id, None)
        if menu is None:
            return False
        player = self._player(menu.player)
        if player is not None:
            self._send(player, self._plugin.core.container_close_packet_id,
                       self._plugin.core.encode_container_close(menu.id, CONTAINER_TYPE, True))
        return True

    def close_all(self) -> int:
        with self._lock:
            ids = list(self._menus.keys())
        for menu_id in ids:
            self.close(menu_id)
        return len(ids)

    def get_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._menus.keys())

    def is_open(self, menu_id: int) -> bool:
        with self._lock:
            return menu_id in self._menus

    def set_item(self, menu_id: int, slot: int, item_type: str, count: int, damage: int = 0, name: str = "", lore_csv: str = "") -> bool:
        return self._set_slot(menu_id, slot, item_type, count, damage, name, lore_csv)

    def set_item_enchanted(self, menu_id: int, slot: int, item_type: str, count: int, damage: int, name: str, lore_csv: str, ench_csv: str = "") -> bool:
        return self._set_slot(menu_id, slot, item_type, count, damage, name, lore_csv)

    def _set_slot(self, menu_id: int, slot: int, item_type: str, count: int, damage: int, name: str, lore: str) -> bool:
        with self._lock:
            menu = self._menus.get(menu_id)
            if menu is None:
                return False
            index = int(slot)
            if index < 0 or index >= menu.size:
                return False
            entry = (str(item_type), max(1, int(count)), int(damage), str(name), str(lore))
            while len(menu.slots) <= index:
                menu.slots.append(("", 0, 0, "", ""))
            menu.slots[index] = entry
            player = self._player(menu.player)
            if player is None:
                return True
            content = self._content(menu)
        return self._send(player, self._plugin.core.inventory_content_packet_id, content)

    def set_title(self, menu_id: int, title: str) -> bool:
        return self.update(menu_id, title)

    def update(self, menu_id: int, title: str) -> bool:
        with self._lock:
            menu = self._menus.get(menu_id)
            if menu is None:
                return False
            menu.title = str(title)
            player = self._player(menu.player)
        if player is None:
            return True
        body = self._plugin.core.encode_add_actor_ex(
            "minecraft:chest_minecart", menu.title, menu.carrier_rid, menu.carrier_uid,
            float(menu.x), float(menu.y), float(menu.z), 0.0, 0.0, 0.0, 0.0,
        )
        return self._send(player, self._plugin.core.add_actor_packet_id, body)

    def preview(self, player_name: str, dim: int, x: float, y: float, z: float, title: str = "") -> int:
        _ = dim
        return self.open(player_name, title or "容器预览", 6)

    def preview_titled(self, player_name: str, dim: int, x: float, y: float, z: float, title: str) -> int:
        return self.preview(player_name, dim, x, y, z, title)

    def set_interactive(self, menu_id: int, enabled: bool) -> bool:
        with self._lock:
            menu = self._menus.get(menu_id)
            if menu is None:
                return False
            menu.interactive = bool(enabled)
            return True

    def block_event(self, menu_id: int, b0: int = 0, b1: int = 0) -> bool:
        """屏蔽某个菜单的容器事件：记录后由出站/入站过滤处理（当前只记录）。"""
        with self._lock:
            menu = self._menus.get(int(menu_id))
            if menu is None:
                return False
            menu.blocked = (int(b0), int(b1))
            return True

    # 以下三个需要拦入站 ItemStackRequest（未实现），保持空实现让调用方不炸
    def poll_clicks(self) -> list[str]:
        with self._lock:
            out = list(self._clicks)
            self._clicks.clear()
        return out

    def clear_clicks(self) -> None:
        with self._lock:
            self._clicks.clear()

    def dropped_clicks(self) -> int:
        return 0
