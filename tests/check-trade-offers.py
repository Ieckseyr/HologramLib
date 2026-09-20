"""Byte-compare the C++ Offers builder against BDS's own tradedata.

Why this exists: the trade menu's core is the `Offers` NBT that goes into
UpdateTradePacket.Data. Its exact shape is not documented anywhere — it was
derived from a real capture (logs/fullpkts/pkt80_*.bin). This check keeps that
derivation honest: the bytes produced by trade::buildOffers must equal the bytes
BDS itself wrote, for the same trade data.

Usage:
    check-trade-offers.py <captured pkt80_*.bin> <generated offers.bin>

Import of the decoder is deliberate — the capture's Data offset must be located
with the same reader that was validated against BDS bytes.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from decode_update_trade import Reader, decode  # noqa: E402


def locate_data_offset(path):
    """Walk the packet fields with the validated reader; return the Data offset."""
    data = Path(path).read_bytes()
    r = Reader(data)
    header, shift = 0, 0
    while True:  # unsigned varint packet header
        b = r.u8()
        header |= (b & 0x7F) << shift
        if b < 0x80:
            break
        shift += 7
    r.u8()                  # ContainerId
    r.u8()                  # ContainerType
    r.zigzag()              # Size
    r.zigzag()              # TraderTier
    r.zigzag()              # EntityUniqueId
    r.zigzag()              # LastTradingPlayer
    r.text()                # DisplayName
    r.bool()                # UseNewTradeScreen
    r.bool()                # UsingEconomyTrade
    return r.offset


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    capture, generated = sys.argv[1], sys.argv[2]

    offset = locate_data_offset(capture)
    expected = Path(capture).read_bytes()[offset:]
    actual = Path(generated).read_bytes()

    # sanity: the capture must still decode cleanly (guards against a stale sample)
    decoded = decode(capture)
    assert decoded["fully_consumed"], "captured packet no longer decodes fully — sample changed?"

    if actual == expected:
        print(f"OK: offers match BDS byte-for-byte ({len(actual)} bytes, "
              f"{decoded['recipe_count']} recipes, tier={decoded['trader_tier_raw']}, "
              f"displayName={decoded['display_name']!r})")
        return 0

    print(f"MISMATCH: generated {len(actual)} bytes, BDS wrote {len(expected)} bytes")
    for i in range(min(len(actual), len(expected))):
        if actual[i] != expected[i]:
            lo = max(0, i - 8)
            print(f"  first difference at offset {i} (0x{i:x})")
            print(f"    BDS      : {expected[lo:i + 12].hex(' ')}")
            print(f"    generated: {actual[lo:i + 12].hex(' ')}")
            break
    else:
        print("  common prefix identical; length differs")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
