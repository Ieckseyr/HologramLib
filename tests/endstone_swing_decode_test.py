#!/usr/bin/env python3
"""endstone 侧挥动解码/分类的纯逻辑测试（不需要跑服务器）。

用法：python tests/endstone_swing_decode_test.py
"""

from __future__ import annotations

import importlib.util
import struct
import sys
import types
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SENSING = ROOT / "platform" / "endstone" / "src" / "hologramlib_endstone" / "sensing.py"


def install_endstone_stub() -> None:
    """sensing.py 只用到 event_handler 与两个事件类型，用桩顶掉。"""

    def event_handler(func):
        return func

    class PacketReceiveEvent:  # noqa: D101 - 仅占位
        pass

    class PlayerInteractEvent:  # noqa: D101
        pass

    event_mod = types.ModuleType("endstone.event")
    event_mod.event_handler = event_handler
    event_mod.PacketReceiveEvent = PacketReceiveEvent
    event_mod.PlayerInteractEvent = PlayerInteractEvent
    endstone_mod = types.ModuleType("endstone")
    endstone_mod.event = event_mod
    sys.modules["endstone"] = endstone_mod
    sys.modules["endstone.event"] = event_mod


def load_sensing():
    spec = importlib.util.spec_from_file_location("holo_sensing_under_test", SENSING)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module  # dataclass 解析字符串注解时要能从 sys.modules 找到模块
    spec.loader.exec_module(module)
    return module


def uvarint(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 0x7F
        value >>= 7
        if value:
            out.append(byte | 0x80)
        else:
            out.append(byte)
            return bytes(out)


def animate(action: int, runtime_id: int, data: float = 0.0, extra: bytes = b"") -> bytes:
    return bytes([action]) + uvarint(runtime_id) + struct.pack("<f", data) + extra


class FakeLevel:
    time = 1200
    actors = []


class FakeInventory:
    item_in_main_hand = None


class FakePlayer:
    def __init__(self, name: str = "Steve") -> None:
        self.name = name
        self.level = FakeLevel()
        self.inventory = FakeInventory()


class FakeLogger:
    def debug(self, *_args, **_kwargs) -> None:
        pass


class FakePlugin:
    def __init__(self) -> None:
        self.logger = FakeLogger()
        self.registered = []

    def register_events(self, listener) -> None:
        self.registered.append(listener)


class FakeInteract:
    LEFT_CLICK_BLOCK = "LEFT_CLICK_BLOCK"
    LEFT_CLICK_AIR = "LEFT_CLICK_AIR"

    def __init__(self, action, x=0, y=0, z=0, face=1, has_block=False) -> None:
        self.action = action
        self.player = FakePlayer()
        self.has_block = has_block
        self.block = types.SimpleNamespace(location=types.SimpleNamespace(x=x, y=y, z=z))
        self.block_face = types.SimpleNamespace(value=face)


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

    print("[1] AnimatePacket 解码")
    check("非 Swing 动作", sensing.decode_animate(animate(0, 1)), None)
    check("只有三字段", sensing.decode_animate(animate(1, 5)), (5, None))
    check("一字节来源", sensing.decode_animate(animate(1, 5, extra=bytes([2]))), (5, 2))
    check("bool+来源", sensing.decode_animate(animate(1, 7, extra=bytes([1, 4]))), (7, 4))
    # 1 个字节 0 既可能是「bool 存在位=0」，也可能是「枚举值 0 = none」；
    # 线上无法区分，统一按枚举值 0 处理（source=none），不猜成 unset
    check("单字节 0 视为 none", sensing.decode_animate(animate(1, 7, extra=bytes([0]))), (7, 0))
    check("完全没带", sensing.decode_animate(animate(1, 7)), (7, None))
    check("截断包", sensing.decode_animate(b"\x01\x05"), None)

    print("[2] 事件流：先包后点击（回溯补目标）")
    plugin = FakePlugin()
    api = sensing.SensingApi(plugin)
    api._ensure_listener()
    check("注册了监听", len(plugin.registered), 1)
    player = FakePlayer()
    api._on_animate(animate(1, 0, extra=bytes([2])), player)  # mine，但没有点击信息 → 暂定 air
    check("先记 air", api.last_target("Steve"), "air")
    api._on_interact(FakeInteract(FakeInteract.LEFT_CLICK_BLOCK, 1, 64, -3, 1, True))
    check("点击后补成 block", api.last_target("Steve"), "block")
    check("方块坐标", api.last_block("Steve"), "1,64,-3")
    lines = api.poll_events()
    check("轮询取走一条", len(lines), 1)
    check("事件行含 target=block", " target=block " in lines[0], True)
    check("事件行含 pos", " pos=1,64,-3 " in lines[0], True)
    check("轮询后清空", api.poll_events(), [])

    print("[3] 事件流：先点击后包")
    api._on_interact(FakeInteract(FakeInteract.LEFT_CLICK_AIR))
    api._on_animate(animate(1, 0), FakePlayer())
    check("air 直接命中", api.last_target("Steve"), "air")

    print("[4] 实体分支与计数")
    api._on_animate(animate(1, 42, extra=bytes([1, 4])), FakePlayer("Alex"))
    check("attack → entity", api.last_target("Alex"), "entity")
    check("按玩家计数", api.count("Alex"), 1)
    check("总计数", api.total(), 3)
    api.reset()
    check("reset 后计数", api.count("Alex"), 0)
    check("reset 后总计数", api.total(), 0)

    print()
    if failures:
        print(f"ENDSTONE-SWING-TEST: {failures} 项失败")
        return 1
    print("ENDSTONE-SWING-TEST: 全部通过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
