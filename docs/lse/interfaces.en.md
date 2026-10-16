# Interfaces & containers · trade menu, virtual container and NPC dialogue

These three domains do the same counter-intuitive thing: **the server does not have this interface, yet the client opens a real vanilla one**.
`trade*` lays out a villager trade table (display-only by default), `container*` opens a chest interface to use as "a list where clicking fires a callback",
and `npcDialog*` opens the NPC dialogue interface for quests and multi-level dialogue.

Clicks and settlement results **do not go into callbacks**: they all go into queues, taken by polling
`containerPollClicks` / `npcDialogPollClicks` / `tradePollSettlements`. Every queue is "take and clear" — one event is taken
exactly once — so you have to poll periodically.

## Quick start

The smallest runnable example: open a 27-slot "quest list" for a player and take the clicks on a timer.

```js
const containerOpen       = ll.import("HologramLib", "containerOpen");
const containerSetItem    = ll.import("HologramLib", "containerSetItem");
const containerPollClicks = ll.import("HologramLib", "containerPollClicks");

const id = containerOpen("Steve", "§8Quest list", 3);   // 27 slots; rows = 6 → 54 slots
containerSetItem(id, 0, "minecraft:diamond_sword", 1, 0, "§bTrial", "§7Click to claim|§8Second line");

setInterval(() => {                                  // you can also hang this on the tick of mc.listen
  for (const line of containerPollClicks()) {        // each is "player=X menuId=N slot=S closed=0|1"
    const m = line.match(/player=(\S+) menuId=(-?\d+) slot=(-?\d+) closed=(\d)/);
    if (!m) continue;
    const [, player, menuId, slot, closed] = m;
    if (closed === "1") continue;                    // the interface was closed (slot = -1 at that point)
    logger.info(`${player} clicked slot ${slot} in menu ${menuId}`);
  }
}, 200);
```

> Opening a container carries an **inherent delay** (7 ticks ≈ 350ms by default): the client has to apply this chest block and
> its block entity first, and only then can the interface-opening packet bind to that position. Filling slots during the delay
> is not lost (per-slot refresh travels over another packet), but the interface appears slightly later.

---

## Trade menu (trade*)

A protocol-layer, self-built trade interface: it opens the interface and lays out the trade table, and the carrier can be swapped (villager / wandering trader).
**This domain has no click callback** — settlement results can only be taken through `tradePollSettlements`; for a list interface where
"clicking an entry fires a callback", use `container*`.

### Open a trade interface

`tradeOpen(playerName, tradeType, tier, experience, realTrades, carrierIdentifier)`

- Parameters:
  - `playerName` : `String`<br>the target player's name, who must be online (`-1` is returned straight away when they are not)
  - `tradeType` : `String`<br>the trade type, such as `"entity.villager.butcher"` or `"entity.villager.priest"`; any custom string is accepted, and a key the client does not recognise is displayed as it is
  - `tier` : `Integer`<br>the villager's displayed tier value, 1-based: 1=novice 2=apprentice 3=journeyman 4=expert 5=master (out-of-range values are clamped)
  - `experience` : `Integer`<br>the experience bar's current experience
  - `realTrades` : `Boolean`<br>`false` (recommended) = protocol-layer display only — the server has no trade table, so payment cannot be placed into a slot and clicking settles nothing; `true` = install a real trade table on the carrier and hand it to the server, so the player really can settle trades and items really are consumed
  - `carrierIdentifier` : `String`<br>the carrier entity type; an empty string = `minecraft:villager_v2`, and `minecraft:wandering_trader` also works
- Return value: the menu id, which every later function uses; `-1` = opening failed (the player is offline, and so on)
- Return type: `Integer`
- Example:

    ```js
    const tradeOpen = ll.import("HologramLib", "tradeOpen");
    // display only: a butcher trade table, tier 1, 0 experience, carrier is a villager
    const id = tradeOpen("Steve", "entity.villager.butcher", 1, 0, false, "");
    ```

### Append a trade and re-send in place

`tradeAddOffer(id, buyAType, buyACount, buyBType, buyBCount, sellType, sellCount, sellName, tier, locked)`

