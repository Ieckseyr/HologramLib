# Fake-player NPCs · building a re-skinnable, clickable fake player on the server

`playerNpc*` creates a **pure-protocol fake player**: the client sees a complete player model with a nametag, and it shows up in the Tab list, but the server **has no such player** — no AI, no place in the entity system, nothing written to the save. It suits characters that simply stand there: shopkeepers, quest givers, statues, human-shaped standees.

A skin gets in four ways: registering a PNG file, capturing it from an online player, importing a directory in bulk, or restoring a base64 blob. **A skin must be registered before you create an NPC**, otherwise creation fails. The library writes nothing to disk: after a restart you register the skins again yourself and rebuild the NPCs from your own save data.

Clicking this fake player does not go into a callback; fetch it by polling `ghost*` (with `domain=npc` in the entry).

## Quick start

```js
const registerSkin = ll.import("HologramLib", "playerNpcRegisterSkin");
const create       = ll.import("HologramLib", "playerNpcCreate");
const setNametag   = ll.import("HologramLib", "playerNpcSetNametag");

// 1. Register a PNG skin (pick the skinId yourself; every later call refers to it)
registerSkin("E:/server/skins/girl.png", "girl", "", "slim");

// 2. Create a fake player in the Overworld wearing that skin
const npc = create(100.5, 64, 100.5, 0, "§eMia", "girl");

// 3. Change the nametag (supports § colour codes)
setNametag(npc, "§eMia §7<Shopkeeper>");
```

`npc` is this fake player's id, and every later `playerNpc*` function uses it.

## The skin registry

### Register a PNG texture as a skin

`playerNpcRegisterSkin(pngPath, skinId, geometry, armSize)`

- Parameters:
  - `pngPath` : `String`<br>the PNG file path (an absolute path is recommended). The image must be **square**, with a side length that is a power of two and no smaller than 64 (64 / 128 / 256 / 512 / 1024)
  - `skinId` : `String`<br>the name you give this skin; an empty string = use the file name (without the extension)
  - `geometry` : `String`<br>the geometry model name; an empty string = use the standard player model `geometry.humanoid.custom`
  - `armSize` : `String`<br>arm thickness, `"wide"` (thick, the default) or `"slim"` (thin)
- Return value: `true` = registered; returns `false` when the file cannot be opened or the size is not allowed. Registering the same `skinId` again overwrites the old skin
- Return type: `Boolean`
- Example:

    ```js
    const reg = ll.import("HologramLib", "playerNpcRegisterSkin");
    reg("E:/server/skins/girl.png", "girl", "", "slim");   // slim = thin arms
    reg("E:/server/skins/guard.png", "", "", "");          // empty skinId → the file name "guard"
    ```

### Capture an online player's skin

`playerNpcCaptureSkin(skinId, playerName)`

- Parameters:
  - `skinId` : `String`<br>the registration name; it cannot be empty
  - `playerName` : `String`<br>the online player's name (`realName`)
- Return value: what is captured is a **complete snapshot of that moment**, so a later re-skin by that player does not affect it; returns `false` when `skinId` is empty or the player is not online. Capturing under the same `skinId` again overwrites
- Return type: `Boolean`
- Example:

    ```js
    const capture = ll.import("HologramLib", "playerNpcCaptureSkin");
    const create  = ll.import("HologramLib", "playerNpcCreate");
    capture("steve_skin", "Steve");
    const npc = create(100.5, 64, 100.5, 0, "Steve's Clone", "steve_skin");
    ```

### Import the skins in a directory in bulk

`playerNpcImportSkins(dirPath)`

- Parameters:
  - `dirPath` : `String`<br>the directory path. The name of each subfolder is the `skinId`, and the folder must hold one `.png` texture (required) and one `.json` geometry model (optional; the standard player model is used when it is missing)
- Return value: how many were imported successfully; returns `-1` when `dirPath` is not a directory; a subfolder with no PNG in it is skipped and does not count
- Return type: `Integer`
- Example:

    ```js
    const importSkins = ll.import("HologramLib", "playerNpcImportSkins");
    // E:/server/skins/girl/girl.png → the skinId is "girl"
    const n = importSkins("E:/server/skins");
    logger.info(`imported ${n} skins`);
    ```

### Check whether a skin is registered

`playerNpcHasSkin(skinId)`

- Parameters:
  - `skinId` : `String`<br>the skin name
- Return value: `true` = registered
- Return type: `Boolean`
- Example:

    ```js
    const hasSkin = ll.import("HologramLib", "playerNpcHasSkin");
    if (!hasSkin("girl")) registerSkin("E:/server/skins/girl.png", "girl", "", "");
    ```

### List every registered skin name

`playerNpcGetSkinIds()`

