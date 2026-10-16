# Item display · one line of item name or a 3D item placed into the world

`itemDetail*` and `itemDisplay*` both make **the client** see a single item: the server gains no entity, and nothing is written to the save.

- `itemDetailShow` shows one line of **auto-translated item name** at a coordinate (for example "Diamond x64"). Behind it is a holographic text, and the id it returns can still be refined with `holo*`.
- `itemDisplay*` puts a **3D item / block model** at a coordinate: you can swap the item, rotate it on three axes, scale it and make it follow a player; once you give it a hit volume it can be clicked too (clicks are fetched back by polling `ghost*`).

Both domains are "create + show" in one step, so there is no separate show call to make. Reconnecting players are re-sent automatically by the library; after a script or server restart the displays are gone and you have to create them again.

## Quick start

```js
const itemDetailShow    = ll.import("HologramLib", "itemDetailShow");
const itemDisplayCreate = ll.import("HologramLib", "itemDisplayCreate");

// 1. One line of text: show "Diamond x64" in the Overworld at (100.5, 65, -200.5)
const tip = itemDetailShow(0, 100.5, 65, -200.5, "minecraft:diamond", 0, 64, "");

// 2. One 3D model: a diamond floating above the same coordinate
const box = itemDisplayCreate(100.5, 66, -200.5, 0, "minecraft:diamond", 0);
```

`tip` is a holographic-text id as well (pass it to the `holo*` family), and `box` is an item-display id (pass it to every `itemDisplay*` function below).

## itemDetail* — one line of auto-translated item name

### Show one line of auto-translated item name

`itemDetailShow(dimId, x, y, z, itemId, aux, count, customText)`

- Parameters:
  - `dimId` : `Integer`<br>the dimension to put it in — `0` Overworld / `1` Nether / `2` End
  - `x` : `Float`<br>world coordinate X
  - `y` : `Float`<br>world coordinate Y
  - `z` : `Float`<br>world coordinate Z
  - `itemId` : `String`<br>the item identifier, for example `minecraft:diamond`
  - `aux` : `Integer`<br>the item aux value (data value); use `0` when there is none
  - `count` : `Integer`<br>the count; only a count greater than `1` adds the ` xN` suffix
  - `customText` : `String`<br>custom text; an empty string = use the auto-translated item name, a non-empty string replaces it completely (supports `§` colour codes and `{variables}`)
- Return value: the holographic-text id (keep refining it with `holo*`); returns `-1` on failure (creation failed, or that dimension cannot be drawn)
- Return type: `Integer`
- Example:

    ```js
    const show = ll.import("HologramLib", "itemDetailShow");
    const tip  = show(0, 100.5, 65, -200.5, "minecraft:diamond", 0, 64, "");   // Diamond x64
    const ad   = show(0, 100.5, 66, -200.5, "minecraft:diamond", 0, 1, "§bToday's Special"); // your own text instead
    ```

### Delete the line of item name

`itemDetailHide(id)`

- Parameters:
  - `id` : `Integer`<br>the id returned by `itemDetailShow`
- Return value: `true` = deleted; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const hide = ll.import("HologramLib", "itemDetailHide");
    hide(tip);   // tip comes from "Quick start"
    ```

## itemDisplay creation

### Create a floating item model

`itemDisplayCreate(x, y, z, dim, itemId, aux)`

- Parameters:
  - `x` : `Float`<br>world coordinate X
  - `y` : `Float`<br>world coordinate Y
  - `z` : `Float`<br>world coordinate Z
  - `dim` : `Integer`<br>the dimension id, `0` / `1` / `2`
  - `itemId` : `String`<br>the item identifier, for example `minecraft:diamond`
  - `aux` : `Integer`<br>the item aux value (data value); use `0` when there is none
- Return value: the item-display id; returns a negative number on failure
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "itemDisplayCreate");
    // Put a diamond above the Overworld origin (the last argument is the aux value)
    const gem = create(0.5, 66, 0.5, 0, "minecraft:diamond", 0);
    ```

### Create with translation, rotation and scale preset