- Parameters:
  - `id` : `Integer`<br>the menu id returned by `tradeOpen`
  - `buyAType` : `String`<br>the item id of payment item A (required)
  - `buyACount` : `Integer`<br>the count of payment A; anything below 1 is treated as 1
  - `buyBType` : `String`<br>the id of the second payment item; an empty string = there is no second payment item
  - `buyBCount` : `Integer`<br>the count of payment B; anything below 1 is treated as 1
  - `sellType` : `String`<br>the item id of the product
  - `sellCount` : `Integer`<br>the product count; anything below 1 is treated as 1
  - `sellName` : `String`<br>the product's custom name (supports `§` colour codes); an empty string = none
  - `tier` : `Integer`<br>the displayed tier value at which this trade unlocks (1-based); above the menu's `tier` the client shows it as locked
  - `locked` : `Boolean`<br>`true` = this entry shows as locked no matter how high the menu's `tier` is
- Return value: `true` = it was appended and re-sent; `false` = the menu is gone (the player is offline or the interface was closed)
- Return type: `Boolean`
- Example:

    ```js
    const tradeAddOffer = ll.import("HologramLib", "tradeAddOffer");
    // 3 emeralds + 1 of any item → 1 diamond, displayed as "§bDiamond"
    tradeAddOffer(id, "minecraft:emerald", 3, "", 1, "minecraft:diamond", 1, "§bDiamond", 1, false);
    ```

### Change the displayed tier and experience bar

`tradeSetTier(id, tier, experience)`

- Parameters:
  - `id` : `Integer`<br>the menu id
  - `tier` : `Integer`<br>the new displayed tier value (1-based, 1=novice … 5=master)
  - `experience` : `Integer`<br>the experience bar's new current experience
- Return value: `true` = it was changed and re-sent; `false` = the menu is gone
- Return type: `Boolean`
- Example:

    ```js
    const tradeSetTier = ll.import("HologramLib", "tradeSetTier");
    tradeSetTier(id, 3, 120);   // raise it to tier 3 and set the experience bar to 120
    ```

### Toggle protocol-layer real settlement

`tradeSetSettle(id, on)`

