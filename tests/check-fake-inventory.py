#!/usr/bin/env python3
"""背包虚容器: 用 **BDS 真实抓包**核对 26.40 的线格式（布局真值）

背景（这次卡死事故的根因, 留在检查里）:
  物品描述符里的 User Data 用的是**另一套 NBT 方言** —— 名字长度是 u16 小端（不是 varint）,
  整数/列表计数是 4 字节小端; 而 sculk 的 CompoundTag 写的是 varint 那套（Offers / BlockActorData
  用的才是那套, 两边都逐字节对拍过）。手写描述符因此写错了方言, 客户端解析越界 → 直接卡死。
  现在物品序列化交给 BDS 自己的包类（src/fakeinv/FakeInventoryPackets.h）; 本检查把布局事实钉住,
  将来协议变动时会在这里报警。

核对对象:
  参考抓包（默认 logs/fullpkts/pkt49_115316_007.bin, 玩家登录时 BDS 发的背包整包内容）
    → 核对帧结构 + 容器名/storage 取值 + 物品 UserData 的方言指纹
  其余同前缀抓包（含旧版本库自己发的包）→ 只核对"帧能解析到包尾"

不变量:
  · 帧: ContainerId(varint) → Slots[varint 计数] → 容器名(名字节 + 有无动态id) → StorageItem,
    解析必须正好落在包尾 —— 字段序或宽度错了必然留下尾巴
  · 玩家背包那份: 容器 id 0、36 格、容器名字节 0、storage 是空描述符
  · 物品描述符: Id(i16 LE) → StackSize(u16 LE) → Aux(varint) → NetId?(bool + varint) →
    BlockRuntimeId(varint) → UserData(varint 长度 + 方言数据)
  · 方言指纹: 有 NBT 的物品, 其 UserData 里 "Damage" 前是 u16 长度（06 00）, 不是 varint（06）

用法: check-fake-inventory.py <抓包目录> [参考内容包] [单格包]
"""
import glob
import io
import os
import struct
import sys

INVENTORY_CONTAINER_ID = 0
INVENTORY_SLOTS = 36
DEFAULT_REFERENCE = "pkt49_115316_007.bin"


class Reader:
    def __init__(self, data, pos=0):
        self.data = data
        self.pos = pos

    def byte(self):
        v = self.data[self.pos]
        self.pos += 1
        return v

    def varint(self):
        r = 0
        s = 0
        while True:
            b = self.byte()
            r |= (b & 0x7F) << s
            if not (b & 0x80):
                return r
            s += 7

    def i16(self):
        v = struct.unpack_from("<h", self.data, self.pos)[0]
        self.pos += 2
        return v

    def u16(self):
        v = struct.unpack_from("<H", self.data, self.pos)[0]
        self.pos += 2
        return v

    def left(self):
        return len(self.data) - self.pos


def decode_item(r):
    item = {"id": r.i16(), "count": r.u16(), "aux": r.varint()}
    item["net_id"] = r.varint() if r.byte() else None
    item["block_runtime_id"] = r.varint()
    ud_len = r.varint()
    item["user_data"] = r.data[r.pos:r.pos + ud_len]
    r.pos += ud_len
    return item


def decode_content(data):
    r = Reader(data)
    packet_id = r.varint() & 0x3FF
    container_id = r.varint()
    slot_count = r.varint()
    slots = [decode_item(r) for _ in range(slot_count)]
    name = r.byte()
    dyn = r.varint() if r.byte() else None
    storage = decode_item(r)
    return dict(
        packet_id=packet_id, container_id=container_id, slots=slots,
        container_name=name, dynamic_id=dyn, storage=storage, leftover=r.left(),
    )


def decode_slot(data):
    r = Reader(data)
    packet_id = r.varint() & 0x3FF
    container_id = r.varint()
    slot = r.varint()
    name = None
    dyn = None
    if r.byte():
        name = r.byte()
        dyn = r.varint() if r.byte() else None
    storage = decode_item(r) if r.byte() else None
    item = decode_item(r)
    return dict(
        packet_id=packet_id, container_id=container_id, slot=slot,
        container_name=name, dynamic_id=dyn, storage=storage, item=item, leftover=r.left(),
    )


