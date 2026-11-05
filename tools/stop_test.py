#!/usr/bin/env python3
"""启动/关服冒烟脚本：不依赖 PowerShell 桥，直接拉起 BDS，按时间点写 stdin，读退出码。

用法:
    python tools/stop_test.py                # 起服 → 22s → stop → 报告退出码
    python tools/stop_test.py --disable      # 起服 → 22s → ll disable HologramLib → stop
    python tools/stop_test.py --wait 30 --timeout 25
退出码: 0 = 服务端干净退出；其它 = 崩溃或超时（脚本会打印 crash 片段）。
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import threading
import time

SERVER_DIR = os.environ.get("BDS_DIR", r"E:\111FWQ\xb26.60.x")
EXE = os.path.join(SERVER_DIR, "bedrock_server_mod.exe")


def pump(stream, sink: list[str]) -> None:
    for raw in iter(stream.readline, b""):
        sink.append(raw.decode("utf-8", "replace").rstrip())
    stream.close()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--wait", type=int, default=22, help="启动后等多少秒再下命令")
    ap.add_argument("--timeout", type=int, default=25, help="stop 之后等多少秒退出")
    ap.add_argument("--disable", action="store_true", help="先 ll disable HologramLib 再 stop")
    args = ap.parse_args()

    if not os.path.isfile(EXE):
        print(f"找不到服务端: {EXE}", file=sys.stderr)
        return 2

    proc = subprocess.Popen(
        [EXE],
        cwd=SERVER_DIR,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    lines: list[str] = []
    reader = threading.Thread(target=pump, args=(proc.stdout, lines), daemon=True)
    reader.start()

    time.sleep(args.wait)
    if proc.poll() is not None:
        print(f"服务端提前退出，退出码={proc.returncode}")
        print("\n".join(lines[-20:]))
        return proc.returncode or 1

    if args.disable:
        proc.stdin.write(b"ll disable HologramLib\n")
        proc.stdin.flush()
        time.sleep(3)

    proc.stdin.write(b"stop\n")
    proc.stdin.flush()

    try:
        code = proc.wait(timeout=args.timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait(timeout=10)
        print(f"TIMEOUT: stop 之后 {args.timeout}s 仍未退出（已强杀）")
        code = "timeout"

    reader.join(timeout=5)

    crash = [
        ln
        for ln in lines
        if "at pc 0x" in ln or "Stacktrace" in ln or "Last Assembly" in ln or "崩溃" in ln and "已成功启用" not in ln
    ]
    print(f"EXIT={code}  crash_frames={len(crash)}")
    if crash:
        for ln in crash[:8]:
            print("  " + ln)

    tail = [ln for ln in lines if "禁用 HologramLib" in ln or "disabled" in ln]
    if tail:
        print("  " + tail[-1])

    return 0 if code == 0 and not crash else 1


if __name__ == "__main__":
    sys.exit(main())
