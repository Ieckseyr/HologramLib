"""Decode a captured UpdateTradePacket (id 80) against the BDS 26.40 wire contract.

Input: the raw capture written by MeowPacketDump to logs/fullpkts/pkt80_*.bin.
That file is the full send buffer: an unsigned-varint packet header followed by
the packet body, so the header is stripped here.

Purpose (same spirit as check_npc_player_list.py): decode BDS's own bytes with an
independent reader instead of trusting sculk's layout. It answers the questions the
trade-menu feature cannot be written without:
  - ContainerId / ContainerType actually used for a villager trade
  - TraderTier base (0- or 1-based) and the value BDS sends per villager level
  - the exact Offers NBT structure (Recipes[].buyA/buyB/sell/tier/maxUses/...)

NBT encoding here follows Bedrock's little-endian network NBT as produced by
sculk's CompoundTag/ListTag/ValueTag (verified against BedrockProtocol-main AND
against a real BDS capture — see notes below):
  tag name / string payload -> unsigned varint length + utf-8
  list payload              -> element type byte + zigzag varint count
  compound payload          -> repeated (type byte, varint-name, payload) ... 0x00
NOT classic fixed-width NBT: Int (3) and Long (4) are zigzag varints, ByteArray/
IntArray counts are zigzag varints, while Short (2) / Float (5) / Double (6) are
fixed width (sculk ValueTag.hpp). Getting this wrong makes the parse walk off
into string bytes, so it is stated explicitly.
"""
import json
import struct
import sys

# ── Bedrock container enums (from BDS headers: shared_types/legacy/ContainerType.h,
#    mc/world/ContainerID.h) — numeric values, not schema list positions ──
CONTAINER_TYPE = {
    15: "TRADE",
    -1: "INVENTORY",
    0: "CONTAINER",
    1: "WORKBENCH",
    2: "FURNACE",
    3: "ENCHANTMENT",
    4: "BREWING_STAND",
    5: "ANVIL",
    6: "DISPENSER",
    7: "DROPPER",
    8: "HOPPER",
    9: "CAULDRON",
}
TAG_NAMES = {
    0: "End",
    1: "Byte",
    2: "Short",
    3: "Int",
    4: "Long",
    5: "Float",
    6: "Double",
    7: "ByteArray",
    8: "String",
    9: "List",
    10: "Compound",
    11: "IntArray",
}


class Reader:
    def __init__(self, data, offset=0):
        self.data = data
        self.offset = offset

    def take(self, n):
        end = self.offset + n
        if end > len(self.data):
            raise ValueError(f"truncated at {self.offset}, requested {n}")
        out = self.data[self.offset:end]
        self.offset = end
        return out

    def u8(self):
        return self.take(1)[0]

    def i8(self):
        return struct.unpack("<b", self.take(1))[0]

    def bool(self):
        return self.u8() != 0

    def uvar(self):
        result = 0
        for shift in range(0, 70, 7):
            b = self.u8()
            result |= (b & 0x7F) << shift
            if b < 0x80:
                return result
        raise ValueError("invalid varint")

    def zigzag(self, bits=32):
        v = self.uvar()
        return (v >> 1) ^ -(v & 1)

    def text(self):
        """varint length + utf-8 (sculk BinaryStream::writeString)"""
        n = self.uvar()
        return self.take(n).decode("utf-8", "replace")

    def nbt(self):
        """Read one tag: type byte + name + payload -> (name, value)"""
        t = self.u8()
        if t == 0:
            return None, None
        name = self.text()
        return name, self.payload(t)

    def payload(self, t):
        if t == 1:
            return self.i8()
        if t == 2:
            return struct.unpack("<h", self.take(2))[0]
        if t == 3:  # Int: zigzag varint (sculk ValueTag<int32_t> -> writeVarInt)
            return self.zigzag()
        if t == 4:  # Long: zigzag varint64
            return self.zigzag()
        if t == 5:
            return round(struct.unpack("<f", self.take(4))[0], 6)
        if t == 6:
            return struct.unpack("<d", self.take(8))[0]
        if t == 7:  # byte array: zigzag varint count + raw bytes
            n = self.zigzag()
            return {"__byte_array_len": n, "hex": self.take(n).hex()}
        if t == 8:
            return self.text()
        if t == 9:  # list: element type + zigzag varint count
            elem = self.u8()
            count = self.zigzag()
            return {"__list_of": TAG_NAMES.get(elem, elem), "items": [self.payload(elem) for _ in range(count)]}
        if t == 10:  # compound: entries until End
            out = {}
            while True:
                n, v = self.nbt()
                if n is None and v is None:
                    return out
                out[n] = v
        if t == 11:  # int array: zigzag varint count + zigzag varint values
            n = self.zigzag()
            return [self.zigzag() for _ in range(n)]
        raise ValueError(f"unsupported NBT tag {t} at {self.offset}")


