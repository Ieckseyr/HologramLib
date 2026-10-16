# Protocol-layer implementation notes

This page records the **protocol-layer facts shared across domains**: the send primitive, wire-format traps, tick semantics, and the offline checks that pin them down.

> When debugging a "packet does not go out / the client drops the update" problem, start with the **wire-format traps** and **tick semantics** sections here.

---

## The send primitive: SculkPacketSend

Apart from ItemDisplay, which keeps a copy of its own (known technical debt), the whole library shares one send path:

```
sculk serialisation builds the packet body
      ▼
BDS createPacket of the same type → read-back check (decode the bytes just built and compare)
      ▼
manually prepend the VarUInt packet header
      ▼
NetworkPeer::sendPacket sends the raw bytes
```

**Why the read-back check**: the protocol library's write path and BDS's actual wire format can disagree (the "wire-format traps" below include two real cases). The read-back check turns "wrote it wrong" into a failure **before sending**, rather than into a client crash.

**How failures become visible**: `sculkSendFailureCount()` is the **library-wide shared count of read-back check failures** — the only entry point for a silent failure (emitted with `describeFor`). The library itself prints no logs at all, so this counter is the first stop when debugging.

---

## Wire-format traps

Every one of these has been hit, and each has a packet capture or a crash as evidence. **Read this section before changing any hand-written packet body.**

> **The complete set of differences between the 26.51 line (protocol 2193) and 26.40 (2168)** (2026-10-16, comparing the two lines' protocol library `include/` file by file):
> ① `PrimitiveShapes.hpp`: `TextDataPayload` has one extra `LineGapHeight`(float) between `BackgroundColor` and `DepthTest`
> — it must be emitted explicitly (the library uses `kTextLineGapHeight = 0.0f`), otherwise every field after it is misaligned;
> ② `ItemStackResponse`'s containers became optionally encoded — a success response always writes the `presence` byte (the virtual container's interactive mode sends `presence=false`);
> ③ `PlayerAuthInputPacketPayload::mInputData` changed from `std::bitset<66>` to `Bedrock::EnumSet<InputData, 66>`
> (there is no `operator[]`, so the view-override mask now uses `contains()`);
> ④ packet IDs did not move (`MinecraftPacketIds.hpp` and `codec/actor/ActorDataIDs.hpp` are **byte-identical** across the two lines).
> The rest of the wire format (every item below on this page) is the same on both lines.

### 1. Two NBT dialects exist, and they cannot be mixed

| Dialect | Used for | Format |
|---|---|---|
| **BDS item User Data dialect** | User Data inside item descriptors | name length **u16 little-endian**, integers and counts **4-byte little-endian** |
| **sculk `CompoundTag` dialect** | `UpdateTrade`'s Offers, block entity NBT / NpcData | the **varint** one |

> **A lesson paid for in blood**: the fake inventory's first version hand-wrote item descriptors using the sculk dialect → **the client parsed out of bounds and froze outright**; and because the session was not cleaned up when the player dropped, a tick-driven resend fired on reconnect → which showed up as "**cannot get into the server**".
>
> Conclusion: **let BDS's own `InventoryContentPacket` / `InventorySlotPacket` build the items** (`src/fakeinv/FakeInventoryPackets.h`); the container name and the storage item values also follow the capture (BDS writes container name byte `0` and an empty storage descriptor).
> The other domains (trade menu / virtual container / NPC dialogue) still hand-write the whole packet body — those two segments really are the sculk dialect, and all of them have been byte-compared.

### 2. Colour is **ARGB**, not the old byte order

int32 colour is packed as **ARGB in v2168**. The library used v944's byte order for years → **the R/B channels are swapped** (fill in red and you get blue), and shapes / text colour / background boxes are all affected.

The packer `toPacked` and the unpacker `getColor` were both corrected together (`26.40.8`). **If you previously back-filled values to make a colour come out right, put the literal values back.**

### 3. `SetActorDataPacket` / `UpdateAttributesPacket` both carry `mTick`

In the 26.40 wire format both packets **carry `mTick`**, and **filling in 0 makes the client discard them as stale packets** — the metadata in the spawn packet is all correct, yet nothing changes on screen.

