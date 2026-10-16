# Capability domain guide · Entities & NPCs

Covers 4 domains: **Custom entities**, **Ghost interaction**, **Fake-player NPCs**, **Sulfur-cube display**.

What this group has in common is **protocol-layer entities**: they exist only on the client, and the server has no corresponding real entity (no AI, not written to the save, not part of server-side logic). To "make an existing real mob look different", take [view override](view-sensing.md) instead of this group.

| Domain | C++ | LSE | In one line |
|---|---|---|---|
| Custom entity | `ICustomEntity` | `entity*` · 37 | Protocol-level spawn of a vanilla entity: pose / equipment / animation / riding |
| Ghost interaction | listener + polling | `ghost*` · 2 | Interaction-event routing for non-real entities |
| Fake-player NPC | `IPlayerNpc` | `playerNpc*` · 29 | A pure-protocol fake player whose skin can be registered / captured / injected |
| Sulfur-cube display | `ISulfurDisplay` | `sulfur*` · 10 | "Swallows" a block on the cube; by default you only see the block |

---

## Custom entities

Protocol-level spawn of a vanilla entity, supporting poses, equipment slots, animations and ActorLink riding.

### Per-viewer rotation (`1.20.0`)

`setPlayerRotation` / `clearPlayerRotation` / `clearPlayerRotations` (shared by entities and NPCs) let **different players see different rotations** — the rotation version of per-viewer overrides.

`setRotationLight` is a **light-dirty channel**: it sends only the rotation and does not rebuild the entity, and several calls within the same tick merge into one packet.

### Per-viewer overrides: per-viewer appearance (`1.21.0`)

| Method | Effect |
|---|---|
| `setPlayerNametag` | the nametag this viewer sees |
| `setPlayerScale` | the scale this viewer sees |
| `setPlayerEquipmentSlot` | the equipment this viewer sees |
| `clearPlayerAppearance` | clears every override for this viewer |

Both the spawn packet and incremental packets are sent with the per-viewer override values, and **an equipment change goes out immediately on its own, without flicker**.
On the LSE side these are `entitySetPlayerNametag` / `entitySetPlayerScale` / `entitySetPlayerEquipmentSlot` / `entityClearPlayerAppearance` (the last 4 of `entity*`).

### Attribute sync

`setMobProperty` (public since `26.40.5`) is used to send `client_sync` class attributes — the sulfur cube's appearance tiers take this route.

> **Why attributes must be sent "after spawn"**: `AddActor`'s `PropertySyncData` only has room for index-addressed int / float, **it has no room for an enum**. So an enum attribute (such as `sulfur_cube_archetype`) can only be sent once the client already knows this entity. The library re-sends it automatically after spawn and replays it after respawn — that is "delivery guarantee" work, not doing business on the consumer's behalf.

---

## Ghost interaction

Ghost = interaction routing for **non-real entities**. When a player's `InteractPacket` lands on a target that is not a real entity (a custom entity or a fake-player NPC, say), the library routes it into an event for the consumer.

```cpp
auto token = lib.addGhostInteractListener([](hologramlib::GhostInteractEvent const& e) {
    // e.action is the raw Action value of InteractPacket
});
lib.removeGhostInteractListener(token);
```

- The C++ side supports **multicast listeners** (`addGhostInteractListener` / `removeGhostInteractListener` return a token, since `1.19.1`); the older single listener `setGhostInteractListener` is still there.
- `GhostInteractEvent::action` is the **raw Action value of `InteractPacket`**:

  | Value | Meaning |
  |---|---|
  | 1 | Interact (right-click) |
  | 2 | Attack (left-click attack) |
  | 3 | StopRiding |
  | 4 | InteractUpdate |
  | 5 | NpcOpen |
  | 6 | OpenInventory |

  Check `action == 1 || action == 2` before handling, and do not assume those are the only two values.

