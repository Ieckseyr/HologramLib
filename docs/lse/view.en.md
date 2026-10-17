# View override & sensing · let one player see something different / work out what device they use

`view*` only changes **the client view**: on one player's (or every player's) client, an entity or a block looks different,
while the server-side world, the save, collision and what other players see all stay as they were. `sensing*` looks up a
player's current input device (keyboard-mouse / touch / gamepad / motion), and the most common use is **routing the interface
by device**: touch gets the virtual container, keyboard-mouse gets the trade menu.

Overrides live only in the server's memory: a restart loses them, but an override recorded by player name still applies after
that player rejoins. Neither `view*` nor `sensing*` has **events or a polling queue** — call directly and get the result directly.

## Quick start

```js
const viewEntity     = ll.import("HologramLib", "viewEntity");
const viewBlock      = ll.import("HologramLib", "viewBlock");
const viewSelfType   = ll.import("HologramLib", "viewSelfType");
const viewUniqueIdOf = ll.import("HologramLib", "viewUniqueIdOf");
const viewClearAll   = ll.import("HologramLib", "viewClearAll");

// entity 12345 becomes a cow for Steve, with the nametag "§cDairy cow" always shown
viewEntity("Steve", 12345, "type=minecraft:cow;name=§cDairy cow;always=1");
// (100,64,100) shows up as a diamond block on Steve's client
viewBlock("Steve", 100, 64, 100, "minecraft:diamond_block");
// make everyone see Steve as a zombie
viewSelfType("Steve", "minecraft:zombie");

// getting someone else's uniqueId: only -1 means offline (the uniqueId itself may be negative, so do not write < 0)
const uid = viewUniqueIdOf("Alex");
if (uid !== -1) viewEntity("", uid, "type=minecraft:cow");   // everyone sees Alex as a cow

viewClearAll("");   // wrapping up: clear every player's overrides
```

## The spec string

`viewEntity` / `viewSelf`'s `spec` is separated by semicolons, with each item written `key=value`; keys are case-insensitive, and whitespace around values is trimmed.

| Key | Value | Notes |
|---|---|---|
| `type=` | an entity type name | swap to that type (a bare `cow` becomes `minecraft:cow` automatically); for a player entity = build a mob stand-in for them with the same id |
| `as=` | an online player's name | display as that player (the skin comes from them); only works on player entities |
| `skin=` | a registered skin id | swap to a skin from the registry (registered by `playerNpcRegisterSkin` / `playerNpcCaptureSkin`); only works on non-player entities — the client renders it with the player model, while server-side it is still the same mob |
| `hidden=` | `1` / `true` / `yes` / `on` | hide this entity from that player |
| `name=` | nametag text | the nametag is changed as soon as this key appears; an empty value = clear the name |
| `always=` | `1` / `true` / `yes` / `on` | the nametag is always shown (use it together with `name=`) |
| `pose=` | `0..13` | the pose index; out-of-range values are clamped into range; the client only poses armour stands and players from it |
| `scale=` | `0.0625~10` | the metadata scale (client entity scale + hitbox); out of range and the whole item is ignored; not the same thing as the `skin=` tier's model scale |
| `variant=` | integer | the variant: the sheep's colour tier / the pufferfish's puff state (`0` normal `1` half-puffed `2` fully puffed) / the tropical fish's body shape … |
| `mark=` | integer | the second-level variant (may also be written `markvariant=`): tropical-fish patterns `0..5`, horse markings and so on |
| `color=` | `0..15` | the main colour (may also be written `colorindex=`); out of range and the whole item is ignored |
| `color2=` | `0..15` | the pattern colour (for tropical fish); out of range and the whole item is ignored |

> `pose=` / `scale=` / `variant=` / `mark=` / `color=` / `color2=` are **additional items**, and have to be given together with a main item
> (`type=` / `as=` / `skin=` / `hidden=` / `name=`): a spec with only additional items counts as "no effective item at all",
> which is the same as withdrawing that override.

---

## Main entry points

### Override an entity as one player sees it

`viewEntity(playerName, entityUniqueId, spec)`

- Parameters:
  - `playerName` : `String`<br>the player's name; an empty string = apply to every player
  - `entityUniqueId` : `Integer`<br>the entity's unique id — for a player take it from `viewUniqueIdOf`, and for other entities from `Entity.uniqueId` (convert it to a number with `Number(e.uniqueId)`)
  - `spec` : `String`<br>the override content, see "The spec string" above
- Return value: `true` = it was registered; `false` = the `spec` has no effective item (this call then equals withdrawing the override, and `false` if there was no override to begin with)
- Return type: `Boolean`
- Example:

    ```js
    const viewEntity = ll.import("HologramLib", "viewEntity");
    viewEntity("Steve", 12345, "type=minecraft:cow;name=§cDairy cow;always=1");
    viewEntity("Steve", 12345, "hidden=1");   // let only Steve be unable to see it
    ```

