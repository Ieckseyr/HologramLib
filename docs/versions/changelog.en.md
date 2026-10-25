# Changelog

Newest first. Each entry lists the API version, added or changed exports, behaviour changes, and whether C++ consumers
must recompile. The full release ↔ API mapping is in the [version & ABI matrix](api-version-matrix.en.md).

## Overview

| Release | API | Line | Contents | New exports | C++ consumers |
|---|---|---|---|---|---|
| `26.51.6` | 1.32.0 | 26.51 | Internal refactor (packet / tick / lifecycle), ABI contract tooling, payment planning fix, four new domains (dropped items / form close / locator bar / packet control) | 16 | **Recompile needed** (tail appends: 4 new interfaces + methods on `IContainerMenu` / `IPlayerNpc`) |
| `26.51.5` / `26.40.12` | 1.31.0 | 26.51 · 26.40 | Stretch / live transforms / animation presets / stop recipe / NPC pose-equipment-ride / FMBE spawn-twitch fix / image plane with transparency / container session isolation | several | Recompile recommended (tail appends) |
| `26.40.11` / `26.51.4` | 1.30.0 | 26.40 · 26.51 | Three protocol primitives and one timing notification | 3 | No recompile (vtable unchanged) |
| `26.40.10` / `26.51.3` | 1.29.0 | 26.40 · 26.51 | `AddItemActor` interception, pose/scale/variant/colour, metadata tick fix | 1 (`viewItemSpawnHooked`) | Recompile (`EntityView` grew) |
| `26.40.9` | 1.27.0 | 26.40 | `EntityView::skinId` (same-id model swap), real-actor interaction events, ABI stamp | class methods | Recompile (vtable appended) |
| `26.40.8` | 1.26.0 | 26.40 | Per-viewer overrides, four UI domains | several | Recompile |

## 26.51.6 — API 1.31.0 (internal refactor and fixes, no API change)

The API version and the vtable slots are unchanged: C++ consumers do **not** need to recompile.

### Refactor

- **Packet path**: three equivalent implementations merged into `src/core/PacketSink.h` (`SculkPacketSend.h` becomes a two-line forwarder). The anonymous copy in `ItemDisplayManager.cpp` used to skip counting and warning; it now feeds the same drop counters.
- **Tick driver**: five `Level::$tick` detours collapsed into one (`src/core/TickDriver.h`) with an explicit order table, so execution order no longer depends on cross-TU static initialisation order.
- **Build**: `xmake.lua` uses a `src/**.cpp` glob; `ghost/GhostInteractRouter.cpp` is a standalone translation unit again (it used to be `#include`d into another `.cpp`).
- **Lifecycle**: domain registration moved into `src/core/ServiceHost.h` — start in declaration order, stop in reverse, LSE export in one pass. This also fixes an existing defect: `ContainerMenu` / `NpcDialogue` / `PlayerSensing` / `FakeInventory` had written `shutdown()` implementations that `ModEntry::disable()` never called, so open virtual containers, NPC dialogues and fake inventories survived plugin unload until the client reconnected.
- **Contract tooling**: new `tools/abi_tool.py`, `abi/v1.31.0.json` and `abi/slots.md` baseline; `abi.yml` blocks mid-class slot insertion in CI. The `mirror` subcommand generates a consumer mirror from the real header and proves slot-by-slot equality (312 slots / 15 interfaces).

### Fixes

- **Trade payment planning**: `planConsumption` walked the ledger separately for each cost, so two costs of the same item type (`buyA` / `buyB`) could both be satisfied by one balance. Same-type costs are now merged before consuming, and demand accumulates in 64 bits. Offline fixtures cover both same-type and mixed-type paths.
- **Hot paths**: the ABI check caches the module handle; ghost event formatting and request-action conversion are reused; unused internal state and an empty renderer tick are gone.

### Engineering

- `tests/` is under version control now (the byte-level C++ fixtures and Python assertions), with `tests/README.md` explaining how to run them and what is skipped when a local capture is missing.
- New build workflow: static checks (ABI contract, tool self-test, format check on changed files only) always run; the real compile needs the `HOLO_PROTOCOL_URL` repository variable and skips explicitly when absent instead of pretending to pass.
- Added `.editorconfig`, `.clang-format`, `.clang-tidy`; xmake gained `--holo_werror` (`/WX`) and `--holo_asan` (`/fsanitize=address`).
- Build closure: `HOLO_PROTOCOL_DIR` / `HOLO_REPO_DIR` environment variables; a missing protocol library fails fast with instructions.
- Exception semantics: `/EHa` removed, standard C++ exceptions (`set_exceptions("cxx")`). The old combination enabled `/EHa` while disabling exceptions, which made throws inside `<chrono>` degrade to no stack unwinding (C4530) — undefined behaviour.
- The holographic-text manager now uses a recursive mutex: the outbound packet hook calls back into the manager **on the same thread**, which self-deadlocks with a non-recursive lock.

## 26.51.5 / 26.40.12 — API 1.31.0

The 26.51 line merges the 26.40.12 capabilities (protocol 2193); behaviour matches 26.40.12.