> **Getting events from a script**: `ghostPollInteractions()` takes and clears the pending queue (it returns an array of strings, format documented in [Entities, particles & interaction](../lse/entities.md)); `ghostClearInteractions()` just drops the queue. On the C++ side there are also the `addGhostInteractListener` multicast listeners.

---

## Fake-player NPCs

A pure-protocol fake player: the client sees a player model, and the server **has no such player**. A skin can be registered from a PNG, captured online, imported from a directory, persisted as a blob, and injected to a chosen viewer.

### The four ways a skin gets in

| Source | Method | Purpose |
|---|---|---|
| PNG registration | `registerSkin` | register directly from a PNG + an optional `.json` model |
| Online capture | `captureSkin` | capture an online player's skin |
| Directory import | `importSkins` | one subfolder = a PNG + an optional `.json`; **a one-shot import, the library writes nothing to disk** |
| Blob persistence | `getSkinBlob` / `registerSkinFromBlob` | export/restore a snapshot — **saving it to disk is the consumer's job** |

`PlayerNpcSkin.geometryData` supports custom geometry models (since `26.10.7`). Scaling goes through `PlayerNpcConfig.scale` / `playerNpcSetScale` (**0.0625 ~ 10**, the hitbox scales with it).

> **The library writes nothing to disk.** Disk storage was removed in `26.10.7` in favour of a pure API — a skin snapshot is exported with `getSkinBlob` → **the consumer saves it** → `registerSkinFromBlob` restores it.
> The LSE side has a base64 pair: `playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64` (so it can be restored after a restart even when the source player is offline).

### Skin injection: letting a player see their own skin swapped (`26.40.8`)

```cpp
lib.playerNpcs().injectSkin("Steve", "Alex", skinId);   // only Steve sees it
lib.playerNpcs().injectSkinAll("Alex", skinId);         // every online player sees it
```

One `PlayerList(Add)` keyed on the **target's own UUID / uniqueId**, updating the skin entry in place. A few key details:

