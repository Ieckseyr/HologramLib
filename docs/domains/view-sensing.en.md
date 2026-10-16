# Domain guides · view override & sensing

Covers **client view override** (`IViewOverride`) and the **sensing domain** (`IPlayerSensing`), and explains **per-viewer overrides / per-player variables**, the two capabilities that are "not standalone domains".

| Domain | C++ | LSE | In one line |
|---|---|---|---|
| Client view override | `IViewOverride` | `view*` · 18 | Intercepts outbound packets per player and changes **the client view this one player sees** |
| Sensing | `IPlayerSensing` | `sensing*` · 3 | Captures the client's device type packet by packet |
| Per-viewer overrides | `ICustomEntity` per-viewer overrides | `entity*` tail · 4 | Overrides one entity's appearance per viewer |
| Per-player variable holograms | `IHologramText` + `{var}` | — | Resolves per viewer when the text contains `{var}` |

---

## Client view override

### What it actually changes

**It only changes the client view.** The server-side world / save / collision is **not touched at all**, and other players see the original.

For **the player being overridden**: the entity looks different to them. Everyone other than that player, and the server-side logic, still see **the real thing**.
The converse matters more — if the override is "turn player A into a mob":

> **`runtimeId` / `uniqueId` are unchanged → server-side it is still that real player, so hitting him is hitting him in person.**
> Damage, drops, and what other plugins see are all the real thing.

### Two disciplines (this is the domain's design red line)

**1. Suppress-only**
The outbound hook **only decides "does this packet go to this player" (pass / drop)**, and **never modifies fields on the engine's packet object**.
Wherever something has to "change", the rule is always: **drop the original packet + write the library's own protocol packet instead** (sculk construction → read-back check → raw bytes out).

> History: the approach used to be "change the engine packet fields and let the engine serialise them"; it was removed wholesale under this rule (`26.40.8` cleared out the in-place-rewrite dead code such as `dataListOf` / `setItemById` / `cloneItems` / `restoreItems`, and the whole decision path became `const`).

**2. Data comes only from protocol packets**
Position / rotation / input bits come from the inbound `PlayerAuthInput`; the spawn-packet type / position / name come from the spawn payload; actions come from `Animate` / `ActorEvent`.
**Server-side entity state is not read** (no touching `getAABB` / `fetchEntity` / `Player::getInventory` and the like).

> The one exception is **identity resolution** (network id → player, looking up a `uniqueId` inside `AddPlayer`) — that is "recognising who someone is", not reading state.
>
> **Features that read engine state do not enter the library**: the inventory mirror (read the player inventory + open a container interface + refresh every tick) was briefly in the library and has been **removed wholesale**. If a consumer wants it, compose it yourself from the **container domain**'s protocol capabilities (`IContainerMenu::open` + per-slot `setItem`); whose inventory to read and how often to refresh is the consumer's decision.

### Where the hook is attached (a pitfall we stepped on)

It is attached at BDS's **per-recipient packet-send convergence point**: `NetworkSystem::send` / `sendToMultiple`.

> Verified by decompilation: **entity spawn packets do not go through `LoopbackPacketSender`** (the hook used to be attached there → type swapping had no effect), so it was moved here.

**Heartbeat fallback**: `Level::$tick` self-checks once per tick — when an entity "re-enters view" or a player changes chunk it re-pushes the overrides. A spawn packet that slipped past, or a block override wiped out by a chunk resend, is recovered by it. When the feature is not in use it costs **just one atomic read**.

### What it can do

| What it can do | How |
|---|---|
| **Change an entity's type** | Drop the original spawn packet; the library sends one of the target type itself with **the same `runtimeId` / `uniqueId`**, copying the remaining fields from the original packet |
| **Swap the nametag** | Metadata `Name(4)` / `NametagAlwaysShow(81)`: the original packet goes out as usual, and the library's own `SetActorData` **follows right behind and overrides it** (it also overrides later renames by the server) |
| **Hide it from that player** | Spawn and update packets are not sent to him; if it is already on the client, a `RemoveActor` is added |
| **Swap the model, same id** (`skinId`, `26.40.9`) | Swallow that entity's spawn packet → send `PlayerList(Add)` + `AddPlayer` (a player model + a registered skin) |
| **Player into mob = replacement** | Swallow his spawn packet and send an entity of that type with the same `runtimeId` / `uniqueId`; `MovePlayer` is swallowed for that viewer, and the position is pushed by `MoveActorAbsolute` |
| **Change a player's skin** | Resend a single `PlayerList(Add)` to that viewer only (the entity's own UUID + the source player's skin); the same mechanism as the vanilla "skin update", and **it can be undone immediately** |
| **Change a dropped item's type / hide it** (`26.40.10`) | Dropped items travel via `AddItemActor`(id 15) rather than `AddActor` — both are intercepted |
| **Display a block as another block** | Drop the original `UpdateBlock` / `UpdateBlockSynced` and send the library's own `UpdateBlock` carrying the override block's network id; when the server is already sending the override block it goes out unchanged. After a chunk resend the library re-sends it automatically |

