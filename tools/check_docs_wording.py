#!/usr/bin/env python3
"""check_docs_wording.py —— 文档站上不允许出现的措辞。

当前只拦一类：「未实测」式的免责声明（还没实测 / 尚未实测 / 未实测 / 没实测 / 待实测 / 未验证）。
这类话对读文档的人没有信息量，维护者要记的"哪些还没验"属于 `docs/internal/`（不上站）。

用法：
  python tools/check_docs_wording.py          # 检查
  python tools/check_docs_wording.py -v       # 连命中的行一起打出来
"""
from __future__ import annotations

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCS = os.path.join(ROOT, "docs")
# 维护者内参不上站，允许出现这类备注
ALLOW_DIRS = ("internal",)

BANNED = [
    ("未实测式声明", re.compile(r"还没实测|尚未实测|未实测|没实测|待实测|未验证|未验过|还没验")),
]


def main() -> int:
    ap = argparse.ArgumentParser(description="检查文档措辞")
    ap.add_argument("-v", "--verbose", action="store_true", help="打印命中的行")
    args = ap.parse_args()

    hits: list[tuple[str, int, str, str]] = []
    for root, _dirs, files in os.walk(DOCS):
        if any(part in ALLOW_DIRS for part in os.path.relpath(root, DOCS).split(os.sep)):
            continue
        for fname in sorted(files):
            if not fname.endswith(".md"):
                continue
            path = os.path.join(root, fname)
            rel = os.path.relpath(path, ROOT).replace("\\", "/")
            for lineno, line in enumerate(io_lines(path), 1):
                for label, pattern in BANNED:
                    if pattern.search(line):
                        hits.append((rel, lineno, label, line.strip()))

    if not hits:
        print("[docs-wording] 通过：站内文档没有禁用措辞")
        return 0

    print(f"[docs-wording] {len(hits)} 处禁用措辞：")
    for rel, lineno, label, line in hits:
        print(f"  {rel}:{lineno}  [{label}]")
        if args.verbose:
            print(f"    {line}")
    print("[docs-wording] 维护者备注请写进 docs/internal/（不上站）")
    return 1


def io_lines(path: str):
    import io

    with io.open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            yield line


if __name__ == "__main__":
    sys.exit(main())
