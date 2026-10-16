# Domain guides · interfaces & containers

Covers 4 domains: the **trade menu**, the **virtual container**, the **fake inventory**, and **NPC dialogue**.

All four domains do the same counter-intuitive thing: **they make the client open a real vanilla interface even though that interface does not exist on the server**. Once that lands, every later "why does it roll back" and "why do I have to click twice" has an answer.

| Domain | C++ | LSE | In one line |
|---|---|---|---|
| Trade menu | `ITradeMenu` | `trade*` · 10 | The protocol layer opens the trade interface with `UpdateTrade` |
| Virtual container | `IContainerMenu` | `container*` · 10 | Client-side chest block + block-entity NBT, 27 / 54 slots |
| Fake inventory | `IFakeInventory` | `fakeInv*` · 9 | Rewrites the player inventory content the client sees |
| NPC dialogue | `INpcDialogue` | `npcDialog*` · 9 | `NpcDialoguePacket` + a synthetic NPC carrier |

---

## The shared boundary of all virtual interfaces

**The server does not have this container / this trade table.** Therefore:

| Player action | What happens |
|---|---|
| Click an entry / click an item | The library gets the callback and reports the slot number back; **the item is not really taken** |
| Put an item into a virtual slot | The real server-side slot does not match → BDS fails it → **the client rolls back its prediction** (the item bounces back to where it was) |
| The interface closes | The library calls back `closed = true` (`slot = -1` at that point) |

**The library does not answer with a failure response on the server's behalf.** A matched action is only reported back, never intercepted: once it is let through, BDS itself fails it and makes the client roll back, and the callback still arrives as usual.
If you answered with a failure response yourself (`ItemStackNetResult` 3), the client **pops up an error message**; handing it to BDS to walk its own failure path is **silent** instead.

---

## Virtual container (a list interface)

**If all you want is a "list interface where clicking fires a callback", this is the domain.** A click here is an item pickup, so **every input device sends packets** (touch works too) — that is its core advantage over the trade menu.

```cpp
hologramlib::ContainerMenuSpec menu;
menu.title = "§8任务列表";          // "quest list"
menu.rows  = 3;                    // 3 = small container, 27 slots; 6 = large container, 54 slots
menu.items.resize(27);
menu.items[0].type = "minecraft:diamond_sword";
menu.items[0].name = "§b试炼";      // "trial"
menu.items[0].lore = {"§7点击领取"};   // "click to claim"

auto menuId = lib.containerMenus().open("Steve", menu);
lib.containerMenus().addClickListener([](hologramlib::ContainerClickEvent const& e) {
    if (e.closed) return;          // interface closed (slot = -1)
    // e.slot = index of the entry that was clicked
});
```

### How it is implemented

Replicated item by item from the reference implementation, **GMLIB's `ChestUI`**:

| Step | Packet |
|---|---|
| Place a chest block on the client side | `UpdateBlock` |
| Write the block-entity data | `BlockActorData` — entries travel in the `Items` list, the title in `CustomName` |
| Open the interface | `ContainerOpen` (type `Container(0)`, bound to the block coordinates, target entity `-1`) |

`rows = 3` → a single chest, **27 slots**; `rows = 6` → a large chest, **54 slots** (with `pairx` / `pairz` / `pairlead` pairing, each half carrying `Slot` 0..26).
The **slot number reported back by a click = the entry index**.

> **The block exists only on the client.** There is no such chest block in the server-side world or in the save.

### The open delay: where the 7 ticks come from (measured)

The sequence for opening a container once is "place the block + block entity on the client side" → wait N ticks → send `ContainerOpen`. That wait is **not a mistake made while porting**:

| Implementation | Sequence | Time to open |
|---|---|---|
| GMLIB `ChestUI::sendTo` | `updateBlock` + `updateBlockActor` → **`ticks(10)`** → `ContainerOpen` → **`ticks(4)`** → fill in the player inventory / cursor slot by slot | **14 ticks ≈ 700ms** |
| This library, `ContainerMenuManager` | place block + block entity → **`ticks(openDelayTicks)` (7 by default)** → `ContainerOpen` | **7 ticks ≈ 350ms** |