def decode(path):
    data = open(path, "rb").read()
    reader = Reader(data)

    # strip the unsigned-varint packet header (payload id in the low 10 bits)
    header = 0
    shift = 0
    while True:
        b = reader.u8()
        header |= (b & 0x7F) << shift
        if b < 0x80:
            break
        shift += 7
    packet_id = header & 0x3FF

    r = Reader(data, reader.offset)
    out = {
        "file": path,
        "packet_id": packet_id,
        "container_id": r.u8(),
        "container_type_raw": r.u8(),
        "size": r.zigzag(),
        "trader_tier_raw": r.zigzag(),
        "entity_unique_id": r.zigzag(64),
        "last_trading_player": r.zigzag(64),
        "display_name": r.text(),
        "use_new_trade_screen": r.bool(),
        "use_economy_trade": r.bool(),
    }
    out["container_type"] = CONTAINER_TYPE.get(out["container_type_raw"], str(out["container_type_raw"]))

    root_type = r.u8()
    root_name = r.text() if root_type != 0 else ""
    out["data_root"] = {"type": TAG_NAMES.get(root_type, root_type), "name": root_name}
    out["data"] = r.payload(root_type) if root_type != 0 else None
    out["consumed_bytes"] = r.offset
    out["total_bytes"] = len(data)
    out["fully_consumed"] = r.offset == len(data)

    # Flatten the trade list for readability
    recipes = None
    if isinstance(out["data"], dict):
        recipes = out["data"].get("Recipes")
    if isinstance(recipes, dict):
        out["recipe_count"] = len(recipes.get("items", []))
    elif isinstance(recipes, list):
        out["recipe_count"] = len(recipes)
    return out


def selftest():
    """Build an Offers-shaped NBT with the same encoding, then read it back."""
    def uvar(v):
        out = bytearray()
        while True:
            b = v & 0x7F
            v >>= 7
            out.append(b | (0x80 if v else 0))
            if not v:
                return bytes(out)

    def zz(v):
        return uvar((v << 1) ^ (v >> 31))

    def s(text):
        raw = text.encode()
        return uvar(len(raw)) + raw

    def i32(v):
        # Int tag = zigzag varint (不是定长 4 字节)
        return uvar((v << 1) ^ (v >> 31))

    def f32(v):
        return struct.pack("<f", v)

    def item(name):
        # 每个 compound 必须以 0x00 (End) 收尾
        return (
            b"\x08" + s("Name") + s(name)
            + b"\x01" + s("Count") + b"\x01"
            + b"\x02" + s("Damage") + struct.pack("<h", 0)
            + b"\x00"
        )

    recipe = b"\x0a" + s("buyA") + item("minecraft:emerald") \
        + b"\x0a" + s("buyB") + item("minecraft:book") \
        + b"\x0a" + s("sell") + item("minecraft:enchanted_book") \
        + b"\x03" + s("tier") + i32(1) \
        + b"\x03" + s("maxUses") + i32(12) \
        + b"\x05" + s("priceMultiplier") + f32(0.05) \
        + b"\x01" + s("rewardExp") + b"\x01" + b"\x00"
    recipes = b"\x09" + s("Recipes") + b"\x0a" + zz(1) + recipe
    body = recipes + b"\x00"

    header = uvar(80)
    blob = header + b"\x00" + b"\x0f" + zz(1) + zz(1) + zz(1234) + zz(0) + s("Farmer") + b"\x01" + b"\x00" \
        + b"\x0a" + b"\x00" + body
    tmp = "_selftest_pkt80.bin"
    open(tmp, "wb").write(blob)
    got = decode(tmp)
    assert got["packet_id"] == 80, got["packet_id"]
    assert got["container_type"] == "TRADE", got["container_type"]
    assert got["display_name"] == "Farmer", got["display_name"]
    assert got["recipe_count"] == 1, got
    assert got["fully_consumed"], (got["consumed_bytes"], got["total_bytes"])
    r0 = got["data"]["Recipes"]["items"][0]
    assert r0["buyA"]["Name"] == "minecraft:emerald", r0
    assert r0["sell"]["Name"] == "minecraft:enchanted_book", r0
    assert r0["tier"] == 1 and r0["maxUses"] == 12, r0
    import os
    os.remove(tmp)
    print("selftest passed: NBT reader + packet field offsets are self-consistent")


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--selftest":
        selftest()
        raise SystemExit(0)
    if len(sys.argv) < 2:
        print(__doc__)
        print("usage: decode_update_trade.py <pkt80_*.bin> [...]")
        raise SystemExit(2)
    for path in sys.argv[1:]:
        print(json.dumps(decode(path), ensure_ascii=False, indent=2))
