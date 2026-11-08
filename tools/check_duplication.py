#!/usr/bin/env python3
"""check_duplication.py —— 8 行窗口查重（回归门槛）。

只统计"像代码"的行：跳过 include / 预处理、空行、注释、单独的 `{` `}`/`namespace ... {`、
以及纯转发（`return xxx;` 单行）。这样剩下的重复才是真的逻辑复制。

用法：
    python tools/check_duplication.py                 # 打印统计 + Top 簇
    python tools/check_duplication.py --update        # 写回基线 tools/duplication-baseline.json
    python tools/check_duplication.py --check         # 与基线比，超过阈值退出码 1
    python tools/check_duplication.py --w 10 --top 30 # 换窗口/展示条数
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
BASELINE = ROOT / "tools" / "duplication-baseline.json"
ROOTS = ("core", "platform/levilamina/src", "include/hologramlib")
EXTS = (".h", ".cpp")
NOISE = re.compile(
    r"^(#|namespace\b|\{|\}|else\b|public:|private:|protected:|"
    r"return\s+[\w:\.\(\)]+;|break;|continue;|//|/\*|\*)"
)


def sh(text: str, maxlen: int = 120) -> str:
    return text if len(text) <= maxlen else text[: maxlen - 1] + "…"


def collect() -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    for base in ROOTS:
        for path in (ROOT / base).rglob("*"):
            if path.suffix in EXTS and path.is_file():
                files.append(path)
    return sorted(files)


def normalize(line: str) -> str | None:
    line = line.strip()
    if not line or NOISE.match(line):
        return None
    return re.sub(r"\s+", " ", line)


def scan(window: int) -> tuple[int, dict[str, list[tuple[str, int]]], dict[str, list[str]]]:
    windows: dict[str, list[tuple[str, int]]] = collections.defaultdict(list)
    texts: dict[str, list[str]] = {}
    for path in collect():
        rel = str(path.relative_to(ROOT))
        lines = [x for x in (normalize(l) for l in path.read_text(encoding="utf-8", errors="replace").splitlines()) if x]
        texts[rel] = lines
        for i in range(len(lines) - window + 1):
            digest = hashlib.sha1("\n".join(lines[i : i + window]).encode()).hexdigest()
            windows[digest].append((rel, i))
    dups = {h: v for h, v in windows.items() if len({f for f, _ in v}) > 1}
    return len(dups), dups, texts


def top_pairs(dups: dict[str, list[tuple[str, int]]], n: int) -> list[tuple[int, str, str]]:
    pairs: collections.Counter[tuple[str, str]] = collections.Counter()
    for locations in dups.values():
        files = sorted({f for f, _ in locations})
        for i in range(len(files)):
            for j in range(i + 1, len(files)):
                pairs[(files[i], files[j])] += 1
    return [(count, a, b) for (a, b), count in pairs.most_common(n)]


def main() -> int:
    parser = argparse.ArgumentParser(description="8 行窗口查重")
    parser.add_argument("--w", type=int, default=8, help="窗口行数，默认 8")
    parser.add_argument("--top", type=int, default=15, help="展示前 N 个文件对")
    parser.add_argument("--update", action="store_true", help="写回基线")
    parser.add_argument("--check", action="store_true", help="与基线比对")
    parser.add_argument("--json", action="store_true", help="输出 JSON")
    args = parser.parse_args()

    total, dups, _texts = scan(args.w)
    pairs = top_pairs(dups, args.top)

    if args.json:
        print(json.dumps({"window": args.w, "duplicate_windows": total,
                          "top_pairs": [{"count": c, "a": a, "b": b} for c, a, b in pairs]},
                         ensure_ascii=False, indent=1))
    else:
        print(f"窗口 {args.w} 行：重复窗口 {total}")
        for count, a, b in pairs:
            print(f"  {count:4d}  {a}  <->  {b}")

    if args.update:
        BASELINE.write_text(json.dumps({"window": args.w, "duplicate_windows": total}, indent=1) + "\n",
                            encoding="utf-8")
        print(f"基线已写入 {BASELINE.relative_to(ROOT)}")
        return 0

    if args.check:
        if not BASELINE.exists():
            print("没有基线文件，先跑 --update")
            return 2
        base = json.loads(BASELINE.read_text(encoding="utf-8"))
        limit = int(base["duplicate_windows"]) + int(base.get("allow_growth", 0))
        if total > limit:
            print(f"重复窗口 {total} 超过基线 {base['duplicate_windows']}")
            return 1
        print(f"通过：{total} <= 基线 {base['duplicate_windows']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
