"""Independently decode the C++ fixture against Cloudburst's v2168 wire contract.

This does not use Sculk's reader: a matching encoder/decoder bug must not pass.
Reference: CloudburstMC/Protocol, codec/v2168/PlayerListSerializer_v2168.java
and BedrockCodecHelper_v2168.readSkin/writeSkin.
"""
import json
import struct
import sys
from pathlib import Path


class Reader:
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def take(self, size):
        end = self.offset + size
        if end > len(self.data):
            raise ValueError(f"truncated at {self.offset}, requested {size}")
        result = self.data[self.offset:end]
        self.offset = end
        return result

    def uvar(self):
        result = 0
        for shift in range(0, 70, 7):
            byte = self.take(1)[0]
            result |= (byte & 127) << shift
            if byte < 128:
                return result
        raise ValueError("invalid varint")

    def text(self):
        return self.take(self.uvar()).decode("utf-8")

    def u32(self):
        return struct.unpack("<I", self.take(4))[0]

    def image(self):
        width, height = self.u32(), self.u32()
        data = self.take(self.uvar())
        assert len(data) == width * height * 4, "image byte count mismatch"
        return width, height, data


def check(directory):
    add = (directory / "PlayerList-add.body.bin").read_bytes()
    r = Reader(add)
    assert (r.uvar(), r.uvar(), r.take(1)[0]) == (1, 1, 0), "2168 Add framing mismatch"
    uuid = r.take(16)
    assert uuid == struct.pack("<QQ", 0xF0B34E5043000000, 42)
    assert r.uvar() == 0x6F000001 * 2, "ActorUniqueId zigzag mismatch"
    assert r.text() == "NPC-wire-check"
    assert r.text() == "0"
    assert r.text() == ""
    assert r.u32() == 1
    skin_id = r.text()
    assert skin_id == "HoloLibNpcSkin_wire-check"
    assert r.text() == ""  # PlayFabId
    assert json.loads(r.text()) == {"geometry": {"default": "geometry.humanoid.custom"}}
    assert r.image() == (64, 64, b"\xff" * (64 * 64 * 4))
    assert r.uvar() == 0  # animations
    assert r.image() == (0, 0, b"")
    assert r.text() == "{}"
    assert r.text() == "1.12.0"
    assert r.text() == ""  # animation data
    assert r.text() == ""  # cape id
    full_id = r.text()
    assert full_id == skin_id
    assert r.take(1) == b"\x01", "wide must be a byte, not a string"
    assert r.u32() == 0xFF123456, "skin color must be a little-endian integer"
    assert r.uvar() == 0  # Persona pieces
    assert r.uvar() == 0  # tint maps
    assert r.take(5) == b"\0\0\0\0\x01"  # premium/persona/cape/primary/override
    assert r.text() == "true", "trusted flag must be inside SerializedSkin"
    assert r.text() == ""  # ProfileHash: present but empty
    assert r.take(3) == b"\0\0\0"  # teacher/host/subclient
    assert r.u32() == 0  # PlayerList color
    assert r.offset == len(add), "unexpected trailing bytes (legacy trusted bool?)"

    remove = (directory / "PlayerList-remove.body.bin").read_bytes()
    assert remove == b"\x01\x00\x01" + uuid, "2168 Remove framing mismatch"
    framed = (directory / "PlayerList-add.packet.bin").read_bytes()
    assert framed == b"\x3f" + add
    return {"protocol": 2168, "add_bytes": len(add), "add_body_prefix": add[:24].hex(" "),
            "remove_bytes": len(remove), "remove_body_prefix": remove.hex(" "),
            "skin_id": skin_id, "full_id": full_id, "trusted": True,
            "decoder_consumed_bytes": r.offset, "validation": "independent decoder passed",
            "scope": "offline serialization fixture; no BDS/client/network session"}


if __name__ == "__main__":
    output = Path(sys.argv[1])
    result = check(output)
    (output / "wire-check.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False, indent=2))
