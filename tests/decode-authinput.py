#!/usr/bin/env python3
"""decode-authinput.py - 按 .cache/schema-1.26.40 的线格式真值解码 AuthInput 抓包

字段顺序来自 `.cache/schema-1.26.40/PlayerAuthInputPacketPayload.json` 的 x-ordinal-index:

   0 Player Rotation  Vec2(2 float)   1 Position Vec3(3 float)   2 Move Vector Vec2
   3 Player Head Rotation float        4 Input Data  **Compression 枚举数组(变长)**
   5 Input Mode varint                 6 Play Mode varint          7 New Interaction Model varint
   8 Interact Rotation Vec2            9 Client Tick               10 Pos Delta Vec3
  11 Item Use Transaction（可选）      12 Item Stack Request（可选）
  13 Player Block Actions 数组         14 Vehicle Rotation Vec2    15 Client Predicted Vehicle
  16 Analog Move Vector Vec2           17 Camera Orientation Vec3  18 Raw Move Vector Vec2

关键的坑: `Input Data` 在协议里不是位图, 而是 `Compression` + `Enum-as-Value` 的**变长数组**
（协议库 sculk 按 bitset<65> 固定 9 字节读 → 读不满, 差的字节数随数组长度变化, 正好对应抓包里
包长 97/98 与 103~108 的跳变）。本脚本同时尝试两种常见编码（等长 varint / 增量 varint）,
**以"整包恰好消费完"为准**挑出正确的那种 —— 不靠猜。

用法: decode-authinput.py <authinput.hex> <outdir>
"""
import json
import re
import struct
import sys
from pathlib import Path

LINE = re.compile(r"\[(\d\d:\d\d:\d\d)\]\s+#(\d+)\s+len=(\d+)\s+hex=([0-9a-f]+)")


class Reader:
    def __init__(self, data, pos=0):
        self.data = data
        self.pos = pos

    def left(self):
        return len(self.data) - self.pos

    def u8(self):
        if self.pos >= len(self.data):
            raise EOFError("u8 at %d" % self.pos)
        v = self.data[self.pos]
        self.pos += 1
        return v

    def varint(self):
        out = shift = 0
        while True:
            b = self.u8()
            out |= (b & 0x7F) << shift
            if not (b & 0x80):
                return out
            shift += 7
            if shift > 63:
                raise ValueError("varint too long")

    def zigzag(self):
        v = self.varint()
        return (v >> 1) ^ -(v & 1)

    def f32(self):
        v = struct.unpack_from("<f", self.data, self.pos)[0]
        self.pos += 4
        return v

    def vec2(self):
        return (self.f32(), self.f32())

    def vec3(self):
        return (self.f32(), self.f32(), self.f32())


def decode_input_data(r, mode):
    """Input Data: varint 计数 + 计数个 varint（mode='plain'）或增量（'delta'）"""
    count = r.varint()
    if count > 200:
        raise ValueError("input data count too large: %d" % count)
    values = []
    prev = 0
    for _ in range(count):
        raw = r.varint()
        if mode == "delta":
            prev += raw
            values.append(prev)
        else:
            values.append(raw)
    return values


def decode_packet(data, mode):
    r = Reader(data)
    out = {}
    out["playerRotation"] = r.vec2()
    out["position"] = r.vec3()
    out["moveVector"] = r.vec2()
    out["headRotation"] = r.f32()
    out["inputData"] = decode_input_data(r, mode)
    out["inputMode"] = r.varint()
    out["playMode"] = r.varint()
    out["newInteractionModel"] = r.varint()
    out["interactRotation"] = r.vec2()
    out["clientTick"] = r.varint()
    out["posDelta"] = r.vec3()
    # 可选字段: 一个 bool 表示有没有
    out["hasItemUseTxn"] = r.u8()
    if out["hasItemUseTxn"] not in (0, 1):
        raise ValueError("itemUseTxn flag not a bool: %d" % out["hasItemUseTxn"])
    if out["hasItemUseTxn"]:
        raise ValueError("itemUseTxn present (需要继续解, 见下文)")
    out["hasItemStackRequest"] = r.u8()
    if out["hasItemStackRequest"] not in (0, 1):
        raise ValueError("itemStackRequest flag not a bool")
    if out["hasItemStackRequest"]:
        raise ValueError("itemStackRequest present")
    # Player Block Actions: varint 计数 + 每项(动作类型 varint, BlockPos 3 zigzag, 朝向 varint)
    blocks = r.varint()
    if blocks > 100:
        raise ValueError("block actions count too large: %d" % blocks)
    out["blockActions"] = []
    for _ in range(blocks):
        out["blockActions"].append((r.varint(), (r.zigzag(), r.zigzag(), r.zigzag()), r.varint()))
    out["vehicleRotation"] = r.vec2()
    out["clientPredictedVehicle"] = r.zigzag()
    out["analogMoveVector"] = r.vec2()
    out["cameraOrientation"] = r.vec3()
    out["rawMoveVector"] = r.vec2()
    out["consumed"] = r.pos
    out["total"] = len(data)
    out["ok"] = r.pos == len(data)
    return out


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    src, outdir = sys.argv[1], Path(sys.argv[2])
    outdir.mkdir(parents=True, exist_ok=True)

    rows = []
    for raw in Path(src).read_text(encoding="utf-8", errors="replace").splitlines():
        m = LINE.match(raw.strip())
        if m:
            rows.append((m.group(1), int(m.group(2)), bytes.fromhex(m.group(4))))
    if not rows:
        print("no packets")
        return 1

    # 用"整包恰好消费完"判定 Input Data 的编码方式（不猜）
    for mode in ("plain", "delta"):
        ok = 0
        for _, _, data in rows:
            try:
                if decode_packet(data, mode)["ok"]:
                    ok += 1
            except Exception:
                pass
        print("Input Data 编码 %-6s → 整包完整解码 %d/%d" % (mode, ok, len(rows)))
        if ok == len(rows):
            chosen = mode
            break
    else:
        chosen = max(("plain", "delta"), key=lambda m: sum(
            1 for _, _, d in rows if _try(d, m)
        ))

    print("采用编码:", chosen)
    lines = []
    for stamp, seq, data in rows:
        try:
            f = decode_packet(data, chosen)
        except Exception as exc:
            lines.append("%s\t#%d\tDECODE_FAIL\t%s" % (stamp, seq, exc))
            continue
        lines.append(
            "%s\t#%d\tlen=%d\tok=%s\tbits=%s\tinputMode=%d\tplayMode=%d\tmodel=%d\ttick=%d\thasUse=%d\thasReq=%d\tblocks=%s"
            % (
                stamp, seq, len(data), f["ok"], ",".join(str(v) for v in f["inputData"]),
                f["inputMode"], f["playMode"], f["newInteractionModel"], f["clientTick"],
                f["hasItemUseTxn"], f["hasItemStackRequest"],
                ";".join(str(b) for b in f["blockActions"]) or "-",
            )
        )
    (outdir / "decoded-schema.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("wrote", outdir / "decoded-schema.txt")
    return 0


def _try(data, mode):
    try:
        return decode_packet(data, mode)["ok"]
    except Exception:
        return False


if __name__ == "__main__":
    sys.exit(main())