- **Non-uniform stretch**: `setStretch` / `setStretchLive` (XZ and Y channels); blocks use `setExtend`.
- **Live transforms**: `setRotationLive` / `setOffsetLive` / `setScaleLive` / `setTransformLive`; animation packets only, no respawn.
- **Animation presets**: orbit / tumble / wobble / heartbeat / pendulum / billboard pitch / LOD / first-person hide; single-channel `setAnimPreset` and LSE `itemDisplaySetPreset`.
- **Stop recipe**: `animation.player.attack.positions` + `stopExpression=1`; `stopAnimation` / `stopAnimationTo`.
- **NPC pose/equipment/ride**: `setPose` / `setEquipment` / `setRideUniqueId` / `setRidePlayer` / `setRideEntity` / `clearRide`.
- **FMBE spawn-twitch fix**: equipment delayed until sleeping / swelling / posrot (item +5, block +7 ticks); live resend keeps pending equipment.
- **Image plane**: `playerNpcRegisterImageSkin(pngPath, skinId, sizeBlocks, transparent)`; `transparent=false` composites over white, `true` thresholds alpha.
- **Container session isolation**: `uiOpen` only refreshes open UIs; `ownerEpoch` invalidates old-session packets; `containerId` is not reused for 200 ticks.
- **Skin semantics**: `PlayerSkin` uses `TrustedSkinFlag=true`; skin `Id` / `FullId` are rebuilt from the registry id.

### 26.51 protocol line

- Protocol 2193; shape text payload includes `LineGapHeight`.
- Container success response uses `presence=false`.
- `PlayerAuthInput` input bits read through `EnumSet::contains`.

## 26.40.11 / 26.51.4 — API 1.30.0

The 26.40 and 26.51 lines ship identical content.

### New exports

| Export | Description | Returns |
|---|---|---|
| `viewPollSpawnEvents()` | Returns and clears the spawn-event queue. Each entry is `drop\|uid\|dim\|x\|y\|z\|` or `actor\|uid\|dim\|x\|y\|z\|<identifier>`; one entry per entity | `Array<String>` |
| `itemDisplayCreateFull(...)` | Creates an item display with item NBT and enchant glint supplied at creation, avoiding the rebuild caused by calling `itemDisplaySetItemWithNbt` / `itemDisplaySetGlint` afterwards. Same parameters as `itemDisplayCreateAdvanced` plus trailing `nbt` and `glint` | display id |
| `itemDisplayEmitForEntity(playerName, uid, item, aux, offsetY, rotX, rotY, rotZ, scale, nbt, glint)` | Renders the existing entity with uniqueId `uid` as an FMBE item model for one player: sends `RemoveActor`, `AddActor` (fox), `MobEquipment`, `SetActorData` and the animation sequence. Uses the caller-supplied id and keeps no state. When `rotY` is `"0"` the orientation is derived from the entity's current velocity | `1` item plane · `2` 3D block · `<0` failure |

### Fixed

- **Enchanted items rendered without the enchantment glint.** Previously no export accepted `glint` at creation or emit
  time, so consumers had to call `itemDisplaySetGlint` separately — if that call was missing or arrived late there was no
  glint, and combining `itemDisplaySetItemWithNbt` with `itemDisplaySetGlint` triggered an extra rebuild because the
  enchantment lives in `nbt`.
  In this release `itemDisplayCreateFull` and `itemDisplayEmitForEntity` both take `nbt` and `glint` on the first send, so
  an enchanted item shows the glint from its first frame without follow-up calls.

### Scope

This release adds the three exports above only. The library does not implement the following; the caller decides them:
whether an entity is taken over, which players receive it, how the angle is restored after landing, and which size is used
for blocks versus items.

### Compatibility

Only LSE exports are added (registered by name). The vtable in `include/hologramlib/HologramLib.h` is unchanged,
`HOLOGLIB_ABI_STAMP` stays at 1, and compiled C++ consumers need no recompile.

### Reference implementation

`ItemPhys` 1.8.0 (an LSE plugin) implements dropped-item model takeover by combining `viewEntity`'s `hidden=1` with
`itemDisplayEmitForEntity`.

## 26.40.10 / 26.51.3 — API 1.29.0

26.51.3 moves the 26.40.8, 26.40.9 and 26.40.10 batches onto the 26.51 protocol line with the same capabilities as
26.40.10; the only 26.51-specific adaptation is reading `PlayerAuthInput` input bits.

### Added and changed

- **`AddItemActor` (id 15) is now intercepted.** Dropped items spawn through it rather than `AddActor`, so retyping or hiding a drop previously only affected players already watching when the override was set; the engine's own `AddItemActor` then rewrote the item icon (capture of 2026-10-16: an `AddItemActor` with the same uniqueId arrived 13 ms after `RemoveActor` and `AddActor(cod)`). Now `hidden` drops the packet, and `identifier` drops it and writes an `AddActor` with the same `runtimeId` / `uniqueId`, using the verified health 20 / scale 1 attribute recipe.
- **`EntityView` gained post-transformation appearance fields (appended at the tail).** `pose`, `metaScale`, `variant`, `markVariant`, `colorIndex`, `color2`, each with a `hasXxx` flag where `false` leaves the item alone. The LSE `viewEntity` spec accepts `pose=`, `scale=`, `variant=`, `mark=`, `color=`, `color2=`. Protocol IDs: `PoseIndex(79)`, `Reserved38`, `Reserved53`/`Reserved54`, `Variant(2)` as Int, `MarkVariant(43)` as Int, `ColorIndex(3)` as Byte, `Color2Index(82)` as Byte; types taken from a decoded real spawn packet.
- **New export `viewItemSpawnHooked()`** for capability negotiation from scripts.

### Fixed

