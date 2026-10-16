# Architecture overview

How HologramLib is put together, and why it costs what it costs.

[中文](overview.md) · [English](overview.en.md) · [Docs home](../index.en.md)

For why the boundaries sit where they do (what may live in the library and what may not), see [README · Design notes](../../README.en.md#design-notes). For wire formats, the send primitive and capture-based verification, see [Protocol notes](protocol-notes.md) *(zh)*.

---

## Layering

```
Consumer
  ├─ LSE (JS):  ll.import("HologramLib", "holoCreate") ...
  └─ C++ plugin: IHologramLib::getInstance().holograms() ...

        ▼
┌─ Contract layer ────────────────────────────────────────────┐
│ include/hologramlib/HologramLib.h                            │
│ Pure-virtual interfaces (IHologramLib + one per domain),      │
│ frozen ABI, monotonically increasing version macro            │
├─ Bridge layer ──────────────────────────────────────────────┤
│ src/HologramLibImpl.cpp   pure forwarding, no business logic  │
│ src/lse/LseBridge.h       exportAs → LegacyRemoteCall         │
│ src/*Exporter.cpp         namespace "HologramLib", one prefix │
│                           per domain                          │
├─ Domain layer ──────────────────────────────────────────────┤
│ PacketDebugRenderer       shape domain: shape table + resend  │
│ FloatingTextManager       holographic text (reuses shapes)    │
│ GradientLineManager       gradient lines (its own segments)   │
│ ParticleShapeManager      particle shapes (engine's own path) │
│ itemdisplay / playernpc / view / trade / container /          │
│ npcdialog / sensing / fakeinv / sulfur / itemdetail / ghost   │
├─ Protocol layer ────────────────────────────────────────────┤
│ ProtocolPackets.h   DebugShape list → PrimitiveShapesPacket   │
│ SculkPacketSend.h   the shared send primitive                 │
│ view/OutboundViewHook  NetworkSystem::send hook (intercept    │
│                        only) + Level::$tick heartbeat         │
└─────────────────────────────────────────────────────────────┘
Event layer: ll::event::EventBus (PlayerJoin / Disconnect / ServerLevelTick / ServerStarted)
             + LL_TYPE_INSTANCE_HOOK instance hooks (outbound send, inbound AuthInput, …)
```

Three things worth noting:

- The bridge layer is stateless. Interface calls land directly on the domain layer, and LSE exports take the same path as the C++ interface; there is no "JavaScript-only logic".
- The shape domain is the only exit that writes shapes into packets. Holographic text and gradient lines both reuse it (a hologram is a multi-line text shape; a gradient line is a batch of line segments).
- There is one send primitive. Apart from ItemDisplay, which keeps its own copy (known technical debt), the whole library shares `SculkPacketSend.h`.

---

## Source layout

```
HologramLib/
├── include/hologramlib/HologramLib.h   # the only public header (interface compatibility documented at the top)
├── src/                                # internal implementation — not API, free to change
│   ├── HologramLibImpl.cpp             #   interface implementation (delegates to domain singletons)
│   ├── EventIdCompat.h                 #   event-id alignment (required by MSVC builds)
│   ├── DiagLog.*                       #   diagnostic logging switch (off at compile time by default)
│   ├── SculkPacketSend.h               #   the protocol-layer send primitive
│   ├── PacketDebugRenderer.*           #   shape rendering
│   ├── ProtocolShape.h / ProtocolPackets.*
│   ├── FloatingTextManager.*           #   holographic text (one block shape + per-viewer shapes)
│   ├── GradientLineManager.*           #   gradient lines
│   ├── itemdetail/ itemdisplay/ customentity/ particles/ ghost/
│   ├── trade/ container/ interaction/ npcdialog/ sensing/
│   ├── playernpc/ fakeinv/ sulfur/ view/
│   ├── lse/                            #   LSE compatibility layer (mounts lrca at runtime)
│   ├── MemoryOperators.cpp             #   cross-DLL memory pairing
│   └── ModEntry.*                      #   lifecycle + two-phase LSE mounting
├── tests/                              # offline byte-level regression checks (local tools, not shipped)
├── docs/                               # this documentation
├── xmake.lua · manifest.json
└── README.md
```

---

## The client-side in-memory shape model

Every rendering behaviour in the library rests on this model:

- DebugDrawer shapes (shapes / text / holograms / gradient lines) exist only in client memory. The server does not track their display state; it only keeps the table of shapes that should be sent.
- Sending the same `networkId` again overwrites in place on the client (idempotent, no side effects). Every resend strategy rests on this property: a resend does not flicker, does not stack, and needs no delete-then-recreate.
- Removal is an empty-payload shape with the same `networkId` (`makeRemoveShape`).

Since `26.40.8`, `destroyBatch` / `destroyAll` also send removal packets. Before that they only cleared server-side memory, leaving stale visuals on the client.

---

## Resend strategy

The client persists nothing, so reconnects and dropped packets both have to be covered by the server. There are three moments:

| Moment | Behaviour | Where |
|---|---|---|
| Player joins | Two delayed resends at +1 s and +5 s after `PlayerJoinEvent` (covers packet loss during client loading) | `PacketDebugRenderer::init()` |
| Periodic safety net | Accumulated per tick on `ServerLevelTickEvent`, then a full silent resend to everyone every 15 s | `tickResend()` |
| No shapes at all | A quick locked check on `mShapes.empty()` returns immediately | `tickResend()` |

Complexity: a full resend costs `O(shape count × online players)` packet construction plus serialisation, every 15 s. Per-viewer hologram shapes go through the same path; each viewer's shape counts as its own shape.

> This is the first wall you hit when scaling. With shapes in the thousands and hundreds of players, the packet-construction volume per 15 s becomes observable load. To estimate your own case, add "tiered resend by dimension / dirty flags" to your requirements; it is on the maintainer's optimisation candidate list.

---

## The holographic-text composition model

| Fact | Detail |
|---|---|
| A line is text | Multiple lines merge into one text shape (joined by `'\n'`), sharing a single colour / scale / background / depth-test / rotation |
| Per-viewer shapes exist only when needed | Only when the text contains `{var}` and the target is not a single player does the library build one shape per viewer (`rebuildTextShapesPerViewer`); clearing the variables or targeting one player switches back to a shared shape |
| Following resolves in place | Since `26.40.8`, the follow position is resolved when `setFollowPlayer` / `draw` / `refresh` is called, with no internal tick. On-demand `refresh` is the consumer's job (ZXPanel, for example, resends in place every 2 ticks) |
| Permanent holograms never set a duration | `mTotalTimeLeft` stays empty; setting it lets the client's countdown delete the hologram |

---

## Self-driven work: what moves on its own

The "basic wheels" principle: internal self-driving is allowed in exactly two categories, delivery guarantees (resend / heartbeat) and the engine's own path (particle broadcast). It may never animate on the consumer's behalf.

| Self-driven item | Nature | Verdict |
|---|---|---|
| 15 s safety-net resend / join resend | Delivery guarantee | Kept |
| View-override per-tick heartbeat | Delivery guarantee (re-push overrides) | Kept |
| Hologram dynamic-line 0.5 s throttled refresh | Delivery guarantee (only resends when content changed) | Kept |
| Particle domain (sampling / view-distance culling / periodic resend) | BDS tick flush aggregates into one Batch, the same path as vanilla particle broadcast | Kept |
| `gradientTick` | Consumer-driven (an LSE export, called by the script per tick) | Kept |
| Hologram self-driven animation / `holoTick` | Animating on the consumer's behalf | Removed in `26.40.8` |

To judge whether an internal timer is legitimate, ask: is it making sure something arrives, or is it deciding how something moves?

---

## View override at runtime

- Outbound hook: `NetworkSystem::send` / `sendToMultiple`. It only decides pass or drop, and never mutates engine packet fields.
- Heartbeat: `Level::$tick` calls `tickPulse()` every tick (re-pushing overrides when an entity re-enters view or a player changes chunk).
- Data comes only from protocol packets (inbound AuthInput, spawn-packet payloads, Animate / ActorEvent); server-side entity state is never read, identity resolution being the one exception.
- Decision logic is isolated in `src/view/ViewOverrideLogic.h` and touches no BDS types, so it is testable offline (`tests/check-view-override.bat`, 59 assertions).

That pattern of decoupling decision logic from engine types so it can be tested offline appears three times in this library: `ViewOverrideLogic.h`, `TradeSettlementLogic.h` (43 assertions) and `ContainerPackets.h` (byte-compared against captures). Follow it when you add a domain with real decision logic.

---

## ABI: why "append-only" is load-bearing

`IHologramLib`: new capabilities are appended at the end of the class only, existing methods are never modified or reordered, and `HOLOGLIB_API_VERSION` is bumped.

It is an ABI requirement, not a courtesy. A virtual function's slot index in the vtable *is* its identity:

| Change | Consequence |
|---|---|
| Append at the end | Existing slots keep their indices, so pre-compiled consumers stay safe |
| Insert or delete in the middle | Every later slot shifts, so consumers built against an older header silently call the wrong function |

> This actually happened. The first version of `addActorInteractListener` / `removeActorInteractListener` was inserted mid-class in `IHologramLib` (after the ghost section), shifting every later vtable slot. A consumer compiled against the older header (MeowHolographicRenderer, which then fetched the wrong slot for `playerNpcs()`) crashed with `0xC0000005` immediately.
>
> They were moved to the end of the class, and the header now carries a ⚠ warning.

Since `26.40.9` the header also ships a vtable-layout descriptor plus a consumer-side compile-time check, and the library rejects layout-mismatched consumers at runtime. That turns "don't insert mid-class" from a rule people must remember into a mechanical guard.

### What happens when API has to shrink

Removing a released method moves slots too. The library has two documented approaches:

| Case | Approach | Example |
|---|---|---|
| The removed methods are at the end | Delete them outright, bump the minor version, document it in the version matrix | The six trade-menu listener methods in `26.40.3` |
| The removed methods are mid-class | Keep them as deprecated empty slots (non-pure + inline empty body + `[[deprecated]]`) so vtable slots are unchanged; the real cleanup waits for a major version | The eight per-line hologram methods in `26.40.8` |

The reasons and migration paths for both shrinks are in the [version & ABI matrix](../versions/api-version-matrix.md) *(zh)*.

---

## The LSE compatibility layer: mounted at runtime, never a hard dependency

HologramLib does not depend on LegacyRemoteCall (`lrca`) or MeowPAPI: it links against none of their symbols and declares nothing in its manifest.

Instead it resolves seven C++ functions exported by `lrca` at runtime via `GetModuleHandle` + `GetProcAddress` (MSVC-mangled names, character-identical to the export table):

| Situation | Behaviour |
|---|---|
| `lrca` not loaded / symbols missing | `attach()` returns `false` and every export / import call is a safe no-op |
| `lrca` present | `ll.import` / `ll.hasExported` behave exactly as before |
| Native C++ interface | Never goes through this bridge; unaffected by whether `lrca` exists |

Mounting is two-phase: it is attempted during `enable()`, and if that fails the library listens for `ServerStartedEvent` and retries (by which point every plugin is loaded). That is why load order does not matter.

> ⚠ Known issue: the two `ghost*` exports do call `exportAs`, but they are not wired into `ModEntry::exportLseFunctions()`, so they are never registered at runtime. See the [LSE API reference · known issues](../reference/lse-api.md#已知问题) *(zh)*.

---

## Related

- Wire formats, send primitive, capture verification: [Protocol notes](protocol-notes.md) *(zh)*
- Boundaries and rules: [README · Design notes](../../README.en.md#design-notes)
- Picking a capability domain: [Domain overview](../domains/README.en.md)
