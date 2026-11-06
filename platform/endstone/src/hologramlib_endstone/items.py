"""物品名 → 数字 id 查表。"""

from __future__ import annotations

import json
import threading
from pathlib import Path

_DATA_DIR = Path(__file__).resolve().parent / "data"
_LOCK = threading.RLock()
_TABLE: dict[str, int] | None = None
_META: dict = {}


def _load() -> dict[str, int]:
    global _TABLE, _META
    with _LOCK:
        if _TABLE is not None:
            return _TABLE
        path = _DATA_DIR / "item_ids.json"
        try:
            raw = json.loads(path.read_text(encoding="utf-8"))
            _TABLE = {str(k): int(v) for k, v in raw.get("items", {}).items()}
            _META = {"minecraft": raw.get("minecraft", "?"), "count": raw.get("count", len(_TABLE))}
        except Exception:
            _TABLE = {}
            _META = {"minecraft": "missing", "count": 0}
        return _TABLE


def numeric_id(name: str) -> int:
    """物品名 → 数字 id；未知返回 -1。支持 'stone' 与 'minecraft:stone' 两种写法。"""
    key = str(name).strip()
    if not key:
        return -1
    if ":" not in key:
        key = "minecraft:" + key
    table = _load()
    value = table.get(key)
    if value is None:
        value = table.get(key.lower())
    return int(value) if value is not None else -1


def table_info() -> str:
    table = _load()
    return f"item_ids: {len(table)} 条 (bedrock {_META.get('minecraft', '?')})"


def known(name: str) -> bool:
    return numeric_id(name) >= 0