- Return value: every `skinId`; an empty array when there are none
- Return type: `Array<String>`
- Example:

    ```js
    const getSkinIds = ll.import("HologramLib", "playerNpcGetSkinIds");
    logger.info(`registered skins: ${getSkinIds().join(", ")}`);
    ```

### Unregister a registered skin

`playerNpcUnregisterSkin(skinId)`

- Parameters:
  - `skinId` : `String`<br>the skin name to unregister
- Return value: `true` = unregistered; **it is refused while an NPC still uses this skin** (it returns `false`; destroy those NPCs or re-skin them first), and a skin that never existed returns `false` as well
- Return type: `Boolean`
- Example:

    ```js
    const unregisterSkin = ll.import("HologramLib", "playerNpcUnregisterSkin");
    unregisterSkin("girl");   // still used by an NPC → false
    ```

### Export a skin as a base64 string

`playerNpcGetSkinBlobB64(skinId)`

- Parameters:
  - `skinId` : `String`<br>the skin name
- Return value: the base64 text of this skin's complete snapshot (you can write it straight into a file or a database); returns the empty string `""` when the skin is not registered
- Return type: `String`
- Example:

    ```js
    const getBlob = ll.import("HologramLib", "playerNpcGetSkinBlobB64");
    const b64 = getBlob("girl");   // save it in your own config file; "" when not registered
    logger.info(`skin blob length ${b64.length}`);
    ```

### Restore a skin from a base64 string

`playerNpcRegisterSkinFromBlobB64(b64)`

- Parameters:
  - `b64` : `String`<br>the string exported by `playerNpcGetSkinBlobB64`
- Return value: `true` = registered (the skin keeps the name it had when it was exported); returns `false` when the string cannot be decoded, is empty or is malformed
- Return type: `Boolean`
- Example:

    ```js
    const regBlob = ll.import("HologramLib", "playerNpcRegisterSkinFromBlobB64");
    // On server start, read the string you saved last time back in and register it
    const b64 = "...the string you saved last time...";
    if (!regBlob(b64)) logger.warn("skin restore failed");
    ```

### Send a skin to online players to re-skin them

`playerNpcInjectSkin(viewerName, targetName, skinId)`

- Parameters:
  - `viewerName` : `String`<br>who gets to see it; **an empty string = every online player, the re-skinned player included** (this is the proper route for "let players see themselves re-skinned")
  - `targetName` : `String`<br>the name of the online player being re-skinned (`realName`)
  - `skinId` : `String`<br>a registered skin name
- Return value: `true` = sent; returns `false` when the target is offline, when `skinId` is not registered, or when the viewer named by `viewerName` is offline. **It does not resend automatically**: if the client drops one now and then, call it once more after a moment
- Return type: `Boolean`
- Example:

    ```js
    const inject = ll.import("HologramLib", "playerNpcInjectSkin");
    inject("", "Alex", "girl");        // every online player (Alex included) sees Alex re-skinned with "girl"
    inject("Steve", "Alex", "girl");   // only Steve sees it
    ```

### Send a skin to every online player

`playerNpcInjectSkinAll(targetName, skinId)`

- Parameters:
  - `targetName` : `String`<br>the name of the online player being re-skinned
  - `skinId` : `String`<br>a registered skin name
- Return value: equivalent to `playerNpcInjectSkin` with an empty `viewerName`, and it fails for the same reasons
- Return type: `Boolean`
- Example:

    ```js
    const injectAll = ll.import("HologramLib", "playerNpcInjectSkinAll");
    injectAll("Alex", "girl");
    ```

## Creation & destruction

### Create a fake player at a coordinate

`playerNpcCreate(x, y, z, dim, name, skinId)`

- Parameters:
  - `x` : `Float`<br>world coordinate X
  - `y` : `Float`<br>world coordinate Y
  - `z` : `Float`<br>world coordinate Z
  - `dim` : `Integer`<br>the dimension id, `0` / `1` / `2`
  - `name` : `String`<br>the display name, which is also the name above its head and in the Tab list; an empty string = use the default name `NPC`
  - `skinId` : `String`<br>a registered skin name; an empty string = use the default `default` (it fails when that one was never registered)
