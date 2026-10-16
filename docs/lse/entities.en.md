# Entities, particles & interaction · Spawning entities out of thin air on the client, catching clicks, laying out particle shapes

This page covers three groups of capabilities:

- `entity*` (37): **protocol-layer custom entities** — conjure an entity on the client out of a vanilla entity type; suited to NPC shells, decorative mobs and armour-stand props. The server does not have this entity: no AI, no collision, nothing written to the save, and no part in any server-side logic.
- `ghost*` (2): **interaction event polling** — when a player clicks one of those protocol entities, the library routes the click into an event and puts it in a queue, and your script polls it out itself.
- `particle*` (22): **particle shapes** — lay out points, lines, rectangular rings, filled planes, box frames, six faces and polyhedra from vanilla particles, with translation, smooth movement, rotation, spin, scaling and following a player.

All three groups only handle "draw it" and "catch the click"; the business logic (commands, permissions, persistence) lives in your script.

> In this section's examples, `id` is the library id returned by an **entity** creation function, and `pid` is the shape id returned by a **particle** creation function. They are two unrelated numbering spaces: entity ids increment from 1 (you can also specify one or take a random one), and particle ids increment from 1.

## Quick start

```js
const entityCreate          = ll.import("HologramLib", "entityCreate");
const entitySetNametag      = ll.import("HologramLib", "entitySetNametag");
const ghostPollInteractions = ll.import("HologramLib", "ghostPollInteractions");
const particleCreateBox     = ll.import("HologramLib", "particleCreateBox");

// 1. Spawn an armour stand in the Overworld at (100.5, 64, 100.5) (dim: 0=Overworld 1=Nether 2=End)
const id = entityCreate("armor_stand", 100.5, 64, 100.5, 0);

// 2. Give it a nametag (§ colour codes go straight into the text)
entitySetNametag(id, "§eGuard");

// 3. Take click events every 200 milliseconds — events go into a queue, you must poll yourself
setInterval(() => {
  for (const line of ghostPollInteractions()) {
    logger.info(line);   // e.g. player=Steve action=2 domain=entity id=1
  }
}, 200);

// 4. Wrap a particle box around it (interval=2: re-send every 2 ticks; lifetime=0: permanent)
const pid = particleCreateBox("demo", 0, 100.5, 64.5, 100.5, 1.5, 1.5, 1.5, 1, "minecraft:endrod", 2, 0);
```

## Custom entities · Creation

This group creates the entity. Once created it appears on the clients of **players who meet the visibility conditions**: same dimension, inside the view distance, `enabled` switched on, and on the whitelist (no whitelist set means everyone).

### Create a custom entity

`entityCreate(identifier, x, y, z, dim)`

- Parameters:
  - `identifier` : `String`<br>the entity type, such as `minecraft:cow` or `armor_stand`; a short name (`cow`) gets the `minecraft:` prefix added automatically
  - `x` : `Float`<br>the world coordinate X (a float; `.5` lands at the centre of a block)
  - `y` : `Float`<br>the world coordinate Y (a float; `.5` lands at the centre of a block)
  - `z` : `Float`<br>the world coordinate Z (a float; `.5` lands at the centre of a block)
  - `dim` : `Integer`<br>the dimension, `0` Overworld / `1` Nether / `2` End
- Return value: the library entity id (every later `entity*` call uses it); returns `-1` when `identifier` is empty
- Return type: `Integer`
- Example:

    ```js
    const entityCreate = ll.import("HologramLib", "entityCreate");
    const id = entityCreate("minecraft:cow", 100.5, 64, 100.5, 0);
    ```

### Set orientation, scale and nametag at creation

`entityCreateAdvanced(identifier, x, y, z, dim, yaw, pitch, scale, nametag)`

- Parameters:
  - `identifier` : `String`<br>the same as `entityCreate`
  - `x` : `Float`<br>the same as `entityCreate`
  - `y` : `Float`<br>the same as `entityCreate`
  - `z` : `Float`<br>the same as `entityCreate`
  - `dim` : `Integer`<br>the same as `entityCreate`
  - `yaw` : `Float`<br>the yaw angle (degrees), the horizontal facing
  - `pitch` : `Float`<br>the pitch angle (degrees), looking up / down
  - `scale` : `Float`<br>overall scaling; it is clamped to `0.0625 ~ 10` when the packet is sent
  - `nametag` : `String`<br>the nametag text, supporting `§` colour codes; an empty string = no nametag
- Return value: the library entity id; returns `-1` when `identifier` is empty
- Return type: `Integer`
- Example:

    ```js
    const entityCreateAdvanced = ll.import("HologramLib", "entityCreateAdvanced");
    // A zombie facing east, scaled up 1.5×, with "§cGuard" written above its head
    const id = entityCreateAdvanced("zombie", 100.5, 64, 100.5, 0, 90, 0, 1.5, "§cGuard");
    ```

### Create an entity with a random id

`entityCreateRandom(identifier, x, y, z, dim)`

- Parameters:
  - `identifier` : `String`<br>the same as `entityCreate`
  - `x` : `Float`<br>the same as `entityCreate`
  - `y` : `Float`<br>the same as `entityCreate`
  - `z` : `Float`<br>the same as `entityCreate`
  - `dim` : `Integer`<br>the same as `entityCreate`
- Return value: the library entity id, taken from `[0x10000000, 0x7FFFFFFF)` (kept apart from the incrementing range for good, so they never collide); returns `-1` when no free id can be found
- Return type: `Integer`
- Example:

    ```js
    const entityCreateRandom = ll.import("HologramLib", "entityCreateRandom");
    const id = entityCreateRandom("armor_stand", 100.5, 64, 100.5, 0);
    ```

### Create an entity under an id you choose

`entityCreateWithId(identifier, x, y, z, dim, desiredId)`

- Parameters:
  - `identifier` : `String`<br>the same as `entityCreate`
  - `x` : `Float`<br>the same as `entityCreate`
  - `y` : `Float`<br>the same as `entityCreate`
  - `z` : `Float`<br>the same as `entityCreate`
  - `dim` : `Integer`<br>the same as `entityCreate`
  - `desiredId` : `Integer`<br>the library id you want to occupy; it must be `> 0`
