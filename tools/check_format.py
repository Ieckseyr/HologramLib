#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_format.py —— 只检查**改动的行**是否符合 .clang-format

为什么不是 `clang-format --dry-run --Werror <整文件>`：
本仓库没有做过一次性格式化（.clang-format 注释里写明了这个取舍），所以整文件检查一定会红
——实测 CustomEntityManager.cpp 单文件 298 处、ItemDisplayManager.cpp 377 处、
PlayerNpcManager.cpp 203 处历史 violation，与这次改动无关。只查改动行才能让门禁有意义。

用法:
    python tools/check_format.py                 # 与 HEAD~1 比（CI 默认）
    python tools/check_format.py --base origin/main
    python tools/check_format.py --staged        # 暂存区（本地提交前）
    python tools/check_format.py --all           # 整文件（等一次性格式化落地后改用这个）

退出码: 0 = 干净；1 = 有 violation；2 = 环境问题（缺 clang-format / 不是 git 仓库）。
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

for _stream in (sys.stdout, sys.stderr):
    try:
        _stream.reconfigure(encoding="utf-8", errors="replace")
    except Exception:  # noqa: BLE001
        pass

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXTS = (".cpp", ".h", ".hpp")
HUNK_RE = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")


def git(*args):
    return subprocess.run(
        ["git", "-C", ROOT, *args], capture_output=True, text=True, encoding="utf-8", errors="replace"
    )


def changed_files(base, staged):
    if staged:
        out = git("diff", "--cached", "--name-only", "--diff-filter=ACMR").stdout
    else:
        out = git("diff", "--name-only", "--diff-filter=ACMR", f"{base}...HEAD").stdout
    return [f.strip() for f in out.split("\n") if f.strip().endswith(EXTS)]


def changed_line_ranges(base, staged, path):
    """返回该文件在新版本里的改动行区间 [(start, end), ...]；整文件新增 = 一整段。"""
    if staged:
        diff = git("diff", "--cached", "-U0", "--", path).stdout
    else:
        diff = git("diff", "-U0", f"{base}...HEAD", "--", path).stdout
    ranges = []
    for line in diff.split("\n"):
        m = HUNK_RE.match(line)
        if not m:
            continue
        start = int(m.group(1))
        count = int(m.group(2)) if m.group(2) is not None else 1
        if count > 0:
            ranges.append((start, start + count - 1))
    # 合并相邻/重叠区间，减少 clang-format 调用次数
    ranges.sort()
    merged = []
    for s, e in ranges:
        if merged and s <= merged[-1][1] + 1:
            merged[-1] = (merged[-1][0], max(merged[-1][1], e))
        else:
            merged.append((s, e))
    return merged


def main():
    ap = argparse.ArgumentParser(description="只检查改动行的 clang-format 门禁")
    ap.add_argument("--base", default="HEAD~1", help="与哪个起点比较（默认 HEAD~1）")
    ap.add_argument("--staged", action="store_true", help="检查暂存区而不是 base...HEAD")
    ap.add_argument("--all", action="store_true", help="整文件检查（默认只查改动行）")
    ap.add_argument("--fix", action="store_true", help="就地只格式化改动的行（不做整文件重排）")
    args = ap.parse_args()

    exe = shutil.which("clang-format")
    if exe is None:
        print("[format] 找不到 clang-format；装一个再跑（Windows: winget install LLVM.LLVM / pip install clang-format）")
        return 2

    if git("rev-parse", "--is-inside-work-tree").returncode != 0:
        print("[format] 不是 git 仓库")
        return 2

    files = changed_files(args.base, args.staged)
    if not files:
        print("[format] 没有 C++ 改动")
        return 0

    failed = []
    for path in files:
        full = os.path.join(ROOT, path)
        if not os.path.exists(full):
            continue  # 已删除
        cmd = [exe, "--style=file", "--dry-run", "--Werror"]
        if not args.all:
            ranges = changed_line_ranges(args.base, args.staged, path)
            if not ranges:
                continue
            for s, e in ranges:
                cmd.append(f"--lines={s}:{e}")
        res = subprocess.run(cmd + [full], capture_output=True, text=True, encoding="utf-8", errors="replace")
        if res.returncode != 0:
            detail = (res.stdout + res.stderr).strip()
            failed.append((path, detail))

    if failed and args.fix:
        # 只把**改动的行**就地格式化（不动文件其余部分，避免整文件重排的噪音）
        for path, _detail in failed:
            full = os.path.join(ROOT, path)
            fix = [exe, "-i", "--style=file"]
            if not args.all:
                for s, e in changed_line_ranges(args.base, args.staged, path):
                    fix.append(f"--lines={s}:{e}")
            subprocess.run(fix + [full], capture_output=True, text=True, encoding="utf-8", errors="replace")
            print(f"[format] 已就地格式化改动行: {path}")
        print("[format] 修完请重新 `git add` 后再跑一次门禁")
        return 0

    if failed:
        print(f"[format] {len(failed)} 个文件有格式问题（只看改动行）：")
        for path, detail in failed:
            print(f"\n--- {path}")
            for line in detail.split("\n")[:20]:
                print(f"    {line}")
        print("\n修法: python tools/check_format.py --fix（只改改动的行）后重新提交")
        return 1

    print(f"[format] 通过（{len(files)} 个文件，{'整文件' if args.all else '仅改动行'}）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