When you build these two packets yourself you must fill in `currentViewTick()`. By contrast, the customentity domain has always sent them with a tick (`sadp.mTick = tick`), which is why that side has always been fine.

### 4. The protocol value of `NametagAlwaysShow` **is inverted**

**`0` = always shown**. Do not fill in `1` on intuition.

### 5. The y a player reports at the protocol layer is at the **eyes**

This is Bedrock-specific: the y in `PlayerAuthInput` is the eye position, **not the feet position**.
When you need the real feet position, take the bottom of the hitbox, or do an "eye to feet" delta conversion (standing / sneaking / swimming all differ).

### 6. A dropped item's spawn packet goes through `AddItemActor`, not `AddActor`

`AddItemActor` = id **15**. If you intercept only `AddActor`, "re-type / hide a dropped item" is always only half done —
the moment you set the override you can replace what is visible then, but the `AddItemActor` the engine sends afterwards **pushes the item icon back into the client** (capture evidence: after `RemoveActor` + `AddActor(minecraft:cod)`, an `AddItemActor` with the same `uniqueId` arrives **13ms** later).

> One recipe detail along the way: when you build a dropped-item spawn packet yourself, **do not** copy `ItemActor`'s `maxHealth = 0`; use the verified recipe of `health 20 / scale 1`.

### 7. What the client requires is not necessarily what the protocol library writes

The trade menu's "trade-slot slot correction" segment: **the protocol library `sculk`'s wire format does not match what the client requires** (the protocol library writes one extra string field, and writes `DurabilityCorrection` as a varint while the client wants a fixed-length `short`) — **hand-writing it to match the protocol library crashes the client outright** (already hit).

This one is not "wrote it wrong" but "the library and the client see the same segment differently". In this situation **do not force it to fit**; take a path that works (that is what the trade menu did: switch to a real trade table or a container UI).

---

## A few wire-format rules that "must be written this way"

| Where | Must be | Otherwise |
|---|---|---|
| NPC dialogue carrier position | **`y = -66`** (below the world) | the client can see the entity; switching to an "invisible flag" makes the dialogue portrait **disappear along with it** |
| `PlayerList` packet-body prefix | validated before sending: Add `01 01 00`, Remove `01 00 01` | on an abnormal prefix the library **drops the packet and records the actual prefix** (not silent) |
| Dropped-item spawn packet attributes | `health 20 / scale 1` | see trap 6 |
| Self-built spawn packet | the flags metadata entry + `minecraft:health` / `minecraft:scale` attributes, **and immediately after it an `UpdateAttributesPacket`** | the client **only initialises attributes in a spawn packet without applying them**; a heartbeat resend on join makes the client report an error |
| `EntityView`'s pose / color types | `ColorIndex(3)` / `Color2Index(82)` = **Byte**; `Variant(2)` / `MarkVariant(43)` = **Int** | the types were taken from **decoding the engine's own real spawn packet** (a real sheep), not guessed from a protocol table |

---

## Item requests have two packet channels

A client's `ItemStackRequest` does not have only one way of being delivered:

| Channel | Form | Who uses it |
|---|---|---|
| `ItemStackRequestPacket`(147) | a standalone request, with actions in cereal form | taking and placing in container slots (**the main channel** in practice) |
| `PlayerAuthInputPacket`(144, AuthInput) | **embeds** `mItemStackRequest` (input flag `PerformItemStackRequest = 36`) | menu-screen interaction (in practice: **on touch there may not be a single 147**) |

The library **hooks both**: the AuthInput one uses BDS's own `ItemStackRequestCereal::toActionData()` to turn the parsed actions into the same cereal form as 147, and then feeds them into the same dispatch function.

> **AuthInput only observes, it never intercepts** — it also carries player movement, and intercepting it would swallow the movement too.

---

## The timing floor for container screens

Opening a container = "place a block + a block entity on the client side" → wait N ticks → send `ContainerOpen`. That wait is **inherent to the approach** (sending the two back to back does not open the screen in practice, because the client has not applied the block yet).