- `SetActorDataPacket` and `UpdateAttributesPacket` carry `mTick` on the 26.40 line and the view-override path sent 0, so the client discarded them as stale and metadata or attribute updates had no effect (captures show correct `id2`, `id43`, `id3`, `id82` in the spawn packet while the screen showed the default appearance). Both are now stamped with `currentViewTick()`, with an immediate `SetActorData` after the spawn packet and a confirmation scheduled +2 ticks later.
- The two `ghost*` exports (`ghostPollInteractions`, `ghostClearInteractions`) were not wired into `ModEntry::exportLseFunctions()`, so `ll.import` returned `null`; they are wired back.

### Recompile required

`EntityView` changed size (six fields appended), so C++ consumers that construct the struct must recompile; vtable slots are unchanged.

### Notes

- `26.51.2` was withdrawn; use `26.51.3` or `26.51.4`.
- The metadata and `AddItemActor` findings in 26.40.9 and 26.40.10 come from 26.40 packet captures; the 26.51 line only verified that the protocol constants match byte for byte, and was not retested on a 26.51 client.

## History

Earlier entries are kept verbatim as submitted.

- `26.40.9` (API 1.27.0): **`EntityView::skinId` (+ `scale` / `yOffset`) — non-player entities "swapped to our model"** (the intercepted spawn
  packet + the library's own packets as the replacement, **the same runtimeId/uniqueId**): the client sees a player model with a registered
  skin (custom geometry allowed), while on the server side it is still that mob — **clicking it / hitting it / aiming at it all hit the mob
  itself**. Supporting work: eat that entity's `MoveActor*`(18/111) and push position/rotation every tick with `MoveActorAbsolute`, remove the
  `PlayerList` entry after 20 ticks, and resend the real spawn packet on revocation; `scale` scales the hitbox proportionally (metadata 53/54),
  `yOffset` aligns the model origin to the feet. Consumer example: MSkinventory's "same-id reskin".
  - **(appended inside the batch, unreleased) removal: the `skin=` branch of the NPC dialogue `avatarViewSpec`** (a shrink, on the record).
    It used `PlayerList(Add)`+`AddPlayer` to swap the carrier for a "player model" — in testing, the client closed the interface when clicking
    into the menu on that carrier (in the same test the `type=` carrier: zombie / chicken, everything was normal), so that path is currently
    unusable. Following the "the library only does mechanisms" positioning, the "make the client believe it is a player" route is left for
    consumers to build themselves (`playerNpcs()` making a real fake player + `npcUniqueIdOverride` hanging the dialogue on its id). **Kept**:
    `type=` / `name=` (custom entity type / title) and `avatarSkinVariant` (the NPC's built-in skin variant). The `avatarViewSpec` /
    `avatarSkinVariant` fields themselves do not change (**ABI unchanged**, consumers need no recompile); a piece of dead code that never took
    effect (positive-segment id allocation for player-model carriers) went with it.
  - **Real entity interaction events**: `addActorInteractListener` / `removeActorInteractListener` (appended at the end of the class) —
    callbacks when a player right-clicks / attacks a **server-side real entity**; they reuse the library's existing inbound-packet hook (no new
    detour layer), the multicasts do not overwrite each other, and the returned token is used to remove.
  - **ABI layout stamp**: the header carries a vtable layout descriptor + a consumer-side compile-time self-check (the library intercepts
    "layout-mismatched consumers" at runtime) — turning the two "mid-class slot move" startup crashes of 2026-10-09 / 10-10 into a mechanism;
    see "ABI incident (this time, on the record)" in the [version & ABI matrix](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.en.md).
  - LSE: the `viewEntity` spec string gained **`skin=<skin id>`** (equivalent to `EntityView::skinId`; the skin comes from the `IPlayerNpc`
    registry).
- `26.40.8` (API 1.26.0): **view override debris cleanup + five methods appended to `IPlayerNpc` + holographic-text/text domain shrink and
  completion** (the shrink is on the record, see the [version & ABI matrix](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.en.md)).
  - **View override (`IViewOverride`) code tidy-up**: behaviour unchanged — the outbound hook only decides "pass / drop", and **original packet
    fields are never modified** (that has been so since 26.40.7). This pass cleared out **old-world debris** that did not match the behaviour:
    rewrote the stale "rewrite the original packet wherever possible" header comment, deleted the in-place-rewrite dead code (`dataListOf` /
    `setItemById` / `cloneItems` / `restoreItems`), and made the whole decision path const (packets are read only to identify identity).
    Wherever a "change" is needed it is always **drop the original packet + the library hand-writes its own protocol packet** (for a type
    change the spawn packet copies the original packet's remaining fields; the nametag is overridden by the library's `SetActorData`
    immediately after; a block override is resent automatically once the chunk is received; a replaced player's movement packets are dropped
    and the position is pushed by the heartbeat with `MoveActorAbsolute`). The `view*` LSE exports were completed **field by field**
    (6→16 functions; a unique-id decision fix — a player's id is itself negative).
  - **Five methods appended at the tail of `IPlayerNpc`** (+ `setEntitySpawnCallback`; the existing ABI is unchanged):
  - **`setPositionLight(id, x, y, z, dim)`**: sends only `MoveActorAbsolute` (no entity rebuild, no skin resend, no flicker), shares the same
    light-dirty channel as `setRotationLight`, and several sends within one tick are merged into a single packet. Its purpose is "follow
    something that moves, every tick" (the typical case: hanging a custom model on a moving mob). It returns `false` when `dim` differs from
    the current dimension; across dimensions still use `setPosition` to rebuild.
  - **`injectSkin(viewerName, targetName, skinId)` / `injectSkinAll(targetName, skinId)`**: sends a skin from the registry as one
    `PlayerList(Add)` using **the target's own UUID / uniqueId**, updating the skin entry in place. An empty `viewerName` = all online players
    (**including the target themselves** — this is the path for "let a player see their own reskin"; Geyser goes through PlayerList rather than
    `PlayerSkinPacket` for the session player themselves as well). The entry carries the real xuid and `TrustedSkinFlag = true`; the skin
    object itself carries `OverridesPlayerAppearance = true` (it overrides the skin already equipped on the player's client; without it the
    client rejects the entry). **No automatic resend**: when the client drops a frame now and then, the consumer calls it again (Geyser
    re-sends after ~100 ms).
  - **`playAnimation(id, animation, stopExpression, durationTicks)` / `playAnimationTo(id, playerName, ...)`**: sends `AnimateEntityPacket` to
    the players that have already seen this NPC (`animation` = the animation identifier in the resource pack; the `controller` name is made
    unique per NPC id inside the library, so no controller of the same name has to exist in the resource pack beforehand; an empty
    `stopExpression` = permanent, `"query.any_animation"` = stop at once; `durationTicks > 0` sends the stop packet automatically when it
    expires). The same release appends `setEntitySpawnCallback` (a callback when an NPC has finished spawning for a given player, for resending
    animations/state). The same queue as the customentity domain (sent at +2 ticks).
  - `setPositionLight` / `playAnimation(To)` / `setEntitySpawnCallback` are **C++ interfaces (no LSE exports yet)**; **the skin family now has
    LSE exports**: `playerNpcInjectSkin` / `playerNpcInjectSkinAll` (inject for the given / all viewers, **including the reskinned player
    themselves — see yourself reskinned**) and `playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64` (the base64 persistence pair for
    skin blobs: the consumer writes them to disk, and after a restart the skins can be restored even when the source player is offline) —
    `playerNpc*` 25→29, 265 in total (the script-side proper route for Disguise / MSkinventory style "player sees their own reskin"). Consumers
    gate these methods with `>= 0x012000`.
  - **Shrink (8 holo exports + 2 groups of internal capability removed)**: `holoSetLineGradient` (gradient) / `holoSetLineRainbow` (rainbow) /
    `holoSetLineColor` (per-line colour) / `holoSetLineScroll` (scroll) / `holoSetVerticalAnimation` (vertical animation) /
    `holoSetLineSpacing` (line spacing) / `holoSetLineScale` (per-line scale) / `holoTick` — under the "one whole block of text = a single text
    shape" model these are **unimplementable or spin uselessly** (per-line styling only takes effect on line 0, rainbow lands as white, the
    line-spacing field does not exist in this protocol, and nothing consumes the self-driven tick's offset). For per-line colour use inline §
    colour codes in the text instead; for per-line scale use `holoSetScale` (whole block).
  - **Completion (6 new holo exports + 3 shape exports)**: `holoSetScale` (whole-block scale) / `holoSetBackgroundColor` /
    `holoClearBackgroundColor` (background-box colour; unset = the client default colour) / `holoSetDepthTest` (see-through visibility:
    `false` = always rendered (the default, visible through walls), `true` = occluded by blocks/entities) / `holoSetRotation` / `holoClearRotation`
    (three-axis Euler [Pitch, Yaw, Roll], **unit = degrees** (the official script API's convention); after it is set the text no longer faces
    the camera); the shape domain gained `shapeSetBackgroundColor` / `shapeClearBackgroundColor` / `shapeSetDepthTest` in step.
  - **Dynamic lines return (refactored from Phantom — LGPL-3.0, the origin is recorded as the licence requires, see "third-party attribution"
    at the end of the text)**: `holoSetLinePool(id, line, [content pool], intervalMs)` (the content pool is **rotated by time modulo** the
    interval, stateless, and several holograms sharing a pool stay in phase) + `holoSetLineParseVariables(id, line, on)` (per-line variable
    toggle, on by default). Rotation/variable refresh is handled by a **0.5 s throttle** inside the library, and **only resends when the content
    changed** (unlike the self-driven "offset" removed in this release: that moved on every tick, but nothing consumed what it drove); the
    variable set gained `{dimension}` / `{x}` / `{y}` / `{z}`. Also fixed that **a per-viewer (with `{var}`) text refresh only changed memory
    and sent no packet** (previously an update only reached the client with the 15 s safety-net resend) — now `refresh`/driving resends in place
    per player (same networkId, no flicker). `holo*` 24→26, total exports 265→267.
  - **NPC dialogue avatar customisation**: two new parameters at the tail of `NpcDialogSpec` — `avatarSkinVariant` (the NPC's **built-in skin
    variant** 0..59: written into the carrier's ActorData as `SkinId(104)`, swapping the avatar/model in the chat box to a built-in skin;
    `-1` = do not add the item, byte-identical to the old behaviour) + `avatarViewSpec` (**layers an IViewOverride on this carrier**: the spec
    syntax is the same as `viewEntity` — `type=` changes the carrier type (test matrix: zombie / chicken); the `skin=` branch has been removed
    (see above, a shrink inside an unreleased batch). The original description is kept on file: `skin=` used **a skin from the playernpc
    registry** (everything MHR/MeowSkin registers/collects is in that table) → the carrier became a **player model** (PlayerList+AddPlayer with
    the same id, the Tab entry removed after 1 s), `name=` changed the interface title; whether `type` swapping to a non-NPC family still pops
    the interface normally needs an on-device test).
    Supporting LSE: `npcDialogSetAvatar(id, skinVariant, viewSpec)` (in-place update + reopen, the same 0-flicker path as paging);
    `npcDialog*` 8→9, total exports 267→268. For "a fully custom whole model" there is also the existing `npcUniqueIdOverride` route: use the
    view domain / custom entity to create a display entity and hang the dialogue on its id (the same-id model swap route of 1.27.0's
    `EntityView::skinId` can be used for that display entity too).
  - **Fixes**: ① the text shape's `setRotation` previously **had no effect** (the `useRotation` flag was missing, so text always faced the
    camera) — now `useRotation` and double-sided rendering are both sent, and `clearRotation` restores camera-facing; ② `shapeDestroyAll` /
    `shapeDestroyBatch` only cleared server-side memory and **sent no removal packet** (the client kept showing them), now completed;
    ③ following a player no longer depends on the library's tick — the position is resolved in place when `setFollowPlayer` / `draw` /
    `refresh` is called; ④ **the colour R/B channels were swapped** (int32 colours followed the old v944 byte order, whereas v2168 is actually
    **ARGB** — filling in red showed blue): the packing `toPacked` and the unpacking `getColor` are both corrected (shapes / text colour /
    background box are all affected; after the fix, colours in configuration show up literally, and if you previously reversed values to
    compensate for the colour, change them back); ⑤ **"disguise as some player" was treated as a revocation** (the "all-empty means revoke"
    null check in `overrideEntity` missed the `asPlayer` field) — it reported "无法伪装" ("cannot disguise") and cleared the old override along
    the way; `asPlayer` is now part of the null check; ⑥ **turning a player into a mob froze that player's own client** (the override applied
    to "self" as well: `RemoveActor` + an `AddActor` with the same runtimeId were sent to the player themselves) — a new rule "**your own
    client does not take part in overriding your own entity**" was added, and all three paths — override / restore / outbound — skip the player
    themselves (their own `SetActorData` and `MovePlayer` are no longer eaten either); **the reskin family is the exception: it includes the
    player themselves** (`PlayerList(Add)` sent to the player themselves is the proper route for "see yourself reskinned", and the self-view
    changes with it); ⑦ **a replaced player's movement was never pushed** (`sendMoveActorAbsolute` was dead code, so the stand-in stood still) —
    now every time their input packet arrives, `MoveActorAbsolute` is pushed to the viewers that can see them, using the input position
    (eye position → feet position) (skipping the player themselves).
  - Consumer hints: after upgrading, gate the new methods behind `>= 0x012000`; code written against the old per-line holo API **gets a
    deprecation warning when recompiled** (`[[deprecated]]`), and on the LSE side `ll.import` gets null and fails explicitly — **it will not
    silently change behaviour**. **ABI-safe**: the 8 shrunk methods are kept in the C++ interface as **deprecated empty slots** (non-pure +
    empty body), so the vtable slots are the same as in 1.25.0 — old consumer binaries that were not recompiled do not misalign (the difference
    from the 26.40.3 trade-menu shrink: that one deleted methods at the tail, this one deletes in the middle, and deleting outright would shift
    the whole vtable forward). In-library consumers were updated in step (MeowHolographicRenderer, ZXPanel, Meow).
- `26.40.7` (API 1.25.0): **new client view override `IViewOverride`** (protocol-layer interception; suppress-only).
  - **What is intercepted is "the packet the server was going to send to this player"**: the library hooks BDS's per-recipient send convergence
    points (`NetworkSystem::send` / `sendToMultiple`) — at those points the library **only decides whether to intercept** (pass / drop) and
    **never modifies engine packet fields**; wherever a "change" is needed it always drops the original packet and the library **hand-writes its
    own protocol packet** to resend (sculk construction → read-back check → raw bytes sent). Decompilation showed that entity spawn packets do
    not go through `LoopbackPacketSender` (hooking there, as before → type changes had no effect), so the hook was moved here.
  - **Heartbeat fallback**: the server self-checks once per tick — re-pushing the override when an entity "re-enters view" or a player changes
    chunk. A missed spawn-packet interception or a block override washed away by a chunk resend is recovered by it; when the feature is not used
    at all it is just one atomic read.
  - **What it can do**: change an entity's type (drop the original spawn packet and the library sends one of the target type itself with the same
    `runtimeId`/`uniqueId`, copying the original packet's remaining fields), swap the nametag (metadata `Name(4)` / `NametagAlwaysShow(81)`: the
    original packet goes out as-is and the library's own `SetActorData` immediately after overrides it, and later renames by the server are
    overridden too), **hide it from that player** (spawn and update packets are not sent to them, and one `RemoveActor` is sent for one already
    on the client), display a block as a different block (drop the original `UpdateBlock` / `UpdateBlockSynced` and the library sends an
    `UpdateBlock` carrying the override block's network id; when the server was already sending the override block, it is passed through
    unchanged).
  - **Per-player isolation**: the override table is keyed by player name plus one global table; multi-recipient paths (`sendToClients` /
    broadcast) are expanded and applied per recipient — only so that "pass / drop" takes effect per player; because fields are never modified,
    there is no bleed where "the next recipient sees the previous recipient's override". **Idempotent**: the decision recognises ids, not
    positions, so the same packet decided once per path gives the same result; an update packet that is already the override block is passed
    straight through.
  - **Zero overhead**: when the library holds no override at all, the hook takes one branch and passes straight through (no player lookup, no
    packet read); the hit test also only looks at whether the target player has an override, and a miss passes the packet through unchanged.
    **fail-safe**: unrecognised packets and unidentifiable entities are always passed through unchanged.
  - **What 26.40 cannot do (so it did not enter the API)**: per-player **scale / glowing / invisibility** — the metadata table has no `scale`,
    and no `glowing` / `invisible` flag either (in Bedrock, invisibility is an effect and goes through `MobEffectPacket`). Per-client
    scale/equipment slots for custom entities still belong to `ICustomEntity`'s per-viewer overrides (1.21.0).
  - **Entity-class changes all take effect immediately** (type change / hide / revoke): the library does one "remove + resend one in the target
    state" for that entity (the same `runtimeId` and `uniqueId`; the server's later movement/state updates apply to it as usual). The cost is
    that the client-side entity is rebuilt — details that only ever arrived with the spawn packet (variant, initial equipment) wait for the
    server's next sync. On revocation, entities that are too far away (>64 blocks) or already gone are not rebuilt; player entities cannot be
    rebuilt (`AddPlayer` has no type field).
  - Block overrides are resent automatically by the library after a chunk resend (when the chunk is received).
  - The decision logic is isolated in `src/view/ViewOverrideLogic.h` (coordinate packing / chunk ownership / idempotence test / nametag plan /
    type-name sanitisation), testable offline: `tests\check-view-override.bat` (**54 items**).
  - **Player entities can be overridden too** (within the same release): `EntityView::asPlayer = "<online player name>"` → resend one
    `PlayerList(Add)` to that viewer only (the entity's own UUID + the source player's skin), the same mechanism as a vanilla "skin update",
    revocable immediately; `identifier = "<mob>"` → a player model cannot render a mob → the library **replaces**: it eats their spawn packet
    and sends an entity of that type with **the same runtimeId/uniqueId**, pushing position/rotation from the input packets they send with
    `MoveActorAbsolute`. **The id did not change → on the server side it is still that real player, and hitting it hits the player themselves.**
    On revocation the substitute entity is moved away.
  - **The spawn packet recipe is aligned with the customentity domain** (the set verified online): it must carry the flags metadata item +
    the `minecraft:health` / `minecraft:scale` attributes, and **an `UpdateAttributesPacket` immediately after it** (the client only
    initialises attributes in the spawn packet and does not apply them) — with those items missing, a heartbeat resend when a player joined the
    server made the client error out ("error on the first join/leave"). Also fixed the **reversed protocol value** of `NametagAlwaysShow`
    (0 = always shown), and gave players who just joined about a 2-second grace period (no entity packets are pushed while the client is still
    receiving chunks).
  - **Input actions → proxy (executed at the protocol layer)**: inbound `PlayerAuthInput` is taken as the truth — position/rotation/head
    rotation, the 66 input bits (jump/sneak/sprint/swim/crawl/fly…), `ClientPlayMode` (interface state) and the item action fields; the proxy
    reproduces A's actions from it (falling back to the server state only when there is no new input within 1 second). **Action packets**
    (swing/attack/hurt/death) are copied from the server's `Animate` / `ActorEvent` to the proxy (the runtimeId is swapped for the proxy's, with
    100 ms deduplication to avoid replaying twice). The held item/hotbar is mirrored continuously.
  - **Proxy position correction**: the y a player reports at the protocol layer is at the **eyes** (a Bedrock peculiarity) — the proxy takes
    the true foot position from the bottom of the hitbox instead, converting the input packet's y with the "eye to feet" difference
    (standing/sneaking/swimming are all correct).
  - **Two disciplines**: the outbound hook **only decides whether to intercept** (pass / drop) and **never modifies engine packet fields** —
    wherever a "change" is needed it always drops the original packet + the library **hand-writes its own protocol packet** to resend;
    **the only data source is protocol packets** (inbound `PlayerAuthInput` / spawn-packet payloads / `Animate`·`ActorEvent`), server-side entity
    state is not read.