- Return value: on success it is exactly `desiredId`; `desiredId <= 0`, or an id that is already taken, returns `-2`
- Return type: `Integer`
- Example:

    ```js
    const entityCreateWithId = ll.import("HologramLib", "entityCreateWithId");
    const entityIsIdUsed = ll.import("HologramLib", "entityIsIdUsed");
    const wanted = 1001;                       // the id you saved to disk yourself
    const id = entityIsIdUsed(wanted) ? -2 : entityCreateWithId("armor_stand", 100.5, 64, 100.5, 0, wanted);
    ```

## Custom entities · Properties

Every property change takes effect "on the next tick": structural changes (type, pose, equipment, flags and the like) merge into a single rebuild, while coordinates / orientation / nametag / scale / invisibility go out as delta packets — neither case flickers.

### Change the entity type

`entitySetIdentifier(id, identifier)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `identifier` : `String`<br>the new entity type; a short name gets `minecraft:` added automatically; an empty string is refused
- Return value: `true` when the change went through; `false` when the id does not exist or `identifier` is empty
- Return type: `Boolean`
- Example:

    ```js
    const entitySetIdentifier = ll.import("HologramLib", "entitySetIdentifier");
    entitySetIdentifier(id, "minecraft:skeleton");   // turn the zombie into a skeleton
    ```

### Move the entity to new coordinates

`entitySetPosition(id, x, y, z, dim)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `x` : `Float`<br>the new world coordinate X
  - `y` : `Float`<br>the new world coordinate Y
  - `z` : `Float`<br>the new world coordinate Z
  - `dim` : `Integer`<br>`>= 0` puts the entity in that dimension; `< 0` = change the coordinates only and leave the dimension alone
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetPosition = ll.import("HologramLib", "entitySetPosition");
    entitySetPosition(id, 105.5, 64, 100.5, -1);   // move the position only, still in the same dimension
    entitySetPosition(id, 0.5, 70, 0.5, 1);        // move it to the Nether
    ```

### Set the entity's orientation

`entitySetRotation(id, yaw, pitch)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `yaw` : `Float`<br>the yaw angle (degrees), the horizontal facing
  - `pitch` : `Float`<br>the pitch angle (degrees)
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetRotation = ll.import("HologramLib", "entitySetRotation");
    entitySetRotation(id, 180, 0);   // turn around to face south
    ```

### Set the nametag above its head

`entitySetNametag(id, text)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `text` : `String`<br>the nametag text, supporting `§` colour codes; an empty string = clear the nametag
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetNametag = ll.import("HologramLib", "entitySetNametag");
    entitySetNametag(id, "§aMerchant §7(right-click)");
    entitySetNametag(id, "");                 // clear the nametag
    ```

### Set the entity's scale

`entitySetScale(id, scale)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `scale` : `Float`<br>the target scale (the model and the hitbox grow together); `<= 0` is refused outright; when the packet is actually sent it is clamped to the client's valid range `0.0625 ~ 10`
- Return value: `true` on success; `false` when `scale <= 0` or the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetScale = ll.import("HologramLib", "entitySetScale");
    entitySetScale(id, 2);      // scale up to 2×
    entitySetScale(id, 1);      // back to the original size
    ```

### Scale relative to the current size

`entityScaleBy(id, factor)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `factor` : `Float`<br>the multiplier, `> 1` enlarges and `0 < factor < 1` shrinks; the result is clamped to `0.0625 ~ 10` automatically
- Return value: `true` on success; `false` when `factor <= 0` or the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entityScaleBy = ll.import("HologramLib", "entityScaleBy");
    entityScaleBy(id, 1.2);   // every call makes it ten per cent bigger
    entityScaleBy(id, 0.5);   // shrink it to half
    ```

### Set the main variant number

`entitySetVariant(id, variant)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `variant` : `Integer`<br>the variant number (the skin / subspecies); the same number means different things on different entities (a tropical fish's body shape, a pufferfish's puff state, say), and the library does not validate it
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetVariant = ll.import("HologramLib", "entitySetVariant");
    entitySetVariant(id, 1);
    ```

### Set the secondary variant number

`entitySetMarkVariant(id, markVariant)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `markVariant` : `Integer`<br>the secondary variant number; what it means depends on the entity type (a tropical fish's pattern, a horse's markings, say)
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetMarkVariant = ll.import("HologramLib", "entitySetMarkVariant");
    entitySetMarkVariant(id, 3);
    ```

### Set the colour index

`entitySetColorIndex(id, colorIndex)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `colorIndex` : `Integer`<br>the colour number, used for dyeable entities such as sheep and collars
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetColorIndex = ll.import("HologramLib", "entitySetColorIndex");
    entitySetColorIndex(id, 14);   // the red range
    ```

### Overwrite the raw flags bitmask

`entitySetFlags(id, flags)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `flags` : `Integer`<br>the raw bitmask value (an advanced use), e.g. `0x01` = on fire, `0x20` = invisible; it replaces the whole block and is not merged with the old value
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetFlags = ll.import("HologramLib", "entitySetFlags");
    entitySetFlags(id, 0x01);   // set it on fire (visually)
    entitySetFlags(id, 0);      // clear every flag
    ```

### Toggle entity invisibility

`entitySetInvisible(id, on)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `on` : `Boolean`<br>`true` invisible / `false` visible again; it touches only that one bit, and it also returns `true` when the value already matches (a repeated call has no side effects)
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetInvisible = ll.import("HologramLib", "entitySetInvisible");
    entitySetInvisible(id, true);
    entitySetInvisible(id, false);   // visible again
    ```

### Set the entity pose

`entitySetPose(id, pose)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `pose` : `Integer`<br>the pose number (`PoseIndex`), which only works on models that pose accordingly; `0..13` (`0`=standing, `1`=no base plate, `2`=show arms, `3..13` are sitting / sleeping / dancing and the like); an out-of-range value is clamped into this range automatically
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetPose = ll.import("HologramLib", "entitySetPose");
    entitySetPose(id, 2);    // the armour stand shows its arms
    entitySetPose(id, 0);    // back to standing
    ```

### Set the item in an equipment slot

`entitySetEquipmentSlot(id, slot, name, aux, nbt)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `slot` : `Integer`<br>`0`=mainhand `1`=offhand `2`=head `3`=chest `4`=legs `5`=feet; any other value returns `false`
  - `name` : `String`<br>the item id; a short name gets `minecraft:` added automatically; an empty string = clear that slot
  - `aux` : `Integer`<br>the item's data value; what exactly it means depends on the item
  - `nbt` : `String`<br>extra data as SNBT text; an empty string = no extra data