`itemDisplayCreateAdvanced(x, y, z, dim, itemId, aux, offX, offY, offZ, rotX, rotY, rotZ, scale)`

- Parameters:
  - `x` : `Float`<br>world coordinate X, the same as `itemDisplayCreate`
  - `y` : `Float`<br>world coordinate Y, the same as `itemDisplayCreate`
  - `z` : `Float`<br>world coordinate Z, the same as `itemDisplayCreate`
  - `dim` : `Integer`<br>the dimension id, the same as `itemDisplayCreate`
  - `itemId` : `String`<br>the item identifier, the same as `itemDisplayCreate`
  - `aux` : `Integer`<br>the item aux value (data value), the same as `itemDisplayCreate`
  - `offX` : `String`<br>translation in model units along X; write a constant number or a Molang expression string
  - `offY` : `String`<br>translation in model units along Y; write a constant number or a Molang expression string
  - `offZ` : `String`<br>translation in model units along Z; write a constant number or a Molang expression string
  - `rotX` : `String`<br>rotation around X (degrees); write a constant number or a Molang expression string
  - `rotY` : `String`<br>rotation around Y (degrees); write a constant number or a Molang expression string
  - `rotZ` : `String`<br>rotation around Z (degrees); write a constant number or a Molang expression string
  - `scale` : `String`<br>scaling; write a constant number or a Molang expression string
- Return value: the item-display id; returns a negative number on failure
- Return type: `Integer`
- Example:

    ```js
    const advanced = ll.import("HologramLib", "itemDisplayCreateAdvanced");
    // A diamond that spins forever: rotY is driven by Molang, so the server needs no per-tick packet
    const spin = advanced(0.5, 66, 0.5, 0, "minecraft:diamond", 0,
                          "0", "-4", "0", "180", "math.sin(query.life_time*90)*360", "180", "0.375");
    ```

### Create with mode, view distance and visible players preset

`itemDisplayCreateSeamless(x, y, z, dim, itemId, aux, offX, offY, offZ, rotX, rotY, rotZ, scale, mode, viewDistance, visiblePlayer)`

- Parameters:
  - The first 13 parameters: the same as `itemDisplayCreateAdvanced`.
  - `mode` : `Integer`<br>`-1` = use the default (decided automatically), `1` = item flat rendering, `2` = block solid rendering
  - `viewDistance` : `Float`<br>the visible distance (blocks); `-1` = use the default, `0` = unlimited distance
  - `visiblePlayer` : `String`<br>show it to this one player only; an empty string = visible to everyone
- Return value: the item-display id; returns a negative number on failure
- Return type: `Integer`
- Example:

    ```js
    const seamless = ll.import("HologramLib", "itemDisplayCreateSeamless");
    // A 3D diamond block only Steve can see, 30-block view distance
    const blk = seamless(100.5, 66, -200.5, 0, "minecraft:diamond_block", 0,
                         "0", "-4", "0", "180", "0", "180", "0.5", 2, 30, "Steve");
    ```

### Create with a random id

`itemDisplayCreateRandom(x, y, z, dim, itemId, aux)`

- Parameters: the same as `itemDisplayCreate`.
- Return value: the item-display id from the random range (starting at `0x10000000`), which never collides with an incrementing id; returns a negative number on failure
- Return type: `Integer`
- Example:

    ```js
    const createRandom = ll.import("HologramLib", "itemDisplayCreateRandom");
    const gem = createRandom(0.5, 66, 0.5, 0, "minecraft:diamond", 0);
    ```

### Create under a chosen id to restore a display

`itemDisplayCreateWithId(x, y, z, dim, itemId, aux, desiredId)`

- Parameters:
  - The first 6 parameters: the same as `itemDisplayCreate`.
  - `desiredId` : `Integer`<br>the id you want; it must be greater than `0` and must not be taken right now
- Return value: on success it is exactly `desiredId`; `desiredId <= 0`, or an id that is already taken, returns `-2`
- Return type: `Integer`
- Example:

    ```js
    const createWithId = ll.import("HologramLib", "itemDisplayCreateWithId");
    const id = createWithId(0.5, 66, 0.5, 0, "minecraft:diamond", 0, 42);   // 42 is already taken → -2
    ```