- Return value: the fake-player id; returns a negative number on failure (`-3` = the skin is not registered)
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "playerNpcCreate");
    const npc = create(100.5, 64, 100.5, 0, "§eMia", "girl");
    if (npc < 0) logger.warn("creation failed — is the skin registered?");
    ```

### Create a fake player with a random id

`playerNpcCreateRandom(x, y, z, dim, name, skinId)`

- Parameters: the same as `playerNpcCreate`.
- Return value: the fake-player id from the random range (starting at `0x10000000`), which never collides with an incrementing id; returns a negative number on failure
- Return type: `Integer`
- Example:

    ```js
    const createRandom = ll.import("HologramLib", "playerNpcCreateRandom");
    const npc = createRandom(100.5, 64, 100.5, 0, "§eMia", "girl");
    ```

### Create under a chosen id to restore an NPC

`playerNpcCreateWithId(x, y, z, dim, name, skinId, desiredId)`

- Parameters:
  - The first 6 parameters: the same as `playerNpcCreate`.
  - `desiredId` : `Integer`<br>the id you want; it must be greater than `0` and must not be taken right now
- Return value: on success it is exactly `desiredId`; `desiredId <= 0`, or an id that is already taken, returns `-2`; an unregistered skin returns `-3`
- Return type: `Integer`
- Example:

    ```js
    const createWithId = ll.import("HologramLib", "playerNpcCreateWithId");
    const npc = createWithId(100.5, 64, 100.5, 0, "§eMia", "girl", 7);   // restore the old id 7
    ```

### Delete one fake player

`playerNpcDestroy(id)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
- Return value: `true` = deleted (players who could already see it watch it disappear); returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const destroy = ll.import("HologramLib", "playerNpcDestroy");
    destroy(npc);
    ```

### Delete every fake player

`playerNpcDestroyAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const destroyAll = ll.import("HologramLib", "playerNpcDestroyAll");
    destroyAll();   // clear everything
    ```

## Properties

### Move the fake player and optionally switch dimension

`playerNpcSetPos(id, x, y, z, dim)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `x` : `Float`<br>the new world coordinate X
  - `y` : `Float`<br>the new world coordinate Y
  - `z` : `Float`<br>the new world coordinate Z
  - `dim` : `Integer`<br>the target dimension; a negative number = change the coordinates only and keep the current dimension
- Return value: `true` = changed; returns `false` when the id does not exist. Changes merge into a single resend at the end of the tick, so it never flickers repeatedly
- Return type: `Boolean`
- Example:

    ```js
    const setPos = ll.import("HologramLib", "playerNpcSetPos");
    setPos(npc, 105.5, 64, 100.5, -1);   // dim = -1: move the coordinates only, keep the dimension
    setPos(npc, 105.5, 64, 100.5, 1);    // switch to the Nether
    ```

### Set the fake player's facing

`playerNpcSetRotation(id, yaw)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `yaw` : `Float`<br>the facing angle in degrees
- Return value: `true` = changed; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setRotation = ll.import("HologramLib", "playerNpcSetRotation");
    setRotation(npc, 90);   // face the +X direction
    ```

### Change the nametag above the fake player's head

`playerNpcSetNametag(id, text)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `text` : `String`<br>the nametag text, supporting `§` colour codes; an empty string = clear the nametag. Setting the same text again does not trigger a resend
- Return value: `true` = changed; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setNametag = ll.import("HologramLib", "playerNpcSetNametag");
    setNametag(npc, "§eMia §7<Shopkeeper>");
    setNametag(npc, "");   // clear the nametag
    ```

### Give the fake player a different skin

`playerNpcSetSkin(id, skinId)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `skinId` : `String`<br>a registered skin name
- Return value: `true` = changed; returns `false` when the skin is not registered or the id does not exist. A re-skin makes players who could already see it watch it rebuild once
- Return type: `Boolean`
- Example:

    ```js
    const setSkin = ll.import("HologramLib", "playerNpcSetSkin");
    setSkin(npc, "guard");   // switch to the guard skin
    ```

### Scale the model and sync the hitbox

`playerNpcSetScale(id, scale)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `scale` : `Float`<br>the scaling factor; the client hard limit is `0.0625 ~ 10`
- Return value: `true` = set; returns `false` when it is below `0.0625` or above `10`, and when the id does not exist. Changes merge into a single resend at the end of the tick
- Return type: `Boolean`
- Example:

    ```js
    const setScale = ll.import("HologramLib", "playerNpcSetScale");
    setScale(npc, 1.5);     // 1.5x size
    setScale(npc, 0.5);     // half size
    ```

### Set the fake player's visible distance

`playerNpcSetViewDistance(id, dist)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `dist` : `Float`<br>the distance in blocks; `<= 0` = unlimited distance (the default is 96 blocks)
- Return value: `true` = set; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setView = ll.import("HologramLib", "playerNpcSetViewDistance");
    setView(npc, 32);   // visible within 32 blocks
    setView(npc, 0);    // unlimited distance
    ```

### Turn this fake player on or off

`playerNpcSetEnabled(id, enabled)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `enabled` : `Boolean`<br>`false` = players cannot see it (the id stays valid, so you can turn it back on at any time), `true` = show it again
- Return value: `true` = set; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setEnabled = ll.import("HologramLib", "playerNpcSetEnabled");
    setEnabled(npc, false);   // hide it first (closed for the night, say)
    setEnabled(npc, true);    // then open up again
    ```

## Visible-player whitelist

### Show it only to the players on the list