- Return value: `true` on success; `false` when `slot` is out of range or the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetEquipmentSlot = ll.import("HologramLib", "entitySetEquipmentSlot");
    entitySetEquipmentSlot(id, 0, "diamond_sword", 0, "");      // a diamond sword in the main hand
    entitySetEquipmentSlot(id, 2, "golden_helmet", 0, "");      // a golden helmet on the head
    entitySetEquipmentSlot(id, 0, "", 0, "");                   // clear the main hand
    ```

## Custom entities · Visibility

Who can see this entity is decided by **dimension + view distance + whitelist + master switch** together; the four conditions stack.

### Toggle master visibility

`entitySetEnabled(id, enabled)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `enabled` : `Boolean`<br>`false` = withdraw it from every client, `true` = send it out again
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetEnabled = ll.import("HologramLib", "entitySetEnabled");
    entitySetEnabled(id, false);   // hide it from everyone
    entitySetEnabled(id, true);    // restore it
    ```

### Set the per-player view radius

`entitySetViewDistance(id, dist)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `dist` : `Float`<br>the distance (blocks); `<= 0` = no distance limit. The default at creation time is 64 blocks
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetViewDistance = ll.import("HologramLib", "entitySetViewDistance");
    entitySetViewDistance(id, 32);   // only visible within 32 blocks
    entitySetViewDistance(id, 0);    // no distance limit
    ```

### Restrict visibility with a whitelist

`entitySetVisiblePlayers(id, playerNames)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `playerNames` : `Array<String>`<br>an array of player names (matched by LSE's `player.realName`); an empty array = clear the restriction and go back to visible to everyone
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetVisiblePlayers = ll.import("HologramLib", "entitySetVisiblePlayers");
    entitySetVisiblePlayers(id, ["Steve", "Alex"]);   // show it to these two only
    entitySetVisiblePlayers(id, []);                  // drop the restriction
    ```

### Show the entity to a single player

`entitySetVisiblePlayer(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `playerName` : `String`<br>the one visible player's name; it **replaces** the whole list rather than appending to it. Use `entitySetVisiblePlayers` when you want several players to see it; on the script side this is the one to prefer (no array to build)
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetVisiblePlayer = ll.import("HologramLib", "entitySetVisiblePlayer");
    entitySetVisiblePlayer(id, "Steve");
    ```

### Clear the whitelist and restore everyone

`entityClearVisiblePlayers(id)`

- Parameters:
  - `id` : `Integer`<br>the entity id
- Return value: `true` on success (it also returns `true` when no whitelist was set in the first place); `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entityClearVisiblePlayers = ll.import("HologramLib", "entityClearVisiblePlayers");
    entityClearVisiblePlayers(id);
    ```

## Custom entities · Riding & animation

### Make the entity ride a player

`entitySetRidePlayer(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `playerName` : `String`<br>the player name, which must be online; an empty string = clear the riding link
- Return value: `true` on success (it also counts as `true` when there was no link at all, and when an empty string clears one); `false` when the id does not exist or the player is not online
- Return type: `Boolean`
- Example:

    ```js
    const entitySetRidePlayer = ll.import("HologramLib", "entitySetRidePlayer");
    entitySetRidePlayer(id, "Steve");   // hang it on Steve's head
    entitySetRidePlayer(id, "");        // get down
    ```

### Make the entity ride another custom entity

`entitySetRideEntity(id, vehicleEntityId)`

- Parameters:
  - `id` : `Integer`<br>the entity id (the passenger)
  - `vehicleEntityId` : `Integer`<br>the library id of the vehicle entity; `0` = clear the link
- Return value: `true` on success; `false` when the vehicle does not exist, the vehicle is the entity itself, or the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entitySetRideEntity = ll.import("HologramLib", "entitySetRideEntity");
    const horse = ll.import("HologramLib", "entityCreate")("horse", 100.5, 64, 100.5, 0);
    entitySetRideEntity(id, horse);   // id rides on horse
    entitySetRideEntity(id, 0);       // release
    ```

### Clear player and entity riding links

`entityClearRide(id)`

- Parameters:
  - `id` : `Integer`<br>the entity id
- Return value: `true` on success (it also returns `true` when there was no link in the first place); `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const entityClearRide = ll.import("HologramLib", "entityClearRide");
    entityClearRide(id);
    ```

### Play a vanilla animation for players who have seen it

`entityPlayAnimation(id, animation, stopExpression, durationTicks)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `animation` : `String`<br>the animation identifier in the resource pack, such as `animation.humanoid.base_pose`; an empty string is refused
  - `stopExpression` : `String`<br>a Molang stop expression; the animation stops when the expression is true; an empty string = it stays and never stops by itself
  - `durationTicks` : `Integer`<br>when `> 0`, a stop is sent out automatically after that many ticks; `0` = no time limit
- Return value: `true` means it has been queued for sending (it reaches the client in about 2 ticks); `false` when the id does not exist, `animation` is empty, the entity has been switched off with `entitySetEnabled`, or no player has seen this entity yet
- Return type: `Boolean`
- Example:

    ```js
    const entityPlayAnimation = ll.import("HologramLib", "entityPlayAnimation");
    entityPlayAnimation(id, "animation.humanoid.base_pose", "", 0);        // permanent
    entityPlayAnimation(id, "animation.humanoid.attack", "", 20);          // play for 1 second, then stop automatically
    ```

## Custom entities · Per-client appearance

One entity, showing different nametags, scales and equipment to different players. An override takes effect only on the client of the player you name; everyone else still sees the shared configuration.

### Change the nametag one player sees

`entitySetPlayerNametag(id, playerName, text)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `playerName` : `String`<br>the viewer's name; they must be online and must have already seen this entity
  - `text` : `String`<br>the text only this player can see; an empty string = clear that player's override and go back to the shared nametag
- Return value: `true` on success; `false` when the id does not exist, the player is not online, or that player has not seen this entity yet
- Return type: `Boolean`
- Example:

    ```js
    const entitySetPlayerNametag = ll.import("HologramLib", "entitySetPlayerNametag");
    entitySetPlayerNametag(id, "Steve", "§cOnly Steve sees it like this");
    entitySetPlayerNametag(id, "Steve", "");   // restore
    ```

### Change the scale one player sees

`entitySetPlayerScale(id, playerName, scale)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `playerName` : `String`<br>the viewer's name; they must be online and must have already seen this entity
  - `scale` : `Float`<br>the scale that player sees, clamped to `0.0625 ~ 10`; `<= 0` = clear the override and go back to the shared scale
- Return value: `true` on success; `false` when the id does not exist, the player is not online, or that player has not seen this entity yet
- Return type: `Boolean`
- Example:

    ```js
    const entitySetPlayerScale = ll.import("HologramLib", "entitySetPlayerScale");
    entitySetPlayerScale(id, "Steve", 3);    // in Steve's eyes this entity is especially big
    entitySetPlayerScale(id, "Steve", 0);    // restore
    ```

### Change the equipment one player sees

`entitySetPlayerEquipmentSlot(id, playerName, slot, name, aux, nbt)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `playerName` : `String`<br>the viewer's name; they must be online and must have already seen this entity
  - `slot` : `Integer`<br>`0`=mainhand `1`=offhand `2`=head `3`=chest `4`=legs `5`=feet; any other value returns `false`
  - `name` : `String`<br>the item id; an empty string = this slot falls back to the shared equipment for that player
  - `aux` : `Integer`<br>the item's data value; what exactly it means depends on the item
  - `nbt` : `String`<br>extra data as SNBT text; an empty string = no extra data
- Return value: `true` on success; `false` when `slot` is out of range, the id does not exist, the player is not online, or that player has not seen this entity yet
- Return type: `Boolean`
- Example:

    ```js
    const entitySetPlayerEquipmentSlot = ll.import("HologramLib", "entitySetPlayerEquipmentSlot");
    entitySetPlayerEquipmentSlot(id, "Steve", 2, "diamond_helmet", 0, "");  // Steve sees a diamond helmet
    entitySetPlayerEquipmentSlot(id, "Steve", 2, "", 0, "");                // restore that slot
    ```

### Clear one player's appearance overrides

`entityClearPlayerAppearance(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the entity id
  - `playerName` : `String`<br>the viewer's name; they must be online
- Return value: `true` means the overrides really were cleared; returns `false` when that player had no overrides at all, the id does not exist, or the player is not online
- Return type: `Boolean`
- Example:

    ```js
    const entityClearPlayerAppearance = ll.import("HologramLib", "entityClearPlayerAppearance");
    entityClearPlayerAppearance(id, "Steve");   // in Steve's eyes it goes back to the shared configuration
    ```

## Custom entities · Queries & destruction

### Find the nearest custom entity

`entityFindNearest(x, y, z, dim, maxDist)`

- Parameters:
  - `x` : `Float`<br>the reference point's world coordinate X
  - `y` : `Float`<br>the reference point's world coordinate Y
  - `z` : `Float`<br>the reference point's world coordinate Z
  - `dim` : `Integer`<br>only entities in the same dimension are searched (a different dimension is skipped outright)
  - `maxDist` : `Float`<br>the maximum distance (blocks); `<= 0` = no limit
- Return value: the library id of the nearest entity; `-1` when no entity is in range
- Return type: `Integer`
- Example:

    ```js
    const entityFindNearest = ll.import("HologramLib", "entityFindNearest");
    const pos = player.pos;
    const near = entityFindNearest(pos.x, pos.y, pos.z, 0, 8);   // the nearest custom entity within 8 blocks
    if (near !== -1) entitySetNametag(near, "§eHello");
    ```

### Read the entity runtime summary

`entityGetInfo(id)`

- Parameters:
  - `id` : `Integer`<br>the entity id
- Return value: the summary text (a diagnostic probe: id, dimension, coordinates, type, switch, scale, pose, equipment count, view distance, whitelist, number of players who have seen it, riding); returns `"not_found"` when the id does not exist
- Return type: `String`
- Example:

    ```js
    const entityGetInfo = ll.import("HologramLib", "entityGetInfo");
    logger.info(entityGetInfo(id));   // e.g. id=1 dim=0 pos=(100.5,64.0,100.5) identifier='minecraft:cow' ...
    ```

### Check whether an entity exists

`entityExists(id)`

- Parameters:
  - `id` : `Integer`<br>the entity id
- Return value: `true` when it exists, `false` when it does not
- Return type: `Boolean`
- Example:

    ```js
    const entityExists = ll.import("HologramLib", "entityExists");
    if (entityExists(id)) entitySetNametag(id, "§aStill here");
    ```

### Check whether an id is taken

`entityIsIdUsed(id)`

It looks at the same record as `entityExists`, with a name that fits the "probe before you create" use better.

- Parameters:
  - `id` : `Integer`<br>the entity id
- Return value: `true` when it is taken, `false` when it is free
- Return type: `Boolean`
- Example:

    ```js
    const entityIsIdUsed = ll.import("HologramLib", "entityIsIdUsed");
    if (!entityIsIdUsed(1001)) {
      // 1001 is free, so you can claim it with entityCreateWithId
    }
    ```

### List every entity id

`entityGetAllIds()`

- Return value: the id array; an empty array when there are no entities at all
- Return type: `Array<Integer>`
- Example:

    ```js
    const entityGetAllIds = ll.import("HologramLib", "entityGetAllIds");
    for (const one of entityGetAllIds()) logger.info(entityGetInfo(one));
    ```

### Destroy one entity

`entityDestroy(id)`

- Parameters:
  - `id` : `Integer`<br>the entity id
- Return value: `true` when it really was destroyed; `false` when the id does not exist. The one on the client disappears along with it
- Return type: `Boolean`
- Example:

    ```js
    const entityDestroy = ll.import("HologramLib", "entityDestroy");
    entityDestroy(id);
    ```

### Destroy every custom entity

`entityDestroyAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const entityDestroyAll = ll.import("HologramLib", "entityDestroyAll");
    entityDestroyAll();   // tidy up when the server shuts down or the script reloads
    ```

## Interaction events (ghost*)

`ghost*` has only two functions, but they are used differently from the other domains; read these three points first:

1. **Events go into a queue; there is no callback.** When a player clicks a library protocol entity (a custom entity / an item display / a fake-player NPC), the library puts that interaction into a pending queue, and you have to call `ghostPollInteractions()` yourself, per tick or on a timer, to take it out; if you never take it, it just piles up in the queue.
2. **The events come from client interaction packets.** The library hooks the packet-receiving path, looks the target of each interaction packet from the client back up to its **library id**, and formats the result into a string for the queue. Only interactions that hit a **library protocol entity** enter this queue — a click on a real server-side entity (a vanilla mob, a player) never shows up here.
3. **The queue has a limit.** It holds at most 256 entries, and once it is full a new event pushes the oldest one out; not polling = losing events.

### Take and clear the interaction events

`ghostPollInteractions()`

- Return value: each entry is one event string; an empty array when the queue is empty
- Return type: `Array<String>`

  The format: `player=<playerName> action=<1..6> domain=<entity|itemDisplay|npc> id=<libraryId>`; when the client carries coordinates (`InteractUpdate` and the like) a `pos=(x,y,z)` is appended at the end.

  `action` values: `1`=Interact (right-click interaction) `2`=Attack (left-click attack) `3`=StopRiding `4`=InteractUpdate `5`=NpcOpen `6`=OpenInventory. Check `action` before handling, and do not assume there are only 1 and 2.

  `domain` says which kind was clicked: `entity` = a custom entity, `itemDisplay` = an FMBE item display, `npc` = a fake-player NPC.
- Example:

    ```js
    const ghostPollInteractions = ll.import("HologramLib", "ghostPollInteractions");
    setInterval(() => {
      for (const line of ghostPollInteractions()) {
        const m = line.match(/player=(\S+) action=(\d+) domain=(\S+) id=(-?\d+)/);
        if (!m) continue;
        const [, who, action, domain, targetId] = m;
        if (action === "1" && domain === "entity") logger.info(`${who} right-clicked entity ${targetId}`);
        if (action === "2" && domain === "entity") entityDestroy(Number(targetId));   // left-click = knock it out
      }
    }, 100);
    ```

### Discard unhandled interaction events

`ghostClearInteractions()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const ghostClearInteractions = ll.import("HologramLib", "ghostClearInteractions");
    // Clear it once when a player reconnects or the scene changes, so that you do not handle stale clicks
    ghostClearInteractions();
    ```

## Particle shapes · Creation

Particle shapes are defined in **world coordinates**; once created, the library emits and re-sends them on its own tick and you only control them. Seven shapes:

| Function | What it lays out |
|---|---|
| `particleCreatePoint` | a single point |
| `particleCreateLine` | a line between two points |
| `particleCreateRect` | a rectangular **outline** (a ring) |
| `particleCreatePlane` | a filled rectangular **face** (a mesh) |
| `particleCreateBox` | the 12-edge wireframe of a cuboid |
| `particleCreateBoxFaces` | all six faces of a cuboid, filled |
| `particleCreatePoly` | a custom polyhedron (you supply the vertices and edges) |

Several of the creation parameters are shared:

- `owner`: a creator name tag. **It is used only for the diagnostic display of `particleGetInfo` and has no effect on who can see the shape** — visibility is decided by `particleSetVisible`.
- `dimId`: the dimension, `0` Overworld / `1` Nether / `2` End.
- `step`: the sampling step (blocks), which is roughly how far apart two neighbouring particles are; anything below `0.25` is treated as `0.25`. The smaller the step, the denser the particles and the more packets are sent.
- `effect`: the particle effect name, such as `minecraft:endrod` or `minecraft:heart_particle`; an empty string uses the default `minecraft:endrod`.
- `interval`: the whole-batch re-send interval (ticks). Particles are transient by nature, so re-sending at this interval is what keeps the "resident" look; anything below `1` is treated as `1`.
- `lifetime`: how long it lives (ticks); `0` = permanent, and the shape disappears by itself when the time is up.

### Lay out a particle point

`particleCreatePoint(owner, dimId, x, y, z, effect, interval, lifetime)`

- Parameters:
  - `owner` : `String`<br>see the shared notes above
  - `dimId` : `Integer`<br>see the shared notes above
  - `effect` : `String`<br>see the shared notes above
  - `interval` : `Integer`<br>see the shared notes above
  - `lifetime` : `Integer`<br>see the shared notes above
  - `x` : `Float`<br>the world coordinate X the particles sit at
  - `y` : `Float`<br>the world coordinate Y the particles sit at
  - `z` : `Float`<br>the world coordinate Z the particles sit at
- Return value: the shape id (incrementing from 1); a negative number on failure
- Return type: `Integer`
- Example:

    ```js
    const particleCreatePoint = ll.import("HologramLib", "particleCreatePoint");
    const pid = particleCreatePoint("demo", 0, 100.5, 64.5, 100.5, "minecraft:endrod", 4, 0);
    ```

### Lay out a particle line

`particleCreateLine(owner, dimId, x1, y1, z1, x2, y2, z2, step, effect, interval, lifetime)`

- Parameters:
  - `owner` : `String`<br>see the shared notes above
  - `dimId` : `Integer`<br>see the shared notes above
  - `step` : `Float`<br>see the shared notes above
  - `effect` : `String`<br>see the shared notes above
  - `interval` : `Integer`<br>see the shared notes above
  - `lifetime` : `Integer`<br>see the shared notes above
  - `x1` : `Float`<br>the world coordinate X of the first endpoint
  - `y1` : `Float`<br>the world coordinate Y of the first endpoint
  - `z1` : `Float`<br>the world coordinate Z of the first endpoint
  - `x2` : `Float`<br>the world coordinate X of the second endpoint
  - `y2` : `Float`<br>the world coordinate Y of the second endpoint
  - `z2` : `Float`<br>the world coordinate Z of the second endpoint
- Return value: the shape id; a negative number on failure. The anchor is the line's **midpoint**: later rotation and scaling both happen about the midpoint
- Return type: `Integer`
- Example:

    ```js
    const particleCreateLine = ll.import("HologramLib", "particleCreateLine");
    const pid = particleCreateLine("demo", 0, 100, 64, 100, 110, 64, 100, 0.5, "minecraft:endrod", 4, 0);
    ```

### Lay out a particle rectangle outline

`particleCreateRect(owner, dimId, cx, cy, cz, w, h, axis, step, effect, interval, lifetime)`

- Parameters:
  - `owner` : `String`<br>see the shared notes above
  - `dimId` : `Integer`<br>see the shared notes above
  - `step` : `Float`<br>see the shared notes above
  - `effect` : `String`<br>see the shared notes above
  - `interval` : `Integer`<br>see the shared notes above
  - `lifetime` : `Integer`<br>see the shared notes above
  - `cx` : `Float`<br>the world coordinate X of the rectangle's centre
  - `cy` : `Float`<br>the world coordinate Y of the rectangle's centre
  - `cz` : `Float`<br>the world coordinate Z of the rectangle's centre
  - `w` : `Float`<br>the rectangle's width, its size along the plane's first axis; a negative number is treated as `0`
  - `h` : `Float`<br>the rectangle's height, its size along the plane's second axis; a negative number is treated as `0`
  - `axis` : `Integer`<br>the plane the rectangle lies in: `0`=XY (`w` along X, `h` along Y), `1`=YZ (`w` along Z, `h` along Y), `2`=XZ (`w` along X, `h` along Z); any other value is treated as `0`
- Return value: the shape id; a negative number on failure. The anchor is the rectangle's centre
- Return type: `Integer`
- Example:

    ```js
    const particleCreateRect = ll.import("HologramLib", "particleCreateRect");
    // An 8×8 halo lying in the XZ plane (axis=2), with 1.5 blocks between particles
    const pid = particleCreateRect("demo", 0, 100.5, 64.1, 100.5, 8, 8, 2, 1.5, "minecraft:endrod", 2, 0);
    ```

### Lay out a filled particle plane

`particleCreatePlane(owner, dimId, cx, cy, cz, w, h, axis, step, effect, interval, lifetime)`

The parameters are exactly the same as `particleCreateRect`.

- Parameters:
  - `owner` : `String`<br>see the shared notes above
  - `dimId` : `Integer`<br>see the shared notes above
  - `cx` : `Float`<br>the world coordinate X of the plane's centre
  - `cy` : `Float`<br>the world coordinate Y of the plane's centre
  - `cz` : `Float`<br>the world coordinate Z of the plane's centre
  - `w` : `Float`<br>the plane's size along its first axis; a negative number is treated as `0`
  - `h` : `Float`<br>the plane's size along its second axis; a negative number is treated as `0`
  - `axis` : `Integer`<br>the plane's orientation, `0`=XY `1`=YZ `2`=XZ; any other value is treated as `0`
  - `step` : `Float`<br>see the shared notes above. With face shapes, turn `step` down carefully — the particle count grows with the area
  - `effect` : `String`<br>see the shared notes above
  - `interval` : `Integer`<br>see the shared notes above
  - `lifetime` : `Integer`<br>see the shared notes above
- Return value: the shape id; a negative number on failure. The anchor is the plane's centre
- Return type: `Integer`
- Example:

    ```js
    const particleCreatePlane = ll.import("HologramLib", "particleCreatePlane");
    // A particle wall standing in the XY plane (axis=0), re-sent every 10 ticks
    const pid = particleCreatePlane("demo", 0, 100, 64, -200, 12, 6, 0, 3, "minecraft:heart_particle", 10, 0);
    ```

### Lay out a particle box wireframe

`particleCreateBox(owner, dimId, cx, cy, cz, hx, hy, hz, step, effect, interval, lifetime)`

- Parameters:
  - `owner` : `String`<br>see the shared notes above
  - `dimId` : `Integer`<br>see the shared notes above
  - `step` : `Float`<br>see the shared notes above
  - `effect` : `String`<br>see the shared notes above
  - `interval` : `Integer`<br>see the shared notes above
  - `lifetime` : `Integer`<br>see the shared notes above
  - `cx` : `Float`<br>the world coordinate X of the cuboid's centre
  - `cy` : `Float`<br>the world coordinate Y of the cuboid's centre
  - `cz` : `Float`<br>the world coordinate Z of the cuboid's centre
  - `hx` : `Float`<br>the **half size** in the X direction (in blocks), i.e. half the edge length; a negative number is treated as `0`
  - `hy` : `Float`<br>the **half size** in the Y direction (in blocks), i.e. half the edge length; a negative number is treated as `0`
  - `hz` : `Float`<br>the **half size** in the Z direction (in blocks), i.e. half the edge length; a negative number is treated as `0`
- Return value: the shape id; a negative number on failure. The anchor is the cuboid's centre
- Return type: `Integer`
- Example:

    ```js
    const particleCreateBox = ll.import("HologramLib", "particleCreateBox");
    // A cube wireframe with an edge length of 6 (half size 3)
    const pid = particleCreateBox("demo", 0, 100.5, 64.5, 100.5, 3, 3, 3, 1, "minecraft:endrod", 2, 0);
    ```

### Lay out a particle box with all six faces filled

`particleCreateBoxFaces(owner, dimId, cx, cy, cz, hx, hy, hz, step, effect, interval, lifetime)`

The parameters are exactly the same as `particleCreateBox`.

- Parameters:
  - `owner` : `String`<br>see the shared notes above
  - `dimId` : `Integer`<br>see the shared notes above
  - `cx` : `Float`<br>the world coordinate X of the cuboid's centre
  - `cy` : `Float`<br>the world coordinate Y of the cuboid's centre
  - `cz` : `Float`<br>the world coordinate Z of the cuboid's centre
  - `hx` : `Float`<br>the half size in the X direction; a negative number is treated as `0`
  - `hy` : `Float`<br>the half size in the Y direction; a negative number is treated as `0`
  - `hz` : `Float`<br>the half size in the Z direction; a negative number is treated as `0`
  - `step` : `Float`<br>see the shared notes above. All six faces are filled, so the particle count is many times that of the wireframe
  - `effect` : `String`<br>see the shared notes above
  - `interval` : `Integer`<br>see the shared notes above
  - `lifetime` : `Integer`<br>see the shared notes above
- Return value: the shape id; a negative number on failure. The anchor is the cuboid's centre
- Return type: `Integer`
- Example:

    ```js
    const particleCreateBoxFaces = ll.import("HologramLib", "particleCreateBoxFaces");
    const pid = particleCreateBoxFaces("demo", 0, 100.5, 64.5, 100.5, 2, 2, 2, 1, "minecraft:endrod", 4, 0);
    ```

### Lay out a custom polyhedron

`particleCreatePoly(owner, dimId, verts, edges, step, effect, interval, lifetime)`

- Parameters:
  - `owner` : `String`<br>see the shared notes above
  - `dimId` : `Integer`<br>see the shared notes above
  - `step` : `Float`<br>see the shared notes above
  - `effect` : `String`<br>see the shared notes above
  - `interval` : `Integer`<br>see the shared notes above
  - `lifetime` : `Integer`<br>see the shared notes above
  - `verts` : `String`<br>the vertex string, in world coordinates, in the format `"x,y,z;x,y,z;..."`, with at least 2 vertices
  - `edges` : `String`<br>the edge string, pairs of vertex indices, in the format `"i-j;i-j"`, with indices starting at `0` and having to fall inside the vertex range
- Return value: the shape id; returns `-1` when there are fewer than 2 vertices, not one usable edge in the edge string, or an index is out of range. The anchor is the **centroid** of all the vertices, and rotation and scaling happen about the centroid
- Return type: `Integer`
- Example:

    ```js
    const particleCreatePoly = ll.import("HologramLib", "particleCreatePoly");
    // A tetrahedron: 4 vertices, 6 edges
    const pid = particleCreatePoly(
      "demo", 0,
      "100,64,100;102,64,100;101,64,102;101,66,101",
      "0-1;1-2;2-0;0-3;1-3;2-3",
      1, "minecraft:endrod", 4, 0
    );
    ```

## Particle shapes · Movement & transforms

Every transform happens about the shape's **anchor** (line = midpoint, rectangle / plane / cuboid = centre, polyhedron = centroid).

### Teleport the particle shape

`particleSetPos(id, x, y, z)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `x` : `Float`<br>the new world coordinate X of the shape's anchor
  - `y` : `Float`<br>the new world coordinate Y of the shape's anchor
  - `z` : `Float`<br>the new world coordinate Z of the shape's anchor
- Return value: `true` on success; `false` when the id does not exist. On the next emission the whole batch of particles appears at the new position
- Return type: `Boolean`
- Example:

    ```js
    const particleSetPos = ll.import("HologramLib", "particleSetPos");
    particleSetPos(pid, 200.5, 70, 100.5);
    ```

### Move the shape by an offset

`particleMoveBy(id, dx, dy, dz)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `dx` : `Float`<br>the offset in the X direction (blocks)
  - `dy` : `Float`<br>the offset in the Y direction (blocks)
  - `dz` : `Float`<br>the offset in the Z direction (blocks)
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const particleMoveBy = ll.import("HologramLib", "particleMoveBy");
    particleMoveBy(pid, 0, 0.5, 0);   // raise it half a block
    ```

### Slide the shape smoothly to a target point

`particleMoveTo(id, x, y, z, durationTicks)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `x` : `Float`<br>the target world coordinate X of the anchor
  - `y` : `Float`<br>the target world coordinate Y of the anchor
  - `z` : `Float`<br>the target world coordinate Z of the anchor
  - `durationTicks` : `Integer`<br>the slide duration (ticks); `<= 0` = arrive at once (equivalent to `particleSetPos`)
- Return value: `true` means it has started moving; `false` when the id does not exist. A whole face, or the whole shape, moves along with it
- Return type: `Boolean`
- Example:

    ```js
    const particleMoveTo = ll.import("HologramLib", "particleMoveTo");
    particleMoveTo(pid, 150, 64, -200, 100);   // slide to the target point over 5 seconds
    ```

### Set the shape's three-axis Euler angles

`particleSetRot(id, rx, ry, rz)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `rx` : `Float`<br>the rotation angle about the X axis (degrees), rotating about the anchor
  - `ry` : `Float`<br>the rotation angle about the Y axis (degrees), rotating about the anchor
  - `rz` : `Float`<br>the rotation angle about the Z axis (degrees), rotating about the anchor
- Return value: `true` on success; `false` when the id does not exist. It sets absolute angles, so it overwrites whatever angle `particleSpin` has accumulated
- Return type: `Boolean`
- Example:

    ```js
    const particleSetRot = ll.import("HologramLib", "particleSetRot");
    particleSetRot(pid, 0, 45, 0);   // rotate 45 degrees about the Y axis
    ```

### Make the shape spin continuously

`particleSpin(id, sx, sy, sz)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `sx` : `Float`<br>the spin rate about the X axis (degrees/tick); `0,0,0` = stop spinning
  - `sy` : `Float`<br>the spin rate about the Y axis (degrees/tick); `0,0,0` = stop spinning
  - `sz` : `Float`<br>the spin rate about the Z axis (degrees/tick); `0,0,0` = stop spinning
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const particleSpin = ll.import("HologramLib", "particleSpin");
    particleSpin(pid, 0, 3, 0);    // rotate 3 degrees about Y every tick
    particleSpin(pid, 0, 0, 0);    // stop
    ```

### Scale the shape about its anchor

`particleSetScale(id, s)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `s` : `Float`<br>the scale factor; a value of `0.01` or less is treated as `0.01`
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const particleSetScale = ll.import("HologramLib", "particleSetScale");
    particleSetScale(pid, 2);
    particleSetScale(pid, 1);   // restore
    ```

### Make the shape follow a player

`particleFollow(id, uuid, offX, offY, offZ)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `uuid` : `String`<br>the **uuid string** of the player to follow (not the player name); a string that does not parse into a uuid is refused outright
  - `offX` : `Float`<br>the offset relative to the player in the X direction (blocks); a halo at the player's feet, for instance, uses `0, 0.3, 0`
  - `offY` : `Float`<br>the offset relative to the player in the Y direction (blocks); a halo at the player's feet, for instance, uses `0, 0.3, 0`
  - `offZ` : `Float`<br>the offset relative to the player in the Z direction (blocks); a halo at the player's feet, for instance, uses `0, 0.3, 0`
- Return value: `true` on success; `false` when `uuid` is not a valid uuid or the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const particleFollow = ll.import("HologramLib", "particleFollow");
    particleFollow(pid, player.uuid, 0, 0.3, 0);   // shape anchor = the player's position + the offset
    ```

### Release the follow

`particleUnfollow(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` on success; `false` when the id does not exist. After releasing, the anchor goes back to the coordinates the shape recorded for itself (following never overwrote them), so the shape jumps back to where it was created or where the last `particleSetPos` put it
- Return type: `Boolean`
- Example:

    ```js
    const particleUnfollow = ll.import("HologramLib", "particleUnfollow");
    particleUnfollow(pid);
    particleSetPos(pid, 100.5, 64.5, 100.5);   // set the position once more yourself if you want it to stay put
    ```

## Particle shapes · Appearance & visibility

### Swap the particle effect

`particleSetEffect(id, effect)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `effect` : `String`<br>the new particle effect name, such as `minecraft:flame`; an empty string is refused
- Return value: `true` on success; `false` when `effect` is empty or the id does not exist. It takes effect from the next emission
- Return type: `Boolean`
- Example:

    ```js
    const particleSetEffect = ll.import("HologramLib", "particleSetEffect");
    particleSetEffect(pid, "minecraft:flame");
    ```

### Restrict who can see the shape

`particleSetVisible(id, playersCsv)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `playersCsv` : `String`<br>a comma-separated string of player uuids, such as `"uuid1,uuid2"`; an empty string = everyone in the shape's dimension can see it. Entries that do not parse into a uuid are dropped
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const particleSetVisible = ll.import("HologramLib", "particleSetVisible");
    particleSetVisible(pid, player.uuid);        // only this player can see it
    particleSetVisible(pid, `${player.uuid},${other.uuid}`);   // two players
    particleSetVisible(pid, "");                 // back to visible to everyone in the dimension
    ```

### Change the particle re-send interval

`particleSetInterval(id, ticks)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `ticks` : `Integer`<br>the new re-send interval (ticks); anything below `1` is treated as `1`
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const particleSetInterval = ll.import("HologramLib", "particleSetInterval");
    particleSetInterval(pid, 2);    // more continuous, but the packets come denser
    particleSetInterval(pid, 10);   // save a little bandwidth
    ```

### Set the per-player culling radius

`particleSetViewDistance(id, blocks)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `blocks` : `Integer`<br>the culling radius (blocks); `0` = no culling (a negative value is treated as `0`)
- Return value: `true` on success; `false` when the id does not exist. The defaults at creation time: a point shape is `0` (no culling) and the other shapes are `96` blocks
- Return type: `Boolean`
- Example:

    ```js
    const particleSetViewDistance = ll.import("HologramLib", "particleSetViewDistance");
    particleSetViewDistance(pid, 48);   // players more than 48 blocks away get nothing
    particleSetViewDistance(pid, 0);    // no limit
    ```

### Reset the particle lifetime

`particleSetLifetime(id, ticks)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `ticks` : `Integer`<br>how many more ticks it can live from now on; `0` = make it permanent
- Return value: `true` on success; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const particleSetLifetime = ll.import("HologramLib", "particleSetLifetime");
    particleSetLifetime(pid, 200);   // it stays for another 10 seconds
    particleSetLifetime(pid, 0);     // make it permanent
    ```

## Particle shapes · Queries & destruction

### Destroy a particle shape at once

`particleDestroy(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` when it really was destroyed; `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const particleDestroy = ll.import("HologramLib", "particleDestroy");
    particleDestroy(pid);
    ```

### Read the particle shape summary

`particleGetInfo(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: the summary text (a diagnostic probe: shape type, `owner`, dimension, anchor, rotation, spin, scale, particle-point count, `step`, effect name, visible range, view distance, re-send interval, end time, whether it is moving smoothly); when the id does not exist it returns an **empty string** `""`
- Return type: `String`
- Example:

    ```js
    const particleGetInfo = ll.import("HologramLib", "particleGetInfo");
    logger.info(particleGetInfo(pid));   // e.g. particle shape #1: kind=box owner=demo dim=0 anchor=(100.50,64.50,100.50) ...
    ```

## Notes

1. **`ghost*` must be polled.** Interaction events do not go into a callback, they only sit in the queue; if you never call `ghostPollInteractions()` you will never receive them. The queue holds at most 256 entries, and once it is full the oldest is dropped.
2. **Only interactions that hit a library protocol entity enter the queue.** Clicks on a custom entity, an item display or a fake-player NPC carry a `domain` and a library `id`; a click on a real server-side entity (a vanilla mob, a player) never appears in the return value of `ghostPollInteractions()`.
3. **A custom entity does not "appear the moment it is created".** Who can see it is recalculated by the library on a timer, so a newly created entity shows up on the client within about 1 second at the latest; a property change triggers a recalculation along the way, which is usually faster.
4. **Protocol entities take no part in server-side logic.** They have no AI, no collision and nothing written to the save: hitting one does not cost health, walking through one does not bump into it, and they all vanish after a restart — if you need persistence, save the ids yourself and restore them with `entityCreateWithId`.
5. **Visibility is the intersection of four conditions.** Only a player who satisfies dimension + view distance + whitelist + the `entitySetEnabled` switch sees the entity; the whitelist does not "append", and `entitySetVisiblePlayer` replaces the whole list.
6. **By default a nametag only shows when the crosshair is on it.** There is no function on the LSE side to switch "always shown" on; if you want it visible all the time like holographic text, hang a separate line of text on it with the `holo*` domain.
7. **Per-client orientation has no LSE export.** "Each player sees a different orientation" (the `setPlayerRotation` family) exists only on the C++ interface; the per-client functions on this page cover nametags, scale and equipment only.
8. **`entityPlayAnimation` sends one packet and does not persist.** A player who only sees this entity later does not get the animation re-sent automatically; calling it while `entitySetEnabled` has the entity switched off returns `false` outright.
9. **Particle shapes have no "destroy all" or "list every id" export.** Your script has to keep the ids returned at creation itself, and `particleDestroy` them one by one when tidying up.
10. **The particle whitelist works by uuid and stacks with the dimension.** When not one valid player on the whitelist is in the shape's dimension, the shape is removed automatically (it does not stay around waiting for someone); a shape whose dimension is merely empty is kept, spinning idle.
11. **Particles are a transient visual, kept alive by re-sending.** `interval` decides both the look and the packet volume: a larger interval saves more bandwidth and makes the "batch by batch" flicker easier to see; a smaller `step` means denser particles, and with face shapes the particle count deserves particular care.
12. **Purely visual, with no change to server-side state.** These capabilities appear on the client only; the server's real world, save and logic are completely unaffected.
