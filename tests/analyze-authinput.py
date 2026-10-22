#!/usr/bin/env python3
"""analyze-authinput.py - AuthInput 抓包的两段式离线分析

输入: MeowTradeTest 写出的 logs/authinput.hex（一行一包: 时间戳 + 序号 + 长度 + 完整字节）
输出: <outdir>/packets.bin（varint 长度前缀的包流, 供 C++ 解码 fixture 读入）
      <outdir>/index.tsv（序号 / 时间戳 / 长度, 用来把解码结果对回时间）
      <outdir>/analysis.txt（逐包字节 diff 汇总: 哪些偏移在变、变化频次、长度跳变点）

设计意图: 只做机械统计, 不预设"点击在哪个字段" —— 偏移变化的频次分布 + 长度跳变点,
配合解码结果（decoded.txt）就能定位点击每次到底改了什么。
"""
import re
import sys
import collections
from pathlib import Path

LINE = re.compile(r"\[(\d\d:\d\d:\d\d)\]\s+#(\d+)\s+len=(\d+)\s+hex=([0-9a-f]+)")


def load(path):
    rows = []
    for raw in Path(path).read_text(encoding="utf-8", errors="replace").splitlines():
        m = LINE.match(raw.strip())
        if m:
            rows.append((m.group(1), int(m.group(2)), int(m.group(3)), bytes.fromhex(m.group(4))))
    return rows


def write_varint(out, value):
    while True:
        b = value & 0x7F
        value >>= 7
        if value:
            out.append(b | 0x80)
        else:
            out.append(b)
            return


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    src, outdir = sys.argv[1], Path(sys.argv[2])
    outdir.mkdir(parents=True, exist_ok=True)

    rows = load(src)
    if not rows:
        print("no packets in capture: %s" % src)
        return 1
    print("packets: %d  window: %s -> %s" % (len(rows), rows[0][0], rows[-1][0]))

    # packets.bin + index.tsv
    blob = bytearray()
    index = []
    for stamp, seq, length, data in rows:
        write_varint(blob, len(data))
        blob += data
        index.append("%d\t%s\t%d" % (seq, stamp, length))
    (outdir / "packets.bin").write_bytes(bytes(blob))
    (outdir / "index.tsv").write_text("\n".join(index) + "\n", encoding="utf-8")

    # 逐包字节 diff: 每个偏移的变化次数（仅比较同长度的相邻包, 避免错位）
    changed = collections.Counter()
    pairs = 0
    for (_, _, l0, b0), (_, _, l1, b1) in zip(rows, rows[1:]):
        if l0 != l1:
            continue
        pairs += 1
        for i, (x, y) in enumerate(zip(b0, b1)):
            if x != y:
                changed[i] += 1

    lines = []
    lines.append("同长度相邻对比: %d 次" % pairs)
    lines.append("偏移变化频次（前 30）:")
    for off, n in changed.most_common(30):
        lines.append("  off=%-4d 变化 %-5d 次 (%.1f%%)" % (off, n, 100.0 * n / max(1, pairs)))

    lines.append("")
    lines.append("长度跳变点（长度一旦与上一包不同就记一行）:")
    prev = None
    for stamp, seq, length, _ in rows:
        if length != prev:
            lines.append("  %s #%d len=%d" % (stamp, seq, length))
            prev = length

    lines.append("")
    lines.append("长度分布:")
    for length, n in collections.Counter(r[2] for r in rows).most_common():
        lines.append("  len=%-4d %d 包" % (length, n))

    (outdir / "analysis.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("wrote %s, %s, %s" % (outdir / "packets.bin", outdir / "index.tsv", outdir / "analysis.txt"))
    for line in lines[:8]:
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