`playerNpcSetVisiblePlayers(id, players)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `players` : `Array<String>`<br>an array of player names, matched by `realName`; an empty array = clear the restriction and go back to visible to everyone
- Return value: `true` = set; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setVisiblePlayers = ll.import("HologramLib", "playerNpcSetVisiblePlayers");
    setVisiblePlayers(npc, ["Steve", "Alex"]);   // visible to those two only
    setVisiblePlayers(npc, []);                  // empty = visible to everyone
    ```

### Put a single player on the whitelist

`playerNpcSetVisiblePlayer(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
  - `playerName` : `String`<br>the only player who can see it (`realName`)
- Return value: `true` = set; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setVisiblePlayer = ll.import("HologramLib", "playerNpcSetVisiblePlayer");
    setVisiblePlayer(npc, "Steve");   // only Steve can see this NPC
    ```

### Clear the whitelist and go back to visible to everyone

`playerNpcClearVisiblePlayers(id)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
- Return value: `true` = cleared (it counts as success even when there was no restriction to begin with); returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const clearVisible = ll.import("HologramLib", "playerNpcClearVisiblePlayers");
    clearVisible(npc);   // everyone can see it again
    ```

## Queries

### Check whether the fake player still exists

`playerNpcExists(id)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
- Return value: `true` = it exists
- Return type: `Boolean`
- Example:

    ```js
    const exists = ll.import("HologramLib", "playerNpcExists");
    if (!exists(npc)) logger.warn("the NPC has already been destroyed");
    ```

### Check whether an id is already taken

`playerNpcIsIdUsed(id)`

- Parameters:
  - `id` : `Integer`<br>the id to check
- Return value: `true` = already taken (in which case `playerNpcCreateWithId` returns `-2`)
- Return type: `Boolean`
- Example:

    ```js
    const isUsed = ll.import("HologramLib", "playerNpcIsIdUsed");
    if (!isUsed(7)) createWithId(100.5, 64, 100.5, 0, "§eMia", "girl", 7);
    ```

### List the ids of all fake players

`playerNpcGetAllIds()`

- Return value: every fake-player id; an empty array when there are none
- Return type: `Array<Integer>`
- Example:

    ```js
    const getAllIds = ll.import("HologramLib", "playerNpcGetAllIds");
    for (const id of getAllIds()) logger.info(`NPC ${id}`);
    ```

### Get a runtime summary for troubleshooting

`playerNpcGetDebugInfo(id)`

- Parameters:
  - `id` : `Integer`<br>the fake-player id
- Return value: the summary text, containing the id, name, skin, coordinates, dimension, facing, view distance, on/off state, the number of players that have seen it and the state of the visible list; returns `not_found` when the id does not exist
- Return type: `String`
- Example:

    ```js
    const getInfo = ll.import("HologramLib", "playerNpcGetDebugInfo");
    logger.info(getInfo(npc));
    ```

## Things to know

- **Register the skin before building the character**: when a `skinId` was never registered, creation fails (`-3`). If a skin is unregistered or goes missing while the server runs, this NPC will simply never spawn, and there is no error on the script side.
- **The library writes nothing to disk**: both skins and NPCs live in memory only. After a server or script restart you register the skins again (or save them with `playerNpcGetSkinBlobB64` / `playerNpcRegisterSkinFromBlobB64` and restore them on restart), and rebuild the NPCs from your own saved records (`playerNpcCreateWithId` can restore the original id).
- **A fake player stays in the Tab list**: the library does not send a PlayerList removal entry (removing it while the skin entry is still alive crashes the client), so once a player has seen this NPC, its name stays in the Tab list until the NPC is destroyed.
- **At most 2 NPCs spawn per tick** (spawning several at once makes the client receive a burst of data and disconnect), so visibility can be up to one tick late; entering and leaving the view distance carries 4 blocks of hysteresis, so a skin is not re-sent over and over on the boundary.
- **Property changes merge**: changing position / facing / scale / skin in the same tick rebuilds only once (no flicker). Every rebuild does resend the skin, though, so high-frequency tracking movement does not belong in a script tick — the LSE side has no lightweight position channel, so that kind of requirement belongs on the C++ side with `setPositionLight`.
- **Clicking needs polling**: a fake player's interaction does not go into a callback; fetch it with `ghostPollInteractions()`, where the entry looks like `player=Steve action=2 domain=npc id=7` (`action=1` is a right-click interact, `action=2` an attack).
- **Some capabilities have no script interface**: `setRotationLight` / `setPositionLight` / `playAnimation` / `playAnimationTo` / `setEntitySpawnCallback` are C++-only for now, so `ll.import` returns `null` for them in a script; check first with `ll.hasExported("HologramLib", "...")` before deciding whether to use one.
- **The whitelist matches by name**: it uses player names (LSE's `realName`), not uuid; once a player renames, the old list stops matching.