### What 26.40 cannot do (and therefore did not enter the API)

**Per-player scale / glowing / invisibility** — the 26.40 metadata table has no `scale`, and no `glowing` / `invisible` flags (Bedrock invisibility is an **effect** and travels via `MobEffectPacket`).

Per-client scale / equipment slots for custom entities still come from `ICustomEntity`'s per-viewer overrides (`1.21.0`); do not expect them from the view override domain.

### Per-player isolation / idempotence / zero cost / fail-safe

| Property | How |
|---|---|
| **Per-player isolation** | The override table is per player name plus one global table; multi-recipient paths (`sendToClients` / broadcast) are expanded into **per-recipient application**. Because fields are never changed, there is no cross-contamination where "the next recipient sees the previous one's override" |
| **Idempotence** | Decisions are made **by id, not by position**, so the same packet decided once by each of several paths gives the same result; an update packet that is already the override block is passed straight through |
| **Zero cost** | When the library holds no override at all, the hook **passes through on a single branch** (it does not look up the player or read the packet); the match check likewise only looks at whether the target player has an override |
| **fail-safe** | Unrecognised packets and entities it cannot identify are **always passed through untouched** |

### Same-id model swap (`EntityView::skinId`)

Since `26.40.9`, `skinId` lets a **non-player entity** be swapped to "our model": the client sees a player model with a registered skin (custom geometry allowed), while **server-side it is still that mob**.
Supporting pieces:

- That entity's `MoveActor*`(18/111) is **swallowed** for these viewers, and the position / rotation is pushed every tick by the heartbeat with `MoveActorAbsolute` (reusing PlayerNpc's light-push channel);
- The `PlayerList` entry is **removed after about 20 ticks** (it does not go into Tab, the entity stays);
- **Undo = `clearEntity` resends the real spawn packet**;
- `scale` scales the collision box proportionally (metadata 53/54), and `yOffset` lines the model origin up with the foot position;
- The skin comes from the `IPlayerNpc` registry (`registerSkin` / `captureSkin` / `registerSkinFromBlob`).

> **This is the right way to "give a mob a custom model".** The PlayerNpc carrier approach needs "an invisible real body + per-tick following + ghost-forwarded attacks"; this route needs none of that.

### The spawn-packet recipe (the one verified on a live server)

A self-built spawn packet **must** carry:

1. the flags metadata entry;
2. the `minecraft:health` / `minecraft:scale` **attributes**;
3. and **an `UpdateAttributesPacket` immediately after it** — attributes inside a spawn packet are **only initialised, not applied** by the client.

> When these items were missing, the first heartbeat resend after a player joined made the client throw an error (the symptom was "an error the first time you go in").
> It also fixed the **inverted protocol value** of `NametagAlwaysShow` (`0` = always shown), and gives a player who has just joined about a **2-second grace period** (no entity packets are pushed while the client is still receiving chunks).
>
> `26.40.10` fixed one more instance of the same root problem: `SetActorDataPacket` / `UpdateAttributesPacket` **both carry `mTick`** in the 26.40 wire format, and the view override path never filled it in (`= 0`) → the client **discarded the "metadata update / attribute update" as stale packets** (captures prove it: `id2`/`id43`/`id3`/`id82` are all present in the spawn packet with correct values, yet the model still looked default on screen).
> Both packets now fill in `currentViewTick()` (`tickPulse` keeps it in step every beat); the spawn packet is **followed immediately by a `SetActorData` on the same plan**, and then a confirmation **+2 ticks** later is scheduled (`scheduleMetaPush` / `flushMetaPushes`).
> For comparison: the customentity domain always sent the tick (`sadp.mTick = tick`), which is why `setVariant` / `setColorIndex` have always worked there.

### Your own client does not take part in overrides of your own entity

> **Turning a player into a mob froze that player's own client** (the override applied to "self" as well: the player themself received a `RemoveActor` + an `AddActor` with the same runtimeId).
> The rule now is: **your own client does not take part in overrides of your own entity** — all three paths, override / restore / outbound, skip the player themself (their own `SetActorData` and `MovePlayer` are no longer swallowed either).
>
> **Skin-change overrides are the exception: they include the player.** Sending `PlayerList(Add)` to the player themself is exactly the right way to let **a player see their own skin swapped**, and their own view changes with it.

### Entity-level changes all take effect immediately

Changing the type / hiding / undoing all do one thing to that entity: **remove it + resend one in the target state** (the same `runtimeId` and `uniqueId`, so later server-side movement / state updates still apply to it as usual).

The cost: **the entity on the client is rebuilt** — details that only ever arrived with the spawn packet (variant, initial equipment state) have to wait for the server's next sync.
On undo, **entities that are too far away (> 64 blocks) or already gone are not rebuilt**; **player entities cannot be rebuilt** (`AddPlayer` has no type field).

