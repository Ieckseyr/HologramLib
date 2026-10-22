#!/usr/bin/env python3
"""check-container-packets.py - 虚拟容器（GMLIB 方案）的离线逐字段核对

核对对象是 tests/container_packets_fixture.cpp 用库内同一份 src/container/ContainerPackets.h
产出的字节。检查分两类:

  1. ContainerOpen: 与**真实抓包** logs/fullpkts/pkt46_165803_011.bin 做整包逐字节对拍
     （fixture 的输入就是照抄那条抓包的: 容器 102 号, 方块 (376,-66,288)）
  2. UpdateBlock / BlockActorData / ContainerClose: 按 26.40 线格式解回字段逐项核对
     —— 无对应抓包样本, 所以是"格式与取值"级核对（与 check-npc-carrier.py 同一类）:
       UpdateBlock : pos(3 个 zigzag) + 运行 id(varuint) + flags(=3) + layer(=0)
       BlockActor  : pos + 根 compound, 含 GMLIB 的键集与 Items 列表（每项带 Slot）
       容器大小    : 单箱子 27 格（无配对键）/ 大箱子 54 格（pairx/pairz/pairlead 配对）

用法: check-container-packets.py <fixture 输出目录> [容器抓包 pkt46_*.bin]
"""
import sys
from pathlib import Path

failures = []
checks = 0


def check(ok, label):
    global checks
    checks += 1
    print(("OK   " if ok else "FAIL ") + label)
    if not ok:
        failures.append(label)
    return ok


# ── Bedrock 网络 NBT 读取（与 sculk 的 ValueTag/ListTag/CompoundTag 写方言一致:
#    Int/Long 是 zigzag varint, Short/Float/Double 定长, 字符串与列表计数 varint）──
class Reader:
    def __init__(self, data, pos=0):
        self.data = data
        self.pos = pos

    def u8(self):
        v = self.data[self.pos]
        self.pos += 1
        return v

    def varint(self):
        result = shift = 0
        while True:
            b = self.u8()
            result |= (b & 0x7F) << shift
            if not (b & 0x80):
                return result
            shift += 7

    def zigzag(self):
        raw = self.varint()
        return (raw >> 1) ^ -(raw & 1)

    def string(self):
        n = self.varint()
        v = self.data[self.pos:self.pos + n].decode("utf-8", "replace")
        self.pos += n
        return v

    def payload(self, tag_type):
        if tag_type == 1:  # Byte
            return self.u8()
        if tag_type == 2:  # Short
            v = int.from_bytes(self.data[self.pos:self.pos + 2], "little", signed=True)
            self.pos += 2
            return v
        if tag_type == 3:  # Int
            return self.zigzag()
        if tag_type == 4:  # Long
            return self.zigzag()
        if tag_type == 5:  # Float
            import struct
            v = struct.unpack_from("<f", self.data, self.pos)[0]
            self.pos += 4
            return v
        if tag_type == 6:  # Double
            import struct
            v = struct.unpack_from("<d", self.data, self.pos)[0]
            self.pos += 8
            return v
        if tag_type == 8:  # String
            return self.string()
        if tag_type == 9:  # List
            elem_type = self.u8()
            # 列表计数是 **zigzag** varint（sculk ListTag::write → writeVarInt）, 不是无符号 varint
            count = self.zigzag()
            return [self.payload(elem_type) for _ in range(count)]
        if tag_type == 10:  # Compound
            return self.compound()
        raise ValueError("unsupported tag type %d at %d" % (tag_type, self.pos))

    def compound(self, rooted=False):
        out = {}
        if rooted:
            tag_type = self.u8()
            if tag_type != 10:
                raise ValueError("rooted compound must start with 0x0a, got %d" % tag_type)
            name_len = self.u8()  # 根名长度, 恒为空
            if name_len != 0:
                raise ValueError("rooted compound name must be empty, got len %d" % name_len)
        while True:
            tag_type = self.u8()
            if tag_type == 0:
                return out
            name = self.string()
            out[name] = self.payload(tag_type)

    def remaining(self):
        return len(self.data) - self.pos


