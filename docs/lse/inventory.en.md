# Inventory & sulfur cube · change the inventory the client sees / swallow a block in a cube

`fakeInv*` makes the client "believe it is carrying these items in its inventory": what it changes is **the client display**, and not one byte of
the server-side inventory moves — the player clicking those slots reports the slot number back to you. `sulfur*` creates a sulfur-cube display
entity that "swallows" the block / item you want to show — besides custom entities, this is the second way to put something into the world.

What the two domains share: **everything they put out is display only**, and the server-side data is untouched — `fakeInv*` does not move the real
inventory, and `sulfur*` is purely visual.

## Quick start

Turn a player's hotbar into "a sword + 64 diamonds", then take the clicks on a timer:

```js
const fakeInvApply      = ll.import("HologramLib", "fakeInvApply");
const fakeInvPollClicks = ll.import("HologramLib", "fakeInvPollClicks");
const fakeInvClear      = ll.import("HologramLib", "fakeInvClear");
// slot 0 = hotbar slot 1, slot 9 = main-inventory slot 1; each entry is "slot|item id|count|name|lore line~lore line"
fakeInvApply("Steve", "0|minecraft:diamond_sword|1|§cFake blade|§7Attack +7;1|minecraft:diamond|64|§bFake diamond", 20);

setInterval(() => {
  for (const line of fakeInvPollClicks()) {   // each is "player=Steve slot=0"
    const m = line.match(/player=(\S+) slot=(-?\d+)/);
    if (m) logger.info(`${m[1]} clicked fake slot ${m[2]}`);
  }
}, 200);

// wrapping up: fakeInvClear("Steve") undoes the fake and restores the real inventory
```

Place a sulfur cube "swallowing a bookshelf" (invisible by default, so you only see that block):

```js
const sulfurCreate = ll.import("HologramLib", "sulfurCreate");
const id = sulfurCreate(100.5, 65, -200.5, 0, "minecraft:bookshelf", 1, 0, "§6Bookshelf", "regular", true);
```

---

## Fake inventory (fakeInv*)

The faked content covers 36 slots of the player's inventory: `0..8` is the hotbar and `9..35` is the main inventory.
Clicks go through the polling queue `fakeInvPollClicks`, and **only a slot that is both faked and non-empty** reports a click back.

### Apply a faked inventory and re-send it periodically

`fakeInvApply(playerName, itemsSpec, refreshIntervalTicks)`

- Parameters:
  - `playerName` : `String`<br>the target player's name, who must be online
  - `itemsSpec` : `String`<br>the whole faked content. Each entry is `slot|item id|count|name|lore line~lore line`, with entries separated by `;` or a newline; the slot is `0..35` (an out-of-range entry is skipped), an empty count is treated as 1, and lore lines are separated by `~`
  - `refreshIntervalTicks` : `Integer`<br>the periodic re-send interval (ticks); `0` = turn periodic re-sending off, and a value below 5 is treated as 5 (5 ticks at the fastest)
- Return value: `true` = it was applied; `false` = the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const fakeInvApply = ll.import("HologramLib", "fakeInvApply");
    // hotbar slot 0 gets a sword, slot 1 gets 64 diamonds; slot 9 = the first main-inventory slot
    fakeInvApply("Steve", "0|minecraft:diamond_sword|1|§cFake blade|§7Attack +7;1|minecraft:diamond|64|§bFake diamond", 20);
    ```

### Change one slot in the faked inventory

`fakeInvSetSlot(playerName, slot, type, count, damage, name, loreCsv)`

- Parameters:
  - `playerName` : `String`<br>the target player's name
  - `slot` : `Integer`<br>the slot, `0..35` (`0..8` hotbar, `9..35` main inventory)
  - `type` : `String`<br>the item id; an empty string = clear that slot
  - `count` : `Integer`<br>the count; anything below 1 is treated as 1
  - `damage` : `Integer`<br>the item's aux data value; normally `0`
  - `name` : `String`<br>the custom name (supports `§` colour codes); an empty string = none
  - `loreCsv` : `String`<br>the lore lines, separated by `~`; an empty string = no lore lines
- Return value: `true` = it was changed; `false` = that player has not had `apply` called on them, the player is offline or the slot is out of range
- Return type: `Boolean`
- Example:

    ```js
    const fakeInvSetSlot = ll.import("HologramLib", "fakeInvSetSlot");
    fakeInvSetSlot("Steve", 1, "minecraft:emerald", 3, 0, "§aReward", "§7Just handed out~§8Do not waste it");
    ```

### Re-send the faked inventory immediately

`fakeInvRefresh(playerName)`

- Parameters:
  - `playerName` : `String`<br>the target player's name
- Return value: `true` = it was re-sent; `false` = that player has no active fake, or the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const fakeInvRefresh = ll.import("HologramLib", "fakeInvRefresh");
    fakeInvRefresh("Steve");   // top it up right after a chest / trade interface opens
    ```

