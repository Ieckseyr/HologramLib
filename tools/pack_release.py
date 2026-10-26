#!/usr/bin/env python3
"""pack_release.py —— 把 bin/<mod>/ 打成发布包 bin/<mod>.zip。

发布包里除了 DLL / PDB / manifest.json，还必须带上脚本导入层（三个文件，和 DLL 平级）：

    HologramLib/
      HologramLib.dll
      HologramLib.pdb
      manifest.json
      HologramLib.js       ← 脚本 require 这个（和 DLL 平级）
      HologramLib.d.ts     ← 编辑器补全
      exports.json         ← 机器可读清单

用法：
  python tools/pack_release.py            # 打 bin/HologramLib.zip
  python tools/pack_release.py --check    # 只检查导入层在不在发布目录里（CI 用）
"""
from __future__ import annotations

import argparse
import io
import os
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOD = "HologramLib"
BIN = os.path.join(ROOT, "bin", MOD)
ZIP = os.path.join(ROOT, "bin", MOD + ".zip")
REQUIRED = [
    "HologramLib.dll",
    "manifest.json",
    "HologramLib.js",
    "HologramLib.d.ts",
    "exports.json",
]


def check() -> int:
    missing = [rel for rel in REQUIRED if not os.path.isfile(os.path.join(BIN, rel))]
    if missing:
        print("[pack] bin/%s/ 里缺文件：" % MOD)
        for rel in missing:
            print("  - " + rel)
        print("[pack] 跑一次 xmake build（after_build 会把 lse/ 拷进来），或 python tools/gen_lse_shim.py")
        return 1
    print("[pack] 发布目录齐全（含脚本导入层 3 个文件）")
    return 0


def pack() -> int:
    if check() != 0:
        return 1

    entries: list[tuple[str, str]] = []
    for root, _dirs, files in os.walk(BIN):
        for name in sorted(files):
            full = os.path.join(root, name)
            rel = os.path.relpath(full, BIN).replace("\\", "/")
            entries.append((MOD + "/" + rel, full))
    entries.sort()

    with zipfile.ZipFile(ZIP, "w", zipfile.ZIP_DEFLATED) as zf:
        for arcname, full in entries:
            zf.write(full, arcname)

    size = os.path.getsize(ZIP)
    print(f"[pack] 写出 {os.path.relpath(ZIP, ROOT)}（{len(entries)} 个文件，{size / 1024:.0f} KB）")
    for arcname, _ in entries:
        print("  " + arcname)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="打发布包")
    ap.add_argument("--check", action="store_true", help="只检查发布目录是否齐全")
    args = ap.parse_args()
    return check() if args.check else pack()


if __name__ == "__main__":
    sys.exit(main())