This library uses 7 ticks fewer than the reference implementation: entries live in the **block-entity NBT**, which removes GMLIB's per-slot fill step (4 ticks); the remaining wait was itself squeezed to the measured lower bound (3 ticks).

**That wait is inherent to the approach**: the client has to apply this block and its block entity first, and only then can `ContainerOpen` bind to this position (sending back to back does not open the interface in practice).

> **Measured (26.40, local client): `openDelayTicks = 7` opens the interface normally, `6` does not.**
> The lower bound depends on the client / machine / load, and may have to be raised in another environment, so it stays a tunable (`ContainerMenuSpec::openDelayTicks`, default `7`).

### Three tunables (plus one trap: do not use `open` for paging)

| You want to | Use this | Cost |
|---|---|---|
| Page / replace a whole page of content | `update(menuId, spec)` | **No delay** — it reuses the same carrier block, does not tear the interface down, does not re-place the block, does not wait; it only resends the block-entity NBT + `ContainerOpen` |
| Change one slot (a tick mark / a count / a price) | `setItem(menuId, slot, item)` | **No delay, no flicker** — it sends a single `InventorySlotPacket` and the client swaps that slot in place; an empty `item.type` clears the slot |
| Change only the title | `setTitle(menuId, title)` | Internally it is "edit the spec, then `update`" |

> ⚠ **Do not page through `open()`.** `open()` first closes the old menu and restores the real block, so **you pay the open delay again every time** (≈350ms). Paging is a content change within the same interface: use `update()`.

### Interactive mode (off by default)

```cpp
menu.interactive = true;   // or containerSetInteractive(menuId, true)
```

Once it is on, when the client drags / swaps / splits items **inside** the container, the library **catches that request and answers success itself** (it sends `ItemStackResponse` Success) and records the change into the entry table — the item really stays in its new slot and is no longer bounced back by BDS.

With the default (off) the behaviour is exactly "the click is only reported back, the item bounces back to where it was".

**Only actions whose whole request lands inside this container are caught**; actions that involve the player inventory are still let through to BDS — the server does not have this container, and that is the inherent boundary of the real inventory (if you really want to change the inventory along with it, combine it with the `IFakeInventory` route).

### Item requests travel over two packet channels (measured, 26.40)

The client's `ItemStackRequest` **does not have just one way of being sent**:

| Channel | Form | Who uses it |
|---|---|---|
| `ItemStackRequestPacket`(147) | A standalone request, with the action in cereal form | Taking from / putting into container slots (the **main channel** in practice) |
| `PlayerAuthInputPacket`(144, AuthInput) | **Embedded** `mItemStackRequest` (input flag `PerformItemStackRequest = 36`) | Interactions in menu interfaces (measured: **on touch there may not be a single 147**, only this one) |

The library **hooks both** (`src/interaction/PlayerInteractionHooks.cpp`): the AuthInput one uses BDS's own `ItemStackRequestCereal::toActionData()` to convert the parsed actions into the same cereal form as 147, then feeds them into the same dispatch function.

> **AuthInput only observes, it never intercepts** — it also carries player movement, so intercepting it would swallow the movement as well; and the embedded request is handled by BDS itself.
>
> This is also why "virtual container clicks" used to be missed entirely on touch: before `26.40.3` both hooks only engaged while a **trade** menu was open.

### How to get the clicks

```cpp
lib.containerMenus().addClickListener(cb);   // C++: a callback
```

```js
// LSE: a polling queue (drains and clears it); each entry is "player=X menuId=N slot=S closed=0|1"
for (const line of containerPollClicks()) { /* split it yourself */ }
```

The queue and the C++ listener receive the **same event**.

> `useMinecart = true` keeps the old path alive (it only spawns a synthetic chest minecart). **The interface does not come up on the client in practice**; it is there only for comparison and troubleshooting — do not use it.

---

## Trade menu

The protocol layer builds its own `UpdateTrade` to open the interface: the packet-level fields and the Offers NBT are both **byte-compared** against BDS captures. An invisible carrier entity drives the experience bar.

### Three paths

