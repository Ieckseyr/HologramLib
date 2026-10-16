# Design notes

This page covers HologramLib's interface evolution rules, its scope, and the restrictions on the view override domain. Together they decide what may live in the library and what has to stay in the consumer.

A short version is in [README · Design notes](../README.en.md#design-notes). The sections below give the mechanisms and the boundary cases that are already settled.

[中文](design-contracts.md) · [English](design-contracts.en.md) · [Docs home](index.en.md)

---

## Interface compatibility

`IHologramLib` and the domain interfaces gain virtual functions only at the end of the class, and existing signatures and semantics do not change. A new capability increments `HOLOGLIB_API_VERSION`, which is the only version source in code:

```cpp
// Compile time
static_assert(HOLOGLIB_API_VERSION >= 0x011B00, "requires 1.21.0 (ITradeMenu)");

// Runtime, in case an older build is already installed on the server
auto const ver = hologramlib::IHologramLib::getInstance().version();
if (ver < 0x011B00) { /* refuse to start */ }
```

`version()` returns `HOLOGLIB_API_VERSION`, so both checks read the same value. With both in place, a consumer does not compile against a new header and then meet an old library at runtime.

A virtual function's slot position in the vtable is its identity, so virtuals are only appended at the end. Appending leaves existing slots in place, and a consumer binary compiled against an older header stays valid. Inserting a virtual mid-class moves every later slot by one, and a consumer built against the old header calls the wrong function instead of failing to compile. Since `26.40.9` the header ships a vtable layout descriptor: consumers self-check at compile time, and the library rejects layout-mismatched consumers at runtime.

### Removing existing API

Removing a released method moves slots as well. How it is handled depends on where the method sits, and either case is recorded in the version matrix.

| Case | Approach | Example |
|---|---|---|
| Methods sit at the end of the class | Delete them, bump the minor version, record the removal | The six trade-menu listener methods removed in `26.40.3` |
| Methods sit mid-class | Keep them as deprecated empty slots: non-pure, inline empty body, `[[deprecated]]`. Vtable slots do not move, so pre-compiled consumers keep working | The eight per-line hologram methods removed in `26.40.8`; the real cleanup waits for a major version |

The reasons for both shrinks and the migration paths are in the [version & ABI matrix](versions/api-version-matrix.md) *(zh)*; a recompiled consumer gets a deprecation warning rather than a silent behaviour change.

---

## Scope

The library provides rendering primitives (`create` / `destroy` / `set*`), protocol wrappers (the packet details of `AnimateEntityPacket` / `MoveActorAbsolute` / `AddActor` and the like), timing notifications (`setEntitySpawnCallback` and similar callbacks), and data export/import primitives (`getSkinBlob` / `registerSkinFromBlob` / `importSkins`, each a single operation that leaves no state in the library). Everything else stays in the consumer:

1. Nothing is written to disk, and no consumer data is persisted. Skins, entity animations and display configuration are stored by the consumer. For skins the pattern is: the library exports a snapshot with `getSkinBlob`, the consumer writes it wherever it likes, and `registerSkinFromBlob` restores it when it is needed.
2. The library does not decide when a consumer resends what. It reports timing (`setEntitySpawnCallback` fires after an entity has spawned or respawned for a player) and provides single-shot primitives (`playAnimationTo` sends one animation packet to a given player). Which data to resend, whether to resend it, and where the data comes from are the consumer's decisions.
3. Commands, forms, placeholders (PAPI), permissions and command-on-click execution stay out.

To judge whether a new capability belongs in the library, ask whether it would still hold for a different consumer. If it does (protocol layer, rendering layer, pure timing), it goes in; if it depends on one consumer's storage format or business semantics, it stays in the consumer.

### Boundary split

| Capability | Library provides | Consumer owns |
|---|---|---|
| Skin snapshot persistence | `getSkinBlob` / `registerSkinFromBlob` | Writing to disk, restoring on startup, cleaning up |
| Entity animation resend | `setEntitySpawnCallback` + `playAnimationTo` | Storing the animation name and stop expression, resending from the spawn callback |
| Skin directory import | `importSkins` (one import, nothing persisted) | The import command, and where the imported sources are shown |
| Follow visibility | `follow` / `unfollow` / `setEnabled` | Deciding visibility when the target player joins or leaves |

Entity animation fields were once stored in `CustomEntityConfig` and replayed automatically at spawn time. That is consumer work, and it was rolled back.

### Internal timers

Timers inside the library run for two reasons only: delivery guarantees (the 15 s safety-net resend, resend on join, the view-override per-tick heartbeat, the hologram dynamic-line 0.5 s throttled refresh) and the engine's own path (the particle domain follows BDS's tick flush aggregation and goes out through the vanilla particle broadcast path, with view-distance culling and periodic resend).

Timers that animate on the consumer's behalf stay out. The hologram self-driven animation and `holoTick` were removed in `26.40.8` on that rule. `gradientTick` is not self-driven at all: it is an LSE export the script calls itself. To judge a proposed timer, ask whether it makes sure something arrives or decides how something moves. The full inventory is in the architecture overview's [self-driven work](architecture/overview.en.md#self-driven-work-what-moves-on-its-own) section.

---

## View override restrictions

Client view override (`IViewOverride`) changes what the client sees, so three further restrictions apply.

The outbound hook sits on `NetworkSystem::send` / `sendToMultiple` and decides only whether this packet goes out; it does not modify fields on the engine's packet object. When content has to change, the library drops the original packet and writes its own protocol packet (sculk construction, read-back check, raw bytes out). Changing an entity's type, swapping a nametag, hiding an entity from one player, replacing the model while keeping the runtime id, re-typing dropped items and overriding blocks all take that route. The in-place rewrite path was deleted in `26.40.8`.

Data comes only from protocol packets. Position, rotation and input bits come from inbound `PlayerAuthInput`; spawn packet type, position and name come from spawn payloads; actions come from `Animate` / `ActorEvent`. Server-side entity state is not read: no `getAABB`, `fetchEntity` or `Player::getInventory`. The one exception is identity resolution (mapping a network id to a player, and looking up a uniqueId inside `AddPlayer`).

Features that read engine state stay out of the library. The inventory mirror is the example: reading a player's inventory, opening a container screen and refreshing it every tick lived in the library briefly and was removed. A consumer that wants it composes it from the container domain with `IContainerMenu::open` plus per-slot `setItem`, and decides whose inventory to read and how often to refresh.

The `Level::$tick` heartbeat and the shape domain's resend mechanism are not covered by these restrictions; they are timing notifications. The heartbeat re-pushes overrides when an entity re-enters view or a player changes chunk, which covers a missed spawn packet or a block override overwritten by a chunk resend; while no override is active it costs one atomic read.

---

## Related

- Layering, source layout, resend strategy, self-driven inventory: [Architecture overview](architecture/overview.en.md)
- Summary of this page: [README · Design notes](../README.en.md#design-notes)
