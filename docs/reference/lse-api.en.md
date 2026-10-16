# LSE API reference

Exported automatically whenever LegacyRemoteCall (`lrca`) is present. **A single namespace, `HologramLib`**; the prefix selects the capability domain:

| Prefix domain | Capability | C++ interface | Functions |
|--------|------|---------------|--------|
| `shape*` | Shape rendering | IShapeDrawer | 39 |
| `holo*` | Holographic text | IHologramText | 26 |
| `gradient*` | Gradient lines | GradientLineManager | 11 |
| `itemDetail*` | Item detail | IItemDetail | 2 |
| `itemDisplay*` | FMBE item display | IItemDisplay | 33 |
| `entity*` | Custom entities | ICustomEntity | 37 |
| `ghost*` | Interaction event polling | IHologramLib | 2 |
| `particle*` | Generic particle shape system | ParticleShapeManager | 22 |
| `playerNpc*` | Fake-player NPCs (with skin registration / capture / directory import / injection / blob persistence) | IPlayerNpc | 29 |
| `fakeInv*` | Fake inventory (1.23.0) | IFakeInventory | 9 |
| `sulfur*` | Sulfur-cube display (1.23.0) | ISulfurDisplay | 10 |
| `trade*` | Villager trade menu | ITradeMenu | 10 |
| `container*` | Virtual container / list UI | IContainerMenu | 10 |
| `npcDialog*` | NPC dialogue | INpcDialogue | 9 |
| `sensing*` | Player sensing (client device) | IPlayerSensing | 3 |
| `view*` | Client view override (1.25.0) | IViewOverride | 18 |

The table above totals **270** `exportAs` calls, and all of them register at runtime.

It degrades safely when absent (`ll.import` returns null) — **that is the only failure mode, and it is explicit**; "the call succeeded but had no effect" never happens.

**Type notation**:

| Type in signatures | Engine data type | JavaScript | Notes |
|---|---|---|---|
| `Float` | `Float` | `Number` | floating point |
| `Integer` | `Integer` | `Number` | integer; this library's ids / runtimeIds / uniqueIds are always int64 |
| `String` | `String` | `String` | string |
| `Boolean` | `Boolean` | `Boolean` | boolean |
| `Null` | `Null` | `null` / `undefined` | no return value |
| `Array<T>` | `Array` | `Array` | array; coordinate / colour arrays are `[x, y, z]` or `[r, g, b, a]` as noted per function |

Optional parameters are shown in square brackets: `foo(name : String[, count : Integer])`.

> This page lists only HologramLib's own 270 exports. The standard interfaces (`mc`, `ll`, `logger`, events, NBT, forms, databases …) are documented at <https://lse.levimc.org/apis/>.

## 2.1 shape* (shapes, 39 functions)

**Creation**

| Function | Signature |
|------|------|
| shapeCreateText | `(x : Float, y : Float, z : Float, text : String) -> Integer` |
| shapeCreateLine | `(x1,y1,z1 : Float, x2,y2,z2 : Float) -> Integer` |
| shapeCreateBox | `(x1,y1,z1 : Float, x2,y2,z2 : Float) -> Integer` |
| shapeCreateCircle | `(x : Float, y : Float, z : Float, scale : Float) -> Integer` |
| shapeCreateSphere | `(x : Float, y : Float, z : Float, scale : Float) -> Integer` |
| shapeCreateArrow | `(x1,y1,z1 : Float, x2,y2,z2 : Float) -> Integer` |

**Properties**

