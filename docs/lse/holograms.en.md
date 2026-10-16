# Holographic text & gradient lines · Multi-line text boards and multi-colour trail lines

This page has two parts: `holo*` for visible multi-line text boards (colour codes, `{variables}`, content rotation, following a player), and `gradient*` for drawing a trail line that fades from one end to the other (or cycles through a rainbow). Both exist **on the client only**: they occupy no server-side entity and write nothing to the save.

The model for holographic text is "multiple lines = one whole block": every line shares one colour, scale, background box, through-walls switch and orientation; to give one line another colour, write a `§` colour code in that line's text. Line numbers start at `0` and follow the order the lines were added in.

## Quick start

```js
const holoCreate   = ll.import("HologramLib", "holoCreate");
const holoAddLine  = ll.import("HologramLib", "holoAddLine");
const holoSetScale = ll.import("HologramLib", "holoSetScale");
const holoDraw     = ll.import("HologramLib", "holoDraw");

const holo = holoCreate(100.5, 66, 100.5);
holoAddLine(holo, "§eWelcome to spawn");  // the § colour code only affects this line
holoAddLine(holo, "Online: {online}");    // {online} is replaced with the current online count
holoSetScale(holo, 1.5);                  // scale up the whole block
holoDraw(holo);                           // show it to every online player
```

The smallest useful gradient line:

```js
const gradientCreate = ll.import("HologramLib", "gradientCreate");
const gradientDraw   = ll.import("HologramLib", "gradientDraw");

const trail = gradientCreate(100, 65, 100, 130, 70, 130, 20);   // built from 20 segments
gradientDraw(trail);                                            // red → blue by default
```

## Holographic text: creation

### Create a holographic text at a coordinate

`holoCreate(x, y, z)`

- Parameters:
  - `x` : `Float`<br>the world coordinate X the holographic text sits at
  - `y` : `Float`<br>the world coordinate Y the holographic text sits at
  - `z` : `Float`<br>the world coordinate Z the holographic text sits at
- Return value: the holographic-text id (incrementing from `1`); `<= 0` means creation failed
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "holoCreate");
    const holo = create(100.5, 66, 100.5);   // not shown yet; add the lines first, then draw
    ```

### Delete a holographic text and sync the client

`holoDestroy(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: `true` when the deletion succeeds; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const destroy = ll.import("HologramLib", "holoDestroy");
    destroy(holo);
    ```

### Delete every current holographic text

`holoDestroyAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const destroyAll = ll.import("HologramLib", "holoDestroyAll");
    destroyAll();      // tidy up when the plugin unloads or reloads
    ```

## Holographic text: lines and content

### Append a line of text at the end

`holoAddLine(id, text)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `text` : `String`<br>the text of this line; it may use `§` colour codes and `{variables}`
- Return value: `true` when the append succeeds; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "holoCreate");
    const addLine = ll.import("HologramLib", "holoAddLine");
    const draw = ll.import("HologramLib", "holoDraw");
    const holo = create(100.5, 66, 100.5);
    addLine(holo, "§eFirst line");
    addLine(holo, "§7Second line");
    draw(holo);        // adding a line while it is on screen refreshes it immediately, no need to draw again
    ```

### Replace the text of one line

`holoSetLineText(id, lineIndex, text)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `lineIndex` : `Integer`<br>the line number, starting at `0`
  - `text` : `String`<br>the new text
- Return value: `true` on success; `false` when `id` does not exist or the line number is out of range
- Return type: `Boolean`
- Example:

    ```js
    const setLineText = ll.import("HologramLib", "holoSetLineText");
    setLineText(holo, 1, "§7Online: {online} players");   // change line 2
    ```

### Give one line a pool of rotating contents

`holoSetLinePool(id, lineIndex, content, intervalMs)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `lineIndex` : `Integer`<br>the line number, starting at `0`
  - `content` : `Array<String>`<br>the array of candidate contents; passing an empty array = clear the content pool, and this line goes back to the text set by `holoSetLineText`
  - `intervalMs` : `Integer`<br>the rotation interval, in **milliseconds**. With only 1 entry in the pool, or with `intervalMs <= 0`, there is no rotation and entry 0 is shown permanently
- Return value: `true` when the setting succeeds; `false` when `id` does not exist or the line number is out of range
- Return type: `Boolean`
- Example:

    ```js
    const setLinePool = ll.import("HologramLib", "holoSetLinePool");
    const pool = ["§aOnline: {online}", "§eTPS: {tps}", "§bTime: {time}"];
    setLinePool(holo, 0, pool, 2000);   // change entry every 2 seconds
    ```

### Set whether one line parses variables

`holoSetLineParseVariables(id, lineIndex, enabled)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `lineIndex` : `Integer`<br>the line number, starting at `0`
  - `enabled` : `Boolean`<br>`true` = parse variables (the default); `false` = the `{}` on this line is shown literally, as it is written