## itemDisplay properties

### Swap the item the display shows

`itemDisplaySetItem(id, itemId, aux)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `itemId` : `String`<br>the new item identifier
  - `aux` : `Integer`<br>the new item aux value (data value)
- Return value: `true` = changed; returns `false` when the id does not exist. Swapping the item clears any extra data set earlier
- Return type: `Boolean`
- Example:

    ```js
    const setItem = ll.import("HologramLib", "itemDisplaySetItem");
    setItem(box, "minecraft:emerald", 0);   // box comes from "Quick start"
    ```

### Swap the item together with extra data

`itemDisplaySetItemWithNbt(id, itemId, aux, nbt)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `itemId` : `String`<br>the new item identifier
  - `aux` : `Integer`<br>the new item aux value (data value)
  - `nbt` : `String`<br>an SNBT string; an empty string = clear the extra data. A parse failure is treated as "no extra data" and logged
- Return value: `true` = changed; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setNbt = ll.import("HologramLib", "itemDisplaySetItemWithNbt");
    setNbt(box, "minecraft:diamond_sword", 0, '{display:{Name:"§bTrial Sword"}}');
    setNbt(box, "minecraft:diamond_sword", 0, "");   // an empty string = drop the extra data
    ```

### Toggle the item's enchantment glint

`itemDisplaySetGlint(id, on)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `on` : `Boolean`<br>`true` = add the glint, `false` = remove it; setting the same value again does not trigger a resend
- Return value: `true` = set; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setGlint = ll.import("HologramLib", "itemDisplaySetGlint");
    setGlint(box, true);    // add the glint
    setGlint(box, false);   // remove it
    ```

### Move the display to new coordinates

`itemDisplaySetPosition(id, x, y, z, dim)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `x` : `Float`<br>the new world coordinate X
  - `y` : `Float`<br>the new world coordinate Y
  - `z` : `Float`<br>the new world coordinate Z
  - `dim` : `Integer`<br>the target dimension; a negative number = change the coordinates only and keep the current dimension
- Return value: `true` = changed; returns `false` when the id does not exist. Setting the position of a display that is following a player by hand releases the follow
- Return type: `Boolean`
- Example:

    ```js
    const setPos = ll.import("HologramLib", "itemDisplaySetPosition");
    setPos(box, 105.5, 66, -200.5, -1);   // dim = -1: move the coordinates only, keep the dimension
    setPos(box, 105.5, 66, -200.5, 1);    // switch to the Nether
    ```

### Adjust the item's translation inside the model

`itemDisplaySetOffset(id, ox, oy, oz)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `ox` : `String`<br>a constant number or a Molang expression string
  - `oy` : `String`<br>a constant number or a Molang expression string; the default Y in item mode is `"-4"`
  - `oz` : `String`<br>a constant number or a Molang expression string
- Return value: `true` = changed; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setOffset = ll.import("HologramLib", "itemDisplaySetOffset");
    setOffset(box, "0", "-2", "0");   // the default Y is "-4"; this sets a different translation
    ```

### Adjust the render-pixel-level base offset

`itemDisplaySetBaseOffset(id, ox, oy, oz)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `ox` : `String`<br>a constant number or a Molang expression string; the default is `"0"`
  - `oy` : `String`<br>a constant number or a Molang expression string; the default is `"0"`
  - `oz` : `String`<br>a constant number or a Molang expression string; the default is `"0"`
- Return value: `true` = changed; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setBase = ll.import("HologramLib", "itemDisplaySetBaseOffset");
    setBase(box, "0", "8", "0");   // all default to "0"; this stacks a base offset on top of the translation
    ```

### Set the three-axis rotation

