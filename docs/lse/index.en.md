# LSE scripting API

On the script side HologramLib occupies **a single namespace**, `HologramLib`: once the prerequisite plugins are installed, call `ll.import("HologramLib", "functionName")` to get a function and call it directly. There is no dependency to declare in your script plugin's `manifest.json`, and no configuration file.

This page shows you how to get it running. The individual functions are paged by domain — see the directory at the end.

---

## 1. Prerequisites

| Install | Role | Notes |
|---|---|---|
| LeviLamina | The server itself | — |
| LegacyRemoteCall (`lrca`) | The bridge between the script engine and C++ plugins | HologramLib probes for it at startup; if the probe fails it tries again once the server has finished starting |
| legacy-script-engine-quickjs or nodejs | Runs your `.js` scripts | Either one; the interface is the same |

**Load order does not matter**: the library looks for `lrca` inside `enable()` and, if it cannot find it, waits for the `ServerStarted` event as a safety net, so whichever loads first, they still connect.

---

## 2. Three-line quick start

```js
const holoCreate = ll.import("HologramLib", "holoCreate");   // get the function
const id = holoCreate(0, 70, 0);                             // create a hologram 70 blocks above the world origin
ll.import("HologramLib", "holoAddLine")(id, "§eHello");      // add a line of text
ll.import("HologramLib", "holoDraw")(id);                    // draw it for everyone
```

`ll.import` gives you **the function itself**, so call it directly. Storing it in a variable first, as in the first line above, is recommended: it saves looking the function up on every call.

---

## 3. You cannot get the function / the call does nothing

| Symptom | Cause | What to do |
|---|---|---|
| `ll.import(...)` returns `null` | The library is not loaded, `lrca` is not loaded, or the function name is misspelled | First confirm the log contains `Loading HologramLib`; then check the function name, case included, against the pages in this directory |
| The call returns `false` / `-1` | Invalid arguments (a coordinate is not a number, the id does not exist, the player is offline …) | Read that function's "Return value" section |
| You called it but nothing appears on screen | You created it but never displayed it | Most domains need **a separate display call** (such as `holoDraw` / `shapeDraw` / `entitySpawn`), or the content passed in as an argument to the create function |
| Everything disappears after a player reconnects | The client persists nothing | `draw` it again in `PlayerJoin` (the library does not decide the resend timing for you) |

To find out whether a function exists in your copy of the library, use `ll.hasExported`:

```js
if (ll.hasExported("HologramLib", "viewItemSpawnHooked")) {
  // this version supports re-typing dropped items
}
```

---

## 4. A full example

Drop this into the `.js` file that `plugins/my-script/manifest.json` points at (`type` is `lse-quickjs` or `lse-nodejs`):

```js
const holoCreate = ll.import("HologramLib", "holoCreate");
const holoAddLine = ll.import("HologramLib", "holoAddLine");
const holoDraw = ll.import("HologramLib", "holoDraw");
const holoDestroy = ll.import("HologramLib", "holoDestroy");
const shapeCreateBox = ll.import("HologramLib", "shapeCreateBox");
const shapeDraw = ll.import("HologramLib", "shapeDraw");

// A player joins: hang a line of text above their head and draw a box at their feet
mc.listen("onJoin", (player) => {
  const pos = player.pos;                       // FloatPos

  const holo = holoCreate(pos.x, pos.y + 2.5, pos.z);
  holoAddLine(holo, `§eWelcome, ${player.realName}`);
  holoDraw(holo);

  const box = shapeCreateBox(pos.x - 1, pos.y, pos.z - 1, pos.x + 1, pos.y + 1, pos.z + 1);
  shapeDraw(box);

  // The player leaves: clean up after yourself, leave no litter
  mc.listen("onLeft", (p) => {
    if (p.xuid === player.xuid) {
      holoDestroy(holo);
    }
  });
});
```

> The library **does no business logic**: commands, permissions and persistence all live in your script. It also writes nothing to disk — after a script restart you have to create everything again yourself.

---

