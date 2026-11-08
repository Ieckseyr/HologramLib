#!/usr/bin/env python3
"""endstone 侧挥动计数的纯逻辑测试（不需要跑服务器）。

用法：python tests/endstone_swing_decode_test.py
"""

from __future__ import annotations

import importlib.util
import sys
import types
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SENSING = ROOT / "platform" / "endstone" / "src" / "hologramlib_endstone" / "sensing.py"


def install_endstone_stub() -> None:
    """sensing.py 只用到 event_handler 与 PacketReceiveEvent，用桩顶掉。"""

    def event_handler(func):
        return func

    class PacketReceiveEvent:  # noqa: D101 - 仅占位
        pass

    event_mod = types.ModuleType("endstone.event")
    event_mod.event_handler = event_handler
    event_mod.PacketReceiveEvent = PacketReceiveEvent
    endstone_mod = types.ModuleType("endstone")
    endstone_mod.event = event_mod
    sys.modules["endstone"] = endstone_mod
    sys.modules["endstone.event"] = event_mod


def load_sensing():
    spec = importlib.util.spec_from_file_location("holo_sensing_under_test", SENSING)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class FakeLogger:
    def debug(self, *_args, **_kwargs) -> None:
        pass


class FakePlugin:
    def __init__(self) -> None:
        self.logger = FakeLogger()
        self.registered = []

    def register_events(self, listener) -> None:
        self.registered.append(listener)


def main() -> int:
    install_endstone_stub()
    sensing = load_sensing()
    failures = 0

    def check(name: str, got, want) -> None:
        nonlocal failures
        if got != want:
            failures += 1
            print(f"  FAIL {name}: got={got!r} want={want!r}")
        else:
            print(f"  PASS {name}")

    print("[1] AnimatePacket 判定（只看首个 Action 字节）")
    check("Swing", sensing.is_swing(bytes([1, 0, 0, 0, 0, 0])), True)
    check("NoAction", sensing.is_swing(bytes([0, 0, 0, 0, 0, 0])), False)
    check("WakeUp", sensing.is_swing(bytes([3])), False)
    check("空包", sensing.is_swing(b""), False)

    print("[2] 计数与重置")
    plugin = FakePlugin()
    api = sensing.SensingApi(plugin)
    check("注册前无监听", len(plugin.registered), 0)
    check("初始 count", api.count("Steve"), 0)
    check("首次调用后注册监听", len(plugin.registered), 1)
    api._note("Steve")
    api._note("Steve")
    api._note("Alex")
    check("Steve 计数", api.count("Steve"), 2)
    check("Alex 计数", api.count("Alex"), 1)
    check("总数", api.total(), 3)
    api.reset()
    check("reset 后 count", api.count("Steve"), 0)
    check("reset 后 total", api.total(), 0)

    print()
    if failures:
        print(f"ENDSTONE-SWING-TEST: {failures} 项失败")
        return 1
    print("ENDSTONE-SWING-TEST: 全部通过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