def read(path):
    with io.open(path, "rb") as f:
        return f.read()


def newest(directory, prefix, limit=8):
    hits = sorted(glob.glob(os.path.join(directory, prefix + "*.bin")), key=os.path.getmtime)
    return hits[-limit:]


def main():
    directory = sys.argv[1] if len(sys.argv) > 1 else "../../logs/fullpkts"
    explicit_content = sys.argv[2] if len(sys.argv) > 2 else None
    explicit_slot = sys.argv[3] if len(sys.argv) > 3 else None

    checks = []

    def check(ok, label, detail=""):
        checks.append(ok)
        print("%s %s%s" % ("OK  " if ok else "FAIL", label, ("  (%s)" % detail) if detail else ""))

    # ── 参考内容包: 布局事实 ──
    reference = explicit_content or os.path.join(directory, DEFAULT_REFERENCE)
    if not os.path.exists(reference):
        candidates = newest(directory, "pkt49_")
        reference = candidates[-1] if candidates else None
    if reference is None:
        print("no pkt49 capture found in %s (pass one explicitly); skipping" % directory)
    else:
        print("content reference: %s" % reference)
        c = decode_content(read(reference))
        check(c["packet_id"] == 49, "包头 = 49 (InventoryContent)", "got %d" % c["packet_id"])
        check(c["leftover"] == 0, "帧解析落在包尾", "leftover=%d" % c["leftover"])
        check(c["container_id"] == INVENTORY_CONTAINER_ID and len(c["slots"]) == INVENTORY_SLOTS,
              "容器 id = 0 且 36 格", "cid=%d slots=%d" % (c["container_id"], len(c["slots"])))
        check(c["container_name"] == 0,
              "容器名字节 = 0（BDS 就是写 0, 不是 InventoryContainer）", "got %d" % c["container_name"])
        check(c["dynamic_id"] is None, "不带动态容器 id")
        check(c["storage"]["id"] == 0 and len(c["storage"]["user_data"]) == 0, "storage item = 空描述符")

        dialect_seen = False
        dialect_ok = True
        for s in c["slots"]:
            ud = s["user_data"]
            if not ud or b"Damage" not in ud:
                continue
            dialect_seen = True
            # u16 名字长度方言: 03 06 00 "Damage"; varint 方言会是 03 06 "Damage"
            if (b"\x06\x00Damage" not in ud) or (b"\x06Damage" in ud):
                dialect_ok = False
                print("    UE NBT 方言指纹不符: %s" % ud[:24].hex(" "))
        check(dialect_seen, "含带 NBT 的物品（方言指纹可判定）")
        check(dialect_ok, "物品 UserData 用 u16 名字长度方言（06 00 Damage）, 不是 varint（06 Damage）")

    # ── 其余内容包: 只核对帧 ──
    scanned = newest(directory, "pkt49_")
    frames_ok = True
    counted = 0
    for path in scanned:
        if reference and os.path.abspath(path) == os.path.abspath(reference):
            continue
        counted += 1
        c = decode_content(read(path))
        if c["packet_id"] != 49 or c["leftover"] != 0:
            frames_ok = False
            print("    %s: packet_id=%d leftover=%d" % (os.path.basename(path), c["packet_id"], c["leftover"]))
    if counted:
        check(frames_ok, "其余 pkt49 抓包帧解析也落在包尾（%d 个）" % counted)

    # ── 单格包 ──
    slot_files = [explicit_slot] if explicit_slot else newest(directory, "pkt50_")
    if not slot_files:
        print("no pkt50 capture found in %s (pass one explicitly); skipping" % directory)
    else:
        slot_ok = True
        for path in slot_files:
            s = decode_slot(read(path))
            if s["packet_id"] != 50 or s["leftover"] != 0:
                slot_ok = False
                print("    %s: packet_id=%d leftover=%d" % (os.path.basename(path), s["packet_id"], s["leftover"]))
        check(slot_ok, "pkt50 帧解析全部落在包尾（%d 个）" % len(slot_files))

    if not checks:
        print("nothing to check (no captures found)")
        return 0

    failed = checks.count(False)
    print("\n%d checks, %d failed" % (len(checks), failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
