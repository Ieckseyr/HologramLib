# Capability domain guide · Rendering & text

Covers 6 domains: **Shape rendering**, **Holographic text**, **Gradient lines**, **Item detail**, **FMBE item display**, **Particle shapes**.

None of these domains **creates a real entity or writes to the save** (the one exception is FMBE item display, which uses a "fake entity + packets"); what the client sees exists only in client memory.

| Domain | C++ | LSE | In one line |
|---|---|---|---|
| Shape rendering | `IShapeDrawer` | `shape*` · 39 | line / box / circle / sphere / arrow / text |
| Holographic text | `IHologramText` | `holo*` · 26 | multi-line text, whole-block styling, dynamic lines |
| Gradient lines | — | `gradient*` · 11 | multi-colour gradient trail lines |
| Item detail | `IItemDetail` | `itemDetail*` · 2 | automatically translates "钻石 x64" ("diamond x64") |
| FMBE item display | `IItemDisplay` | `itemDisplay*` · 33 | a fox holding an item; Molang-driven animation |
| Particle shapes | `IParticleShape` | `particle*` · 22 | point / line / plane / box frame + move, rotate, follow |

---

## Shape rendering

```cpp
auto line = lib.shapes().createLine(0, 64, 0, 10, 64, 10);
lib.shapes().setColor(line, 1.0f, 0.2f, 0.2f, 1.0f);   // components 0.0 ~ 1.0
lib.shapes().setDuration(line, 10.0f);                 // unit: seconds
lib.shapes().draw(line);
```

**Common conventions** (shared by three domains):

- every `id` is an `int64_t`, and a failed creation returns `< 0`;
- colour components are `0.0 ~ 1.0` (RGBA), coordinates are world coordinates, and `setDuration` is in seconds;
- the methods are mutually exclusive internally (thread-safe), but **the packet is sent on the calling thread → call from the main thread**.

### The text shape's three switches (completed in `26.40.8`)

A text shape is only one branch of the shape domain, but it has three properties other shapes do not:

