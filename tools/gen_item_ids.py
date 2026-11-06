#!/usr/bin/env python3
"""生成 endstone 侧用的「物品名 -> 数字 id」表。

为什么需要：endstone 的 Python API 只暴露 `ItemType.id`（字符串）和 `ItemStack.data`（aux），
拿不到 BDS 的 `Item::mId`；而 AddItemActor / MobEquipment / ItemStackResponse 这些报文里的
`NetworkItemStackDescriptor` **只写数字 id、不写物品名**（见 BedrockProtocol
NetworkItemStackDescriptor::write）。没有这张表，物品域全部发不出去。

数据源：PrismarineJS/minecraft-data 的 bedrock/<version>/items.json（name + 数字 id）。
数字 id 对原版物品在同一 BDS 版本内稳定；换版本时重跑本脚本即可。

用法：
    python tools/gen_item_ids.py                       # 默认拉 1.26.60
    python tools/gen_item_ids.py --version 1.26.51
    python tools/gen_item_ids.py --source items.json   # 用本地文件（离线）
"""

from __future__ import annotations

import argparse
import json
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "platform" / "endstone" / "src" / "hologramlib_endstone" / "data" / "item_ids.json"
URL = "https://raw.githubusercontent.com/PrismarineJS/minecraft-data/master/data/bedrock/{version}/items.json"


def load(source: str, version: str) -> list:
    if source:
        raw = Path(source).read_bytes()
    else:
        url = URL.format(version=version)
        print(f"[item-ids] 下载 {url}")
        with urllib.request.urlopen(url, timeout=60) as response:
            raw = response.read()
    return json.loads(raw.decode("utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description="生成物品名 -> 数字 id 表")
    parser.add_argument("--version", default="1.26.60")
    parser.add_argument("--source", default="", help="本地 items.json 路径（离线用）")
    args = parser.parse_args()

    items = load(args.source, args.version)
    table: dict[str, int] = {}
    for entry in items:
        name = entry.get("name")
        item_id = entry.get("id")
        if not name or not isinstance(item_id, int):
            continue
        table[f"minecraft:{name}"] = item_id

    if not table:
        print("[item-ids] 没解析出任何条目", file=sys.stderr)
        return 1

    OUT.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "source": f"PrismarineJS/minecraft-data bedrock/{args.version}/items.json",
        "minecraft": args.version,
        "count": len(table),
        "items": dict(sorted(table.items())),
    }
    OUT.write_text(json.dumps(payload, ensure_ascii=False, indent=0) + "\n", encoding="utf-8")
    print(f"[item-ids] 写入 {OUT.relative_to(ROOT)}（{len(table)} 条，{OUT.stat().st_size} 字节）")
    for probe in ("minecraft:stone", "minecraft:diamond", "minecraft:totem_of_undying"):
        print(f"  {probe} -> {table.get(probe)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