- `26.40.5` (API 1.23.0): **new fake inventory `IFakeInventory` and sulfur-cube display `ISulfurDisplay`**.
  - **Protocol-layer rewrite of the player inventory the client sees**: one `InventoryContentPacket` (`ContainerId = Inventory(0)` +
    `FullContainerName = InventoryContainer(29)` + descriptors 0..35) replaces the whole content with the one the caller gives; a single-slot
    change goes through `InventorySlotPacket`; the server-side inventory is not touched at all. The approach is taken from the reference
    implementation GMLIB's `ChestUI` (it writes `InventorySlot` per slot, with the same container id `Inventory(0)` +
    `InventoryContainer(29)`).
  - **The feature set matches the virtual container**: clicking a forged item in your own inventory → the callback reports the slot number; the
    item is not really taken away (the client sends a request according to the forged content, the server's real slot does not match → BDS
    judges it a failure → the client rolls back its prediction). The library then resends that one slot so the forged content is not washed away
    by that rollback.
  - **Coexisting with the trade menu / virtual container**: opening an interface makes BDS resend the inventory content and overwrite the forged
    content, so this domain resends on a `refreshIntervalTicks` period (20 ticks = 1 s by default), or `refresh()` manually.
  - **LSE**: `fakeInvApply` / `fakeInvSetSlot` / `fakeInvRefresh` / `fakeInvClear` / `fakeInvClearAll` / `fakeInvIsActive` /
    `fakeInvGetPlayers` / `fakeInvPollClicks` / `fakeInvClearClicks`.
  - **Offline check** `tests/check-fake-inventory.bat` (a local tool): checks the wire format against **a real BDS packet capture** (the frame
    lands at the end of the packet, container id 0 / 36 slots / container-name byte 0 / an empty storage descriptor, plus the item UserData
    dialect fingerprint: `06 00 "Damage"` rather than `06 "Damage"`), 10 items all pass. The item itself is serialised by BDS, so there is no
    longer a "hand-built byte by byte" check; it pins the layout facts instead.
  - The item request hooks (147 + the one embedded in AuthInput) are now shared by two domains and moved from `container/` to
    `src/interaction/PlayerInteractionHooks.cpp`: the container domain claims `LevelEntityContainer(7)` and dyn 101..199, the inventory domain
    claims `CombinedHotbarAndInventory(12)` / `Hotbar(28)` / `Inventory(29)`, and they do not fight.
  - **Fixes (field incidents)**: the first version hand-wrote the item descriptor with the wrong User Data NBT dialect → the client froze; and
    the session was not cleaned up on disconnect, so on reconnect the tick driver sent it again → "cannot get into the server". Now the item is
    serialised by BDS, and a `PlayerDisconnectEvent` is registered to clean up the session (`FakeInventoryManager::onPlayerLeave`); the periodic
    resend interval also has a clamped lower bound (5 ticks at the fastest).
  - **New sulfur-cube display `ISulfurDisplay` (a second placement method)**: spawns `minecraft:sulfur_cube` and puts the block **in its main
    hand** (in the behaviour pack the cube holds the block it "swallows" via `slot.weapon.mainhand`, and the client renders it as equipment) —
    `SulfurDisplaySpec::block` and `setBlock` are this domain's core API; **invisibility is one parameter on the spec** (`invisible`, off by
    default). The appearance tier is delivered by a **hand-written `ChangeMobProperty`(182)** as `minecraft:sulfur_cube_archetype`. The whole
    domain delegates to `ICustomEntity`, so scale / view distance / whitelist / per-client rotation are inherited directly;
    `ICustomEntity::setMobProperty` is also opened up for other entities' `client_sync` properties. **"Swallowing mobs" is not done**:
    protocol-layer entities have no AI, and the approximation of "letting the entity ride the cube" was tried, but the on-device look did not
    hold up, so it was removed wholesale.