| Capability | Method | Notes |
|---|---|---|
| Background box | `setBackgroundColor` / `clearBackgroundColor` | unset = the client's default colour |
| Through walls | `setDepthTest` | `false` = always rendered (**the default, visible through walls**); `true` = occluded by blocks / entities |
| Three-axis rotation | `setRotation` / `clearRotation` | Euler `[Pitch, Yaw, Roll]`, **unit = degrees** (aligned with the official scripting API's convention; the library does no conversion). Once set, the text no longer faces the camera |

> **These two are fixed bugs, and worth knowing about**: ① `setRotation` used to be **silently ineffective** (it wrote `mRotation` but never `mUseRotation`); ② `destroyBatch` / `destroyAll` used to **clear only server-side memory and send no removal packet** (leaving stale visuals on the client). If you wrote workaround code for the old behaviour, you can delete it now.

---

## Holographic text

### The composition model: multiple lines = one shape

```cpp
auto holo = lib.holograms().create(0, 70, 0);
lib.holograms().addLine(holo, "§eWelcome to spawn");
lib.holograms().addLine(holo, "Online: {online}");
lib.holograms().setScale(holo, 1.5f);
lib.holograms().draw(holo);
```

**A line = text**: multiple lines merge into a **single** text shape (joined by `'\n'`), and the whole block shares one colour / scale / background box / depth test / rotation.

This is decided by the protocol, not a shortcut in the implementation: protocol 2168's `TextDataPayload` field order is
`Text(0) / UseRotation(1) / BackgroundColor(2, optional) / DepthTest(3) / ShowBackface(4) / ShowTextBackface(5)`
— **there are no per-line colour / scale / gradient / line-spacing fields**, and `lineGapHeight` does not exist at all in protocol 2168.

> **The 26.51 line (protocol 2193)**: `TextDataPayload` **gains a `LineGapHeight`(float) between `BackgroundColor` and `DepthTest`**
> — this is the only difference from 2168 to 2193 on this table. On 26.51 the field **must be emitted explicitly** (not emitting it misaligns every field after it and the client fails to parse),
> and the library sends `kTextLineGapHeight = 0.0f` (= the protocol default value, i.e. no extra line spacing, see `src/ProtocolShape.h`).
> Per-line colour / scale / gradient still do not exist on 2193; to lay multiple lines out more loosely, this one constant is what you tune.

| What you want | The right way |
|---|---|
| a different colour for one line | embed `§` colour codes in the text |
| to scale the whole block up | `holoSetScale` |
| line spacing | lay it out yourself with **multiple** text shapes |

> ⚠ **Those 8 per-line APIs were removed wholesale in `26.40.8`** (`holoSetLineGradient` / `holoSetLineRainbow` / `holoSetLineColor` / `holoSetLineScroll` / `holoSetVerticalAnimation` / `holoSetLineSpacing` / `holoSetLineScale` / `holoTick`).
> They had been exported since `26.10.4`, yet there was never a matching protocol capability underneath — per-line styling only ever took effect on line 0, rainbow mode was measured painting the text white, the line-spacing field did not exist, and nothing consumed the self-driven tick's offset.
> **The ABI is safe**: these 8 methods are kept in the C++ interface as **deprecated empty slots** (non-pure + inline empty body + `[[deprecated]]`), the vtable slots are exactly the same as 1.25.0, and a pre-compiled consumer binary that has not been recompiled will not misalign. Recompiling gets you a **compile-time deprecation warning**; on the LSE side `ll.import` gets `null` (an explicit failure, not a silent behaviour change). The real cleanup of the slots waits for the next major version.

### Dynamic lines (content-pool rotation + per-line variables)

```cpp
lib.holograms().setLinePool(holo, 0, {"§aOnline: {online}", "§eTPS: {tps}"}, 2000); // one round every 2000ms
lib.holograms().setLineParseVariables(holo, 1, true);   // per-line variable switch, on by default
```

- the content pool **rotates by taking the time modulo the interval**, and is **stateless** — several holograms using the same pool and the same interval are naturally in phase;
- the refresh is carried by the library's **0.5 s throttle**, and it **only resends when the content changed**;
- variable set: `{player}` / `{online}` / `{dimension}` / `{x}` / `{y}` / `{z}`.

> This part was **refactored from [Phantom](https://github.com/GroupMountain/Phantom)** (LGPL-3.0); attribution is in the project README and `src/FloatingTextManager.h`.

### Per-player variables (per-viewer overrides)

As soon as the text contains `{var}` and the target is not a single player, the library **switches to keeping one shape per viewer**, each receiving a result resolved for that viewer (when a viewer goes offline / leaves the dimension, that shape is destroyed in place).

```cpp
lib.holograms().addLine(holo, "§b{player}§r hello");   // every player sees their own name
```

Two costs you must know about:

- **the shape count becomes N times larger** (N = the number of viewers), and the resend complexity grows with it — it suits **small-audience cases** like a party-leader view or quest tracking, not high-frequency refreshes broadcast to the whole server;
- when the variables are cleared, or the target is switched to a single player, it switches back to a shared shape automatically.

### Lifecycle and resend

- **never set a `duration` on a permanent hologram** — leaving it unset is what avoids the client's countdown deleting it;
- the client persists nothing, so a reconnect must be covered by a resend. The resend strategy is centralised in the shape domain (see the [architecture overview](../architecture/overview.md#resend-strategy));
- **following a player no longer depends on an internal tick** (since `26.40.8`): the position is **resolved in place** when `setFollowPlayer` / `draw` / `refresh` is called, and on-demand refresh is the consumer's job (example: ZXPanel resends in place every 2 ticks).

---

## Gradient lines (LSE only)

`gradient*` has 11 functions and no C++ interface. It builds its own batch of line-segment shapes, using a multi-colour gradient to imitate a trail line.

`gradientTick` is **consumer-driven** (an LSE export, called by the script per tick) — the library does not animate on the consumer's behalf.

---

## Item detail

`itemDetail*` (2 functions) translates an item stack into human-readable text, e.g. `minecraft:diamond` + count 64 → "钻石 x64" ("diamond x64"). It suits sidebar / tooltip text, and does no rendering.

---

## FMBE item display

```cpp
hologramlib::ItemDisplayConfig cfg;
cfg.x = 100.5f; cfg.y = 65.0f; cfg.z = -200.5f;
cfg.rotY = "math.sin(query.life_time*90)*360";   // Molang expression
auto disp = lib.itemDisplays().create(cfg);
```

"Fox + packets": it spawns a **partially invisible fake entity** holding an item, visible only to the target player. Three-axis rotation / translation / scale all accept **Molang expressions** (driven by things like `query.life_time`), so animation needs no per-tick packet pushing from the server.

Other capabilities: seamless creation (`createSeamless`), a visible-player whitelist, view distance, `scaleTo`.

> ⚠ **Technical debt (known)**: ItemDisplay has an **independent copy of the send implementation**, and does not go through the library-wide shared `SculkPacketSend.h`. The header comment in `SculkPacketSend.h` records "migrate it in once it is verified stable". Do not forget this copy when you change the send logic.

---

## Particle shapes

```cpp
auto wall = lib.particleShapes().createPlane(
    "Steve", 0, 100, 64, -200, 48, 32, /*axis=*/2, /*step=*/3,
    "minecraft:heart_particle", /*interval=*/10, /*lifetime=*/0);

lib.particleShapes().setVisiblePlayers(wall, {"uuid1", "uuid2"});  // empty = everyone in the dimension
lib.particleShapes().moveTo(wall, 150, 64, -200, 100);            // a smooth 5-second slide
```

Shapes: point / line / rectangular ring / filled plane / box frame / six faces / polyhedron.
Transforms: `moveTo` (anchor interpolated with `easeOutCubic`, **the whole plane moves with the anchor**), rotation, spin, following a player.

**Send path**: it goes through the vanilla `SpawnParticleEffectPacket` batch channel — BDS's tick flush aggregates and compresses them into a single Batch datagram automatically. This path is the same as the "vanilla particle broadcast", so the library's self-driven sampling / view-distance culling / periodic resend belong to the **engine's own path**, not to animating on the consumer's behalf.

---

## Related

- Full signatures: [C++ API reference](../reference/cpp-api.md) (§1.2 shapes / §1.3 holographic text / §1.4 item detail / §1.5 item display / §1.8 particles)
- LSE function list: [LSE API reference](../reference/lse-api.md) (§2.1 ~ §2.5, §2.8)
- Resend and cost: [Architecture overview](../architecture/overview.md) · [Protocol-layer implementation notes](../architecture/protocol-notes.md)
