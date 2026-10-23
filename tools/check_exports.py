#!/usr/bin/env python3
"""check_exports.py —— LSE 导出清单的一致性检查（源码口径）

为什么要有这个：导出名重复会让后注册的静默覆盖前面的，运行时才发现；
文档里的「一共 N 个导出」也反复和源码对不上（README 写 273 时源码已经是 298）。
两条都是可以在 CI 里用几十毫秒挡住的。

用法：
    python tools/check_exports.py                # 重名检查 + 打印清单
    python tools/check_exports.py --check-docs   # 再把 README / docs/index.md 里的数字对一遍
    python tools/check_exports.py --expect 314   # 固定数量（可选）

退出码：0 = 通过，1 = 有问题。
"""
from __future__ import annotations

import argparse
import io
import os
import re
import sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

# exportAs(NAMESPACE, "name" —— 允许换行、允许第一个实参是任意标识符
EXPORT_RE = re.compile(r'exportAs\s*\(\s*[^,\n]+,\s*"([A-Za-z0-9_]+)"')

# 文档里出现导出总数的写法：「（273 个导出）」「全部 **273 个脚本函数**」
DOC_NUMBER_RES = [
    re.compile(r"(\d+)\s*个导出"),
    re.compile(r"(\d+)\s*个脚本函数"),
]
DOC_FILES = ["README.md", os.path.join("docs", "index.md")]


def collect() -> list[tuple[str, str]]:
    """返回 [(文件相对路径, 导出名)]"""
    found: list[tuple[str, str]] = []
    for root, _dirs, files in os.walk(SRC):
        for name in files:
            if not name.endswith(".cpp"):
                continue
            path = os.path.join(root, name)
            text = io.open(path, encoding="utf-8", errors="replace").read()
            rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
            for match in EXPORT_RE.finditer(text):
                found.append((rel, match.group(1)))
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description="LSE 导出清单一致性检查")
    ap.add_argument("--check-docs", action="store_true", help="核对 README / docs/index.md 里的导出总数")
    ap.add_argument("--expect", type=int, default=0, help="期望的导出总数（0 = 不校验）")
    args = ap.parse_args()

    items = collect()
    names = [n for _f, n in items]
    total = len(names)
    per_file = Counter(f for f, _n in items)

    print(f"[exports] 源码口径: {total} 个导出，分布在 {len(per_file)} 个文件")
    for path, count in sorted(per_file.items(), key=lambda kv: (-kv[1], kv[0])):
        print(f"  {count:3d}  {path}")

    problems: list[str] = []

    duplicates = sorted(n for n, c in Counter(names).items() if c > 1)
    if duplicates:
        for dup in duplicates:
            where = [f for f, n in items if n == dup]
            problems.append(f"导出名重复: {dup}（{', '.join(where)}）")
    else:
        print("[exports] 重名: 无")

    if args.expect and total != args.expect:
        problems.append(f"导出总数 {total} != 期望 {args.expect}")

    if args.check_docs:
        for rel in DOC_FILES:
            path = os.path.join(ROOT, rel)
            if not os.path.exists(path):
                problems.append(f"文档不存在: {rel}")
                continue
            text = io.open(path, encoding="utf-8", errors="replace").read()
            numbers = []
            for rx in DOC_NUMBER_RES:
                numbers += [int(m.group(1)) for m in rx.finditer(text)]
            numbers = sorted(set(numbers))
            if not numbers:
                problems.append(f"{rel}: 没找到导出总数（应写明「{total} 个导出」）")
            elif total not in numbers:
                problems.append(f"{rel}: 文档写 {numbers}，源码是 {total}")
            else:
                print(f"[exports] {rel}: 文档数字与源码一致（{total}）")

    if problems:
        print("\n[exports] 有问题：")
        for p in problems:
            print("  -", p)
        return 1

    print("\n[exports] 通过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