### Override a block as one player sees it

`viewBlock(playerName, x, y, z, type)`

- Parameters:
  - `playerName` : `String`<br>the player's name; an empty string = apply to every player
  - `x` : `Integer`<br>the block coordinate X (integer)
  - `y` : `Integer`<br>the block coordinate Y (integer)
  - `z` : `Integer`<br>the block coordinate Z (integer)
  - `type` : `String`<br>the block id, such as `"minecraft:diamond_block"`; an empty string = withdraw the override at this coordinate
- Return value: `true` = it was registered; `false` = the type name is not in the client's registry (when `type` is empty, `false` = this coordinate had no override to begin with)
- Return type: `Boolean`
- Example:

    ```js
    const viewBlock = ll.import("HologramLib", "viewBlock");
    viewBlock("Steve", 100, 64, 100, "minecraft:diamond_block");
    viewBlock("Steve", 100, 64, 100, "");   // withdraw this block
    ```

### Withdraw one entity's override

`viewClearEntity(playerName, entityUniqueId)`

- Parameters:
  - `playerName` : `String`<br>the player's name; an empty string = apply to every player
  - `entityUniqueId` : `Integer`<br>the entity's unique id
- Return value: `true` = it was withdrawn and restored; `false` = there is no such override
- Return type: `Boolean`
- Example:

    ```js
    const viewClearEntity = ll.import("HologramLib", "viewClearEntity");
    viewClearEntity("Steve", 12345);
    ```

### Withdraw one block's override

`viewClearBlock(playerName, x, y, z)`

- Parameters:
  - `playerName` : `String`<br>the player's name; an empty string = apply to every player
  - `x` : `Integer`<br>the block coordinate X (integer)
  - `y` : `Integer`<br>the block coordinate Y (integer)
  - `z` : `Integer`<br>the block coordinate Z (integer)
- Return value: `true` = it was withdrawn and the real block pushed back; `false` = this coordinate had no override to begin with
- Return type: `Boolean`
- Example:

    ```js
    const viewClearBlock = ll.import("HologramLib", "viewClearBlock");
    viewClearBlock("Steve", 100, 64, 100);
    ```

### Withdraw every override for one player

`viewClearAll(playerName)`

- Parameters:
  - `playerName` : `String`<br>the player's name; an empty string = all players
- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const viewClearAll = ll.import("HologramLib", "viewClearAll");
    viewClearAll("Steve");   // clear Steve only; passing "" clears the whole server
    ```

---

## The "self" family

This group acts on **the caller themself** and is shown to **every player** (for things like an admin disguising themselves or an event transformation).

### Put an override on yourself

`viewSelf(playerName, spec)`

- Parameters:
  - `playerName` : `String`<br>the player's name, who must be online
  - `spec` : `String`<br>the override content (the same as `viewEntity`, see "The spec string" above)
- Return value: `true` = it was registered; `false` = the player is offline, or the `spec` has no effective item
- Return type: `Boolean`
- Example:

    ```js
    const viewSelf = ll.import("HologramLib", "viewSelf");
    viewSelf("Steve", "type=minecraft:cow;scale=0.8");   // everyone sees Steve as a small cow
    ```

### Withdraw your own override

`viewSelfClear(playerName)`

- Parameters:
  - `playerName` : `String`<br>the player's name
- Return value: `true` = it was withdrawn; `false` = the player is offline or there was no override to begin with
- Return type: `Boolean`
- Example:

    ```js
    const viewSelfClear = ll.import("HologramLib", "viewSelfClear");
    viewSelfClear("Steve");
    ```

### Make yourself look like another mob to everyone

`viewSelfType(playerName, type)`

- Parameters:
  - `playerName` : `String`<br>the player's name
  - `type` : `String`<br>an entity type name, such as `"minecraft:zombie"` (player and mob types both work)
- Return value: `true` = it was registered; `false` = the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const viewSelfType = ll.import("HologramLib", "viewSelfType");
    viewSelfType("Steve", "minecraft:zombie");
    ```

### Display yourself with someone else's skin

`viewSelfSkin(playerName, skinOfPlayer)`

- Parameters:
  - `playerName` : `String`<br>the player's name
  - `skinOfPlayer` : `String`<br>the name of the online player the skin comes from