- `26.40.4` (API 1.22.0): **the trade menu became display-only + new domains completed with LSE exports**.
  - **The trade menu no longer has any click listener**: `TradeClickEvent` / `TradeActionCallback` / `TradeRawAction` and `ITradeMenu`'s six
    listener methods were removed wholesale, and `TradeMenuSpec` lost `displayOnly` / `acceptPaymentPlacement` — from now on this domain only
    handles "open the interface + lay out the trade table". The old 147/AuthInput hooks and the payment-catching logic went with them (**what
    stays in `src/container/ContainerInteractionHooks.cpp`**: those two hooks now only serve virtual-container clicks). For a list interface
    that "can be clicked and has a callback", use the virtual container (`IContainerMenu`).
  - `ITradeMenu` gained `addOffer` / `setTier`: appending a trade or changing a tier/experience bar **resends the trade table in place**, with
    no need to close and reopen.
  - **Four new domains completed with LSE exports**: `trade*` (7), `container*` (9), `npcDialog*` (7), `sensing*` (3); container and NPC
    dialogue clicks go through a **polling queue** on the script side (`containerPollClicks` / `npcDialogPollClicks`, which take the entries
    away and clear them; entries are splittable strings), the same approach `ghost*` has used for a long time.
  - `entity*` gained per-client rendering exports: `entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` /
    `entityClearPlayerAppearance`.
  - The API version therefore went up to **1.22.0**: retracting something already released is a shrink (normally a major-version change); here
    the minor version was bumped and the reason recorded explicitly in the [version & ABI matrix](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.en.md). Code written against
    1.21.0 that listened for trade-menu clicks will get a **compile error** when upgraded (not a silent behaviour change).
