#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""发布不变量检查: HologramLib 是纯前置库, 正式产物不得携带诊断日志。

用法: python check-no-diagnostics.py <diag_off.obj> <diag_on.obj> <HologramLib.dll>

1. 宏极性: 关闭诊断时 marker 串不得进目标文件; 打开 (-DHOLOGLIB_DIAG_LOG=1) 时必须进
2. 产物不变量: DLL 里不得出现任何库内日志前缀/启动串
"""
import sys
from pathlib import Path

MARKER = b"[HOLOGLIB-DIAG-MARKER]"

# 库内日志前缀与启动串 —— 正式产物里一个都不该有
DLL_MARKERS = [
    b"[NpcDialog]",
    b"[TradeMenu]",
    b"[PlayerNpc]",
    b"[CustomEntity]",
    b"[ItemDisplay]",
    b"[DebugDrawer]",
    b"HologramLib enabling",
    b"HologramLib disabled",
    b"Exporting DebugShape",
    b"Exporting FloatingText",
    b"Exporting GradientLine",
    b"sculk packet validation failed",
    b"PlayerList 2168 framing rejected",
]

FAIL = []
OK = []


def check(cond, msg):
    (OK if cond else FAIL).append(msg)
    print(("  OK   " if cond else "  FAIL ") + msg)


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    off, on, dll = (Path(p) for p in sys.argv[1:4])

    print("== 宏极性 (if constexpr 开关) ==")
    off_bytes = off.read_bytes()
    on_bytes = on.read_bytes()
    check(off_bytes.count(MARKER) == 0,
          "关闭诊断: marker 未进目标文件 (%s, %d B)" % (off.name, len(off_bytes)))
    check(on_bytes.count(MARKER) > 0,
          "打开诊断 (-DHOLOGLIB_DIAG_LOG=1): marker 已进目标文件 (%d 次)" % on_bytes.count(MARKER))
    check(len(on_bytes) >= len(off_bytes),
          "打开后目标文件不小于关闭时 (%d >= %d B)" % (len(on_bytes), len(off_bytes)))

    print("== 产物不变量 (%s, %d B) ==" % (dll.name, dll.stat().st_size))
    dll_bytes = dll.read_bytes()
    for m in DLL_MARKERS:
        check(dll_bytes.count(m) == 0, "无日志串 %-34s" % m.decode())

    print("\n%d passed, %d failed" % (len(OK), len(FAIL)))
    return 1 if FAIL else 0


if __name__ == "__main__":
    sys.exit(main())
