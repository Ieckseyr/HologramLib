# Easter egg: creeper swelling (`easterEgg*`)

A purely client-side effect: make an entity or a display object "swell up like a creeper about to explode".

The server only sends the instruction — no damage, no explosion, no status effect, nothing written to the save.
The client animates the swelling from `q.life_time`.

## Entities

```js
const rid = actorRuntimeIdOfPlayer("Steve");   // or a custom entity / NPC runtime id
easterEggCreeper(rid);
```

- Sends one `AnimateEntity`(158): `animation.creeper.swelling` plus a fixed stop expression; the client then evaluates it
  every frame, so the motion is smooth.
- Limitation: that animation only drives the bones it names. Player and most mob models do not match, so **the lower body
  does not move** (only the upper body pulses). That is a client-model limitation; pushing scale metadata every tick makes
  the whole body move but at 20 discrete updates per second it looks choppy, so the client-side animation stays.
- For a whole-body, non-uniform jelly effect use the display-block version below: `stretch` is horizontal/vertical by
  design and is also evaluated client-side, so it stays smooth.
- Rotation is not supported (the local player's model orientation is locked to the client camera; `MovePlayer` cannot take
  it back).
- **It only plays if the client can find `animation.creeper.swelling` in that entity's animation set** — the creeper family.
  To make other mobs do it, first turn them into creepers client-side with [view override](view.en.md), or spawn a fake
  creeper-typed entity.
- Signature: `easterEggCreeper(runtimeId, spin)`; get the runtime id from `actorRuntimeIdOfPlayer` / `playerNpcRuntimeId` /
  `actorRuntimeIdOfEntity` / `itemDisplayRuntimeId`.

## Display blocks / item displays

```js
const id = itemDisplayCreate("minecraft:creeper_head", 100.5, 64, 100.5);
easterEggCreeperDisplay(id, 80, true);   // 80 ticks ≈ 4 s; third argument = rotate too
```

- Display blocks already play `animation.creeper.swelling` (the library builds the expression from `stretch`); this call
  swaps `stretchXZ` / `stretchY` for expressions driven by `q.life_time`:
  `1.0-0.1*math.max(sin,0)+0.15*math.max(-sin,0)` / `1.0+0.1*math.max(sin,0)-0.4*math.max(-sin,0)`.
- With `rotate=true` it also swaps `rotY` for `q.life_time*720` — the same angular term as the swelling, one full turn per swell.
- When the duration ends the library's tick restores the previous `stretch` / `rotY` automatically.
- Signature: `easterEggCreeperDisplay(displayId, durationTicks, rotate)`; `durationTicks <= 0` means 60 ticks.

## Notes

- Both functions only change what the client sees; server entities, blocks and inventories are untouched.
- The display version relies on the swelling animation display blocks already play; plain item displays work too, since
  the stretch expression is applied in both modes.
- Calling it again on the same display resets the timer instead of stacking.
