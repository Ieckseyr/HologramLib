#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""整包对拍: 我们构造的 UpdateTrade 包 vs BDS 抓包（26.40 / 协议 2168）。

用法: python check-trade-packet.py <captured pkt80_*.bin> <generated update_trade.bin>

比 check-trade-offers.py 更严: 那个只比 Data 段, 这个连包级字段一起比 ——
容器 id/类型、size、tier、entityUniqueId、lastTradingPlayer、displayName、两个 flag
逐字段核对, 最后整段字节逐字节比较。字段顺序取自 schema-1.26.40/UpdateTradePacket.json:
  ContainerId(u8) ContainerType(u8) Size(varint) TraderTier(varint)
  EntityUniqueId(zigzag) LastTradingPlayer(zigzag) DisplayName(str)
  UseNewTradeScreen(bool) UsingEconomyTrade(bool) Data(NBT)
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from decode_update_trade import Reader, decode  # noqa: E402

FIELDS = [
    ("containerId", "u8"),
    ("containerType", "u8"),
    ("size", "zigzag"),
    ("traderTier", "zigzag"),
    ("entityUniqueId", "zigzag"),
    ("lastTradingPlayer", "zigzag"),
    ("displayName", "text"),
    ("useNewTradeScreen", "bool"),
    ("useEconomyTrade", "bool"),
]

FAIL = []
OK = []


def read_fields(data, skip_header):
    r = Reader(data)
    if skip_header:
        shift = 0
        while True:
            b = r.u8()
            if b < 0x80:
                break
            shift += 7
    out = {}
    for name, kind in FIELDS:
        out[name] = getattr(r, kind)()
    out["__dataOffset"] = r.offset
    return out


def check(cond, msg):
    (OK if cond else FAIL).append(msg)
    print(("  OK   " if cond else "  FAIL ") + msg)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    capture_path, generated_path = sys.argv[1], sys.argv[2]

    capture = Path(capture_path).read_bytes()
    generated = Path(generated_path).read_bytes()

    decoded = decode(capture_path)
    if decoded["recipe_count"] != 7:
        print("  警告: 参考样本的配方数 = %d（期望 7）—— 可能不是屠夫参考样本, "
              "请显式传入 pkt80_194951_007.bin" % decoded["recipe_count"])

    print("== 包级字段 (capture vs generated) ==")
    cap = read_fields(capture, skip_header=True)
    gen = read_fields(generated, skip_header=False)
    for name, _ in FIELDS:
        c, g = cap[name], gen[name]
        if isinstance(c, bytes):
            c = c.decode("utf-8", "replace")
        if isinstance(g, bytes):
            g = g.decode("utf-8", "replace")
        check(c == g, "%-20s capture=%-28r generated=%r" % (name, c, g))

    cap_data = capture[cap["__dataOffset"]:]
    gen_data = generated[gen["__dataOffset"]:]
    print("== Data 段 (%d B vs %d B) ==" % (len(cap_data), len(gen_data)))
    check(cap_data == gen_data, "Offers NBT 逐字节一致（%d 条配方）" % decoded["recipe_count"])
    if cap_data != gen_data:
        for i in range(min(len(cap_data), len(gen_data))):
            if cap_data[i] != gen_data[i]:
                lo = max(0, i - 8)
                print("    首个差异 @%d (0x%x)" % (i, i))
                print("      capture  : %s" % cap_data[lo:i + 12].hex(" "))
                print("      generated: %s" % gen_data[lo:i + 12].hex(" "))
                break

    # 整包（除包头）最终一致
    check(capture[1:] == generated if capture[:1] == bytes([80]) else capture == generated,
          "整包（除包头）逐字节一致")

    # 包头校验: 抓包首个 varint 必须是 80, 且我们的 fixture 产出不含包头
    check(capture[0] == 80, "抓包包头 = 80 (UpdateTrade), got %d" % capture[0])
    check(len(generated) == len(capture) - 1,
          "fixture 产出 = 抓包长度 - 1（包头由发送方补）: %d vs %d" % (len(generated), len(capture) - 1))

    print("\n%d passed, %d failed" % (len(OK), len(FAIL)))
    return 1 if FAIL else 0


if __name__ == "__main__":
    sys.exit(main())