- Return value: `true` when the setting succeeds; `false` when `id` does not exist or the line number is out of range
- Return type: `Boolean`
- Example:

    ```js
    const setLineText = ll.import("HologramLib", "holoSetLineText");
    const setParse = ll.import("HologramLib", "holoSetLineParseVariables");
    setLineText(holo, 0, "§7Syntax example: {online}");
    setParse(holo, 0, false);      // the screen shows the eight characters "{online}" too
    ```

### Delete one line and move the lines after it up

`holoRemoveLine(id, lineIndex)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `lineIndex` : `Integer`<br>the line number to delete, starting at `0`
- Return value: `true` when the deletion succeeds; `false` when `id` does not exist or the line number is out of range
- Return type: `Boolean`
- Example:

    ```js
    const removeLine = ll.import("HologramLib", "holoRemoveLine");
    removeLine(holo, 0);   // delete line 1; the old line 2 becomes line number 0
    ```

### Clear every line and take the block off screen

`holoClearLines(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const clearLines = ll.import("HologramLib", "holoClearLines");
    clearLines(holo);      // after clearing you can add lines again to lay the content out anew
    ```

### Query the current line count

`holoGetLineCount(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: the line count; returns `0` when `id` does not exist
- Return type: `Integer`
- Example:

    ```js
    const getLineCount = ll.import("HologramLib", "holoGetLineCount");
    logger.info(`This holographic text has ${getLineCount(holo)} lines`);
    ```

## Holographic text: whole-block style

### Set the colour of the whole text block

`holoSetColor(id, r, g, b, a)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `r` : `Float`<br>the red component, `0.0 ~ 1.0`
  - `g` : `Float`<br>the green component, `0.0 ~ 1.0`
  - `b` : `Float`<br>the blue component, `0.0 ~ 1.0`
  - `a` : `Float`<br>the alpha component, `0.0 ~ 1.0`
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setColor = ll.import("HologramLib", "holoSetColor");
    setColor(holo, 1.0, 0.9, 0.3, 1.0);   // the whole block turns gold
    ```

### Set the scale of the whole text block

`holoSetScale(id, scale)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `scale` : `Float`<br>the scale factor, which must be **greater than 0** (`1` = the original size)
- Return value: `true` on success; returns `false` when `id` does not exist or `scale <= 0`
- Return type: `Boolean`
- Example:

    ```js
    const setScale = ll.import("HologramLib", "holoSetScale");
    setScale(holo, 2);     // twice the size
    ```

### Set the colour of the board behind the text

`holoSetBackgroundColor(id, r, g, b, a)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `r` : `Float`<br>the red component of the board colour, `0.0 ~ 1.0`
  - `g` : `Float`<br>the green component of the board colour, `0.0 ~ 1.0`
  - `b` : `Float`<br>the blue component of the board colour, `0.0 ~ 1.0`
  - `a` : `Float`<br>the alpha component of the board colour, `0.0 ~ 1.0`; leave it unset for the client's default colour
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setBackgroundColor = ll.import("HologramLib", "holoSetBackgroundColor");
    setBackgroundColor(holo, 0, 0, 0, 0.5);   // semi-transparent black background
    ```

### Restore the board to the client's default colour

`holoClearBackgroundColor(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const clearBackgroundColor = ll.import("HologramLib", "holoClearBackgroundColor");
    clearBackgroundColor(holo);
    ```

### Decide whether the holographic text is occluded by blocks

`holoSetDepthTest(id, enabled)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `enabled` : `Boolean`<br>`true` = occluded by blocks / entities; `false` = always rendered (visible through walls), which is also the default behaviour
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setDepthTest = ll.import("HologramLib", "holoSetDepthTest");
    setDepthTest(holo, true);   // hidden once it is behind a wall
    setDepthTest(holo, false);  // back to visible through walls
    ```

### Fix the whole text block to a three-axis orientation

`holoSetRotation(id, pitch, yaw, roll)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `pitch` : `Float`<br>one of the rotation angles about the three axes, in **degrees**
  - `yaw` : `Float`<br>one of the rotation angles about the three axes, in **degrees**
  - `roll` : `Float`<br>one of the rotation angles about the three axes, in **degrees**
- Return value: `true` on success; `false` when `id` does not exist. Once set, it no longer faces the camera
- Return type: `Boolean`
- Example:

    ```js
    const setRotation = ll.import("HologramLib", "holoSetRotation");
    setRotation(holo, 0, 45, 0);   // rotate 45 degrees about the Y axis
    ```

### Clear the rotation so the whole text block faces the camera

`holoClearRotation(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const clearRotation = ll.import("HologramLib", "holoClearRotation");
    clearRotation(holo);   // the three angles are reset to zero along with it
    ```

## Holographic text: position and following

### Move the holographic text to a new coordinate

`holoSetLocation(id, x, y, z)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `x` : `Float`<br>the new world coordinate X
  - `y` : `Float`<br>the new world coordinate Y
  - `z` : `Float`<br>the new world coordinate Z
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setLocation = ll.import("HologramLib", "holoSetLocation");
    setLocation(holo, 120.5, 70, 100.5);   // moves along with it when it is already on screen
    ```

### Migrate the holographic text to another dimension

`holoSetDimension(id, dimId)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `dimId` : `Integer`<br>the target dimension number — `0` Overworld, `1` Nether, `2` End
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setDimension = ll.import("HologramLib", "holoSetDimension");
    setDimension(holo, 1);   // re-sent in place when already on screen, no flicker
    ```

### Make a holographic text follow a player with a Y offset

`holoSetFollowPlayer(id, playerName, offsetY)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `playerName` : `String`<br>the name of the player to follow (LSE's `realName`)
  - `offsetY` : `Float`<br>the height offset added on top of the player's foot Y
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setFollowPlayer = ll.import("HologramLib", "holoSetFollowPlayer");
    const refresh = ll.import("HologramLib", "holoRefresh");
    setFollowPlayer(holo, "Steve", 2.5);
    // following does not advance on its own: to keep the text above the player's head, refresh it on a timer yourself
    setInterval(() => refresh(holo), 100);
    ```

### Stop following and stay at the current position

`holoClearFollowPlayer(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const clearFollowPlayer = ll.import("HologramLib", "holoClearFollowPlayer");
    clearFollowPlayer(holo);   // it stays at the last position it resolved
    ```

## Holographic text: display and refresh

### Show a holographic text to every online player (any dimension)

`holoDraw(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: `true` once it has been shown; `false` when `id` does not exist (or there is no content to show)
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "holoCreate");
    const addLine = ll.import("HologramLib", "holoAddLine");
    const draw = ll.import("HologramLib", "holoDraw");
    const holo = create(100.5, 66, 100.5);
    addLine(holo, "§eSpawn");
    draw(holo);
    ```

### Show a holographic text to one dimension only

`holoDrawToDimension(id, dimId)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `dimId` : `Integer`<br>the dimension number — `0` Overworld, `1` Nether, `2` End
- Return value: `true` once it has been shown; `false` when `id` does not exist (or there is no content to show)
- Return type: `Boolean`
- Example:

    ```js
    const drawToDimension = ll.import("HologramLib", "holoDrawToDimension");
    drawToDimension(holo, 0);   // only Overworld players can see it
    ```

### Show a holographic text to one player only

`holoDrawToPlayer(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
  - `playerName` : `String`<br>the player name; nothing is displayed while the player is offline
- Return value: `true` once it has been shown; `false` when `id` does not exist, the player is offline, or there is no content to show
- Return type: `Boolean`
- Example:

    ```js
    const drawToPlayer = ll.import("HologramLib", "holoDrawToPlayer");
    drawToPlayer(holo, "Steve");   // a message for that player alone
    ```

### Hide a holographic text but keep its content

`holoRemove(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const remove = ll.import("HologramLib", "holoRemove");
    const draw = ll.import("HologramLib", "holoDraw");
    remove(holo);   // hide it
    draw(holo);     // show it again with the same id
    ```

### Resolve the variables and follow coordinates again in place

`holoRefresh(id)`

- Parameters:
  - `id` : `Integer`<br>the holographic-text id
- Return value: `true` when the refresh succeeds; `false` when `id` does not exist, or this holographic text has not been shown yet
- Return type: `Boolean`
- Example:

    ```js
    const refresh = ll.import("HologramLib", "holoRefresh");
    // content such as a countdown or player coordinates does not change by itself, so call it as needed
    setInterval(() => refresh(holo), 1000);   // refresh once a second
    ```

## Gradient lines: creation

### Create a multi-segment gradient line (red → blue by default)

`gradientCreate(x1, y1, z1, x2, y2, z2, segments)`

- Parameters:
  - `x1` : `Float`<br>the world coordinate X of the start point
  - `y1` : `Float`<br>the world coordinate Y of the start point
  - `z1` : `Float`<br>the world coordinate Z of the start point
  - `x2` : `Float`<br>the world coordinate X of the end point
  - `y2` : `Float`<br>the world coordinate Y of the end point
  - `z2` : `Float`<br>the world coordinate Z of the end point
  - `segments` : `Integer`<br>the number of segments; more segments give a smoother transition and more line segments; below `2` it is treated as `2`, above `100` as `100`, and 10 ~ 50 is suggested
- Return value: the gradient-line id (incrementing from `1`); `<= 0` means creation failed
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "gradientCreate");
    const draw = ll.import("HologramLib", "gradientDraw");
    const trail = create(100, 65, 100, 140, 75, 140, 30);
    draw(trail);
    ```

## Gradient lines: colour

### Set the start colour and the end colour

`gradientSetGradient(id, r1, g1, b1, r2, g2, b2)`

- Parameters:
  - `id` : `Integer`<br>the gradient-line id
  - `r1` : `Float`<br>the red component of the start colour, `0.0 ~ 1.0`
  - `g1` : `Float`<br>the green component of the start colour, `0.0 ~ 1.0`
  - `b1` : `Float`<br>the blue component of the start colour, `0.0 ~ 1.0`
  - `r2` : `Float`<br>the red component of the end colour, `0.0 ~ 1.0`
  - `g2` : `Float`<br>the green component of the end colour, `0.0 ~ 1.0`
  - `b2` : `Float`<br>the blue component of the end colour, `0.0 ~ 1.0`
- Return value: `true` on success; `false` when `id` does not exist. The middle blends automatically, and the line leaves rainbow mode
- Return type: `Boolean`
- Example:

    ```js
    const setGradient = ll.import("HologramLib", "gradientSetGradient");
    setGradient(trail, 1.0, 0.2, 0.2, 0.2, 0.4, 1.0);   // red → blue
    ```

### Make a gradient line cycle through rainbow colours

`gradientSetRainbow(id, speed)`

- Parameters:
  - `id` : `Integer`<br>the gradient-line id
  - `speed` : `Float`<br>the cycle speed; at `1.0` the hue advances 60 degrees per second, and the larger it is the faster it goes
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setRainbow = ll.import("HologramLib", "gradientSetRainbow");
    const tick = ll.import("HologramLib", "gradientTick");
    setRainbow(trail, 1.0);
    setInterval(() => tick(0.05), 50);   // the rainbow has to be advanced by tick yourself
    ```

### Set the whole line to a single colour

`gradientSetColor(id, r, g, b, a)`

- Parameters:
  - `id` : `Integer`<br>the gradient-line id
  - `r` : `Float`<br>the red component, `0.0 ~ 1.0`
  - `g` : `Float`<br>the green component, `0.0 ~ 1.0`
  - `b` : `Float`<br>the blue component, `0.0 ~ 1.0`
  - `a` : `Float`<br>the alpha component, `0.0 ~ 1.0`
- Return value: `true` on success; `false` when `id` does not exist. This turns off both gradient and rainbow mode
- Return type: `Boolean`
- Example:

    ```js
    const setColor = ll.import("HologramLib", "gradientSetColor");
    setColor(trail, 0.3, 1.0, 0.5, 0.8);   // semi-transparent green line
    ```

### Change the two endpoints of a gradient line

`gradientSetEndpoints(id, x1, y1, z1, x2, y2, z2)`

- Parameters:
  - `id` : `Integer`<br>the gradient-line id
  - `x1` : `Float`<br>the new world coordinate X of the start point
  - `y1` : `Float`<br>the new world coordinate Y of the start point
  - `z1` : `Float`<br>the new world coordinate Z of the start point
  - `x2` : `Float`<br>the new world coordinate X of the end point
  - `y2` : `Float`<br>the new world coordinate Y of the end point
  - `z2` : `Float`<br>the new world coordinate Z of the end point
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const setEndpoints = ll.import("HologramLib", "gradientSetEndpoints");
    setEndpoints(trail, 100, 70, 100, 160, 80, 160);   // rebuilds the segments when already on screen
    ```

## Gradient lines: display and destruction

### Show a gradient line to every online player (any dimension)

`gradientDraw(id)`

- Parameters:
  - `id` : `Integer`<br>the gradient-line id
- Return value: `true` once it has been shown; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "gradientCreate");
    const draw = ll.import("HologramLib", "gradientDraw");
    const trail = create(100, 65, 100, 140, 75, 140, 30);
    draw(trail);
    ```

### Show a gradient line to one dimension only

`gradientDrawToDimension(id, dimId)`

- Parameters:
  - `id` : `Integer`<br>the gradient-line id
  - `dimId` : `Integer`<br>the dimension number — `0` Overworld, `1` Nether, `2` End
- Return value: `true` once it has been shown; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const drawToDimension = ll.import("HologramLib", "gradientDrawToDimension");
    drawToDimension(trail, 0);
    ```

### Hide a gradient line but keep its data

`gradientRemove(id)`

- Parameters:
  - `id` : `Integer`<br>the gradient-line id
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const remove = ll.import("HologramLib", "gradientRemove");
    const draw = ll.import("HologramLib", "gradientDraw");
    remove(trail);   // hide it
    draw(trail);     // show it again
    ```

### Delete a gradient line and all its segments

`gradientDestroy(id)`

- Parameters:
  - `id` : `Integer`<br>the gradient-line id
- Return value: `true` when the deletion succeeds; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const destroy = ll.import("HologramLib", "gradientDestroy");
    destroy(trail);
    ```

### Delete every current gradient line

`gradientDestroyAll()`

- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const destroyAll = ll.import("HologramLib", "gradientDestroyAll");
    destroyAll();
    ```

## Gradient lines: animation

### Advance the rainbow animation time (the script calls it by tick)

`gradientTick(deltaTime)`

- Parameters:
  - `deltaTime` : `Float`<br>the time that passed in this beat, in **seconds**; one beat at 20 tps is `0.05`. Only rainbow-mode lines are advanced; ordinary gradient lines and single-colour lines are unaffected
- Return value: no return value
- Return type: `Null`
- Example:

    ```js
    const tick = ll.import("HologramLib", "gradientTick");
    setInterval(() => tick(0.05), 50);   // roughly one advance per game tick
    ```

## Things to know

- **Pass every parameter the signature asks for**. There are no default values on the LSE side: give too few or too many and the call hands you the failure value straight away (`Boolean` gives `false`, `Integer` gives `0`, `String` gives an empty string), with no exception to warn you.
- **Per-line style does not exist**: multiple lines share one colour / scale / background box / through-walls switch / orientation. To change the colour of one line, write a `§` colour code in that line's text (for example `"§cred §agreen"`); to scale the whole block up, use `holoSetScale`. Since `26.40.8` the 8 per-line functions `holoSetLineScale`, `holoSetLineColor`, `holoSetLineGradient`, `holoSetLineRainbow`, `holoSetLineScroll`, `holoSetVerticalAnimation`, `holoSetLineSpacing` and `holoTick` have been removed, and `ll.import` gets `null` for them.
- **Variables**: `{player}`, `{online}`, `{time}`, `{tps}`, `{dimension}`, `{x}`, `{y}` and `{z}` in the text are replaced (with MeowPAPI installed, the external placeholders it provides are supported as well). When the text contains variables and the display target is not a single player, the library keeps one copy of the text per viewer: **the shape count becomes the number of viewers**, which suits small-audience cases such as a party-leader view or quest tracking — do not use it for high-frequency refreshes broadcast to the whole server.
- **Dynamic lines rotate by time**: the content pool picks its entry by "current time ÷ interval" modulo, so several holographic texts using the same pool and the same interval stay in phase naturally; changing the pool's content is reflected immediately, and the refresh itself is throttled by the library to 0.5 seconds, so nothing is re-sent while the content has not changed.
- **Following does not advance on its own**: `holoSetFollowPlayer` takes the player's coordinate once, at the moment of the call, and `holoDraw` and `holoRefresh` each take it once as well. The text does not move along as the player walks, so you have to call `holoRefresh` on a timer yourself (for example once every 2 ticks); after the player logs off the text stays at the last position it resolved.
- **Switching to a single-player target changes how variables behave**: after `holoDrawToPlayer` only that one player receives the text, and the variables are resolved for that player alone.
- **Colour changes on a gradient line wait for the next resend**: `gradientSetGradient`, `gradientSetColor`, `gradientSetRainbow` and `gradientTick` only update the segment colour values, and the client does not see the new colour until another `gradientDraw` or the library's background resend every 15 seconds.
- **Frequent `gradientDraw` flickers**: it destroys the old segments first and then builds new ones; `gradientSetEndpoints` also rebuilds the segments when the line is already on screen.
- **You do not have to resend when a player joins**: holographic texts and gradient lines live in client memory only, and the library resends them automatically after a player joins (once 1 second and once 5 seconds after joining, then a background resend every 15 seconds).
- **Calling from the main thread is recommended** (inside a server tick or an event callback).
- When the library is not installed or not loaded, `ll.import` returns `null` and calling it directly throws; you can check for null first, or call `ll.hasExported("HologramLib", "holoDraw")` first as a capability check.
