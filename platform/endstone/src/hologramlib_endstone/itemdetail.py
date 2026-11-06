"""物品详情（ItemDetail）域 —— LeviLamina 版 ItemDetailManager 的 endstone 移植。"""

from __future__ import annotations

import threading

from endstone.inventory import ItemType


class ItemDetailApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._details: dict[int, int] = {}  # detailId -> hologramId

    # ── 内部 ──────────────────────────────────────────────────────────────
    def resolve_item_name(self, item_id: str, aux: int = 0) -> str:
        _ = aux
        try:
            item_type = ItemType.get(str(item_id))
        except Exception:
            return str(item_id)
        if item_type is None:
            return str(item_id)
        key = getattr(item_type, "translation_key", None)
        if key:
            try:
                translated = self._plugin.server.language.translate(key)
            except Exception:
                translated = None
            if translated and translated != key:
                return translated
        return str(item_id)

    # ── 对外 ──────────────────────────────────────────────────────────────
    def show(
        self,
        dim_id: int,
        x: float,
        y: float,
        z: float,
        item_id: str,
        aux: int = 0,
        count: int = 1,
        custom_text: str = "",
    ) -> int:
        text = str(custom_text) if custom_text else self.resolve_item_name(item_id, aux)
        if not custom_text and int(count) > 1:
            text = f"{text} x{int(count)}"

        holo = self._plugin.holograms
        holo_id = holo.create(float(x), float(y), float(z))
        if holo_id < 0:
            return -1
        holo.add_line(holo_id, text)
        if not holo.draw_to_dimension(holo_id, int(dim_id)):
            holo.destroy(holo_id)
            return -1
        with self._lock:
            self._details[holo_id] = holo_id
        return holo_id

    def hide(self, detail_id: int) -> bool:
        with self._lock:
            holo_id = self._details.pop(int(detail_id), None)
        if holo_id is None:
            return False
        return self._plugin.holograms.destroy(holo_id)

    def __len__(self) -> int:
        with self._lock:
            return len(self._details)