### The decision logic is isolated and testable

The decision logic is isolated in `src/view/ViewOverrideLogic.h` (coordinate packing / chunk ownership / idempotence checks / nametag planning / type-name sanitisation / `planItemSpawn`), and it **touches no BDS types**, so it can be run offline:

```powershell
./tests/check-view-override.bat    # 59 assertions
```

### On the LSE side: the 18 `view*` functions

| Group | Functions |
|---|---|
| Main entry points | `viewEntity` · `viewBlock` · `viewClearEntity` · `viewClearBlock` · `viewClearAll` |
| Per-field | `viewTargetType` · `viewClearId` · `viewMetaProbe` · `viewDescribe` · `viewEcho` |
| The "self" family | `viewSelf` · `viewSelfClear` · `viewSelfType` · `viewSelfSkin` · `viewSelfName` · `viewSelfHide` |
| Queries | `viewUniqueIdOf` |
| Capability negotiation | `viewItemSpawnHooked` |

The `viewEntity` spec string supports these keys:

```
type=   name=   skin=   pose=   scale=   variant=   mark=   color=   color2=
```

`pose` / `scale` / `variant` / `mark` / `color` / `color2` were added in `26.40.10` (they correspond to `EntityView`'s `pose` / `metaScale` / `variant` / `markVariant` / `colorIndex` / `color2`, each with a `hasXxx`, where `false` = leave that item alone).

> ⚠ **One spot in `viewEntity`'s "unique id decision" was fixed in `26.40.8`**: **player ids are themselves negative** — the old decision treated a player id as an invalid value. If you worked around the old behaviour, you can drop the workaround.
>
> ⚠ **The size of `EntityView` changed in `26.40.10`** (the six fields above were appended at the tail) → **C++ consumers that construct this struct directly must recompile**. The C++ interfaces themselves only append at the tail, so vtable slots do not move.

---

## Player sensing

```cpp
if (lib.playerSensing().isTouch("Steve")) {
    lib.containerMenus().open("Steve", menu);   // touch: use a virtual container, clicks always send packets
}
```

It hooks `PlayerAuthInputPacket` and reads `InputMode` **packet by packet** → `KeyboardMouse` / `Touch` / `Gamepad` / `MotionController`.

| Behaviour | Explanation |
|---|---|
| `Unknown` does not overwrite a known value | The early AuthInput packets may report `Undefined`, and it must not be allowed to smear a known device into unknown |
| Cleared as soon as the player goes offline | — |
| A device change is reflected **in the next packet** | There is no caching delay |

**There is no event, and no polling is needed** — the LSE side queries directly: `sensingDeviceOf` / `sensingIsTouch` / `sensingDeviceCode`.

**The typical use is to route the interface by device**: touch goes to a virtual container (clicks always send packets), keyboard-mouse goes to the trade menu (full interaction).

---

## Per-viewer overrides (per-client rendering)

Not a standalone domain, but per-viewer overrides on `ICustomEntity`:

```cpp
lib.customEntities().setPlayerNametag(entityId, "Steve", "§c队长");   // "captain"
lib.customEntities().setPlayerScale(entityId, "Steve", 1.5f);
lib.customEntities().setPlayerEquipmentSlot(entityId, "Steve", 0, item);
lib.customEntities().clearPlayerAppearance(entityId, "Steve");
```

Both the spawn packet and the delta packets **go out with the per-viewer override values**, and equipment changes are **sent immediately as a single packet, with no flicker**.

The LSE side has the same four exports under the same names: `entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` / `entityClearPlayerAppearance`.

> **Packets go out per player, so it suits small-audience cases** (a squad leader's view, quest tracking) and is **not suited to server-wide broadcast-style high-frequency refresh** — the packet volume grows linearly with the number of viewers.

### Per-player variable holograms

As soon as `IHologramText`'s text contains `{var}`, the library switches to **maintaining one shape per viewer**, each receiving the result resolved for them (when a viewer goes offline / leaves the dimension, that shape is destroyed in place).

```cpp
lib.holograms().addLine(holo, "§b{player}§r 你好");   // "hello"
```

The cost and the suitable scenarios are the same as for per-viewer overrides: **N times the shapes and N times the resend complexity**. See [Rendering & text · per-player variables](rendering.md#per-player-variables-per-viewer-overrides).

---

## Related

- Full signatures: [C++ API reference](../reference/cpp-api.md) (§1.13 sensing / per-viewer overrides / per-player variables, §1.16 view override)
- LSE function list: [LSE API reference](../reference/lse-api.md) (§2.13 / §2.16)
- The short version of the boundary disciplines (suppress-only / data only from protocol packets): [README · Design notes](https://github.com/Ieckseyr/HologramLib/blob/main/README.en.md#design-notes)
- The send primitive and the read-back check: [Protocol notes](../architecture/protocol-notes.md)