## 5. How to get click / interaction events (polling)

Events from the interface domains (container, NPC dialogue, trade) and from ghost interactions **do not go into callbacks**: they go into queues, and you have to poll them yourself on a timer or per tick. One call **takes the queue away and clears it**, and an entry is a line built from `key=value` fields:

| Polling function | Entry format (example) |
|---|---|
| `containerPollClicks` | `player=Steve menuId=3 slot=13 closed=0` |
| `npcDialogPollClicks` | `player=Steve dialogId=1 scene=main button=0 actionId=accept closed=0 commands=…` |
| `tradePollSettlements` | `player=Steve menuId=3 offer=1 ok=1 reason=` |
| `fakeInvPollClicks` | `player=Steve slot=5` |
| `ghostPollInteractions` | `player=Steve action=1 domain=npc id=42 pos=(100.50,65.00,100.50)` |

```js
const containerPollClicks = ll.import("HologramLib", "containerPollClicks");

setInterval(() => {
  for (const line of containerPollClicks()) {       // takes and clears
    const f = Object.fromEntries(                     // simple parse: split on spaces into key=value
      line.split(" ").map(kv => kv.split("=", 2)),
    );
    logger.info(`player ${f.player} clicked slot ${f.slot}`);
  }
}, 100);
```

> Fields are separated by spaces; `npcDialogPollClicks`'s `commands=…` may itself contain spaces, so parse by splitting at `key=` rather than on spaces alone. Each function's page gives the full field list.

---

## 6. Debugging

- Type `jsdebug` (or `luadebug`) into the server console to enter interactive debug mode and run script lines immediately; type it again to leave.
- HologramLib **writes no log output of its own** (a normal release build does not even compile the log strings into the binary). When troubleshooting, read the function's return value, or confirm the basic path works with a simple `shapeCreateText` first.
- If the picture looks wrong, first check the coordinates and the dimension on a single-player / local server: every coordinate is a **world coordinate**, and the dimension parameter is `0` (overworld) / `1` (nether) / `2` (the End).

---

## 7. Look up functions by domain

| I want to | Prefix | Functions | Page |
|---|---|---|---|
| Draw lines / boxes / spheres / text shapes | `shape*` | 39 | [Shapes](shapes.md) |
| Multi-line holograms, gradient lines | `holo*` · `gradient*` | 26 + 11 | [Holograms & gradient lines](holograms.md) |
| Show "Diamond x64", item display entities | `itemDetail*` · `itemDisplay*` | 2 + 33 | [Item display](items.md) |
| Protocol-level fake players (NPCs), skin registration / capture | `playerNpc*` | 29 | [Fake-player NPCs](player-npc.md) |
| Custom entities, particle shapes, interaction events | `entity*` · `particle*` · `ghost*` | 37 + 22 + 2 | [Entities, particles & interaction](entities.md) |
| Trade menu, virtual container, NPC dialogue | `trade*` · `container*` · `npcDialog*` | 10 + 10 + 9 | [Interfaces & containers](interfaces.md) |
| Fake inventory, sulfur-cube display | `fakeInv*` · `sulfur*` | 9 + 10 | [Inventory & sulfur cube](inventory.md) |
| Client view override (swap model / hide / re-skin), device sensing | `view*` · `sensing*` | 18 + 3 | [View override & sensing](view.md) |

270 functions in total, all registered at runtime, and `ll.import` can fetch every one of them.

---

## 8. A few general conventions

- **Every coordinate is a world coordinate** (floating point); the dimension is the integer `0/1/2`.
- **Every id is an `Integer`**: the create functions return an id, which you then use to set properties or to destroy it; a failure returns a negative number or `0` (each function's page says which).
- **`§` colour codes** and `\n` line breaks go straight into the text, for example `"§cfirst line\n§7second line"`.
- **No declared dependency needed**: your script plugin's `manifest.json` only has to depend on the LSE engine itself.
- The C++ plugin side has the same set of capabilities (the interfaces correspond one to one); see the [C++ API reference](../reference/cpp-api.md).