- Return value: `true` = it was registered; `false` = the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const viewSelfSkin = ll.import("HologramLib", "viewSelfSkin");
    viewSelfSkin("Steve", "Alex");   // Steve takes on Alex's look
    ```

### Change the nametag above your own head

`viewSelfName(playerName, nametag, alwaysShow)`

- Parameters:
  - `playerName` : `String`<br>the player's name
  - `nametag` : `String`<br>the new name (supports `§` colour codes); an empty string = clear the name
  - `alwaysShow` : `Boolean`<br>`true` = the nametag is always shown
- Return value: `true` = it was registered; `false` = the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const viewSelfName = ll.import("HologramLib", "viewSelfName");
    viewSelfName("Steve", "§cCaptain", true);
    ```

### Hide yourself from everyone

`viewSelfHide(playerName, hidden)`

- Parameters:
  - `playerName` : `String`<br>the player's name
  - `hidden` : `Boolean`<br>`true` = nobody else can see you; `false` = visible again (equal to withdrawing this override)
- Return value: `true` = it was registered; `false` = the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const viewSelfHide = ll.import("HologramLib", "viewSelfHide");
    viewSelfHide("Steve", true);    // stealth mode: nobody can see Steve
    viewSelfHide("Steve", false);   // reappear
    ```

---

## Per-field and per-id operations

When you cannot get someone else's uniqueId, use `viewTargetType`; to clear every override on one entity in a single call, use `viewClearId`.

### Make all players see it as that type

`viewTargetType(entityUniqueId, type)`

- Parameters:
  - `entityUniqueId` : `Integer`<br>the entity's unique id
  - `type` : `String`<br>an entity type name, such as `"minecraft:cow"`
- Return value: `true` = it was registered; `false` = `type` is empty and there is no override to withdraw
- Return type: `Boolean`
- Example:

    ```js
    const viewTargetType = ll.import("HologramLib", "viewTargetType");
    viewTargetType(12345, "minecraft:sheep");   // everyone sees a sheep
    ```

### Withdraw every override on this entity

`viewClearId(entityUniqueId)`

- Parameters:
  - `entityUniqueId` : `Integer`<br>the entity's unique id
- Return value: `true` = it was withdrawn; `false` = there is no such override
- Return type: `Boolean`
- Example:

    ```js
    const viewClearId = ll.import("HologramLib", "viewClearId");
    viewClearId(12345);
    ```

---

## Queries and diagnostics

### Drain the "entity just spawned" queue (1.30.0 / 26.51.4)

`viewPollSpawnEvents()`

- Parameters: none
- Returns: an array of strings, one per entity that just spawned; reading **empties** the queue (same drain semantics as `ghost*` / `containerPollClicks`)
  - dropped items: `drop|uid|dim|x|y|z|`
  - everything else: `actor|uid|dim|x|y|z|identifier`
- Return type: `Array<String>`
- Notes: the event is recorded **inside the outbound hook** (once per entity), so scripts no longer need a periodic
  0.5–1 s scan to find "whatever just spawned" — one tick is enough. Typical use: take over a dropped item with
  `itemDisplayEmitForEntity(player, uid, ...)` the moment it spawns; apply disguise rules the moment a mob spawns.
  The queue is capped (512 entries); a stalled script loses the oldest ones.
- Example:

    ```js
    const pollSpawn = ll.import("HologramLib", "viewPollSpawnEvents");
    const attach    = ll.import("HologramLib", "itemDisplayEmitForEntity");

    function onTick() {
        for (const ev of pollSpawn()) {
            const [kind, uid, dim, x, y, z, identifier] = ev.split("|");
            if (kind === "drop") attach(Number(uid), "-0.4", "0", "0", "0.375", 180, 2, "", true);
        }
    }
    ```

### Look up a player's uniqueId by name

`viewUniqueIdOf(playerName)`

- Parameters:
  - `playerName` : `String`<br>the player's name
- Return value: that player entity's uniqueId; **`-1` = offline** (the uniqueId itself may be negative, so only `-1` counts as the test — do not write `< 0`)
- Return type: `Integer`
- Example:

    ```js
    const viewUniqueIdOf = ll.import("HologramLib", "viewUniqueIdOf");
    const uid = viewUniqueIdOf("Alex");
    if (uid !== -1) logger.info(`Alex's uniqueId = ${uid}`);
    ```

### Check how many overrides a player carries

`viewDescribe(playerName)`

- Parameters:
  - `playerName` : `String`<br>the player's name
- Return value: a summary starting with `entities=N blocks=N`, followed by several hook counters (read it when working out whether an override took effect)
- Return type: `String`
- Example:

    ```js
    const viewDescribe = ll.import("HologramLib", "viewDescribe");
    logger.info(viewDescribe("Steve"));
    ```

### Echo a string back verbatim

`viewEcho(text)`

- Parameters:
  - `text` : `String`<br>any string
- Return value: looks like `len=12 hex=7479... text=[...]`; `hex` is the first 16 bytes at most
- Return type: `String`
- Example:

    ```js
    const viewEcho = ll.import("HologramLib", "viewEcho");
    logger.info(viewEcho("type=minecraft:cow"));   // first confirm this string was not truncated or mangled
    ```

### Verify the metadata a spec folds into, offline

`viewMetaProbe(spec)`

- Parameters:
  - `spec` : `String`<br>the override content (the same as `viewEntity`)
- Return value: looks like `items=N bytes=N hex=...`; `items=0 plan=empty` when the `spec` has no effective item
- Return type: `String`
- Example:

    ```js
    const viewMetaProbe = ll.import("HologramLib", "viewMetaProbe");
    logger.info(viewMetaProbe("type=minecraft:sheep;color=5"));   // see what this string actually sends
    ```

### Check whether dropped-item spawn packets are intercepted

`viewItemSpawnHooked()`

- Return value: always `true` in this version (re-typing / hiding a dropped item in one shot); an older library does not have this function name (`ll.hasExported` judges it `false`) → treat it as unsupported, and the script has to re-apply the override periodically
- Return type: `Boolean`
- Example:

    ```js
    const viewItemSpawnHooked = ll.import("HologramLib", "viewItemSpawnHooked");
    if (viewItemSpawnHooked()) logger.info("dropped-item overrides land in one shot, no periodic re-applying needed");
    ```

---

## Sensing (sensing*)

The data source is the input method the client reports every tick, and the library keeps it in memory. **No events and no polling** — query it directly when you need it.

### Look up the player's current device

`sensingDeviceOf(playerName)`

- Parameters:
  - `playerName` : `String`<br>the player's name
- Return value: `keyboardMouse` / `touch` / `gamepad` / `motionController` / `unknown` (`unknown` when the player is offline or has not reported input yet)
- Return type: `String`
- Example:

    ```js
    const sensingDeviceOf = ll.import("HologramLib", "sensingDeviceOf");
    logger.info(`Steve's device: ${sensingDeviceOf("Steve")}`);
    ```

### Check whether the player is on touch

`sensingIsTouch(playerName)`

- Parameters:
  - `playerName` : `String`<br>the player's name
- Return value: `true` = touch; `false` = not touch, or the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const sensingIsTouch = ll.import("HologramLib", "sensingIsTouch");
    const containerOpen  = ll.import("HologramLib", "containerOpen");
    const tradeOpen      = ll.import("HologramLib", "tradeOpen");

    // touch gets the container list (a click always sends packets), keyboard-mouse gets the trade menu
    if (sensingIsTouch("Steve")) containerOpen("Steve", "§8Quest list", 3);
    else tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
    ```

