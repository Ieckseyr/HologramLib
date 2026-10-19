# Capability domains

The capability domains are split into ten pages.

| Page | Content |
|---|---|
| [DebugShape (holograms & shapes)](debugshape.md) | Shapes · holograms · gradient lines |
| [Item display](item-display.md) | FMBE · item detail · stretch / live transforms / presets |
| [Particles](particles.md) | Particle shapes |
| [Entities & interaction](entities.md) | Custom entities · ghost interaction · actor interaction events |
| [Fake-player NPC](player-npc.md) | NPCs · skins / image plane · pose / equipment / ride · interaction |
| [Containers & inventory](containers.md) | Virtual containers · fake inventory · session isolation |
| [Interfaces](interfaces.md) | Trade menu · NPC dialogue |
| [Sulfur cube](sulfur.md) | Sulfur-cube display |
| [View override](view.md) | Client view override · per-viewer rendering |
| [Client sensing](sensing.md) | Input-device sensing |

Every page in this set now has an English version, including the four group guides. The selection tables below are the fastest way to pick a domain, and the API references cover the signatures.

---

## Choosing: I want to show something at a position

Six different ways exist to put something into the world, with different looks, costs and capabilities. Start here:

| I want to show | Domain | Look | Cost |
|---|---|---|---|
| Wireframes, geometry, a piece of text | Shapes `IShapeDrawer` (the text branch) | Glowing debug lines / text | Client-side in-memory shapes, no entity, zero server cost |
| A multi-line information board | Holographic text `IHologramText` | A text block that always faces the camera (can be turned off) | Same as above; really just "one multi-line text shape" |
| A coloured trail | Gradient lines `gradient*` (LSE only) | Multi-colour gradient segments | A batch of line shapes |
| An item icon | FMBE item display `IItemDisplay` | A fox holding an item (or a beacon form) | Fake entity (partly invisible, visible only to the target) + packets |
| A block, being swallowed | Sulfur-cube display `ISulfurDisplay` | The block floats there; the cube is invisible by default | Fully delegates to `ICustomEntity`; appearance tiers via `ChangeMobProperty` |
| A vanilla mob / entity | Custom entities `ICustomEntity` | A real entity look: poses, equipment, animations, riding | Protocol-level entity (no AI); no server-side counterpart |
| A humanoid (player model) | Fake-player NPCs `IPlayerNpc` | A player model with registerable / capturable / injectable skins | Pure-protocol fake player; skins travel via PlayerList |
| Making an existing mob look different | Client view override `IViewOverride` (`EntityView::skinId`) | Swapped to your model, id unchanged | Client view only; the server still sees the original mob |

### Change the client's view, or create something new

This is the easiest choice to get wrong:

- Create something new (custom entities / fake-player NPCs / sulfur cube / FMBE item display). The server has no real entity behind it. Players can see it but cannot interact with entity logic.
- Change an existing thing's client view (`IViewOverride`). No new entity is created; only the packets this one player receives are changed. The entity is still that same real server-side entity, so clicking it, hitting it and aiming at it all hit the real thing. Damage, drops and what other plugins see are all the original. The trade-off is that details which only ever arrived with the spawn packet (variant, initial equipment) wait for the server's next sync.

> The right way to "give a mob a custom model" is `EntityView::skinId` (since `26.40.9`), not the older "invisible real entity + per-tick following + ghost-forwarded attacks" approach, which forces you to handle visibility decisions, following and attack forwarding yourself.

---

## Choosing: I want an interface the player can interact with

| Need | Domain | Why |
|---|---|---|
| A list UI where clicking an entry fires a callback, on any input device | Virtual container `IContainerMenu` | A click is an item pickup, so touch players send packets too; 27 or 54 slots |
| Show a villager trade table | Trade menu `ITradeMenu` | Protocol-level `UpdateTrade`, byte-identical to captured BDS packets |
| Trades that actually charge and deliver | `ITradeMenu` + `settleLocally`, or `usePacketOffers=false` | The former settles in the library, the latter hands settlement to BDS (recommended) |
| Multi-level dialogue / NPC story with buttons | NPC dialogue `INpcDialogue` | `NpcDialoguePacket` + synthetic carrier; levels routed by scene name |
| Make the client believe it is carrying certain items | Fake inventory `IFakeInventory` | Rewrites the client's view of the inventory; the real inventory is untouched |
| Detect the player's input device and route accordingly | Player sensing `IPlayerSensing` | Per-packet `InputMode`: touch / gamepad / keyboard-mouse |