- `26.40.3` (API 1.21.0): **four new capability domains (villager trade menu / NPC dialogue / virtual container / sensing) + per-client
  rendering + one trim** (the list is in the 1.21.0 rows of the table above).
  - **Villager trade menu `ITradeMenu`**: builds `UpdateTrade` at the protocol layer to open the interface (packet-level fields and the Offers
    NBT are byte-compared against BDS captures); an invisible carrier entity drives the experience bar; click reports locate the entry precisely
    by the recipe netId (`offerIndex` / `recipeNetId`), plus per-action callbacks with the same semantics as the reference implementation
    GMLIB ChestUI (`src` / `dst` / `amount`, closed with a `slot=-1, amount=-1` sentinel). **(This entry's click reporting was retracted on
    request in 26.40.4 — see the previous entry.)** The two paths are chosen by `usePacketOffers`: **protocol-layer only by default** (the server
    sends no trade table → read-only by nature), setting it to `false` uses the real trade table + BDS `openTrading` (a completed trade really
    swaps items). The carrier type `carrierIdentifier` can be a villager or a wandering trader.
  - **NPC dialogue `INpcDialogue`**: `NpcDialoguePacket` + a synthetic `minecraft:npc` carrier (placed below the world → the client cannot see
    the entity, but the dialogue avatar renders as usual); button / close callbacks, and multi-level dialogue is routed by scene name.
  - **Virtual container `IContainerMenu`**: replicates the reference implementation GMLIB's `ChestUI` item by item — a client-side chest block
    (`UpdateBlock`) + block entity NBT (`BlockActorData`, items via `Items`, title via `CustomName`) + `ContainerOpen` (type `Container(0)`,
    bound to the block coordinates, target entity `-1`); `rows=3` is a small container with 27 slots, `rows=6` a large one with 54 slots
    (`pairx` / `pairz` / `pairlead` pairing); a click report carries the slot number = the entry index, a close report carries `closed=true`.
    **Any input device can click** (a click is an item pickup).
  - **Sensing `IPlayerSensing`**: hooks `PlayerAuthInputPacket` to capture `InputMode` per packet → `KeyboardMouse` / `Touch` / `Gamepad` /
    `MotionController`; cleared as soon as the player disconnects, updated with the next packet when the device changes.
  - **Per-client rendering (per-viewer overrides)**: `ICustomEntity` gained `setPlayerNametag` / `setPlayerScale` /
    `setPlayerEquipmentSlot` / `clearPlayerAppearance` (both spawn packets and incremental packets go out with the per-viewer override values;
    equipment changes are sent immediately on their own with no flicker); `IHologramText` text containing `{var}` automatically switches to a
    **per-viewer shape**, so `{player}` is from then on correct per player even when drawn for everyone.
  - **Trim**: per-client sounds and short-lived floating text were removed (no longer needed after discussion, the code was deleted wholesale;
    both were early experiments done as "protocol-layer hand-written packets").
  - **Fixes**: ① text animation (scroll / bounce) is now self-driven inside the library (listening on `ServerLevelTickEvent`) — previously
    `tick` was only an LSE export and the native side had no driving source, so the animation neither moved nor disappeared (**that self-driving
    and the related API were removed wholesale in the 26.40.8 shrink**); ② the 147 hook previously only engaged when a **trade** menu was open,
    so virtual-container clicks all slipped through; ③ clicks reported the inventory-side slot (now the trade side is reported first); ④ a
    diagnostic switch (`src/DiagLog.h`) left behind in the release artefacts (`check-no-diagnostics.bat` catches it).
  - **Field findings (details in the sections below)**: container opens work after 7 ticks / do not work at 6 ticks — the lower bound; item
    requests have two channels, `ItemStackRequestPacket(147)` and one embedded in `PlayerAuthInputPacket(144)`; an action that hits the virtual
    container is reported back but not intercepted (answering the failure response on its behalf would make the client pop up an error).