### Get the numeric device code

`sensingDeviceCode(playerName)`

- Parameters:
  - `playerName` : `String`<br>the player's name
- Return value: `0` = Unknown `1` = keyboard-mouse `2` = touch `3` = gamepad `4` = motion
- Return type: `Integer`
- Example:

    ```js
    const sensingDeviceCode = ll.import("HologramLib", "sensingDeviceCode");
    logger.info(`device code: ${sensingDeviceCode("Steve")}`);
    ```

---

## Things to watch out for

- **It only changes the client view**: the server-side world, the save, collision and what other players see all stay as they were. When a player is
  displayed as a cow, their id has not changed — hitting them is hitting them in person, and damage and drops are settled against the real thing.
- **Overrides live only in memory**: a server restart loses them. An override recorded by player name still applies after that player rejoins
  (it is applied automatically when the entity spawns again, and blocks are re-sent automatically as chunks are delivered).
- **Your own client does not take part in overrides of your own entity**: re-typing and hiding are for other people to see (otherwise your own client
  would break); only the skin-swap family (`viewSelfSkin` / `as=`) includes the player themself.
- **You can set an override before the entity has even spawned**, and the library applies it when it spawns; conversely, an entity or block that was
  never going to be sent to this player (out of view, and so on) does not appear out of thin air because of an override.
- **On withdrawal, entities that are too far away (> 64 blocks) or already gone are not rebuilt** — the client did not have them anyway, so wait for
  their next spawn.
- **Per-player glowing / invisibility cannot be done**: Bedrock invisibility is an effect rather than an appearance, and this version of the API has
  neither item. Per-player entity scaling uses `scale=` (`0.0625~10`); do not mix it up with the `skin=` tier's model scaling.
- **`sensing*` needs no polling**: after a device change the very next input packet updates it, and it is cleared the moment the player goes offline.
- Only the interface domains (container / dialogue / trade) have events that need polling; see [Interfaces & containers](interfaces.md).

## Related

- Domain guide: [View override & sensing](../domains/view-sensing.md) (what it can and cannot do, per-player isolation and the restore rules)
- Fake player entities: [Fake-player NPCs](player-npc.md) (the skin registry is where `skin=` comes from)
- The full C++ side: [C++ API reference](../reference/cpp-api.md)
