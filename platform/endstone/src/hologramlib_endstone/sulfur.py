"""硫磺方块（Sulfur）域 —— LeviLamina 版 SulfurDisplayManager 的 endstone 移植。"""

from __future__ import annotations

import threading

SULFUR_IDENTIFIER = "minecraft:sulfur_cube"
MAINHAND_SLOT = 0


class _Sulfur:
    __slots__ = ("id", "entity_id", "block_type", "block_count", "block_damage", "block_name", "archetype", "invisible", "scale")

    def __init__(self, sulfur_id: int, entity_id: int) -> None:
        self.id = sulfur_id
        self.entity_id = entity_id
        self.block_type = ""
        self.block_count = 1
        self.block_damage = 0
        self.block_name = ""
        self.archetype = ""
        self.invisible = False
        self.scale = 1.0


class SulfurApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._items: dict[int, _Sulfur] = {}
        self._next_id = 1

    # ── 内部 ──────────────────────────────────────────────────────────────
    def get(self, sulfur_id: int) -> _Sulfur | None:
        with self._lock:
            return self._items.get(sulfur_id)

    # ── 对外 ──────────────────────────────────────────────────────────────
    def create(
        self,
        x: float,
        y: float,
        z: float,
        dim: int = 0,
        block_type: str = "minecraft:stone",
        block_count: int = 1,
        block_damage: int = 0,
        block_name: str = "",
        archetype: str = "",
        invisible: bool = False,
    ) -> int:
        entity_id = self._plugin.entities.create(SULFUR_IDENTIFIER, x, y, z, dim)
        if entity_id < 0:
            return -1
        with self._lock:
            sulfur_id = self._next_id
            self._next_id += 1
            item = _Sulfur(sulfur_id, entity_id)
            item.block_type = str(block_type)
            item.block_count = int(block_count)
            item.block_damage = int(block_damage)
            item.block_name = str(block_name)
            item.archetype = str(archetype)
            item.invisible = bool(invisible)
            self._items[sulfur_id] = item
        self._apply(item)
        return sulfur_id

    def _apply(self, item: _Sulfur) -> bool:
        ok = self._plugin.entities.set_equipment_slot(
            item.entity_id, MAINHAND_SLOT, item.block_type, item.block_damage
        )
        self._plugin.entities.set_invisible(item.entity_id, item.invisible)
        self._plugin.entities.set_scale(item.entity_id, item.scale)
        return ok

    def destroy(self, sulfur_id: int) -> bool:
        with self._lock:
            item = self._items.pop(sulfur_id, None)
        if item is None:
            return False
        return self._plugin.entities.destroy(item.entity_id)

    def destroy_all(self) -> int:
        with self._lock:
            ids = list(self._items.keys())
        for sulfur_id in ids:
            self.destroy(sulfur_id)
        return len(ids)

    def exists(self, sulfur_id: int) -> bool:
        with self._lock:
            return sulfur_id in self._items

    def get_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._items.keys())

    def get_entity_id(self, sulfur_id: int) -> int:
        item = self.get(sulfur_id)
        return item.entity_id if item is not None else -1

    def set_block(self, sulfur_id: int, block_type: str, count: int = 1, damage: int = 0, name: str = "") -> bool:
        item = self.get(sulfur_id)
        if item is None:
            return False
        item.block_type = str(block_type)
        item.block_count = int(count)
        item.block_damage = int(damage)
        item.block_name = str(name)
        return self._apply(item)

    def set_invisible(self, sulfur_id: int, on: bool) -> bool:
        item = self.get(sulfur_id)
        if item is None:
            return False
        item.invisible = bool(on)
        return self._plugin.entities.set_invisible(item.entity_id, item.invisible)

    def set_scale(self, sulfur_id: int, scale: float) -> bool:
        item = self.get(sulfur_id)
        if item is None:
            return False
        item.scale = float(scale)
        return self._plugin.entities.set_scale(item.entity_id, item.scale)

    def set_archetype(self, sulfur_id: int, archetype: str) -> bool:
        item = self.get(sulfur_id)
        if item is None:
            return False
        item.archetype = str(archetype)
        return True  # mob property 同步未实现，只记录