def read_body(path):
    """fixture 产出的是**包体**（不含包头 —— 包头由发送方按 packet.getId() 补）,
    所以这里直接从头解, 不去读包头。"""
    return Reader(Path(path).read_bytes())


def check_container_close(path):
    r = read_body(path)
    check(r.u8() == 102, "ContainerClose 容器 id = 102")
    check(r.u8() == 0, "ContainerClose 容器类型 = 0 (Container)")
    check(r.u8() == 1, "ContainerClose 服务端主动关闭 = 1")
    check(r.remaining() == 0, "ContainerClose 尾部无剩余字节 (leftover=%d)" % r.remaining())


def check_update_block(path):
    r = read_body(path)
    pos = (r.zigzag(), r.zigzag(), r.zigzag())
    check(pos == (376, -66, 288), "UpdateBlock 方块坐标 = %s" % (pos,))
    check(r.varint() == 12345, "UpdateBlock 运行 id 原样带出")
    check(r.varint() == 3, "UpdateBlock flags = 3 ((1<<0)|(1<<1), GMLIB 同值)")
    check(r.varint() == 0, "UpdateBlock layer = 0")
    check(r.remaining() == 0, "UpdateBlock 尾部无剩余字节 (leftover=%d)" % r.remaining())


def read_block_actor(path):
    r = read_body(path)
    pos = (r.zigzag(), r.zigzag(), r.zigzag())
    nbt = r.compound(rooted=True)
    check(r.remaining() == 0, "BlockActorData 尾部无剩余字节 (leftover=%d)" % r.remaining())
    return pos, nbt


def check_small_chest(path):
    pos, nbt = read_block_actor(path)
    check(pos == (376, -66, 288), "单箱子方块实体坐标 = %s" % (pos,))
    check(nbt.get("id") == "Chest", "方块实体 id = 'Chest' (got %r)" % nbt.get("id"))
    check(nbt.get("CustomName") == "任务列表", "标题走 CustomName (got %r)" % nbt.get("CustomName"))
    check(nbt.get("Findable") == 0, "Findable = 0")
    check(nbt.get("isMovable") == 1, "isMovable = 1")
    check((nbt.get("x"), nbt.get("y"), nbt.get("z")) == (376, -66, 288), "x/y/z 与方块坐标一致")
    check("pairx" not in nbt and "pairlead" not in nbt, "单箱子不带配对键（否则客户端会去找另一半）")
    items = nbt.get("Items")
    check(isinstance(items, list), "Items 是 List")
    check(len(items) == 3, "单箱子只落 3 个非空条目 (got %d)" % len(items))
    slots = sorted(i.get("Slot") for i in items)
    check(slots == [0, 2, 13], "条目 Slot 保持原槽位号 %s" % slots)
    first = next(i for i in items if i.get("Slot") == 0)
    check(first.get("Name") == "minecraft:paper", "物品名走 Name (got %r)" % first.get("Name"))
    check(first.get("Count") == 1, "物品 Count = 1")
    check(
        first.get("tag", {}).get("display", {}).get("Name") == "§e任务目标: 清剿僵尸",
        "自定义名走 tag.display.Name",
    )
    check(
        first.get("tag", {}).get("display", {}).get("Lore")
        == ["§7击杀 5 只僵尸", "§8点我看看有没有回调"],
        "描述行走 tag.display.Lore",
    )