| Function | Signature |
|------|------|
| shapeSetText | `(id, text : String) -> Boolean` |
| shapeGetText | `(id) -> String` |
| shapeSetLocation | `(id, x : Float, y : Float, z : Float) -> Boolean` |
| shapeGetLocation | `(id) -> Array<Float> ([x, y, z])` |
| shapeSetColor | `(id, r : Float, g : Float, b : Float, a : Float) -> Boolean` |
| shapeGetColor | `(id) -> Array<Float> ([r, g, b, a])` |
| shapeSetScale | `(id, scale : Float) -> Boolean` |
| shapeSetDuration | `(id, seconds : Float) -> Boolean` |
| shapeSetDimension | `(id, dimId : Integer) -> Boolean` |
| shapeSetRotation | `(id, pitch : Float, yaw : Float, roll : Float) -> Boolean` |
| shapeClearRotation | `(id) -> Boolean` |
| shapeGetRotation | `(id) -> Array<Float> ([x, y, z])` |
| shapeGetShapeType | `(id) -> Integer` (0..5, see ShapeType) |
| shapeSetBackgroundColor | `(id, r : Float, g : Float, b : Float, a : Float) -> Boolean` (1.26.0; text shapes only, background box colour) |
| shapeClearBackgroundColor | `(id) -> Boolean` (1.26.0; back to the client's default background colour) |
| shapeSetDepthTest | `(id, enabled : Boolean) -> Boolean` (1.26.0; text shapes only, true = occluded by blocks/entities) |

**Display**

| Function | Signature |
|------|------|
| shapeDraw | `(id) -> Boolean` |
| shapeDrawToPlayer | `(id, playerName : String) -> Boolean` |
| shapeDrawToDimension | `(id, dimId : Integer) -> Boolean` |
| shapeDrawBatch | `(ids: Array<Integer>) -> Boolean` |
| shapeRemove | `(id) -> Boolean` |
| shapeRemoveToPlayer | `(id, playerName : String) -> Boolean` |
| shapeRemoveToDimension | `(id, dimId : Integer) -> Boolean` |
| shapeUpdate | `(id) -> Boolean` |
| shapeUpdateToPlayer | `(id, playerName : String) -> Boolean` |
| shapeUpdateToDimension | `(id, dimId : Integer) -> Boolean` |

**Lifecycle & queries**

| Function | Signature |
|------|------|
| shapeDestroy | `(id) -> Boolean` |
| shapeDestroyAll | `() -> Null` |
| shapeDestroyBatch | `(ids: Array<Integer>) -> Boolean` |
| shapeFindTextByLocation | `(x : Float, y : Float, z : Float, radius : Float) -> Array<Integer>` |
| shapeFindTextByLocationAndContent | `(x : Float, y : Float, z : Float, radius : Float, text : String) -> Integer` |
| shapeGetAllShapeIds | `() -> Array<Integer>` |
| shapeExists | `(id) -> Boolean` |

## 2.2 holo* (holographic text, 26 functions)

| Function | Signature |
|------|------|
| holoCreate | `(x : Float, y : Float, z : Float) -> Integer` |
| holoDestroy | `(id) -> Boolean` |
| holoDestroyAll | `() -> Null` |
| holoAddLine | `(id, text : String) -> Boolean` |
| holoSetLineText | `(id, lineIndex : Integer, text : String) -> Boolean` |
| holoSetLinePool | `(id, lineIndex : Integer, content: Array<String>, intervalMs : Integer) -> Boolean` (1.26.0; dynamic lines: the content pool rotates by **time modulo** the interval; pool ≤1 entry / interval≤0 = no rotation; empty array = clear the pool) |
| holoSetLineParseVariables | `(id, lineIndex : Integer, enabled : Boolean) -> Boolean` (1.26.0; whether this line parses variables, on by default; off = `{}` is shown literally) |
| holoRemoveLine | `(id, lineIndex : Integer) -> Boolean` |
| holoClearLines | `(id) -> Boolean` |
| holoGetLineCount | `(id) -> Integer` |
| holoSetColor | `(id, r,g,b,a : Float) -> Boolean` (colour of the whole text block) |
| holoSetScale | `(id, scale : Float) -> Boolean` (scale of the whole block; replaces the old holoSetLineScale) |
| holoSetBackgroundColor | `(id, r,g,b,a : Float) -> Boolean` (background box colour; unset = the client's default colour) |
| holoClearBackgroundColor | `(id) -> Boolean` (back to the client's default background colour) |
| holoSetDepthTest | `(id, enabled : Boolean) -> Boolean` (true = occluded by blocks/entities; false = always rendered / visible through walls, the default) |
| holoSetRotation | `(id, pitch : Float, yaw : Float, roll : Float) -> Boolean` (three-axis Euler, **unit = degrees**; once set it no longer faces the camera) |
| holoClearRotation | `(id) -> Boolean` (goes back to facing the camera) |
| holoSetLocation | `(id, x : Float, y : Float, z : Float) -> Boolean` |
| holoSetDimension | `(id, dimId : Integer) -> Boolean` (migrate dimension; if already drawn it is re-sent in place, no flicker) |
| holoSetFollowPlayer | `(id, playerName : String, offsetY : Float) -> Boolean` (the position is resolved in place at setFollowPlayer/draw/refresh time) |
| holoClearFollowPlayer | `(id) -> Boolean` |
| holoDraw | `(id) -> Boolean` |
| holoDrawToDimension | `(id, dimId : Integer) -> Boolean` |
| holoDrawToPlayer | `(id, playerName : String) -> Boolean` |
| holoRemove | `(id) -> Boolean` |
| holoRefresh | `(id) -> Boolean` (re-resolves variables / follow coordinates and re-sends in place) |

> **1.26.0 shrink**: the former `holoSetLineScale` / `holoSetLineColor` / `holoSetLineGradient` / `holoSetLineRainbow` /
> `holoSetLineScroll` / `holoSetVerticalAnimation` / `holoSetLineSpacing` / `holoTick` were removed (under the "one shape
> for the whole block" model they cannot be implemented, or are no-ops). For per-line colour, embed § colour codes in the
> text instead; for per-line scale, use `holoSetScale`.

## 2.3 gradient* (gradient lines, 11 functions)

| Function | Signature |
|------|------|
| gradientCreate | `(x1,y1,z1 : Float, x2,y2,z2 : Float, segments : Integer) -> Integer` |
| gradientSetGradient | `(id, r1,g1,b1,r2,g2,b2 : Float) -> Boolean` |
| gradientSetRainbow | `(id, speed : Float) -> Boolean` |
| gradientSetColor | `(id, r,g,b,a : Float) -> Boolean` |
| gradientSetEndpoints | `(id, x1,y1,z1 : Float, x2,y2,z2 : Float) -> Boolean` |
| gradientDraw | `(id) -> Boolean` |
| gradientDrawToDimension | `(id, dimId : Integer) -> Boolean` |
| gradientRemove | `(id) -> Boolean` |
| gradientDestroy | `(id) -> Boolean` |
| gradientDestroyAll | `() -> Null` |
| gradientTick | `(deltaTime : Float) -> Null` (rainbow animation driver) |

## 2.4 itemDetail* (item detail, 2 functions)

| Function | Signature |
|------|------|
| itemDetailShow | `(dimId : Integer, x,y,z : Float, itemId : String, aux : Integer, count : Integer, customText : String) -> Integer` |
| itemDetailHide | `(id) -> Boolean` |

`itemDetailShow`: shows "localised item name xN" at (x,y,z) (no count suffix when count<=1); pass `""` as `customText` to use the automatic text, or a non-empty string to replace it entirely (supports § colour codes and `{variables}`). Returns a holographic-text id that you can refine further with `holo*`.

## 2.5 itemDisplay* (FMBE item display, 33 functions)

FMBE (fox + packets) technique: an invisible fox holding an item renders a floating display of any item/block. Three-axis rotation / function-based translation / scaling all support **Molang expressions** (e.g. `"math.sin(query.life_time*90)*360"`).

| Function | Signature |
|------|------|
| itemDisplayCreate | `(x : Float, y : Float, z : Float, dim : Integer, itemId : String, aux : Integer) -> Integer` |
| itemDisplayCreateAdvanced | `(x,y,z : Float, dim : Integer, itemId : String, aux : Integer, offX,offY,offZ : String, rotX,rotY,rotZ : String, scale : String) -> Integer` |
| itemDisplayCreateSeamless | `(x,y,z : Float, dim : Integer, itemId : String, aux : Integer, offX,offY,offZ : String, rotX,rotY,rotZ : String, scale : String, mode : Integer, viewDistance : Float, visiblePlayer : String) -> Integer` (seamless creation: mode / view distance / whitelist are written before the first spawn, so only one Add sequence is sent throughout; no need for setMode/setViewDistance/setVisiblePlayer after creation. mode: -1=default auto 1=item 2=block; viewDistance: -1=default 0=unlimited; an empty visiblePlayer string = everyone) |
| itemDisplayCreateRandom | `(x : Float, y : Float, z : Float, dim : Integer, itemId : String, aux : Integer) -> Integer` (non-repeating ID from the random range `[0x10000000,0x7FFFFFFF)`) |
| itemDisplayCreateWithId | `(x : Float, y : Float, z : Float, dim : Integer, itemId : String, aux : Integer, desiredId : Integer) -> Integer` (create with an explicit ID, for persistence restore; <=0 or already taken returns -2) |
| itemDisplayDestroy | `(id) -> Boolean` |
| itemDisplayDestroyAll | `() -> Null` |
| itemDisplayExists | `(id) -> Boolean` |
| itemDisplayIsIdUsed | `(id) -> Boolean` |
| itemDisplayScaleBy | `(id, factor : Float) -> Boolean` (relative scaling: factor>1 enlarges, 0<factor<1 shrinks) |
| itemDisplaySetItemWithNbt | `(id, itemId : String, aux : Integer, nbt : String) -> Boolean` (swap the item with extra data: nbt is SNBT; an empty string clears it) |
| itemDisplaySetGlint | `(id, on : Boolean) -> Boolean` (enchantment glint switch: on BDS's native path a level-1 Sharpness is injected, so the client shows the purple glint) |
| itemDisplaySetVisiblePlayers | `(id, playerNames: Array<String>) -> Boolean` (visible-player whitelist: matched by player name / LSE `player.realName`; an empty list = visible to everyone; the dimension / view-distance conditions still apply on top) |
| itemDisplayClearVisiblePlayers | `(id) -> Boolean` (clear the whitelist and go back to visible-to-everyone) |
| itemDisplaySetVisiblePlayer | `(id, playerName : String) -> Boolean` (scalar single-player whitelist; recommended on the JS side) |
| itemDisplayGetInfo | `(id) -> String` (diagnostic probe: returns a runtime-state summary; returns `not_found` when not found) |
| itemDisplayScaleTo | `(id, targetScale : Float) -> Boolean` (one-shot jump scaling: re-sends the full block animation sequence with the new scale constant, with no respawn and no Remove/Add; a gradual animation is stepped by the consumer (e.g. 5 steps of easeOutCubic). Block mode only; scale<0.01 is clamped to 0.01) |
| itemDisplayGetAllIds | `() -> Array<Integer>` |
| itemDisplaySetItem | `(id, itemId : String, aux : Integer) -> Boolean` (swap the item; clears extra data) |
| itemDisplaySetPosition | `(id, x,y,z : Float, dim : Integer) -> Boolean` (dim<0 changes the coordinates only) |
| itemDisplaySetOffset | `(id, ox,oy,oz : String) -> Boolean` (translation in model units; Molang) |
| itemDisplaySetBaseOffset | `(id, ox,oy,oz : String) -> Boolean` (base render-pixel offset; Molang) |
| itemDisplaySetRotation | `(id, rx,ry,rz : String) -> Boolean` (three-axis rotation / degrees; Molang) |
| itemDisplaySetScale | `(id, scale : String) -> Boolean` (Molang) |
| itemDisplaySetExtend | `(id, scale,rx,ry : String) -> Boolean` (block-mode two-stage transform) |
| itemDisplaySetMode | `(id, mode : Integer) -> Boolean` (0=auto 1=item 2=block) |
| itemDisplaySetEnabled | `(id, enabled : Boolean) -> Boolean` |
| itemDisplaySetViewDistance | `(id, dist : Float) -> Boolean` (<=0 = unlimited) |
| itemDisplayRotateY | `(id, delta : Float) -> Boolean` (additive yaw delta) |
| itemDisplayFindNearest | `(x,y,z : Float, dim : Integer, maxDist : Float) -> Integer` (nearest lookup; -1 when nothing matches) |
| itemDisplayFollow | `(id, playerName : String, offX,offY,offZ : Float) -> Boolean` (1.17.0 display follows the player: the target's live position is synced every tick (including an automatic respawn across dimensions), and MoveActorAbsolute is sent to players who already see it (not a teleport, so the client interpolates), giving smooth movement with no respawn and no flicker anywhere; following is released automatically when the player logs off (the block stays where it is); a manual `itemDisplaySetPosition` also releases it) |
| itemDisplayUnfollow | `(id) -> Boolean` (1.17.0 release following; returns true when there is no follow relationship) |
| itemDisplaySetHitbox | `(id, width : Float, height : Float) -> Boolean` (1.17.0 AABB hit volume: broadcasts SetActorDataPacket(R53=Width R54=Height) to players who already see it, immediately and with no respawn; the values are persisted into the config, so a respawn or a new observer is sent packets with the current values automatically; 0/0 = back to non-hittable. Once enabled the client's raycast can hit it → attacks route through `ghost*` back to the library id) |

Render modes: `auto` (default) picks 3D/2D automatically from the item; `item` is flat item rendering (rotY internally adds +205 to compensate for the fox's head orientation); `block` is 3D block rendering (the full rotation-matrix path plus the two-stage extension transform). Visibility is synced automatically by the library's Level tick hook.

## 2.6 entity* (custom entities, 37 functions)

`AddActorPacket` goes straight to the client to spawn a purely visual entity (an NPC shell / a decorative mob / an armour-stand prop; no collision, no real interaction — clicks route through `ghost*`). Property changes are merged by the tick dirty-refresh into a single respawn (no flicker).

**Creation & lifecycle**

| Function | Signature |
|------|------|
| entityCreate | `(identifier : String, x,y,z : Float, dim : Integer) -> Integer` |
| entityCreateAdvanced | `(identifier : String, x,y,z : Float, dim : Integer, yaw : Float, pitch : Float, scale : Float, nametag : String) -> Integer` |
| entityCreateRandom | `(identifier : String, x,y,z : Float, dim : Integer) -> Integer` (random-range ID) |
| entityCreateWithId | `(identifier : String, x,y,z : Float, dim : Integer, desiredId : Integer) -> Integer` (create with an explicit ID; <=0 or taken returns -2) |
| entityDestroy | `(id) -> Boolean` |
| entityDestroyAll | `() -> Null` |
| entityExists | `(id) -> Boolean` |
| entityIsIdUsed | `(id) -> Boolean` |
| entityGetAllIds | `() -> Array<Integer>` |

**Properties**

| Function | Signature |
|------|------|
| entitySetIdentifier | `(id, identifier : String) -> Boolean` (swap the entity type; a short name gets `minecraft:` prepended automatically) |
| entitySetPosition | `(id, x,y,z : Float, dim : Integer) -> Boolean` (dim<0 changes the coordinates only) |
| entitySetRotation | `(id, yaw : Float, pitch : Float) -> Boolean` (degrees) |
| entitySetNametag | `(id, text : String) -> Boolean` (supports § colour codes; an empty string clears it) |
| entitySetScale | `(id, scale : Float) -> Boolean` (rejected when <=0; clamped automatically to 0.0625~10) |
| entitySetVariant | `(id, variant : Integer) -> Boolean` |
| entitySetMarkVariant | `(id, markVariant : Integer) -> Boolean` |
| entitySetColorIndex | `(id, colorIndex : Integer) -> Boolean` (sheep / collar and other dyeable entities) |
| entitySetFlags | `(id, flags : Integer) -> Boolean` (raw bitmask, 0x01=on fire, 0x20=invisible, etc.) |
| entitySetInvisible | `(id, on : Boolean) -> Boolean` (convenience invisibility switch, idempotent) |
| entitySetEnabled | `(id, enabled : Boolean) -> Boolean` |
| entitySetViewDistance | `(id, dist : Float) -> Boolean` (<=0 = unlimited) |
| entitySetPose | `(id, pose : Integer) -> Boolean` (PoseIndex 0..13; armour-stand sitting / sleeping / dancing poses, etc.) |
| entitySetEquipmentSlot | `(id, slot : Integer, name : String, aux : Integer, nbt : String) -> Boolean` (slot 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet; an empty name clears it; nbt is SNBT) |
| entityFindNearest | `(x,y,z : Float, dim : Integer, maxDist : Float) -> Integer` (nearest lookup; -1 when nothing matches) |

**Extended capabilities**

| Function | Signature |
|------|------|
| entityScaleBy | `(id, factor : Float) -> Boolean` (relative scaling: existing scale × factor, clamped automatically to 0.0625~10) |
| entitySetVisiblePlayers | `(id, playerNames: Array<String>) -> Boolean` (visible-player whitelist; an empty list = visible to everyone) |
| entitySetVisiblePlayer | `(id, playerName : String) -> Boolean` (scalar single-player whitelist; recommended on the JS side) |
| entityClearVisiblePlayers | `(id) -> Boolean` (go back to visible-to-everyone) |
| entityGetInfo | `(id) -> String` (diagnostic probe; returns `not_found` when not found) |
| entitySetRidePlayer | `(id, playerName : String) -> Boolean` (the entity rides on the given player's head; an empty name clears it; the player must be online) |
| entitySetRideEntity | `(id, vehicleEntityId : Integer) -> Boolean` (ride another custom entity; 0 clears it) |
| entityClearRide | `(id) -> Boolean` (break the riding link) |
| entityPlayAnimation | `(id, animation : String, stopExpression : String, durationTicks : Integer) -> Boolean` (play a vanilla animation, e.g. `animation.humanoid.base_pose`; an empty stopExpression = permanent; durationTicks>0 stops automatically when it expires) |
| entitySetPlayerNametag | `(id, playerName : String, text : String) -> Boolean` (1.22.0; changes only the nametag this player sees; an empty string = clear the override and go back to the config) |
| entitySetPlayerScale | `(id, playerName : String, scale : Float) -> Boolean` (1.22.0; changes only the scale this player sees; <=0 = clear the override) |
| entitySetPlayerEquipmentSlot | `(id, playerName : String, slot : Integer, name : String, aux : Integer, nbt : String) -> Boolean` (1.22.0; 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet; an empty name = fall back to the config) |
| entityClearPlayerAppearance | `(id, playerName : String) -> Boolean` (1.22.0; clears this player's nametag / scale / equipment overrides) |

## 2.7 ghost* (interaction event polling, 2 functions)

| Function | Signature |
|------|------|
| ghostPollInteractions | `() -> Array<String>` (takes and clears the pending interaction queue; the queue holds at most 256 entries, and the oldest is dropped when it is full) |
| ghostClearInteractions | `() -> Null` (discards every pending event in the queue) |

An event entry is a parseable string, in this format:

```
player=<realName> action=<1..6> domain=<entity|itemDisplay|npc> id=<libraryId> pos=(x,y,z)
```

- `action`: 1=Interact (right-click interaction) 2=Attack (attack / left-click) 3=StopRiding 4=InteractUpdate 5=NpcOpen 6=OpenInventory
- `pos` only appears when the client carries coordinates (InteractUpdate and the like)
- Example: `player=Steve action=2 domain=entity id=42` — player Steve left-clicked the custom entity with library id 42

LSE polling example:

```js
const poll = ll.import("HologramLib", "ghostPollInteractions");
setInterval(() => {
    for (const line of poll()) {
        const m = line.match(/player=(\S+) action=(\d+) domain=(\S+) id=(-?\d+)/);
        if (!m) continue;
        const [, player, action, domain, id] = m;
        if (action === "2" && domain === "entity") { /* left-clicked an NPC */ }
    }
}, 200);
```

## 2.8 particle* (generic particle shape system, 22 functions)

Shapes (points / lines / rectangular rings / filled planes / box frames / six faces / polyhedra) + transforms (translation / smooth movement / rotation / spin / scaling / following a player) + a multi-player whitelist + per-player view-distance culling — all self-driven by the C++ tick. Sending goes through the vanilla channel, and BDS's tick flush automatically aggregates and compresses it into a single Batch datagram.

**Core concepts**:
- **Shape**: defined in world coordinates at creation and converted internally to local coordinates (anchor = line midpoint / rectangle centre / box centre / polyhedron centroid); sampled points are cached and reused
- **Transform**: `world = anchor + R(Euler ZYX, degrees) · (scale · local)`; `setPos` is "moving the particles" (the next emission re-sends the whole batch at the new position); `moveTo` is smooth point-to-point movement (the anchor approaches the target by easeOutCubic interpolation, and a single point / a whole face (plane/rect) / the whole shape (box/poly) all move with the anchor); `spin` is spin in degrees/tick; with `follow` the anchor is player position + offset every tick (it follows across dimensions; moveTo/setPos release the follow)
- **Visibility**: an empty whitelist (uuid CSV) = everyone in the shape's dimension; per-player 3D view-distance culling (`viewDistance` 0 = no culling)
- **Lifecycle**: `lifetimeTicks` 0 = permanent; `setLifetime` restarts the clock from now; particles are transient → the whole batch is re-sent every `intervalTicks` to keep the visual resident
- **Plane axis** (the `axis` parameter of rect/plane): 0=XY (w along X, h along Y), 1=YZ (w along Z, h along Y), 2=XZ (w along X, h along Z)

| Function | Signature |
|------|------|
| particleCreatePoint | `(owner : String, dimId : Integer, x : Float, y : Float, z : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer` |
| particleCreateLine | `(owner : String, dimId : Integer, x1,y,z1 : Float, x2,y2,z2 : Float, step : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer` (anchor = midpoint, rotation about the midpoint) |
| particleCreateRect | `(owner : String, dimId : Integer, cx,cy,cz : Float, w : Float, h : Float, axis : Integer, step : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer` (rectangular ring) |
| particleCreatePlane | same parameters as rect (filled plane mesh) |
| particleCreateBox | `(owner : String, dimId : Integer, cx,cy,cz : Float, hx,hy,hz : Float, step : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer` (a 12-edge box wireframe, half-extents) |
| particleCreateBoxFaces | same parameters as box (six filled faces) |
| particleCreatePoly | `(owner : String, dimId : Integer, verts : String, edges : String, step : Float, effect : String, interval : Integer, lifetime : Integer) -> Integer` (verts `"x,y,z;x,y,z"` in world coordinates, anchor = centroid; edges `"i-j"` as vertex-index pairs) |
| particleSetPos | `(id : Integer, x, y, z : Float) -> Boolean` (translate the anchor; releases the follow) |
| particleMoveBy | `(id : Integer, dx, dy, dz : Float) -> Boolean` (relative translation) |
| particleMoveTo | `(id : Integer, x, y, z : Float, durationTicks : Integer) -> Boolean` (smooth point-to-point movement, easeOutCubic; releases the follow; duration<=0 = arrive immediately) |
| particleSetRot | `(id : Integer, rx, ry, rz : Float) -> Boolean` (Euler angles, degrees, ZYX order, about the anchor) |
| particleSpin | `(id : Integer, sx, sy, sz : Float) -> Boolean` (spin rate, degrees/tick; 0,0,0 stops it) |
| particleSetScale | `(id : Integer, s : Float) -> Boolean` |
| particleFollow | `(id : Integer, uuid : String, offX, offY, offZ : Float) -> Boolean` (the anchor follows the player + offset, every tick) |
| particleUnfollow | `(id : Integer) -> Boolean` |
| particleSetEffect | `(id : Integer, effect : String) -> Boolean` |
| particleSetVisible | `(id : Integer, playersCsv : String) -> Boolean` (`"uuid1,uuid2"` whitelist; an empty string = everyone in the dimension) |
| particleSetInterval | `(id : Integer, ticks : Integer) -> Boolean` |
| particleSetViewDistance | `(id : Integer, blocks : Integer) -> Boolean` (per-player 3D culling; 0 = no culling) |
| particleSetLifetime | `(id : Integer, ticks : Integer) -> Boolean` (from now on; 0 = permanent) |
| particleDestroy | `(id : Integer) -> Boolean` |
| particleGetInfo | `(id : Integer) -> String` (diagnostic probe) |

LSE example (a spinning cube + a halo that follows the player):

```js
const create = ll.import("HologramLib", "particleCreateBox");
const spin   = ll.import("HologramLib", "particleSpin");
const follow = ll.import("HologramLib", "particleFollow");
const rot    = ll.import("HologramLib", "particleSetRot");
const vis    = ll.import("HologramLib", "particleSetVisible");

// 1. Spinning cube wireframe (rotating about its centre)
const box = create("demo", 0, 100.5, 64.5, 100.5, 3, 3, 3, 1, "minecraft:endrod", 2, 0);
spin(box, 0, 3, 0);            // rotate 3 degrees about Y every tick
vis(box, player.uuid);          // visible to that player only

// 2. A halo at the player's feet (plane XZ + follow)
const ring = ll.import("HologramLib", "particleCreateRect");
const halo = ring("demo", 0, 0, 0, 0, 6, 6, 2, 1.5, "minecraft:endrod", 2, 0);
follow(halo, player.uuid, 0, 0.3, 0); // anchor = player position + offset, updated every tick

// rotate 45 degrees by hand (overrides the accumulated spin)
rot(box, 45, 0, 0);
```

## 2.9 playerNpc* (fake-player NPCs, 29 functions)

> **NPC skins render correctly again (fixed in 26.40.2)**. Every function below is usable (registration / capture / import / re-skinning / export all work), and NPC creation, movement, orientation, scaling, view distance and visibility control all render correctly.

A pure-protocol fake player (it does not take up the server-side entity system): `PlayerList(Add, skin) → AddPlayer → [20t] PlayerList(Remove)`; click interaction goes through the ghost pipeline with `domain="npc"` (§2.7 polling). Skins have three sources: PNG file registration / directory batch import (one subfolder = a PNG + an optional `.json` model) / capture from an online player (a full-field runtime snapshot, so the player re-skinning afterwards does not affect it). The library writes nothing to disk, so on the LSE side re-register or re-capture if you need it to survive a restart.

`setPositionLight` / `playAnimation(To)` / `setEntitySpawnCallback` are **C++ interfaces with no LSE export yet** (likewise `setRotationLight`).
The skin family (exported as of 26.40.8): `playerNpcInjectSkin` / `playerNpcInjectSkinAll` / `playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64` (see the table below).

| Function | Signature |
|------|------|
| playerNpcRegisterSkin | `(pngPath : String, skinId : String, geometry : String, armSize : String) -> Boolean` (an empty skinId = the file name; an empty geometry = geometry.humanoid.custom; armSize: wide/slim) |
| playerNpcImportSkins | `(dirPath : String) -> Integer` (directory batch import: one subfolder = one skin set, the PNG is required and the `.json` geometry optional; skinId = the folder name; returns how many were imported, -1 for an invalid directory) |
| playerNpcCaptureSkin | `(skinId : String, playerName : String) -> Boolean` (capture the current skin from an online player and register a full-field snapshot; their re-skinning afterwards does not affect it) |
| playerNpcHasSkin | `(skinId : String) -> Boolean` |
| playerNpcUnregisterSkin | `(skinId : String) -> Boolean` (refused while an NPC references it) |
| playerNpcGetSkinIds | `() -> Array<String>` |
| playerNpcCreate | `(x, y, z : Float, dim : Integer, name : String, skinId : String) -> Integer` (failure <0: -2 = id taken, -3 = the skin is not registered) |
| playerNpcCreateRandom | same parameters as create (random-range ID) |
| playerNpcCreateWithId | `(x, y, z : Float, dim : Integer, name : String, skinId : String, desiredId : Integer) -> Integer` (for persistence restore) |
| playerNpcDestroy | `(id : Integer) -> Boolean` |
| playerNpcDestroyAll | `() -> Null` |
| playerNpcExists | `(id : Integer) -> Boolean` |
| playerNpcIsIdUsed | `(id : Integer) -> Boolean` |
| playerNpcGetAllIds | `() -> Array<Integer>` |
| playerNpcSetPos | `(id : Integer, x, y, z : Float, dim : Integer) -> Boolean` (dim<0 changes the coordinates only) |
| playerNpcSetRotation | `(id : Integer, yaw : Float) -> Boolean` |
| playerNpcSetNametag | `(id : Integer, text : String) -> Boolean` (an empty string clears it) |
| playerNpcSetSkin | `(id : Integer, skinId : String) -> Boolean` (re-skinning = a respawn) |
| playerNpcSetScale | `(id : Integer, scale : Float) -> Boolean` (model scaling; the client hard-limits it to 0.0625~10 and the hitbox scales with it; changes are merged by the tick dirty-refresh into a single respawn) |
| playerNpcSetViewDistance | `(id : Integer, dist : Float) -> Boolean` (<=0 = unlimited) |
| playerNpcSetEnabled | `(id : Integer, enabled : Boolean) -> Boolean` |
| playerNpcSetVisiblePlayers | `(id : Integer, players: Array<String>) -> Boolean` (an empty list = visible to everyone) |
| playerNpcClearVisiblePlayers | `(id : Integer) -> Boolean` |
| playerNpcSetVisiblePlayer | `(id : Integer, playerName : String) -> Boolean` |
| playerNpcGetDebugInfo | `(id : Integer) -> String` (diagnostic probe) |
| playerNpcInjectSkin | `(viewerName : String, targetName : String, skinId : String) -> Boolean` (1.26.0; sends a skin from the registry to viewers — an empty viewerName = every online player **including the re-skinned player themself** ("see yourself re-skinned"), non-empty = that player only; target must be online and skinId must be registered; **it does not re-send automatically**) |
| playerNpcInjectSkinAll | `(targetName : String, skinId : String) -> Boolean` (1.26.0; convenience: equivalent to an empty viewerName) |
| playerNpcGetSkinBlobB64 | `(skinId : String) -> String` (1.26.0; **base64** of the full-field binary snapshot (LSE string marshalling is unsafe for binary); an empty string = not registered) |
| playerNpcRegisterSkinFromBlobB64 | `(b64 : String) -> Boolean` (1.26.0; pairs with the previous one — the consumer writes it to disk and restores it after a restart, and it works even when the source player is offline) |

LSE example (capture a player's skin and create an NPC):

```js
const capture = ll.import("HologramLib", "playerNpcCaptureSkin");
const create  = ll.import("HologramLib", "playerNpcCreate");
const pos     = ll.import("HologramLib", "playerNpcSetPos");

// 1. Capture the current skin of online player Steve (geometry / cape / animations; registered as the snapshot "steve_skin")
capture("steve_skin", "Steve");

// 2. Create a fake player with the captured skin (next to the overworld spawn point)
const npc = create(100.5, 64, 100.5, 0, "Steve's clone", "steve_skin");
// move / re-skin / whitelist
pos(npc, 105.5, 64, 100.5, -1);
```

PNG registration example (a custom skin + slim arms):

```js
const reg = ll.import("HologramLib", "playerNpcRegisterSkin");
reg("E:/server/skins/girl.png", "girl", "", "slim");
```

---

## 2.10 trade* (villager trade menu, 10 functions)

Opens the interface and lays out the trade table. Display-only by default; `settleLocally` turns on real protocol-only settlement (see the domain contract in 1.10).

| Function | Signature | Notes |
|------|------|------|
| tradeOpen | `(playerName : String, tradeType : String, tier : Integer, experience : Integer, realTrades : Boolean, carrierIdentifier : String) -> Integer` | Open the menu. `realTrades = false` (recommended) = protocol-layer display only; `true` = a real trade table (the player really can complete trades and items really are consumed). An empty `carrierIdentifier` = a villager, or fill in `minecraft:wandering_trader` |
| tradeAddOffer | `(id : Integer, buyAType : String, buyACount : Integer, buyBType : String, buyBCount : Integer, sellType : String, sellCount : Integer, sellName : String, tier : Integer, locked : Boolean) -> Boolean` | Appends one offer and **re-sends in place** (the interface is not reopened); an empty `buyBType` = no second cost item |
| tradeSetTier | `(id : Integer, tier : Integer, experience : Integer) -> Boolean` | Changes the displayed tier values / the experience bar (also re-sent in place) |
| tradeSetSettle | `(id : Integer, on : Boolean) -> Boolean` | **1.24.0** switch to real protocol-only settlement at any time after opening (it only works on the protocol-only path; the real trade-table path is settled by BDS). You can also set the initial value before opening with `TradeMenuSpec::settleLocally` |
| tradeIsSettle | `(id : Integer) -> Boolean` | Queries whether real settlement is currently on |
| tradePollSettlements | `() -> Array<String>` | **Takes and clears** the settlement callbacks; each is `player=Steve menuId=3 offer=1 ok=1 reason=` |
| tradeClose / tradeCloseAll / tradeIsOpen / tradeGetIds | — | Lifecycle and queries |

> **This domain has no click callback.** For a list interface where "clicking an entry fires a callback", use `container*` (a click is an item pickup, and every input device sends the packet). Trade results can only be fetched through `tradePollSettlements`.

```js
const tradeOpen     = ll.import("HologramLib", "tradeOpen");
const tradeAddOffer = ll.import("HologramLib", "tradeAddOffer");
const id = tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
tradeAddOffer(id, "minecraft:emerald", 3, "", 1, "minecraft:diamond", 1, "§bDiamond", 1, false);
```

## 2.11 container* (virtual container / list UI, 1.21.0; LSE added in 1.22.0)

| Function | Signature | Notes |
|------|------|------|
| containerOpen | `(playerName : String, title : String, rows : Integer) -> Integer` | Opens an empty container (`rows` 3 = 27 slots, 6 = 54 slots). Opening has an inherent delay (7 ticks by default), and only after that can slots be filled |
| containerSetItem | `(id : Integer, slot : Integer, type : String, count : Integer, damage : Integer, name : String, loreCsv : String) -> Boolean` | single-slot refresh (one `InventorySlot`, no delay and no flicker); an empty `type` = clear that slot; `loreCsv` splits lines with `\n` or `\|`, empty = no lore lines |
| containerSetTitle | `(id : Integer, title : String) -> Boolean` | swap the title in place (re-sends the block entity + ContainerOpen, without re-placing the blocks) |
| containerSetInteractive | `(id : Integer, on : Boolean) -> Boolean` | **Interactive mode**: after opening, items dragged/swapped by the client **inside** the container are caught by the library and answered with success (the item really stays in its new slot); off by default (clicks are only reported back and the server pushes the item back) |
| containerClose / containerCloseAll / containerIsOpen / containerGetIds | — | Lifecycle and queries |
| containerPollClicks | `() -> Array<String>` | **Takes and clears** pending clicks/close events; each is `player=X menuId=N slot=S closed=0\|1` |
| containerClearClicks | `() -> Null` | discards every pending click in the queue |

```js
const containerOpen       = ll.import("HologramLib", "containerOpen");
const containerSetItem    = ll.import("HologramLib", "containerSetItem");
const containerPollClicks = ll.import("HologramLib", "containerPollClicks");
const id = containerOpen("Steve", "§8Quest list", 3);
containerSetItem(id, 0, "minecraft:diamond_sword", 1, 0, "§bTrial", "§7Click to claim|§8Second line");
// take clicks in a timer (a guard loop such as ll.on works too):
for (const line of containerPollClicks()) { /* split player= / menuId= / slot= / closed= */ }
```

Click semantics: a click is an item pickup — every input device sends the packet (field-tested: both 27 slots and the second half of a large container work).
The server has no such container, so items are never really taken; the library only reports the slot number back (`slot` = the entry index, and `slot=-1` when `closed=1`).

## 2.12 npcDialog* (NPC dialogue, 1.21.0; LSE added in 1.22.0; one avatar function added in 26.40.8 → 9 functions)

| Function | Signature | Notes |
|------|------|------|
| npcDialogOpen | `(playerName : String, npcName : String, sceneName : String, dialogue : String, buttonsSpec : String, carrierIdentifier : String) -> Integer` | Open the dialogue (calling it again for the same player closes the previous level first); the button-string format is below |
| npcDialogUpdate | `(id : Integer, dialogue : String, buttonsSpec : String) -> Boolean` | swap the body text and buttons in place (npcName / sceneName / carrier type stay as they are); **only suitable for tweaks while the interface is still open**, see the field-tested limitation below |
| npcDialogSetAvatar | `(id : Integer, skinVariant : Integer, avatarViewSpec : String) -> Boolean` | **26.40.8** swap the avatar / model inside the chat box: `skinVariant` = the NPC's built-in skin variant `0..59` (`-1` = default, no `SkinId` item added); `avatarViewSpec` = stack an `IViewOverride` on the carrier (same spec syntax as `viewEntity`, e.g. `name=§6Barkeep`). Updated in place and reopened (the **zero-flicker** path); `false` = that dialogue no longer exists |
| npcDialogClose / npcDialogCloseAll / npcDialogIsOpen / npcDialogGetIds | — | Lifecycle and queries |
| npcDialogPollClicks | `() -> Array<String>` | takes and clears; each is `player=X dialogId=N scene=S button=I actionId=A closed=0\|1 commands=...` |
| npcDialogClearClicks | `() -> Null` | discards the queue |

Button-string format: buttons are separated by `;` (empty segments are skipped) and fields by `|` — `label|actionId|mode|command1,command2` (the last three items may be omitted).
`mode`: 0=normal button 1=close 2=open.

**Field-tested limitation**: after the client clicks any button the interface collapses on its own, and `npcDialogUpdate` only re-sends `NpcDialoguePacket(Open)` once,
so the client does not pop it back up — switching to the next dialogue level therefore requires `npcDialogOpen` again (internally it deletes the old carrier, creates a new one and sends Open once more).

```js
const npcDialogOpen       = ll.import("HologramLib", "npcDialogOpen");
const npcDialogPollClicks = ll.import("HologramLib", "npcDialogPollClicks");
const id = npcDialogOpen("Steve", "§eVillage chief", "main", "Want a quest?", "§aAccept|main#0|0|say hello;;§cLeave|main#1|1|", "");
```

## 2.13 sensing* (player sensing / client device, 1.21.0; LSE added in 1.22.0)

The data source is the `PlayerAuthInput` (`InputMode`) the client reports every tick; the library captures it packet by packet and keeps a copy in memory — no events, no polling.

| Function | Signature | Notes |
|------|------|------|
| sensingDeviceOf | `(playerName : String) -> String` | `keyboardMouse` / `touch` / `gamepad` / `motionController` / `unknown` |
| sensingIsTouch | `(playerName : String) -> Boolean` | whether it is touch (the one you will use most) |
| sensingDeviceCode | `(playerName : String) -> Integer` | 0=Unknown 1=keyboard-mouse 2=touch 3=gamepad 4=motion (same values as the C++ enum) |

```js
const sensingIsTouch = ll.import("HologramLib", "sensingIsTouch");
if (sensingIsTouch("Steve")) { /* touch: use the container list */ } else { /* keyboard-mouse: use the trade menu */ }
```

## 2.14 fakeInv* (fake inventory, 1.23.0)

| Function | Signature | Notes |
|------|------|------|
| fakeInvApply | `(playerName : String, itemsSpec : String, refreshIntervalTicks : Integer) -> Boolean` | Applies the whole set. Each `itemsSpec` entry is `slot\|item id\|count\|name\|lore line~lore line`, with entries separated by `;` or a newline; `refreshIntervalTicks` 0 = no periodic re-send |
| fakeInvSetSlot | `(playerName : String, slot : Integer, type : String, count : Integer, damage : Integer, name : String, loreCsv : String) -> Boolean` | change one slot (an empty `type` = clear that slot) |
| fakeInvRefresh | `(playerName : String) -> Boolean` | re-sends the faked content immediately (use it after opening the interface, or when you notice it being overwritten) |
| fakeInvClear / fakeInvClearAll | `(playerName : String) -> Boolean` / `() -> Null` | undo the fake and restore the real inventory |
| fakeInvIsActive / fakeInvGetPlayers | `(playerName : String) -> Boolean` / `() -> Array<String>` | queries |
| fakeInvPollClicks | `() -> Array<String>` | takes and clears; each is `player=X slot=S` |
| fakeInvClearClicks | `() -> Null` | discards the queue |

```js
const fakeInvApply = ll.import("HologramLib", "fakeInvApply");
const fakeInvPollClicks = ll.import("HologramLib", "fakeInvPollClicks");
// hotbar slot 0 gets a sword, slot 1 gets 64 diamonds; slot 9 = the first main-inventory slot
fakeInvApply("Steve", "0|minecraft:diamond_sword|1|§cFake blade|§7Attack +7;1|minecraft:diamond|64|§bFake diamond", 20);
for (const line of fakeInvPollClicks()) { /* "player=Steve slot=0" */ }
```

## 2.15 sulfur* (sulfur-cube display, 1.23.0)

| Function | Signature | Notes |
|------|------|------|
| sulfurCreate | `(x : Float, y : Float, z : Float, dim : Integer, blockType : String, blockCount : Integer, blockDamage : Integer, blockName : String, archetype : String, invisible : Boolean) -> Integer` | spawns a cube "swallowing a block"; an empty `archetype` = no properties are sent; `invisible` = whether the cube itself is invisible (default true → you only see the swallowed block) |
| sulfurSetBlock | `(id : Integer, type : String, count : Integer, damage : Integer, name : String) -> Boolean` | swap the swallowed block/item (it rebuilds the entity once) |
| sulfurSetArchetype | `(id : Integer, archetype : String) -> Boolean` | swap the appearance tier (an empty string = clear the properties; the entity is not rebuilt) |
| sulfurSetInvisible / sulfurSetScale | `(id : Integer, on : Boolean) / (id : Integer, scale : Float) -> Boolean` | invisibility / scaling |
| sulfurDestroy / sulfurDestroyAll | — | destroy (`sulfurDestroy` for one / `sulfurDestroyAll` for all) |
| sulfurExists / sulfurGetIds / sulfurGetEntityId | — | queries (`sulfurGetEntityId` is the custom entity id behind it) |

```js
const sulfurCreate      = ll.import("HologramLib", "sulfurCreate");
const sulfurSetArchetype = ll.import("HologramLib", "sulfurSetArchetype");
const id = sulfurCreate(100.5, 65, -200.5, 0, "minecraft:bookshelf", 1, 0, "§6Bookshelf", "regular");
sulfurSetArchetype(id, "sticky");   // swap the appearance tier (the entity is not rebuilt)
```

## 2.16 view* (client view override, 1.25.0; 18 functions)

Suppress-only: the library intercepts packets that **were going to be sent anyway**, and when the content has to change it drops the original packet and sends a hand-written protocol packet instead; it does not modify engine objects, does not create entities and does not change server-side state. Data comes only from the protocol packets themselves (the one exception is player names and snapshots, see the architecture conventions).

| Function | Signature | Notes |
|------|------|------|
| viewEntity | `(playerName : String, entityUniqueId : Integer, spec : String) -> Boolean` | overrides what this player sees for that entity, per the spec; an empty player name = all players; false = the player is offline / the spec parsed to nothing |
| viewBlock | `(playerName : String, x : Integer, y : Integer, z : Integer, type : String) -> Boolean` | makes (x,y,z) show up as the block `type` on that player's client; an empty type = undo; false = the player is offline / the type name is invalid |
| viewClearEntity | `(playerName : String, entityUniqueId : Integer) -> Boolean` | undo a single entity |
| viewClearBlock | `(playerName : String, x : Integer, y : Integer, z : Integer) -> Boolean` | undo a single block (and push the real block back) |
| viewClearAll | `(playerName : String) -> Null` | undoes every override for that player (an empty string = all players) |
| viewDescribe | `(playerName : String) -> String` | a summary of the current override counts, e.g. `"entities=1 blocks=2"` |
| viewUniqueIdOf | `(playerName : String) -> Integer` | looks up the uniqueId of that player's entity by name; **`-1` = offline** (use this when a script cannot get the id) |
| viewEcho | `(text : String) -> String` | echoes the text back verbatim (with its length and the first 16 bytes in hex); for working out "did the string even arrive" |
| viewMetaProbe | `(spec : String) -> String` | **really serialises** the metadata the spec folds into and reads it back as items / bytes / hex. It goes through **exactly the same** `pushMetaItems` as the real send, so if it is green, that is the id/type string going out (**no client needed** to verify a spec) |
| viewItemSpawnHooked | `() -> Boolean` | **1.29.0**: whether this library's view override intercepts `AddItemActor` (id 15) as well. `true` = dropped items can be re-typed/hidden in one shot; older libraries do not have this name (`ll.hasExported` returns false) → scripts must periodically re-apply the override |

Applying an override to "yourself" (written so everyone can see it; for admins disguising themselves):

| Function | Signature | Notes |
|------|------|------|
| viewSelf | `(playerName : String, spec : String) -> Boolean` | equivalent to `viewEntity("", viewUniqueIdOf(playerName), spec)` |
| viewSelfClear | `(playerName : String) -> Boolean` | undo your own override |

**Per-field version** (one parameter per field, no spec string to build; recommended on the script side, as it avoids string building and escaping):

| Function | Signature | Notes |
|------|------|------|
| viewSelfType | `(playerName : String, type : String) -> Boolean` | you become that mob type (players and mobs both work) |
| viewSelfSkin | `(playerName : String, skinOfPlayer : String) -> Boolean` | you display as another online player (borrowing their skin) |
| viewSelfName | `(playerName : String, nametag : String, alwaysShow : Boolean) -> Boolean` | change the nametag above your own head |
| viewSelfHide | `(playerName : String, hidden : Boolean) -> Boolean` | hide yourself from everyone |
| viewTargetType | `(entityUniqueId : Integer, type : String) -> Boolean` | make **all** players see this entity as that type (not limited to player entities; the workhorse on the JS side when you cannot get someone else's id) |
| viewClearId | `(entityUniqueId : Integer) -> Boolean` | undo the override on this entity (all players) |

**uniqueId is negative, and only -1 means offline**:

> An entity's uniqueId is an int64, and players and some entities **are negative** to begin with (field-tested: a player on one server was `-25769803775` = `-0x5FFFFFFFF`).
> So "is it online" must be written as `viewUniqueIdOf(name) === -1` (`-1` is the library's outward "offline" convention and also BDS's
> INVALID constant, which no real online entity ever is), **never `< 0`** — the first 1.25.0 build used `-1` as an internal sentinel, so a player who was plainly
> online still returned false, while the C++ struct pass-through path never went through that check — so only the script side fell over.
>
> On an LSE `Entity` object: `.id` is the **runtime id** (a different thing), and to get the unique id use `.uniqueId` (a **string**,
> because an int64 may exceed JS's exact-integer range) and convert it with `Number(e.uniqueId)`.

The `spec` string (semicolon-separated, each item `key=value`, keys case-insensitive; whitespace around values is trimmed):

| Key | Value | Notes |
|----|----|------|
| `type` | entity type name | swap the type (a bare `cow` becomes `minecraft:cow` automatically); for a player entity = build a proxy mob for them (same runtimeId/uniqueId) |
| `as` | an online player's name | display as that player (the skin goes through the vanilla skin-update path, and it only works on player entities) |
| `hidden` | `1` / `true` / `yes` / `on` | hide this entity from that player (blocking its spawn and update packets) |
| `name` | nametag text | the nametag is only changed once `name` appears; an empty value = clear the name |
| `always` | `1` / `true` / `yes` / `on` | the nametag is always shown (the protocol value itself is inverted, which the library already handles) |
| `pose` | `0..13` | **1.29.0**: PoseIndex(79) — the client applies a pose **only to armour stands and players**; out-of-range values are clamped into range |
| `scale` | `0.0625~10` | **1.29.0**: **metadata scale** (client entity scale + hitbox 53/54); out of range and the whole item is ignored. ⚠ this is not the same thing as the "skin model scale" of the `skin=` tier |
| `variant` | integer | **1.29.0**: `Variant`(2) — the sheep's colour tier / the pufferfish's puff state (0 normal 1 half-puffed 2 fully puffed) / tropical-fish body shape 0·1 … |
| `mark` | integer | **1.29.0**: `MarkVariant`(43) — tropical-fish patterns 0..5, horse markings, etc. |
| `color` | `0..15` | **1.29.0**: `ColorIndex`(3) — the main colour (tropical fish etc.); out of range and the whole item is ignored |
| `color2` | `0..15` | **1.29.0**: `Color2Index`(82) — the pattern colour (tropical fish); out of range and the whole item is ignored |

> ⚠ **`mTick` is a mandatory field** (the 26.40 wire format): both `SetActorDataPacket` and `UpdateAttributesPacket`
> carry an `mTick` at the end. **Leave it unset (=0) and the client throws the packet away as stale** — the symptom is "every
> metadata value in the spawn packet is right, but nothing changes on screen" (on a live server, 2026-10-16: that is how
> tropical-fish variants and pufferfish puff tiers behaved, and the attribute re-confirmation is affected too).
> The view override domain now always fills in `currentViewTick()` (synced by `tickPulse` every beat); the customentity domain has always sent it
> this way (`CustomEntityManager`: `sadp.mTick = tick;`) — do not leave it out when writing new code.

> The **protocol types** of the four variant items (`variant` / `mark` / `color` / `color2`) come from the engine's own real spawn packet:
> `Variant`/`MarkVariant` are Int, `ColorIndex`/`Color2Index` are **Byte** (decoding a real sheep's AddActor in
> `logs/packetdump.log` gives: `id2(type2)` `id43(type2)` `id3(type0)` `id82(type0)`).
> Use-case example: when turning dropped items into entities, make every tropical-fish drop look different and make pufferfish always
> fully puffed (`viewEntity("", uid, "type=minecraft:tropicalfish;variant=1;mark=3;color=9;color2=6")`).

```js
const viewEntity  = ll.import("HologramLib", "viewEntity");
const viewBlock   = ll.import("HologramLib", "viewBlock");
const viewClearAll = ll.import("HologramLib", "viewClearAll");
const viewSelfType = ll.import("HologramLib", "viewSelfType");
const viewUniqueIdOf = ll.import("HologramLib", "viewUniqueIdOf");

// the zombie Steve sees becomes a cow, with the nametag "§cCow" shown always
viewEntity("Steve", 12345, "type=minecraft:cow;name=§cCow;always=1");
// make Steve unable to see it (in the server-side world it still exists and still hits people)
viewEntity("Steve", 12345, "hidden=1");
// (100,64,100) on Steve's client shows up as a diamond block
viewBlock("Steve", 100, 64, 100, "minecraft:diamond_block");
// turn yourself into a zombie (visible to everyone), then take it off
viewSelfType("Steve", "minecraft:zombie");
// is it online: only -1 counts (uniqueId itself is negative, do not write < 0)
const id = viewUniqueIdOf("Alex");
if (id !== -1) viewEntity("", id, "type=minecraft:cow");
viewClearAll("Steve");   // done
```

---

## Known issues

None at the moment.

The two `ghost*` exports (`ghostPollInteractions` / `ghostClearInteractions`) **used to** sit outside the
registration path (`ModEntry::exportLseFunctions()` never called `GhostInteractRouter::exportLse()`, so
`ll.import` returned `null`). They are wired in now: all 270 `exportAs` calls register at runtime.

> The library maintainers keep a separate authoritative list, **counted file by file from the source** (with the per-function names for each prefix, plus duplicate-name and comment checks), at `docs/internal/LSE导出清单.md` — that directory is local documentation and **is not shipped with the library**.
> When this page and the code disagree, the code wins.