```cpp
hologramlib::TradeMenuSpec trade;
trade.tradeType = "entity.villager.butcher";
// ...
auto tradeId = lib.tradeMenus().open("Steve", trade);
```

| `usePacketOffers` | `settleLocally` | Behaviour |
|---|---|---|
| `true` (default) | `false` (default) | **Display only** — it only sends the self-built `UpdateTrade`; the server registers no trade table, so payment cannot be placed into a slot and clicking does not settle (naturally read-only) |
| `true` | `true` | **Real settlement, pure protocol** — the library catches the client's place-payment / take-back / settle requests and really deducts from the inventory and really delivers the product (see the limitations below) |
| `false` | no effect | The carrier gets a **real trade table** and BDS `openTrading` is used, **settlement is performed by BDS** (items are really consumed; **the full flow works on touch as well**) |

> **For reliable settlement, use `usePacketOffers = false` or a container UI.** Reasons below.

On both paths `UpdateTrade` is sent **a few ticks later**: the carrier entity reaches the client asynchronously, so if the packets are sent back to back the client does not know that entity yet and the interface cannot bind to it.

### Real settlement, pure protocol (`settleLocally = true`)

The interface is still opened by the self-built `UpdateTrade` (the server registers **no** trade table), but the library catches the client's place-payment / take-back / settle requests and lands them itself: it really deducts the payment from the player's inventory and really writes the product into the inventory, and **it refunds the payment that was held but never spent when the interface closes**.

The decision logic is isolated in `src/trade/TradeSettlementLogic.h` (it touches no BDS types; offline self-check `tests/check-trade-settlement.bat`, 43 assertions), and the layer that touches the inventory is `TradeMenuManager`.
Results come back through `TradeSettlementEvent` (a C++ listener / LSE `tradePollSettlements`).

> ### ⚠ Known limitation (measured, still not working end to end)
> The client requires the server to send back a **slot correction** for the trade slot in the item response (keyed by the item network id the client assigned). That section is **inconsistent between this protocol version and the protocol library's `sculk` wire format** (the protocol library writes one extra string field and writes `DurabilityCorrection` as a varint, while the client wants a fixed-length `short`) — hand-writing it after the protocol library **crashes the client outright** (we have stepped on this).
>
> So: even though placing the payment is accepted server-side (the deduction, the holding and the refund on close are real), the client interface may still roll back that placement (it shows up as "it goes into the trade slot and then bounces back to the inventory", and the settle button does nothing).
> **For reliable settlement use `usePacketOffers = false` (BDS settles it itself) or a container UI.**

Other boundaries found in practice:

- **Touch cannot reach settlement** — under the pure protocol layer the payment cannot be dragged into the trade slot (the client keeps bouncing it back). On touch use the real-trade-table path or a container UI; keyboard-mouse / gamepad are fine.
- The client **sends no packet if it cannot afford it** (with no payment item the client sends not a single request) — that is the client's own decision, not something the library intercepts.
- The ledger deducts by **type + count**, **not broken down per trade slot**; when several trades of the same payment type share the payment slots, the ones placed first are the ones deducted.
- The `numCrafts` carried in the packet (how many times you clicked at once) is **not expanded**: one settlement delivers one product.

### Displayed tier values are 1-based

`TradeMenuOffer`'s displayed tier value is **1-based** (`1` = novice … `5` = master); on the wire it is `0..4`, converted and clamped by the library. `locked = true` can force one offer to display as locked.

The experience bar is driven by the carrier entity's `TradeTier` / `MaxTradeTier` / `TradeExperience` metadata — all three are `Int`, matching a real villager spawn packet (`minecraft:villager_v2`) in practice, and **`MaxTradeTier` is always 4** (master).

`TradeMenuSpec::carrierIdentifier` can be a villager or a wandering trader.

### Refreshing content dynamically

```cpp
lib.tradeMenus().addOffer(tradeId, anotherOffer);   // append one offer
lib.tradeMenus().setTier(tradeId, 2, 40);           // change the tier / experience bar
```

Both **resend the trade table in place** — they reuse the same interface and carrier, with no reopen and no waiting for the delay.