- passing an **empty string as `viewerName` = all online players, including the target themself** — this is the correct route for "**letting a player see their own skin swapped**" (Geyser also goes through PlayerList for the session player themself, not `PlayerSkinPacket`);
- the entry carries the real xuid and `TrustedSkinFlag = true`; the skin object itself carries `OverridesPlayerAppearance = true` (**without it the client refuses the override**);
- **no automatic resend** — when the client drops a frame now and then, the consumer calls it once more itself (Geyser's approach is to resend after ~100ms).

### Lightweight position updates (`26.40.8`)

```cpp
if (!lib.playerNpcs().setPositionLight(id, x, y, z, dim)) {
    lib.playerNpcs().setPosition(id, x, y, z, dim);   // crossing dimensions can only go through a rebuild
}
```

It sends only `MoveActorAbsolute` and **does not rebuild the entity, does not resend the skin, and does not flicker**; it shares the same light-dirty channel as `setRotationLight`, and calls in the same tick merge into one packet.
The typical use is "follow something that moves, every tick" (hanging a custom model on a mob that is on the move). **It returns `false` when `dim` differs from the current dimension** — crossing dimensions must go through a `setPosition` rebuild.

### Playing an animation by name (`26.40.8`)

```cpp
lib.playerNpcs().playAnimation(id, "animation.npc.wave", "query.any_animation", 40);
lib.playerNpcs().playAnimationTo(id, "Steve", "animation.npc.wave", "", 0);
```

Sends `AnimateEntityPacket` to players that have already seen this NPC:

- `animation` = the animation identifier in the resource pack;
- the `controller` name is **made unique per NPC id** inside the library — **the resource pack does not need a controller of that name to exist beforehand**;
- an empty `stopExpression` = it stays; `"query.any_animation"` = stop immediately;
- with `durationTicks > 0`, **a stop packet is sent automatically** on expiry.

Paired with `setEntitySpawnCallback`: it fires when an NPC has finished spawning for a given player, so the consumer can resend animations / state (**the library notifies, the consumer decides**). The same queue as the customentity domain (sent at +2 ticks).

`setPositionLight` / `playAnimation(To)` / `setEntitySpawnCallback` are currently **C++-only, with no LSE exports yet**; the skin family does have LSE (`playerNpcInjectSkin` / `playerNpcInjectSkinAll`).

### Three field-tested pitfalls (all found the hard way)

1. **Sending several times within the same tick disconnects the client** — the client receives a burst of "new player" notifications. So NPC creation and dirty refreshes are **merged and sent once at the end of the tick**.
2. **Do not send a PlayerList remove entry** — removing that entry while the skin entry is still active **crashes** the client. The entity disappears as usual, so the library does not send one.
3. **The root cause of a skin not rendering was the identity fields** (fixed in `26.40.2`): if a `PlayerList` skin entry's `Id` / `FullId` are empty, or a stale cache key from the original player is left in them, the client falls back to the default model. Now PNG registration / online capture / blob restore all go through `finalizeSkinIds`, which completes `Id` and rebuilds `FullId`, and only sets them after the Persona rewrite has finished.

---

## Sulfur-cube display

The second way to "put a block somewhere" (alongside FMBE's "fox + item"): spawn a `minecraft:sulfur_cube` and put the block **into its main hand**.

```cpp
hologramlib::SulfurDisplaySpec spec;
spec.block = "minecraft:bookshelf";   // what it has "swallowed"
spec.invisible = true;                // true by default
auto id = lib.sulfurDisplays().create(spec);
```

**This is not a trick we invented**: in the behaviour pack, `entities/sulfur_cube.json` uses `slot.weapon.mainhand` + `behavior.equip_item` to hold the "swallowed" block, and the client renders it as equipment. So what it looks like is a block swallowed inside the cube. A **block-type item** gives the most convincing look.

### Invisibility is a parameter, and it is on by default

**Measured (26.40, local client): while the cube is invisible, the "swallowed" block in its main hand renders as usual.**
So with `invisible = true` (the default) what you see is **a block floating there**, with the cube body itself invisible. Set it to `false` to see the cube as well.

> By contrast: **an NPC carrier's portrait does get erased along with invisibility** — the two entities behave differently, so do not apply one to the other.

### Appearance tiers

Sent through a **hand-written `ChangeMobProperty`(182)** as `minecraft:sulfur_cube_archetype` (the `client_sync: true` enum from the behaviour pack, 13 tiers):
`none` / `regular` / `bouncy` / `slow_bouncy` / `slow_flat` / `fast_flat` / `light` / `fast_sliding` / `slow_sliding` / `sticky` / `high_resistance` / `explosive` / `hot`.

Because it is an **enum**, it does not fit into the spawn packet's `PropertySyncData`, so it must be sent after the entity is already known to the client — the library re-sends it automatically after spawn and replays it after respawn.

### This domain delegates entirely to `ICustomEntity`

The cube is just a custom entity, so **scale / view distance / visible-player whitelist / per-viewer rotation are all inherited directly**. The only protocol content this domain adds is `ChangeMobProperty`.

> **"Swallowing a mob" is not done, and is not planned**: protocol-layer entities have no AI, so real swallowing / digesting cannot be done. We tried approximating it by "letting another entity ride the cube", but neither the riding position nor the collision could be tuned to look like "being swallowed" (it does not hold up in practice), and it was removed entirely. This domain does **blocks + invisibility** only.

---

## Related

- Full signatures: [C++ API reference](../reference/cpp-api.md) (§1.6 custom entities / §1.7 Ghost / §1.9 fake-player NPCs / §1.15 sulfur cube)
- LSE function list: [Entities, particles & interaction](../lse/entities.md) · [Fake-player NPCs](../lse/player-npc.md)
- To "give a real mob a different model" instead of creating a new entity: [View override & sensing](view-sensing.md)
