#!/usr/bin/env python3
"""pack_release.py —— 把 bin/<mod>/ 打成发布包 bin/<mod>.zip。

产出两个包（与仓库既有 Release 的文件名一致）：

  bin/HologramLib.zip        Lip 安装用的主包，不含 PDB
    HologramLib/
      HologramLib.dll
      manifest.json
      lib/HologramLib.js       ← 脚本 require 这个
      lib/HologramLib.d.ts     ← 编辑器补全
      lib/exports.json         ← 机器可读清单

  bin/HologramLib-pdb.zip    只含 PDB，排查崩溃栈时解压覆盖用

用法：
  python tools/pack_release.py            # 打两个包
  python tools/pack_release.py --check    # 只检查发布目录是否齐全（CI 用）
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
PDB_ZIP = os.path.join(ROOT, "bin", MOD + "-pdb.zip")
REQUIRED = [
    "HologramLib.dll",
    "manifest.json",
    os.path.join("lib", "HologramLib.js"),
    os.path.join("lib", "HologramLib.d.ts"),
    os.path.join("lib", "exports.json"),
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

    main_entries: list[tuple[str, str]] = []
    pdb_entries: list[tuple[str, str]] = []
    for root, _dirs, files in os.walk(BIN):
        for name in sorted(files):
            full = os.path.join(root, name)
            rel = os.path.relpath(full, BIN).replace("\\", "/")
            item = (MOD + "/" + rel, full)
            if rel.lower().endswith(".pdb"):
                pdb_entries.append(item)
            else:
                main_entries.append(item)
    main_entries.sort()
    pdb_entries.sort()

    def write_zip(path: str, entries: list[tuple[str, str]], title: str) -> None:
        with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as zf:
            for arcname, full in entries:
                zf.write(full, arcname)
        size = os.path.getsize(path)
        print(f"[pack] 写出 {os.path.relpath(path, ROOT)}（{title}，{len(entries)} 个文件，{size / 1024:.0f} KB）")
        for arcname, _ in entries:
            print("  " + arcname)

    write_zip(ZIP, main_entries, "Lip 主包")
    if pdb_entries:
        write_zip(PDB_ZIP, pdb_entries, "PDB 单独包")
    else:
        print("[pack] 没有 PDB，跳过 PDB 包")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="打发布包")
    ap.add_argument("--check", action="store_true", help="只检查发布目录是否齐全")
    args = ap.parse_args()
    return check() if args.check else pack()


if __name__ == "__main__":
    sys.exit(main())