> **This domain has no click callback.** The three sets of click listeners that briefly existed in `26.40.3` (`addClickListener` / `addActionListener` / `addRawActionListener`, together with `TradeClickEvent` and `TradeRawAction`) were **removed wholesale** in `26.40.4` (withdrawn on request after release). If you want "click an entry and get a callback", use the **virtual container**.

---

## Fake inventory

Makes the client "believe it is carrying these items in its inventory".

```cpp
hologramlib::FakeInventorySpec bag;
bag.items.resize(hologramlib::kFakeInventorySlots);   // 36 slots: 0..8 hotbar, 9..35 main inventory
bag.items[0].type = "minecraft:diamond_sword";
bag.refreshIntervalTicks = 20;                        // periodic resend (default 20 ticks = 1s)
lib.fakeInventories().apply("Steve", bag);
lib.fakeInventories().clear("Steve");                 // undo it and restore the real inventory
```

A single `InventoryContentPacket(49)` (`ContainerId = Inventory(0)`, `FullContainerId = InventoryContainer(29)`, descriptors 0..35) replaces the whole content; single-slot changes go through `InventorySlotPacket(50)`. **Not a single thing in the server-side inventory is touched.**

The approach comes from the reference implementation, GMLIB's `ChestUI` (when it fills the player inventory it writes `InventorySlot` slot by slot, with the same container id, `Inventory(0)`).

### A lesson learned the hard way: item serialisation must be handed to BDS

> The User Data inside an item descriptor uses **a different NBT dialect** (name length as little-endian u16 / integers and counts as 4-byte little-endian), whereas sculk's `CompoundTag` writes the varint dialect (that is the dialect used by `UpdateTrade`'s Offers and by the block-entity NBT, and both have been byte-compared).
>
> **The first hand-written descriptor therefore wrote the wrong dialect → the client's parse ran out of bounds and it froze outright**, and the session was not cleaned up on disconnect, so on reconnect the tick driver resent it → the symptom was "**cannot get into the server**".

This domain's packet bodies are now constructed with **BDS's own** `InventoryContentPacket` / `InventorySlotPacket` (`src/fakeinv/FakeInventoryPackets.h`), and the container name and storage-item values follow the captures (BDS writes container-name byte `0` and an empty storage descriptor).
The remaining domains (trade menu / virtual container / NPC dialogue) still hand-write all their packet bodies.

Alongside that: `PlayerDisconnectEvent` is registered to clean the session up (`FakeInventoryManager::onPlayerLeave`), and the periodic resend interval has its lower bound clamped (5 ticks at the fastest).

### Coexisting with the trade menu / virtual container

When a trade interface or a chest interface opens, **BDS resends the inventory content** (the fake content gets overwritten), so this domain resends once every `refreshIntervalTicks` by default, and you can also top it up manually with `refresh()` — the inventory area seen in the interface is therefore always the fake content, and clicks still fire callbacks as usual.

### Boundaries (spelled out up front so they are not misused)

- The fake content **only affects what the client displays**; what the player can really use / eat is the real server-side item.
- When the **real item and the faked item in a slot happen to be identical**, that operation is **really executed** by the server — if you want a slot to be display-only, **do not fake it as the same thing as the real item**.
- When the real item in that slot changes for some other reason (picking something up / using an item / another plugin changing the inventory), the periodic resend **writes the fake content back over it**.
- **Armour (6) / offhand (34) are outside this domain**, as each has its own container enum.

### How the hook splits the traffic

The item-request hook is **shared** by the container domain and this domain (`src/interaction/PlayerInteractionHooks.cpp`); they are told apart by container id and do not fight over packets:

| Domain | Container ids it claims |
|---|---|
| Container domain | `LevelEntityContainer(7)` and dynamic ids `101..199` |
| Inventory domain | `CombinedHotbarAndInventory(12)` / `Hotbar(28)` / `Inventory(29)` |

---

## NPC dialogue

```cpp
hologramlib::NpcDialogSpec dlg;
dlg.npcName = "§e村长";                              // "village chief"
dlg.dialogue = "要来点任务吗？";                        // "want a quest?"
dlg.buttons.push_back({"§a接受", {}, "main#0", 0});   // "accept"
auto dialogId = lib.npcDialogs().open("Steve", dlg);
```