def check_big_chest(lead_path, second_path):
    lead_pos, lead = read_block_actor(lead_path)
    second_pos, second = read_block_actor(second_path)

    check(lead_pos == (376, -66, 288), "大箱子 lead 半坐标 = %s" % (lead_pos,))
    check(second_pos == (377, -66, 288), "大箱子副半坐标 = %s (lead 右侧一格)" % (second_pos,))

    # GMLIB 的配对约定: lead 半 pairlead=1 且 pairx = x+1; 副半 pairlead=0 且 pairx = x
    check(lead.get("pairlead") == 1, "lead 半 pairlead = 1 (got %r)" % lead.get("pairlead"))
    check(lead.get("pairx") == 377, "lead 半 pairx = 377 (got %r)" % lead.get("pairx"))
    check(second.get("pairlead") == 0, "副半 pairlead = 0 (got %r)" % second.get("pairlead"))
    check(second.get("pairx") == 376, "副半 pairx = 376 (got %r)" % second.get("pairx"))
    check(lead.get("pairz") == second.get("pairz") == 288, "两半 pairz 一致")

    lead_slots = sorted(i.get("Slot") for i in lead["Items"])
    second_slots = sorted(i.get("Slot") for i in second["Items"])
    check(
        lead_slots == [0, 9, 18] and second_slots == [0, 9, 18],
        "两半各自用 0..26 编号 (lead=%s second=%s) —— 与 GMLIB 的 updateBlockActor 一致"
        % (lead_slots, second_slots),
    )
    check(
        len(lead["Items"]) + len(second["Items"]) == 6,
        "大箱子两半合计 6 个非空条目 (54 格里每 9 格一个: 3 + 3, got %d)"
        % (len(lead["Items"]) + len(second["Items"])),
    )


def check_container_open_against_capture(path, capture_path):
    generated = Path(path).read_bytes()
    capture = Path(capture_path).read_bytes()
    check(len(capture) > 1 and capture[0] == 46, "抓包包头 = 46 (ContainerOpen), got %d" % capture[0])
    check(
        capture[1:] == generated,
        "ContainerOpen 与抓包逐字节一致（除包头）: 生成 %d 字节 vs 抓包 %d 字节"
        % (len(generated), len(capture) - 1),
    )
    if capture[1:] != generated:
        limit = min(len(generated), len(capture) - 1)
        for i in range(limit):
            if generated[i] != capture[i + 1]:
                print("     first diff at body offset %d: generated %s vs capture %s"
                      % (i, generated[max(0, i - 4):i + 4].hex(" "),
                         capture[max(1, i - 3):i + 5].hex(" ")))
                break
    # 字段级再确认一遍（不只看字节相等）
    r = read_body(path)
    check(r.u8() == 102, "ContainerOpen 容器 id = 102 (显示区间 101..199)")
    check(r.u8() == 0, "ContainerOpen 容器类型 = 0 (Container) —— 不是 MinecartChest")
    pos = (r.zigzag(), r.zigzag(), r.zigzag())
    check(pos == (376, -66, 288), "ContainerOpen 绑的是方块坐标 %s（不是实体）" % (pos,))
    check(r.zigzag() == -1, "ContainerOpen 目标实体 = -1（绑方块, 不绑实体）")
    check(r.remaining() == 0, "ContainerOpen 尾部无剩余字节 (leftover=%d)" % r.remaining())


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    outdir = Path(sys.argv[1])
    if not outdir.is_dir():
        print("no such directory: %s" % outdir)
        return 2

    print("=== UpdateBlock（客户端侧箱子方块）===")
    check_update_block(outdir / "update_block.bin")

    print("\n=== BlockActorData（单箱子 27 格）===")
    check_small_chest(outdir / "block_actor_small.bin")

    print("\n=== BlockActorData（大箱子 54 格, 两个配对半区）===")
    check_big_chest(outdir / "block_actor_big_lead.bin", outdir / "block_actor_big_second.bin")

    print("\n=== ContainerOpen ===")
    if len(sys.argv) >= 3:
        check_container_open_against_capture(outdir / "container_open.bin", sys.argv[2])
    else:
        r = read_body(outdir / "container_open.bin")
        check(r.u8() == 102, "ContainerOpen 容器 id = 102")
        check(r.u8() == 0, "ContainerOpen 容器类型 = 0 (Container)")
        check((r.zigzag(), r.zigzag(), r.zigzag()) == (376, -66, 288), "ContainerOpen 方块坐标")
        check(r.zigzag() == -1, "ContainerOpen 目标实体 = -1")
        check(r.remaining() == 0, "ContainerOpen 尾部无剩余字节 (leftover=%d)" % r.remaining())

    print("\n=== ContainerClose ===")
    check_container_close(outdir / "container_close.bin")

    print("\n%d checks, %d failed" % (checks, len(failures)))
    for f in failures:
        print("  FAILED: " + f)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