`itemDisplaySetRotation(id, rx, ry, rz)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `rx` : `String`<br>a constant number or a Molang expression string (degrees); the item-mode default `rotX` is `"180"` (in item mode the library compensates the facing itself)
  - `ry` : `String`<br>a constant number or a Molang expression string (degrees); the item-mode default `rotY` is `"0"`
  - `rz` : `String`<br>a constant number or a Molang expression string (degrees); the item-mode default `rotZ` is `"180"`
- Return value: `true` = changed; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setRot = ll.import("HologramLib", "itemDisplaySetRotation");
    setRot(box, "180", "0", "180");   // back to the default item-mode pose
    setRot(box, "180", "45", "180");  // yaw 45 degrees
    ```

### Add a yaw rotation delta

`itemDisplayRotateY(id, delta)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `delta` : `Float`<br>the angle to add (degrees); positive or negative
- Return value: `true` = added; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const rotateY = ll.import("HologramLib", "itemDisplayRotateY");
    rotateY(box, 45);    // another 45 degrees on top of the current yaw
    rotateY(box, -45);   // turn it back
    ```

### Set the display's scale

`itemDisplaySetScale(id, scale)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `scale` : `String`<br>a constant number or a Molang expression string; the item-mode default is `"0.375"`, and block mode suggests `"0.5"`
- Return value: `true` = changed; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setScale = ll.import("HologramLib", "itemDisplaySetScale");
    setScale(box, "0.6");
    setScale(box, "0.5");   // the scale block mode usually wants
    ```

### Scale the display up or down by a factor

`itemDisplayScaleBy(id, factor)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `factor` : `Float`<br>greater than `1` enlarges, `0 < factor < 1` shrinks; `<= 0` is refused outright
- Return value: `true` = changed; returns `false` when `factor <= 0`, when the id does not exist, or when the multiplication leaves the result no greater than `0`
- Return type: `Boolean`
- Example:

    ```js
    const scaleBy = ll.import("HologramLib", "itemDisplayScaleBy");
    scaleBy(box, 1.5);        // 1.5x bigger
    scaleBy(box, 1 / 1.5);    // shrink back again
    ```

### Jump straight to the target scale

`itemDisplayScaleTo(id, targetScale)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `targetScale` : `Float`<br>the target scale; below `0.01` (or not a finite number) it is clamped to `0.01`
- Return value: only the block render path supports it: returns `false` when `mode` is `1` (item flat), and `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const scaleTo = ll.import("HologramLib", "itemDisplayScaleTo");
    // It only does a one-shot jump; step it yourself if you want a gradual animation
    scaleTo(box, 0.8);
    ```

### Set the block mode's second-stage transform

`itemDisplaySetExtend(id, scale, rx, ry)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `scale` : `String`<br>a constant number or a Molang expression string; effective in block mode only, the default is `"1"`
  - `rx` : `String`<br>a constant number or a Molang expression string; effective in block mode only, the default is `"-90"`
  - `ry` : `String`<br>a constant number or a Molang expression string; effective in block mode only, the default is `"0"`
- Return value: `true` = changed; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setExtend = ll.import("HologramLib", "itemDisplaySetExtend");
    setExtend(box, "1", "-90", "0");   // the defaults
    ```

### Switch the item's render mode

`itemDisplaySetMode(id, mode)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `mode` : `Integer`<br>`0` = auto (the default, decided from the item itself), `1` = item flat rendering, `2` = block solid rendering
- Return value: `true` = set; returns `false` when the id does not exist. When the value equals the current one it returns `true` straight away without resending
- Return type: `Boolean`
- Example:

    ```js
    const setMode = ll.import("HologramLib", "itemDisplaySetMode");
    setMode(box, 2);   // force solid block rendering
    setMode(box, 0);   // hand it back to the automatic choice
    ```

### Turn this display on or off

`itemDisplaySetEnabled(id, enabled)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `enabled` : `Boolean`<br>`false` = players cannot see it (the id stays valid, so you can turn it back on at any time), `true` = show it again
- Return value: `true` = set; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setEnabled = ll.import("HologramLib", "itemDisplaySetEnabled");
    setEnabled(box, false);   // hide it first
    setEnabled(box, true);    // then show it again
    ```

### Set the display's visible distance

`itemDisplaySetViewDistance(id, dist)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `dist` : `Float`<br>the distance in blocks; `<= 0` = unlimited distance
- Return value: `true` = set; returns `false` when the id does not exist. The same value as the current one is not resent
- Return type: `Boolean`
- Example:

    ```js
    const setView = ll.import("HologramLib", "itemDisplaySetViewDistance");
    setView(box, 32);   // visible within 32 blocks
    setView(box, 0);    // unlimited distance
    ```