It sends down an `NpcDialoguePacket` (scene name + body + NPC name + button JSON) and synthesises a `minecraft:npc` **carrier entity**.
When the client clicks a button / closes it, it reports back (`NpcRequestPacket`), and the callback gives you `sceneName` + `buttonIndex` + `actionId` + `commands` + `closed`.
**Multi-level dialogue is simply "the click report carries the `sceneName`, and the caller sends the next level from that"**.

### Three points measured in practice

**1. The carrier must be below the world (`y = -66`)**
The client cannot see the entity, but the avatar in the dialogue box **still renders as usual**. Switching to the "invisible flag" makes the avatar **disappear along with it** (measured).

**2. `update()` cannot change the page — after a button click you must use `open()`**

> **Clicking any button makes the client collapse the interface on its own**, and `INpcDialogue::update()` only resends `NpcDialoguePacket(Open)` once — the client **does not** pop it back up because of that.
> So "click a button and change the page in place" is **not workable** for NPC dialogue: to make the client show it again you must go through `open()` (delete the old carrier + create a new carrier + send Open once more).
> `update()` is only good for fine-tuning content **while the interface is still open**.

**3. A synthetic NPC does not run commands on your behalf**
There is no BDS entity on the server side to execute them: `NpcDialogButton::commands` is only **displayed on the button**. To execute them the caller has to run them itself (for example `player.runCommand`).

### Avatar customisation (`26.40.8`)

Two parameters are appended at the tail of `NpcDialogSpec`:

| Parameter | Effect |
|---|---|
| `avatarSkinVariant` | The NPC's **built-in skin variant** `0..59` → written into the carrier's ActorData as `SkinId(104)`; `-1` = do not add that entry (**byte-identical** to the old behaviour) |
| `avatarViewSpec` | **Layers an `IViewOverride`** on top of this carrier, with the same spec syntax as `viewEntity`: `type=` changes the carrier type (test matrix: zombie / chicken), `name=` changes the interface title |

On the LSE side `npcDialogSetAvatar(id, skinVariant, viewSpec)` updates in place + reopens (the same zero-flicker path as paging).

> **The `skin=` branch of `avatarViewSpec` has been removed** (a shrink within the `26.40.9` batch, explicitly recorded).
> It would have switched the carrier to a "player model" (`PlayerList(Add)` + `AddPlayer`), and in practice the client **closes the interface** when you click into the menu on that carrier (in the same test the `type=` carriers — zombie / chicken — were all fine).
> Given the "the library only provides mechanisms" positioning, the "make the client believe it is a player" route is left to the consumer to build: use `playerNpcs()` to create a real fake player + `npcUniqueIdOverride` to hang the dialogue on its id.
> You can also take the `EntityView::skinId` route, **swapping the model while keeping the same id**.
> **The `avatarViewSpec` / `avatarSkinVariant` fields themselves do not move — the ABI is unchanged and consumers do not need to recompile.**

### On the script side

```js
// Button string = label|actionId|mode|command1,command2 (buttons separated by ;)
// the sample strings read "village chief" / "want a quest?" / "accept" / "leave"
npcDialogOpen("Steve", "§e村长", "main", "要来点任务吗？", "§a接受|main#0|0|;;§c离开|main#1|1|", "");

// Clicks go through a polling queue: each entry is "player=X dialogId=N scene=S button=I actionId=A closed=0|1 commands=..."
for (const line of npcDialogPollClicks()) { /* split it yourself */ }
```

---

## Related

- Full signatures: [C++ API reference](../reference/cpp-api.md) (§1.10 trade menu / §1.11 NPC dialogue / §1.12 virtual container / §1.14 fake inventory)
- LSE function list: [LSE API reference](../reference/lse-api.md) (§2.10 ~ §2.14)
- Why these packets are written the way they are: [Protocol notes](../architecture/protocol-notes.md)
- Routing the interface by the player's device (touch → container, keyboard-mouse → trade): [Player sensing](view-sensing.md#player-sensing)