**Measured (26.40, local client): 7 ticks opens, 6 does not.** The library defaults to 7 (≈350ms), half of the equivalent 14 ticks in the reference implementation GMLIB. See [Interfaces & containers · open delay](../domains/interfaces.md#the-open-delay-where-the-7-ticks-come-from-measured).

---

## Offline regression checks

The scripts under `tests/` **pin the formats above down at the byte level** and need no running server. They are **not shipped with the library** (they are in `.gitignore`), and some need real captures under `logs/fullpkts/` on your machine.

| Script | What it verifies | Must be re-run after changing |
|---|---|---|
| `check-no-diagnostics.bat` | **the release invariant**: switch polarity (when off, the marker must not enter the target file; with `-DHOLOGLIB_DIAG_LOG=1` it must) + no in-library log prefix may appear in the built DLL | `src/DiagLog.h` or any logging call |
| `check-trade-packet.bat` | the whole `UpdateTrade` packet (9 packet-level fields + Offers NBT) **byte-compared** against a BDS capture | packet construction in `trade/` |
| `check-trade-offers.bat` | only the Offers NBT segment (a faster single-item check) | same as above |
| `check-trade-settlement.bat` | the settlement decision logic (**43 assertions**, pure logic, no capture needed) | `trade/TradeSettlementLogic.h` |
| `check-container-packets.bat` | the virtual container's four packets: `ContainerOpen` **byte-compared whole-packet** against a real capture; `UpdateBlock` / `BlockActorData` / `ContainerClose` decoded back and checked field by field (including the single-chest 27-slot and double-chest 54-slot pairing) | `container/ContainerPackets.h` |
| `check-fake-inventory.bat` | the fake inventory's wire format (**10 assertions**): the frame lands at the end of the packet, container id 0 / 36 slots / container name byte 0 / empty storage descriptor, and the item UserData **dialect fingerprint** (`06 00 "Damage"` rather than `06 "Damage"`) | `fakeinv/FakeInventoryPackets.h` |
| `check-npc-carrier.bat` | the synthetic NPC carrier packet: AddActor field order and full tail consumption, position `y=-66`, the ids and types of the ActorData five markers, cross-checked against a real BDS NPC spawn packet | `npcdialog/NpcCarrierPacket.h` |
| `check-npc-playerlist.ps1` | `PlayerList` 2168 fields, the trusted-flag position, `Id` / `FullId` and full packet-body consumption | the skin / PlayerList send path |
| `check-view-override.bat` | the view-override decision logic (**59 assertions**, pure logic) | `view/ViewOverrideLogic.h` |
| `check-authinput.bat` | AuthInput decoding | AuthInput parsing |

**Two hard rules**:

1. **Real capture samples are not shipped with the library.** With no sample a script prints `no captured ... found` and exits non-zero — either pass your own capture path in as an argument, or run only the checks that need no sample (`check-no-diagnostics.bat` and the two pure-logic checks need none).
2. **Do not change a capture sample to "take the latest".** For example, the trade-menu check defaults to `logs/fullpkts/pkt80_194951_007.bin` (the fixture's derivation sample, deterministic); a later capture may be a trade the plugin itself generated (different recipe), and switching to "latest" makes the comparison meaningless.

The checks' intermediate artefacts land in `work/` (fixture build outputs + generated packet bytes) and are rebuilt by the scripts — also in `.gitignore`.

---

## What to re-scan when the protocol is upgraded

When you upgrade BDS / the protocol version, the delta in these three places is the main source of work; scanning them regularly is recommended:

| Source | What to look for |
|---|---|
| `.cache/schema-*/` | field additions and removals (fields like `lineGapHeight` that you "assume exist but do not") |
| `bedrock_server_mod.exe` string scan | the field names and enums the engine actually uses |
| `plugins/BedrockProtocol-main/sculk/protocol/codec` | the protocol library's read/write implementation |

Only agreement across all three counts as the truth — several conclusions inside the library (the text payload has no per-line fields, `SetActorData` carries `mTick`, dropped items go through `AddItemActor`) were all established by cross-checking at these three levels.

---

## Related

- Layering and the resend strategy: [Architecture overview](overview.md)
- Per-domain usage and pitfalls: [Capability domain guide](../domains/README.md)
- Why the decision logic can be tested offline: [README · Design notes](https://github.com/Ieckseyr/HologramLib/blob/main/README.en.md#design-notes)
