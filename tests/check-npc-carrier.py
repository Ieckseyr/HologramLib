#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""合成 NPC 载体包的逐字段校验（26.40 / 协议 2168）。

用法: python check-npc-carrier.py <addactor.bin> <npcdialogue.bin> [真实抓包 pkt13_*.bin]

校验内容:
  1. AddActor 的字段顺序与 BDS schema 一致（uid→rid→identifier→pos→vel→rot→yHead→yBody→
     属性表→ActorData→属性同步→链接）, 且尾部恰好读完（无多余/缺失字节）
  2. ActorData 里的 NPC 标记项齐全且类型正确:
     Name(4,String) HasNpc(39,Byte=1) NpcData(40,String=合法JSON/含 skin_list)
     Actions(41,String=按钮JSON) InteractText(100,String)
     —— 且每项都是 [id][type][tag(=type)][value] 的 cereal 变体形态
  3. 位置 y == kCarrierY(-66)（客户端看不到实体但 NPC 界面头像照常）
  4. 若给了真实抓包: 交叉核对真实 BDS NPC 的生成包确实含同样的 id/类型标记
"""
import json
import struct
import sys
from pathlib import Path

FAIL = []
OK = []


def check(cond, msg):
    (OK if cond else FAIL).append(msg)
    print(("  OK   " if cond else "  FAIL ") + msg)


class Reader:
    def __init__(self, data):
        self.d = data
        self.o = 0

    def uv(self):
        v = 0
        sh = 0
        while True:
            b = self.d[self.o]
            self.o += 1
            v |= (b & 0x7F) << sh
            if b < 0x80:
                return v
            sh += 7

    def zz(self):
        u = self.uv()
        return (u >> 1) ^ -(u & 1)

    def f32(self):
        v = struct.unpack_from("<f", self.d, self.o)[0]
        self.o += 4
        return v

    def st(self):
        n = self.uv()
        v = self.d[self.o : self.o + n]
        self.o += n
        return v

    def rest(self):
        return len(self.d) - self.o


TYPES = {0: "Byte", 1: "Short", 2: "Int", 3: "Float", 4: "String",
         5: "CompoundTag", 6: "Pos", 7: "Int64", 8: "Vec3"}


def parse_metadata(r, strict=True):
    """返回 ({id: (typename, value)}, partial); 严格按 [id][type][tag][value] 读。
    非 strict 时遇到未实现的类型就停下 —— 真实 BDS 包里有 CompoundTag 等更多项,
    交叉核对只关心 NPC 标记项, 不要求解完整个列表。"""
    count = r.uv()
    items = {}
    partial = False
    for _ in range(count):
        i0 = r.o
        iid = r.uv()
        t = r.uv()
        tag = r.uv()
        if t != tag:
            raise ValueError("item %d: type(%d) != tag(%d) @%d" % (iid, t, tag, i0))
        if t == 4:
            val = r.st()
        elif t == 0:
            val = r.d[r.o]
            r.o += 1
        elif t in (1,):
            val = struct.unpack_from("<h", r.d, r.o)[0]
            r.o += 2
        elif t in (2, 7):
            val = r.zz()
        elif t == 3:
            val = r.f32()
        elif t == 6:
            val = (r.zz(), r.zz(), r.zz())
        elif t == 8:
            val = (r.f32(), r.f32(), r.f32())
        else:
            if strict:
                raise ValueError("item %d: unsupported type %d" % (iid, t))
            partial = True
            break
        items[iid] = (TYPES.get(t, str(t)), val)
    return items, partial


def parse_addactor(data, has_header):
    """has_header: 抓包文件含包头（发送时由发送方加）; fixture 产物是纯 payload。"""
    r = Reader(data)
    out = {}
    out["packetId"] = r.uv() if has_header else 13
    out["uid"] = r.zz()
    out["rid"] = r.uv()
    out["identifier"] = r.st().decode("utf-8", "replace")
    out["pos"] = (r.f32(), r.f32(), r.f32())
    out["vel"] = (r.f32(), r.f32(), r.f32())
    out["rot"] = (r.f32(), r.f32())
    out["yHead"] = r.f32()
    out["yBody"] = r.f32()
    n_attr = r.uv()
    out["attrCount"] = n_attr
    for _ in range(n_attr):
        r.st()
        r.f32(), r.f32(), r.f32()
    out["meta"], out["metaPartial"] = parse_metadata(r, strict=not has_header)
    out["propsInt"] = r.uv()
    out["propsFloat"] = r.uv()
    out["links"] = r.uv()
    out["leftover"] = r.rest()
    return out


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    addactor = Path(sys.argv[1]).read_bytes()
    npcdialogue = Path(sys.argv[2]).read_bytes()
    capture = Path(sys.argv[3]).read_bytes() if len(sys.argv) > 3 else None

    print("== 合成载体 AddActor (%d B) ==" % len(addactor))
    a = parse_addactor(addactor, has_header=False)
    check(a["packetId"] == 13, "fixture payload 无包头(发送时补), 视为 AddActor(13)")
    check(a["identifier"] == "minecraft:npc", "identifier = %r" % a["identifier"])
    check(abs(a["pos"][1] - (-66.0)) < 1e-6,
          "position.y = %.2f (must be -66: 世界下方 -> 客户端看不到实体)" % a["pos"][1])
    check(a["attrCount"] == 0, "attributes 为空 (count=%d)" % a["attrCount"])
    check(a["propsInt"] == 0 and a["propsFloat"] == 0, "properties 为空")
    check(a["links"] == 0, "links 为空")
    check(a["leftover"] == 0, "尾部无剩余字节 (leftover=%d)" % a["leftover"])

    m = a["meta"]
    want = [(4, "String"), (39, "Byte"), (40, "String"), (41, "String"), (100, "String")]
    for iid, tn in want:
        got = m.get(iid)
        check(got is not None and got[0] == tn,
              "ActorData id=%d 类型=%s, got %s" % (iid, tn, got[0] if got else None))
    check(len(m) == 5, "ActorData 恰好 5 项 (got %d)" % len(m))
    check(m.get(39, (None, None))[1] == 1, "HasNpc(39) = 1")
    npc_data = m.get(40, (None, b""))[1]
    try:
        j = json.loads(npc_data.decode("utf-8"))
        check(isinstance(j.get("skin_list"), list) and len(j["skin_list"]) > 0,
              "NpcData(40) 是合法 JSON 且含 skin_list (%d 项, %d B)" % (len(j["skin_list"]), len(npc_data)))
        check("picker_offsets" in j and "portrait_offsets" in j,
              "NpcData(40) 含 picker_offsets / portrait_offsets (头像取景)")
    except Exception as exc:  # noqa: BLE001
        check(False, "NpcData(40) JSON 解析失败: %s" % exc)
    acts = m.get(41, (None, b""))[1]
    try:
        ja = json.loads(acts.decode("utf-8"))
        check(isinstance(ja, list) and ja and "button_name" in ja[0],
              "Actions(41) 是按钮数组 (顶层为数组!) -> %s" % (ja[0].get("button_name") if ja else None))
    except Exception as exc:  # noqa: BLE001
        check(False, "Actions(41) JSON 解析失败: %s" % exc)

    print("== NpcDialoguePacket (%d B) ==" % len(npcdialogue))
    r = Reader(npcdialogue)
    npc_id = struct.unpack_from("<Q", npcdialogue, 0)[0]
    r.o = 8
    action_type = r.uv()
    dialogue = r.st()
    scene = r.st()
    name = r.st()
    action_json = r.st()
    check(npc_id == a["uid"] & 0xFFFFFFFFFFFFFFFF or npc_id == a["uid"],
          "mNpcId 与 AddActor 的 uniqueId 一致")
    check(action_type == 0, "ActionType = 0 (Open), got %d" % action_type)
    check(scene == b"main", "sceneName = %r" % scene)
    check(name.decode("utf-8", "replace") == "任务发布员", "npcName = %r" % name)
    check(action_json == acts, "包内 ActionJSON 与 ActorData 的 Actions 一致")
    check(r.rest() == 0, "尾部无剩余字节 (leftover=%d)" % r.rest())

    if capture is not None:
        print("== 交叉核对真实 BDS NPC 生成包 (%d B) ==" % len(capture))
        real = parse_addactor(capture, has_header=True)
        check(real["packetId"] == 13, "真实包包头 = 13 (AddActor), got %d" % real["packetId"])
        check(real["identifier"] == "minecraft:npc", "真实包 identifier = %r" % real["identifier"])
        rm = real["meta"]
        for iid in (4, 39, 40, 41):
            check(iid in rm, "真实包的 ActorData 含 id=%d (%s)" % (iid, rm.get(iid, (None,))[0]))
        check(rm.get(39, (None, None))[1] == 1, "真实包 HasNpc(39) = 1")
        rn = rm.get(40, (None, b""))[1]
        sj = json.loads(rn.decode("utf-8")) if rn else {}
        check(set(sj.keys()) == set(json.loads(npc_data.decode("utf-8")).keys()),
              "真实包 NpcData 的字段与合成包一致: %s" % sorted(sj.keys()))
        rsz = len(rn)
        print("  note  真实包 NpcData=%d B, 合成包 NpcData=%d B (皮肤表条目数可不同)" % (rsz, len(npc_data)))
        check(abs(real["pos"][1]) < 1e4, "真实包位置 y=%.2f（世界内, 合成包改为 -66）" % real["pos"][1])

    print("\n%d passed, %d failed" % (len(OK), len(FAIL)))
    return 1 if FAIL else 0


if __name__ == "__main__":
    sys.exit(main())