### Check whether a player has a faked inventory

`fakeInvIsActive(playerName)`

- Parameters:
  - `playerName` : `String`<br>the target player's name
- Return value: `true` = a fake is active; `false` = there is none
- Return type: `Boolean`
- Example:

    ```js
    const fakeInvIsActive = ll.import("HologramLib", "fakeInvIsActive");
    if (!fakeInvIsActive("Steve")) fakeInvApply("Steve", "0|minecraft:paper|1|§7Quest voucher", 20);
    ```

### List the players with a fake active

`fakeInvGetPlayers()`

- Return value: an array of player names; an empty array when there is not a single one
- Return type: `Array<String>`
- Example:

    ```js
    const fakeInvGetPlayers = ll.import("HologramLib", "fakeInvGetPlayers");
    logger.info(`faking the inventory of ${fakeInvGetPlayers().length} players`);
    ```

### Take and clear fake-slot clicks

`fakeInvPollClicks()`

- Return value: each entry looks like `player=X slot=S`; `slot` = the clicked fake slot; an empty array when the queue is empty
- Return type: `Array<String>`
- Example:

    ```js
    const fakeInvPollClicks = ll.import("HologramLib", "fakeInvPollClicks");
    setInterval(() => {
      for (const line of fakeInvPollClicks()) logger.info(line);
    }, 200);
    ```

### Discard every queued click

`fakeInvClearClicks()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const fakeInvClearClicks = ll.import("HologramLib", "fakeInvClearClicks");
    fakeInvClearClicks();
    ```

### Undo the fake and re-send the real inventory

`fakeInvClear(playerName)`

- Parameters:
  - `playerName` : `String`<br>the target player's name
- Return value: `true` = it was undone; `false` = that player had no fake to begin with
- Return type: `Boolean`
- Example:

    ```js
    const fakeInvClear = ll.import("HologramLib", "fakeInvClear");
    fakeInvClear("Steve");   // the player now sees the real inventory
    ```

### Undo every player's fake

`fakeInvClearAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const fakeInvClearAll = ll.import("HologramLib", "fakeInvClearAll");
    fakeInvClearAll();
    ```

---

## Sulfur-cube display (sulfur*)

Spawns a `minecraft:sulfur_cube` and puts the block / item to display into its main hand — the client renders it as equipment, so
it looks as if the block is "swallowed" inside it; the appearance tier is controlled with the `minecraft:sulfur_cube_archetype` attribute.
**When the cube itself is invisible, the swallowed block still renders as usual** — passing `true` for `invisible` gives the look of
"only that block is visible".

### Spawn a sulfur-cube display

`sulfurCreate(x, y, z, dim, blockType, blockCount, blockDamage, blockName, archetype, invisible)`

- Parameters:
  - `x` : `Float`<br>world coordinate X (floating point)
  - `y` : `Float`<br>world coordinate Y (floating point)
  - `z` : `Float`<br>world coordinate Z (floating point)
  - `dim` : `Integer`<br>the dimension, `0` = overworld / `1` = nether / `2` = the End
  - `blockType` : `String`<br>the id of the item / block being "swallowed" (an empty string = empty hand; a block item is recommended, since only then does it look as if a block is swallowed inside it)
  - `blockCount` : `Integer`<br>the count; anything below 1 is treated as 1
  - `blockDamage` : `Integer`<br>the item's aux data value; normally `0`
  - `blockName` : `String`<br>the custom name (supports `§` colour codes); an empty string = none
  - `archetype` : `String`<br>the appearance tier; one of `none` / `regular` / `bouncy` / `slow_bouncy` / `slow_flat` / `fast_flat` / `light` / `fast_sliding` / `slow_sliding` / `sticky` / `high_resistance` / `explosive` / `hot`; an empty string = do not send it (keep the client default)
  - `invisible` : `Boolean`<br>whether the cube itself is invisible; `true` = you only see the swallowed block, `false` = you see the cube as well
- Return value: the display id, which every later function uses; `<0` = failure
- Return type: `Integer`
- Example:

    ```js
    const sulfurCreate = ll.import("HologramLib", "sulfurCreate");
    const id = sulfurCreate(100.5, 65, -200.5, 0, "minecraft:bookshelf", 1, 0, "§6Bookshelf", "regular", true);
    ```

### Swap the swallowed block or item

`sulfurSetBlock(id, type, count, damage, name)`

- Parameters:
  - `id` : `Integer`<br>the display id
  - `type` : `String`<br>the item id; an empty string = empty hand
  - `count` : `Integer`<br>the count; anything below 1 is treated as 1
  - `damage` : `Integer`<br>the item's aux data value; normally `0`
  - `name` : `String`<br>the custom name (supports `§` colour codes); an empty string = none