- `26.40.2` (API 1.20.0): fix fake-player NPC skins not rendering (the client falls back to the default model) — the PlayerList skin entry's
  `Id` / `FullId` were empty or left over the source player's cache key; PNG registration, live capture and blob restore now all go through
  `finalizeSkinIds`, which completes `Id` and rebuilds `FullId`, and it is set only after Persona / capture rewriting is done, no longer reusing
  the source player's identity. Before sending PlayerList the 2168 packet-body prefix is validated (Add `01 01 00`, Remove `01 00 01`); when the
  prefix is abnormal the packet is dropped and the actual prefix is logged; the first submission to NetworkPeer logs `PlayerList submitted`.
  New offline regression check `tests/check-npc-playerlist.ps1` (bytes produced by the actually linked static library + an independent Python
  decoder checking the 2168 fields, the trusted-flag position, and `Id` / `FullId` being fully consumed by the packet body). The accompanying
  Protocol static library gained the PlayerList 2168 frame format port (variant array, `ActionType` Add=0/Remove=1, the trusted flag changed to
  a three-state string inside the skin); this plugin's ABI / API version is unchanged
- `26.40.1` (API 1.20.0): adapt to LeviLamina 26.40 / BDS 1.26.40 (protocol 2168, shape rendering switched to the Protocol v2168 static library);
  fix event ids not matching official LeviLamina, which meant no listener received any event (`src/EventIdCompat.h`); new per-client rotation
  (`setPlayerRotation` / `clearPlayerRotation` / `clearPlayerRotations`, common to entities and NPCs) and lightweight rotation update
  (`setRotationLight`); new ghost interaction multicast listeners (1.19.1); NPC creation/dirty refresh is merged and sent together at the end of
  the tick (several sends within one tick make the client receive a burst of "new players" and disconnect); PlayerList removals are no longer
  sent (removing such an entry while the skin entry is still active crashes the client; the entity disappears as usual); PlayerList / AddPlayer
  sends skip the BDS read-back check
- `26.10.7` (API 1.19.0): new skin export from memory (`getSkinBlob`/`registerSkinFromBlob`, the consumer persists it itself); batch skin import
  from a directory (one sub-folder = PNG + optional `.json` model); `PlayerNpcSkin.geometryData` custom geometry models; the library dropped disk
  storage and became a pure API; new NPC scaling (`PlayerNpcConfig.scale` + `playerNpcSetScale`, 0.0625~10, proportional hitbox); fix NPC
  view-distance culling / dirty refresh / Tab removal not working (tick hook not registered); fix NPC respawn (dirty refreshes such as
  scale/reskin) losing its skin and turning into the default Steve (a stale Tab removal entry deleted the new skin entry)
- `26.10.6` (API 1.17.1): fix permanent storage of captured skins