- Parameters:
  - `id` : `Integer`<br>the menu id
  - `on` : `Boolean`<br>`true` = turn real settlement on (deduct the payment from the player's inventory and deliver the product into it); `false` = back to display only
- Return value: `true` = it was switched; `false` = the menu is gone
- Return type: `Boolean`
- Example:

    ```js
    const tradeSetSettle = ll.import("HologramLib", "tradeSetSettle");
    tradeSetSettle(id, true);    // only effective on the protocol-layer path (realTrades = false)
    ```

### Query whether a menu settles for real

`tradeIsSettle(id)`

- Parameters:
  - `id` : `Integer`<br>the menu id
- Return value: `true` = real settlement is on; `false` = it is off, or the menu is gone
- Return type: `Boolean`
- Example:

    ```js
    const tradeIsSettle = ll.import("HologramLib", "tradeIsSettle");
    if (tradeIsSettle(id)) logger.info("this menu really deducts items");
    ```

### Take and clear the settlement queue

`tradePollSettlements()`

- Return value: each entry looks like `player=Steve menuId=3 offer=1 ok=1 reason=`; `offer` = which trade was settled (`-1` = it could not be located), `ok=1` = it was really settled (the payment was deducted and the product delivered), and when `ok=0` `reason` gives the cause (such as `not-owner` / `payment-missing` / `inventory-full`); an empty array when the queue is empty
- Return type: `Array<String>`
- Example:

    ```js
    const tradePollSettlements = ll.import("HologramLib", "tradePollSettlements");
    setInterval(() => {
      for (const line of tradePollSettlements()) {
        const m = line.match(/player=(\S+) menuId=(-?\d+) offer=(-?\d+) ok=(\d) reason=(\S*)/);
        if (m && m[4] === "1") logger.info(`${m[1]} settled trade ${m[3]}`);
      }
    }, 200);
    ```

### Check whether a menu is still open

`tradeIsOpen(id)`

- Parameters:
  - `id` : `Integer`<br>the menu id
- Return value: `true` = the menu exists; `false` = it is closed or never existed
- Return type: `Boolean`
- Example:

    ```js
    const tradeIsOpen = ll.import("HologramLib", "tradeIsOpen");
    logger.info(tradeIsOpen(id) ? "the interface is still open" : "it is closed");
    ```

### List every open trade menu

`tradeGetIds()`

- Return value: an array of menu ids; an empty array when no menu is open
- Return type: `Array<Integer>`
- Example:

    ```js
    const tradeGetIds = ll.import("HologramLib", "tradeGetIds");
    for (const menuId of tradeGetIds()) logger.info(`menu ${menuId}`);
    ```

### Close one trade menu

`tradeClose(id)`

- Parameters:
  - `id` : `Integer`<br>the menu id
- Return value: `true` = it was closed; `false` = that id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const tradeClose = ll.import("HologramLib", "tradeClose");
    tradeClose(id);
    ```

### Close every trade menu

`tradeCloseAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const tradeCloseAll = ll.import("HologramLib", "tradeCloseAll");
    tradeCloseAll();   // for example when wrapping up on server shutdown or a script reload
    ```

---

## Virtual container (container*)

Places a chest block and a block entity on the client, then opens the chest interface: **the server does not have this container at all**,
so the entries merely "sit there". A click here is an item pickup, and **every input device sends packets** (touch works too), which is why it is
the best fit for "a list interface where clicking fires a callback". Clicks and closes also go through the polling queue `containerPollClicks`.

### Open an empty container interface

`containerOpen(playerName, title, rows)`

- Parameters:
  - `playerName` : `String`<br>the target player's name, who must be online
  - `title` : `String`<br>the interface title (supports `§` colour codes); an empty string = use the default title
  - `rows` : `Integer`<br>the container's row count, `3` = a small container with 27 slots, `6` = a large container with 54 slots
- Return value: the menu id; `-1` = opening failed (the player is offline, the container ids are used up)
- Return type: `Integer`
- Example:

    ```js
    const containerOpen = ll.import("HologramLib", "containerOpen");
    const id = containerOpen("Steve", "§8Quest list", 3);
    ```

### Refresh one slot in place

`containerSetItem(id, slot, type, count, damage, name, loreCsv)`

- Parameters:
  - `id` : `Integer`<br>the menu id
  - `slot` : `Integer`<br>the slot index (0-based), which is also the `slot` reported back on a click
  - `type` : `String`<br>the item id, such as `"minecraft:diamond_sword"`; an empty string = clear that slot
  - `count` : `Integer`<br>the count; anything below 1 is treated as 1
  - `damage` : `Integer`<br>the item's aux data value; normally `0`
  - `name` : `String`<br>the custom name (supports `§` colour codes); an empty string = none
  - `loreCsv` : `String`<br>the lore lines, split with `\n` or `|`; an empty string = no lore lines
- Return value: `true` = it was refreshed; `false` = the menu is gone, the slot is out of range or the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const containerSetItem = ll.import("HologramLib", "containerSetItem");
    containerSetItem(id, 0, "minecraft:diamond_sword", 1, 0, "§bTrial", "§7Click to claim|§8Second line");
    containerSetItem(id, 1, "", 1, 0, "", "");   // clear slot 1
    ```

### Swap the interface title in place

`containerSetTitle(id, title)`

- Parameters:
  - `id` : `Integer`<br>the menu id
  - `title` : `String`<br>the new title (supports `§` colour codes)
- Return value: `true` = it was swapped; `false` = the menu is gone or the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const containerSetTitle = ll.import("HologramLib", "containerSetTitle");
    containerSetTitle(id, "§8Quest list · page 2");
    ```

### Turn interactive mode on or off

`containerSetInteractive(id, on)`

- Parameters:
  - `id` : `Integer`<br>the menu id
  - `on` : `Boolean`<br>`true` = interactive (the item really stays in its new slot); `false` (default) = only the click is reported back and the server pushes the item back where it was
- Return value: `true` = it was switched; `false` = the menu is gone
- Return type: `Boolean`
- Example:

    ```js
    const containerSetInteractive = ll.import("HologramLib", "containerSetInteractive");
    containerSetInteractive(id, true);   // becomes a container where you can really move things around
    ```

### Take and clear container click events

`containerPollClicks()`

- Return value: each entry looks like `player=X menuId=N slot=S closed=0|1`; `slot` = the index of the clicked slot, and `closed=1` means the interface was closed (in that case `slot=-1`); an empty array when the queue is empty
- Return type: `Array<String>`
- Example:

    ```js
    const containerPollClicks = ll.import("HologramLib", "containerPollClicks");
    setInterval(() => {
      for (const line of containerPollClicks()) logger.info(line);
    }, 200);
    ```

### Discard container queue clicks

`containerClearClicks()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const containerClearClicks = ll.import("HologramLib", "containerClearClicks");
    containerClearClicks();   // throw away old clicks before reopening the interface
    ```

### Check whether a container menu still exists

`containerIsOpen(id)`

- Parameters:
  - `id` : `Integer`<br>the menu id
- Return value: `true` = the menu exists; `false` = it is closed or does not exist
- Return type: `Boolean`
- Example:

    ```js
    const containerIsOpen = ll.import("HologramLib", "containerIsOpen");
    if (containerIsOpen(id)) containerSetTitle(id, "§8Quest list");
    ```

### List every open container menu

`containerGetIds()`

- Return value: an array of menu ids; an empty array when no menu is open
- Return type: `Array<Integer>`
- Example:

    ```js
    const containerGetIds = ll.import("HologramLib", "containerGetIds");
    logger.info(`${containerGetIds().length} container interfaces are open`);
    ```

### Close one container interface

`containerClose(id)`

- Parameters:
  - `id` : `Integer`<br>the menu id
- Return value: `true` = it was closed; `false` = that id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const containerClose = ll.import("HologramLib", "containerClose");
    containerClose(id);
    ```

### Close every container interface

`containerCloseAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const containerCloseAll = ll.import("HologramLib", "containerCloseAll");
    containerCloseAll();
    ```

---

## NPC dialogue (npcDialog*)

Sends down a dialogue interface (NPC name + body + button table) and synthesises an NPC carrier entity visible to that player only.
The button table is generated entirely server-side, per player — "decide the buttons from permissions / tags / quest state" simply means working
out the button string yourself before the call. Clicks and closes go through the polling queue `npcDialogPollClicks`.

Button-string format: buttons are separated by `;` (empty segments are skipped), and each button is split by `|` into
`label|actionId|mode|command1,command2` (the last three items may be omitted); `mode`: `0`=normal button `1`=close `2`=open; a `label` left
empty is displayed as "Button".

### Open an NPC dialogue

`npcDialogOpen(playerName, npcName, sceneName, dialogue, buttonsSpec, carrierIdentifier)`

- Parameters:
  - `playerName` : `String`<br>the target player's name, who must be online
  - `npcName` : `String`<br>the name shown inside the dialogue box (supports `§` colour codes); an empty string = the default name
  - `sceneName` : `String`<br>the scene name, which is how multi-level dialogue is routed (it comes back in the click report); an empty string = the default scene
  - `dialogue` : `String`<br>the body text
  - `buttonsSpec` : `String`<br>the button string (format at the top of this section)
  - `carrierIdentifier` : `String`<br>the carrier entity type; an empty string = `minecraft:npc`, and it must be in the NPC family (the client's dialogue interface is bound to that entity type, so swapping in a vanilla entity such as a villager will not bring it up)
- Return value: the dialogue id; `-1` = opening failed (the player is offline, the carrier could not be created)
- Return type: `Integer`
- Example:

    ```js
    const npcDialogOpen = ll.import("HologramLib", "npcDialogOpen");
    const id = npcDialogOpen("Steve", "§eVillage chief", "main", "Want a quest?",
                             "§aAccept|main#0|0|;;§cLeave|main#1|1|", "");
    ```

### Swap the body text and buttons in place

`npcDialogUpdate(id, dialogue, buttonsSpec)`

- Parameters:
  - `id` : `Integer`<br>the dialogue id
  - `dialogue` : `String`<br>the new body text
  - `buttonsSpec` : `String`<br>the new button string (same format as `npcDialogOpen`)
- Return value: `true` = it was re-sent; `false` = that dialogue is gone or the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const npcDialogUpdate = ll.import("HologramLib", "npcDialogUpdate");
    npcDialogUpdate(id, "Quest progress: 3/10", "§aHand in|main#0|0|");   // a tweak while the interface is still open
    ```

### Swap the dialogue avatar and model

`npcDialogSetAvatar(id, skinVariant, avatarViewSpec)`

- Parameters:
  - `id` : `Integer`<br>the dialogue id
  - `skinVariant` : `Integer`<br>the NPC's built-in skin variant, `0..59`; `-1` = default (no skin entry is sent)
  - `avatarViewSpec` : `String`<br>layers a view override on top of the carrier, with the same spec syntax as `viewEntity` (such as `"name=§6Barkeep"`); an empty string = no layer
- Return value: `true` = it was reopened; `false` = that dialogue is gone
- Return type: `Boolean`
- Example:

    ```js
    const npcDialogSetAvatar = ll.import("HologramLib", "npcDialogSetAvatar");
    npcDialogSetAvatar(id, -1, "name=§6Barkeep;always=1");
    ```

### Take and clear button click events

`npcDialogPollClicks()`

- Return value: each entry looks like `player=X dialogId=N scene=S button=I actionId=A closed=0|1 commands=...`; `button` = which button was clicked (`-1` = not a button event), `actionId` = the second segment of the button string carried back verbatim, `closed=1` = the player closed the dialogue, `commands` = the commands attached to the button (several are separated by `,`, and spaces and commas inside a command are replaced with `+`); an empty array when the queue is empty
- Return type: `Array<String>`
- Example:

    ```js
    const npcDialogPollClicks = ll.import("HologramLib", "npcDialogPollClicks");
    setInterval(() => {
      for (const line of npcDialogPollClicks()) {
        const m = line.match(/player=(\S+) dialogId=(-?\d+) scene=(\S*) button=(-?\d+) actionId=(\S*) closed=(\d)/);
        if (m && m[6] === "0") logger.info(`${m[1]} clicked button ${m[4]} (actionId=${m[5]})`);
      }
    }, 200);
    ```

### Discard dialogue queue clicks

`npcDialogClearClicks()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const npcDialogClearClicks = ll.import("HologramLib", "npcDialogClearClicks");
    npcDialogClearClicks();
    ```

### Check whether a dialogue still exists

`npcDialogIsOpen(id)`

- Parameters:
  - `id` : `Integer`<br>the dialogue id
- Return value: `true` = the dialogue exists; `false` = it is closed or does not exist
- Return type: `Boolean`
- Example:

    ```js
    const npcDialogIsOpen = ll.import("HologramLib", "npcDialogIsOpen");
    if (npcDialogIsOpen(id)) npcDialogUpdate(id, "……", "");
    ```

### List every open dialogue

`npcDialogGetIds()`

- Return value: an array of dialogue ids; an empty array when no dialogue is open
- Return type: `Array<Integer>`
- Example:

    ```js
    const npcDialogGetIds = ll.import("HologramLib", "npcDialogGetIds");
    for (const dialogId of npcDialogGetIds()) npcDialogClose(dialogId);
    ```

### Close one dialogue interface

`npcDialogClose(id)`

- Parameters:
  - `id` : `Integer`<br>the dialogue id
- Return value: `true` = it was closed; `false` = that id does not exist
- Return type: `Boolean`
- Example:

    ```js
    const npcDialogClose = ll.import("HologramLib", "npcDialogClose");
    npcDialogClose(id);
    ```

### Close every NPC dialogue

`npcDialogCloseAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const npcDialogCloseAll = ll.import("HologramLib", "npcDialogCloseAll");
    npcDialogCloseAll();
    ```

---

## Things to watch out for

- **The server does not have these interfaces.** A click only reports the slot number back and the item is not really taken; when you put an item into
  a virtual slot the real server-side slot does not match, so the client withdraws its prediction (the item bounces back where it was) — that is the default
  behaviour, not an error. Only with `containerSetInteractive` / `tradeSetSettle` (or `realTrades = true`) on does the inventory really change.
- **The events all sit in polling queues**: `containerPollClicks`, `npcDialogPollClicks` and `tradePollSettlements` are all "take and clear",
  so take from them continuously per tick or with `setInterval`; if you never take, they keep piling up in the queue.
- **Do not page through `containerOpen` again**: reopening closes the old menu first and restores the real block, so you pay the open delay once more
  every time. Paging is a content change within the same interface, so change it slot by slot with `containerSetItem`; to change only the title use
  `containerSetTitle`.
- **NPC dialogue paging must go through `npcDialogOpen` again**: after any button click the client collapses the interface on its own, and
  `npcDialogUpdate` cannot bring it back.
- **Commands on dialogue buttons do not run themselves**: the `commands` reported back by `npcDialogPollClicks` are only the text attached to the button,
  and a synthesised NPC has no server-side entity to run them for you — to execute them you have to do it yourself (for example `player.runCommand`).
- **Swapping the dialogue avatar's model has boundaries**: the `skin=` branch of `npcDialogSetAvatar`'s `avatarViewSpec` has been removed —
  once the carrier becomes a player model, clicking into the menu on that carrier makes the client close the interface; for a player model, create a
  separate fake player and hang the dialogue on its id.
- **A trade menu needs reliable settlement**: use `realTrades = true` (the server settles it itself) or switch to a virtual container.
  Under pure protocol-layer real settlement (`tradeSetSettle(id, true)`) the deduction and the product delivery are real, but the client may still
  withdraw the payment and the settle button may not respond — the slot correction the protocol requires does not line up in this protocol version,
  and writing it wrongly crashes the client, so it is not implemented.
- **Touch players**: under a pure protocol-layer trade interface the payment cannot be dragged into the trade slot (the client keeps bouncing it back),
  so use the real-trade-table path or a container interface. To route by device, query `sensingIsTouch` first (see [View override & sensing](view.md)).

## Related

- Domain guide: [Interfaces & containers](../domains/interfaces.md) (field-tested records of the open delay, interactive mode and the settlement limitations)
- Routing by device: [View override & sensing](view.md)
- The full C++ side: [C++ API reference](../reference/cpp-api.md)