- Return value: `true` = it was swapped; `false` = that display id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const sulfurSetBlock = ll.import("HologramLib", "sulfurSetBlock");
    sulfurSetBlock(id, "minecraft:gold_block", 1, 0, "§6Gold block");   // swap to swallowing a gold block
    ```

### Swap the cube's appearance tier

`sulfurSetArchetype(id, archetype)`

- Parameters:
  - `id` : `Integer`<br>the display id
  - `archetype` : `String`<br>the appearance tier, with the same values as `sulfurCreate`'s `archetype`; an empty string = clear the attribute and go back to the client's default appearance
- Return value: `true` = it was swapped; `false` = that display id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const sulfurSetArchetype = ll.import("HologramLib", "sulfurSetArchetype");
    sulfurSetArchetype(id, "sticky");   // swap to the sticky tier
    ```

### Set whether the cube itself is invisible

`sulfurSetInvisible(id, on)`

- Parameters:
  - `id` : `Integer`<br>the display id
  - `on` : `Boolean`<br>`true` = invisible (only the swallowed block stays visible); `false` = show the cube itself
- Return value: `true` = it was set; `false` = that display id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const sulfurSetInvisible = ll.import("HologramLib", "sulfurSetInvisible");
    sulfurSetInvisible(id, false);   // you want to see the cube itself
    ```

### Set the display's scale

`sulfurSetScale(id, scale)`

- Parameters:
  - `id` : `Integer`<br>the display id
  - `scale` : `Float`<br>the scale value, valid range `0.0625~10`; anything outside is clamped into range automatically
- Return value: `true` = it was set; `false` = that display id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const sulfurSetScale = ll.import("HologramLib", "sulfurSetScale");
    sulfurSetScale(id, 1.5);
    ```

### Check whether a display still exists

`sulfurExists(id)`

- Parameters:
  - `id` : `Integer`<br>the display id
- Return value: `true` = it exists; `false` = it was destroyed or does not exist
- Return type: `Boolean`
- Example:

    ```js
    const sulfurExists = ll.import("HologramLib", "sulfurExists");
    if (sulfurExists(id)) sulfurSetInvisible(id, true);
    ```

### List every sulfur-cube display

`sulfurGetIds()`

- Return value: an array of display ids; an empty array when there is not a single one
- Return type: `Array<Integer>`
- Example:

    ```js
    const sulfurGetIds = ll.import("HologramLib", "sulfurGetIds");
    for (const showId of sulfurGetIds()) logger.info(`display ${showId}`);
    ```

### Look up the entity id behind a display

`sulfurGetEntityId(id)`

- Parameters:
  - `id` : `Integer`<br>the display id
- Return value: the entity id behind it; `-1` = that display does not exist
- Return type: `Integer`
- Example:

    ```js
    const sulfurGetEntityId = ll.import("HologramLib", "sulfurGetEntityId");
    logger.info(`display ${id} has entity id = ${sulfurGetEntityId(id)}`);
    ```

### Destroy one sulfur-cube display

`sulfurDestroy(id)`

- Parameters:
  - `id` : `Integer`<br>the display id
- Return value: `true` = it was destroyed; `false` = that id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const sulfurDestroy = ll.import("HologramLib", "sulfurDestroy");
    sulfurDestroy(id);
    ```

### Destroy every sulfur-cube display

`sulfurDestroyAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const sulfurDestroyAll = ll.import("HologramLib", "sulfurDestroyAll");
    sulfurDestroyAll();
    ```

---

## Things to watch out for

- **A faked inventory only affects the client display.** What the player can really use and eat is the real server-side item; nor is an item taken away
  by a click — once the server's check fails, the client withdraws its prediction (again the default behaviour, not an error).
- **Do not fake a slot as exactly the same thing as the real item**: an action on that slot is then really executed by the server. If you want a slot to
  be display only, make it different from the real item.
- **When that slot's real item changes for some other reason** (picking something up / using an item / another plugin changing the inventory), the
  periodic re-send writes the faked content back over it.
- **Opening a chest or trade interface makes the server resend the inventory content**, which overwrites the fake — rely on
  `refreshIntervalTicks` to re-send periodically, or top it up by hand with `fakeInvRefresh`.
- **Armour (slot 6) and the offhand (slot 34) are outside this domain**, as each has its own container.
- The fake record is cleared when the player goes offline, and after reconnecting they see the real inventory, so there is no need to undo it by hand.
- Click events sit in a polling queue: `fakeInvPollClicks` is "take and clear", so take from it continuously per tick or with `setInterval`.
- **"Swallowing a mob" is not implemented**: these display entities have no AI, so really swallowing and digesting one is impossible; this domain
  only does blocks and invisibility.

## Related

- Domain guide: [Interfaces & containers](../domains/interfaces.md#fake-inventory) (the mechanism and the boundaries of the fake inventory)
- Also want to change a block's appearance: [View override & sensing](view.md)
- The full C++ side: [C++ API reference](../reference/cpp-api.md)