## itemDisplay visible-player whitelist

### Show it only to the players on the list

`itemDisplaySetVisiblePlayers(id, playerNames)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `playerNames` : `Array<String>`<br>an array of player names, matched by LSE's `realName`; an empty array = clear the restriction and go back to visible to everyone
- Return value: `true` = set; returns `false` when the id does not exist. The dimension and view-distance conditions still apply on top
- Return type: `Boolean`
- Example:

    ```js
    const setVisiblePlayers = ll.import("HologramLib", "itemDisplaySetVisiblePlayers");
    setVisiblePlayers(box, ["Steve", "Alex"]);   // visible to those two only
    setVisiblePlayers(box, []);                  // empty = visible to everyone
    ```

### Put a single player on the whitelist

`itemDisplaySetVisiblePlayer(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `playerName` : `String`<br>the only player who can see it (`realName`)
- Return value: `true` = set; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setVisiblePlayer = ll.import("HologramLib", "itemDisplaySetVisiblePlayer");
    setVisiblePlayer(box, "Steve");   // visible to Steve only
    ```

### Clear the whitelist and go back to visible to everyone

`itemDisplayClearVisiblePlayers(id)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
- Return value: `true` = cleared (it counts as success even when there was no restriction to begin with); returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const clearVisible = ll.import("HologramLib", "itemDisplayClearVisiblePlayers");
    clearVisible(box);   // everyone can see it again
    ```

## itemDisplay following & clicking

### Make the display follow a player in real time

`itemDisplayFollow(id, playerName, offX, offY, offZ)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `playerName` : `String`<br>the target player name (`realName`)
  - `offX` : `Float`<br>the offset added on top of the player's coordinate X
  - `offY` : `Float`<br>the offset added on top of the player's coordinate Y
  - `offZ` : `Float`<br>the offset added on top of the player's coordinate Z
- Return value: `true` = following started; returns `false` when the id does not exist. A player logging off releases the follow automatically (the display stays where it is), and a manual `itemDisplaySetPosition` releases it too
- Return type: `Boolean`
- Example:

    ```js
    const follow = ll.import("HologramLib", "itemDisplayFollow");
    follow(box, "Steve", 0, 2.5, 0);   // hover above Steve's head
    ```

### Release the follow and stay where it is

`itemDisplayUnfollow(id)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
- Return value: returns `true` even when there is no follow relationship (so repeating the call is safe)
- Return type: `Boolean`
- Example:

    ```js
    const unfollow = ll.import("HologramLib", "itemDisplayUnfollow");
    unfollow(box);   // stop following the player
    ```

### Give the display a hitbox

`itemDisplaySetHitbox(id, width, height)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
  - `width` : `Float`<br>the width of the hit volume
  - `height` : `Float`<br>the height of the hit volume; `0` for both = back to unhittable (the default behaviour)
- Return value: `true` = set; returns `false` when the id does not exist. The event from a hit is fetched back by polling `ghost*`, under the domain `itemDisplay`
- Return type: `Boolean`
- Example:

    ```js
    const setHitbox = ll.import("HologramLib", "itemDisplaySetHitbox");
    setHitbox(box, 1, 1);   // now a block you can click
    setHitbox(box, 0, 0);   // back to unhittable
    ```

## itemDisplay queries & destruction

### Check whether the display still exists

`itemDisplayExists(id)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
- Return value: `true` = it exists
- Return type: `Boolean`
- Example:

    ```js
    const exists = ll.import("HologramLib", "itemDisplayExists");
    if (exists(box)) logger.info("the display is still there");
    ```

### Check whether an id is already taken

`itemDisplayIsIdUsed(id)`

- Parameters:
  - `id` : `Integer`<br>the item-display id to check
- Return value: `true` = already taken (in which case `itemDisplayCreateWithId` returns `-2`)
- Return type: `Boolean`
- Example:

    ```js
    const isUsed = ll.import("HologramLib", "itemDisplayIsIdUsed");
    if (!isUsed(42)) createWithId(0.5, 66, 0.5, 0, "minecraft:diamond", 0, 42);
    ```

### List the ids of all displays

`itemDisplayGetAllIds()`

- Return value: every display id; an empty array when there are none
- Return type: `Array<Integer>`
- Example:

    ```js
    const allIds = ll.import("HologramLib", "itemDisplayGetAllIds");
    for (const id of allIds()) logger.info(`display ${id}`);
    ```

### Find the nearest display in a dimension

`itemDisplayFindNearest(x, y, z, dim, maxDist)`

- Parameters:
  - `x` : `Float`<br>the world coordinate X of the search centre
  - `y` : `Float`<br>the world coordinate Y of the search centre
  - `z` : `Float`<br>the world coordinate Z of the search centre
  - `dim` : `Integer`<br>the dimension to search in
  - `maxDist` : `Float`<br>the search radius (blocks); `<= 0` = unlimited distance
- Return value: the nearest display id; returns `-1` when nothing in that dimension matches
- Return type: `Integer`
- Example:

    ```js
    const nearest = ll.import("HologramLib", "itemDisplayFindNearest");
    const near = nearest(100.5, 66, -200.5, 0, 10);   // the nearest display within 10 blocks in the Overworld
    ```

### Get a runtime summary for troubleshooting

`itemDisplayGetInfo(id)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
- Return value: the summary text, containing the id, dimension, coordinates, mode, on/off state, item, view distance, visible list and the number of players that have seen it; returns `not_found` when the id does not exist
- Return type: `String`
- Example:

    ```js
    const getInfo = ll.import("HologramLib", "itemDisplayGetInfo");
    logger.info(getInfo(box));
    ```

### Delete one item display

`itemDisplayDestroy(id)`

- Parameters:
  - `id` : `Integer`<br>the item-display id
- Return value: `true` = deleted; returns `false` when the id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const destroy = ll.import("HologramLib", "itemDisplayDestroy");
    destroy(box);
    ```

### Delete every item display

`itemDisplayDestroyAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const destroyAll = ll.import("HologramLib", "itemDisplayDestroyAll");
    destroyAll();   // clear everything
    ```

## Things to know

- **The library supplies the default pose**: in item mode `offsetY` is `"-4"`, `rotX` and `rotZ` are `"180"` and `scale` is `"0.375"` (block mode suggests `"0.5"`); the default visible distance is 64 blocks.
- **Molang expressions**: the translation / rotation / scale parameters all take an expression directly (for example `"math.sin(query.life_time*90)*360"`), and the client computes the animation, so the server does not have to push a packet every tick.
- **Property changes merge**: changing several of them in the same tick rebuilds the display only once (so it cannot flicker repeatedly). A rebuild does resend the whole display, though, so to "follow something every tick" use `itemDisplayFollow` instead of calling `itemDisplaySetPosition` over and over inside a tick loop.
- **`itemDisplayScaleTo` only does a one-shot jump**: it produces no gradual animation of its own, so step it yourself for an eased effect; it returns `false` when `mode` is `1` (item flat).
- **Clicking needs a hit volume**: only once `itemDisplaySetHitbox` has set a non-zero width and height can the client's raycast hit it; the interaction / attack that follows is fetched back by polling `ghost*` (with `domain=itemDisplay` in the entry).
- **Ids are not interchangeable**: an `itemDisplay*` id belongs to the item-display domain only, so do not pass it to `holo*` / `entity*` or any other domain. The other way round, the id `itemDetailShow` returns is a holographic-text id.
- **The library writes nothing to disk**: after a script or server restart the displays are gone and have to be created again; reconnecting players need no handling, because the library re-sends automatically.
