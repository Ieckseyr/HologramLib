#!/usr/bin/env python3
"""一条命令出两个平台产物。

    python build.py                 # LeviLamina DLL + holo_core.dll + endstone wheel
    python build.py --bds=26.51     # 编 26.51 线（协议 2193）；不写则沿用上次 configure 的线
    python build.py --deploy C:\\srv\\plugins   # 额外把 wheel 丢进服务端 plugins/
    python build.py --only endstone # 只打 endstone（已编过 holo_core 时）

产物：
    HologramLib.dll / .pdb / .lib   LeviLamina 版（部署到插件目录 + bin/HologramLib）
    dist/endstone_hologramlib-*.whl  endstone 版（内嵌 holo_core.dll）

两个平台共用 core/ 里的协议编码，形状报文只有一份实现。
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def run(cmd: list[str], cwd: Path = ROOT) -> int:
    print(f"\n[build] {' '.join(cmd)}")
    return subprocess.call(cmd, cwd=str(cwd))


def have(tool: str) -> bool:
    return shutil.which(tool) is not None


def main() -> int:
    parser = argparse.ArgumentParser(description="构建 HologramLib（LeviLamina + endstone）")
    parser.add_argument("--mode", default="release", choices=["release", "debug"])
    parser.add_argument("--only", default="all", choices=["all", "levilamina", "endstone"])
    parser.add_argument("--deploy", default=None, help="endstone 服务端的 plugins 目录")
    parser.add_argument("--configure", action="store_true", help="强制重新 configure")
    parser.add_argument(
        "--bds",
        default=None,
        choices=["26.51", "26.60"],
        help="目标 BDS 版本线（默认沿用上次 configure 的值；26.51 = 协议 2193 向下兼容线）",
    )
    args = parser.parse_args()

    if args.only in ("all", "levilamina", "endstone") and not have("xmake"):
        print("[build] 找不到 xmake，请先安装：https://xmake.io", file=sys.stderr)
        return 1

    build_ll = args.only in ("all", "levilamina")
    build_core = args.only in ("all", "endstone")

    if build_ll or build_core:
        # 模式只在 configure 阶段设置；`xmake build` 不接受 -m
        cfg = ["xmake", "f", "-y", "-m", args.mode]
        if args.bds:
            cfg += [f"--bds={args.bds}"]
        rc = run(cfg)
        if rc != 0:
            return rc
        targets = []
        if build_ll:
            targets.append("HologramLib")
        if build_core:
            targets.append("holo_core")
        rc = run(["xmake", "build", "-y", *targets])
        if rc != 0:
            return rc

    dl = shutil.which("python") or sys.executable
    pack = [dl, str(ROOT / "tools" / "pack_endstone.py")]
    if args.deploy:
        pack += ["--deploy", args.deploy]
    rc = run(pack)
    if rc != 0:
        return rc

    print("\n[build] 完成")
    return 0


if __name__ == "__main__":
    sys.exit(main())