The shared boundary: these "virtual interfaces" do not exist on the server. When a player interacts, the server-side slot does not match, so BDS rejects it and the client rolls back its prediction. The library deliberately does not answer with a failure response on the server's behalf. Doing so makes the client pop up an error, whereas letting BDS walk its own failure path is silent.

---

## Choosing: particles and transient effects

| Need | Domain |
|---|---|
| Points, lines, rectangular rings, filled planes, box frames, six-faced shapes, polyhedra | Particle shapes `IParticleShape` |
| A whole plane sliding smoothly with an anchor, rotating, spinning, following | Same: `moveTo` / rotation / `setFollowPlayer` |
| Visible to only some players | Same: `setVisiblePlayers` whitelist (empty = everyone in the dimension) |
| Per-client sounds, short-lived floating text | Not available. Removed wholesale in `26.40.3` |

Particles are sent through the vanilla `SpawnParticleEffectPacket` batch channel, and BDS's tick flush aggregates them into a single Batch datagram.

---

## Cross-domain dependencies

Almost all domains are independent. Only these relationships matter:

| Domain | Relationship |
|---|---|
| Sulfur cube `sulfur` | Fully delegates to `ICustomEntity`: scale / view distance / whitelist / per-viewer rotation are inherited. Its only new protocol content is `ChangeMobProperty` |
| Holographic text `holo` | Reuses the shape domain and never writes protocol packets directly (a hologram is a multi-line text shape) |
| Gradient lines `gradient` (LSE) | Also builds its own batch of line segments through the shape domain |
| Per-viewer appearance / text variables | Per-viewer overrides on `ICustomEntity` / `IHologramText`, not standalone domains |
| Container / fake inventory | Share one item-request hook (`src/interaction/PlayerInteractionHooks.cpp`) and are told apart by container id |
| View override | Reads the `IPlayerNpc` skin registry when it needs a skin (`registerSkin` / `captureSkin` / `registerSkinFromBlob`) |

---

## Minimum version per domain

Use these values when writing `static_assert` gates:

| Version | Added |
|---|---|
| `0x010900` · 1.9.0 | Early domains: `IShapeDrawer` / `IHologramText` / `IItemDetail` |
| `0x011500` · 1.15.0 | `IParticleShape` |
| `0x011900` · 1.19.0 | Skin export from memory / directory import, `geometryData` custom models, NPC scaling |
| `0x011A00` · 1.20.0 | 26.40 adaptation; per-viewer rotation |
| `0x011B00` · 1.21.0 | `ITradeMenu` / `INpcDialogue` / `IContainerMenu` / `IPlayerSensing` + per-viewer rendering |
| `0x011C00` · 1.22.0 | Trade menu became display-only; four domains gained LSE exports |
| `0x011D00` · 1.23.0 | `IFakeInventory` / `ISulfurDisplay` |
| `0x011E00` · 1.24.0 | Trade menu: protocol-only settlement |
| `0x011F00` · 1.25.0 | `IViewOverride` |
| `0x012000` · 1.26.0 | View-override cleanup; five new `IPlayerNpc` methods; holographic-text shrink and completion |
| `0x012100` · 1.27.0 | `EntityView::skinId`; real-entity interaction listeners; ABI layout stamp |
| `0x012300` · 1.29.0 | `AddItemActor` interception; `EntityView` gains pose / scale / variant / colour |
| `26.51.5` | 1.31.0 | `0x012500` | **26.51 line merges 26.40.12**: stretch, live transforms, animation presets, stop recipe, NPC pose/equipment/ride, FMBE spawn-twitch fix, image plane with transparency, container session isolation, PlayerSkin trusted flag and skin Id rebuild. |
| `26.51.4` | 1.30.0 | `0x012400` | **Three primitives/notifications only, no feature logic**: `viewPollSpawnEvents` (spawn notification) · `itemDisplayCreateFull` (create with NBT/glint) · `itemDisplayEmitForEntity` (render an existing entity as an FMBE model for one player). All decisions live in the consumer (`ItemPhys`). LSE exports only, vtable untouched → `HOLOGLIB_ABI_STAMP` unchanged. |

> ⚠ `0x012200` (= API 1.28.0) is not in the table. That number was skipped and never shipped as a release. See the [version & ABI matrix](https://github.com/Ieckseyr/HologramLib/blob/main/docs/versions/api-version-matrix.en.md).
