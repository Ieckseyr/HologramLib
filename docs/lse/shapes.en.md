# Shape rendering · Drawing lines, boxes, spheres, arrows and text on the client

This domain draws lines, boxes, circles, spheres, arrows and text at a given world coordinate, **visible to the client only**: no entities in the server world, no collision, and nothing written to the save. It suits region markers, building guides, range hints and temporary notice text.

Manage each shape with the integer `id` returned at creation. Setting a property (text, position, colour …) changes the value recorded on the server; a shape that is already on screen does not move until you call `shapeUpdate` or `shapeDraw` once more.

## Quick start

```js
const shapeCreateLine = ll.import("HologramLib", "shapeCreateLine");
const shapeSetColor   = ll.import("HologramLib", "shapeSetColor");
const shapeDraw       = ll.import("HologramLib", "shapeDraw");

// a red line from (100,65,100) to (110,65,110), shown to every online player
const lineId = shapeCreateLine(100, 65, 100, 110, 65, 110);
shapeSetColor(lineId, 1.0, 0.2, 0.2, 1.0);   // RGBA, all four components are 0.0 ~ 1.0
shapeDraw(lineId);
```

Coordinates are always world coordinates; colours are always four components in `0.0 ~ 1.0`.

## Creation

### Create a text shape

`shapeCreateText(x, y, z, text)`

- Parameters:
  - `x` : `Float`<br>the world coordinate X the text sits at
  - `y` : `Float`<br>the world coordinate Y the text sits at
  - `z` : `Float`<br>the world coordinate Z the text sits at
  - `text` : `String`<br>the text to display; it may use `§` colour codes, for example `"§eSpawn"`
- Return value: the shape id (incrementing from `1`); `<= 0` means creation failed
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const draw   = ll.import("HologramLib", "shapeDraw");

    const textId = create(100.5, 66, 100.5, "§eSpawn · Notice board");
    draw(textId);
    ```

### Draw a line segment between two points

`shapeCreateLine(x1, y1, z1, x2, y2, z2)`

- Parameters:
  - `x1` : `Float`<br>the world coordinate X of the start point
  - `y1` : `Float`<br>the world coordinate Y of the start point
  - `z1` : `Float`<br>the world coordinate Z of the start point
  - `x2` : `Float`<br>the world coordinate X of the end point
  - `y2` : `Float`<br>the world coordinate Y of the end point
  - `z2` : `Float`<br>the world coordinate Z of the end point
- Return value: the shape id; `<= 0` means creation failed
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateLine");
    const id = create(100, 65, 100, 110, 65, 110);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Create a box wireframe from two opposite corners

`shapeCreateBox(x1, y1, z1, x2, y2, z2)`

- Parameters:
  - `x1` : `Float`<br>the world coordinate X of the first corner
  - `y1` : `Float`<br>the world coordinate Y of the first corner
  - `z1` : `Float`<br>the world coordinate Z of the first corner
  - `x2` : `Float`<br>the world coordinate X of the opposite corner (the order of the two points does not matter; the edge lengths are the coordinate differences between them)
  - `y2` : `Float`<br>the world coordinate Y of the opposite corner
  - `z2` : `Float`<br>the world coordinate Z of the opposite corner
- Return value: the shape id; `<= 0` means creation failed
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateBox");
    const id = create(100, 64, 100, 110, 70, 110);   // a 10×6×10 box
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Create a circle at a coordinate

`shapeCreateCircle(x, y, z, scale)`

- Parameters:
  - `x` : `Float`<br>the world coordinate X of the circle's centre
  - `y` : `Float`<br>the world coordinate Y of the circle's centre
  - `z` : `Float`<br>the world coordinate Z of the circle's centre
  - `scale` : `Float`<br>the size of the circle; this is the shape scale set at creation time, and `shapeSetScale` can change it later
- Return value: the shape id; `<= 0` means creation failed
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateCircle");
    const id = create(100, 65, 100, 3);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Create a sphere at a coordinate

`shapeCreateSphere(x, y, z, scale)`

- Parameters:
  - `x` : `Float`<br>the world coordinate X of the sphere's centre
  - `y` : `Float`<br>the world coordinate Y of the sphere's centre
  - `z` : `Float`<br>the world coordinate Z of the sphere's centre
  - `scale` : `Float`<br>the size of the sphere; this is the shape scale set at creation time, and `shapeSetScale` can change it later
- Return value: the shape id; `<= 0` means creation failed
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateSphere");
    const id = create(100, 65, 100, 2);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Draw an arrow from a start point to an end point

`shapeCreateArrow(x1, y1, z1, x2, y2, z2)`

- Parameters:
  - `x1` : `Float`<br>the world coordinate X of the arrow's tail
  - `y1` : `Float`<br>the world coordinate Y of the arrow's tail
  - `z1` : `Float`<br>the world coordinate Z of the arrow's tail
  - `x2` : `Float`<br>the world coordinate X the arrow points at
  - `y2` : `Float`<br>the world coordinate Y the arrow points at
  - `z2` : `Float`<br>the world coordinate Z the arrow points at
- Return value: the shape id; `<= 0` means creation failed
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateArrow");
    const id = create(100, 66, 100, 110, 66, 100);   // pointing along +X
    ll.import("HologramLib", "shapeDraw")(id);
    ```

## Properties

### Replace the text of a text shape

`shapeSetText(id, text)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `text` : `String`<br>the new text; it only means something on a text shape, and calling it on another type changes nothing
- Return value: `true` when the write succeeds; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setText = ll.import("HologramLib", "shapeSetText");
    const id = create(100, 66, 100, "§eFirst version");
    setText(id, "§aEdited text");            // the server-side value has changed
    ll.import("HologramLib", "shapeUpdate")(id);   // re-send once when it is already on screen
    ```

### Read back the text of a text shape

`shapeGetText(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: the current text; the empty string `""` when `id` does not exist or is not a text shape
- Return type: `String`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const getText = ll.import("HologramLib", "shapeGetText");
    const id = create(100, 66, 100, "§eSpawn");
    const text = getText(id);   // "§eSpawn"
    ```

### Move a shape to a new coordinate

`shapeSetLocation(id, x, y, z)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `x` : `Float`<br>the new world coordinate X
  - `y` : `Float`<br>the new world coordinate Y
  - `z` : `Float`<br>the new world coordinate Z
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateLine");
    const setLocation = ll.import("HologramLib", "shapeSetLocation");
    const id = create(100, 65, 100, 110, 65, 110);
    setLocation(id, 200, 70, 200);
    ll.import("HologramLib", "shapeUpdate")(id);   // only needed when it is already on screen
    ```

### Query a shape's current coordinate

`shapeGetLocation(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `[x, y, z]`; an empty array when `id` does not exist
- Return type: `Array<Float>`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateLine");
    const getLocation = ll.import("HologramLib", "shapeGetLocation");
    const id = create(100, 65, 100, 110, 65, 110);
    const [x, y, z] = getLocation(id);
    logger.info(`Shape at ${x}, ${y}, ${z}`);
    ```

### Set a shape's colour

`shapeSetColor(id, r, g, b, a)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `r` : `Float`<br>the red component, `0.0 ~ 1.0`
  - `g` : `Float`<br>the green component, `0.0 ~ 1.0`
  - `b` : `Float`<br>the blue component, `0.0 ~ 1.0`
  - `a` : `Float`<br>the alpha component, `0.0 ~ 1.0`
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateBox");
    const setColor = ll.import("HologramLib", "shapeSetColor");
    const id = create(100, 64, 100, 110, 70, 110);
    setColor(id, 0.2, 1.0, 0.2, 0.6);      // semi-transparent green
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Read back a shape's current colour

`shapeGetColor(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `[r, g, b, a]`, each component `0.0 ~ 1.0`; an empty array when `id` does not exist. Colours are stored at 8 bits, so a value read back may differ from the value written by `±1/255`
- Return type: `Array<Float>`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateCircle");
    const getColor = ll.import("HologramLib", "shapeGetColor");
    const id = create(100, 65, 100, 3);
    const [r, g, b, a] = getColor(id);
    logger.info(`Colour ${r} ${g} ${b} ${a}`);
    ```

### Set the shape scale

`shapeSetScale(id, scale)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `scale` : `Float`<br>the scale factor; `1` is the size at creation time
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateSphere");
    const setScale = ll.import("HologramLib", "shapeSetScale");
    const id = create(100, 66, 100, 2);
    setScale(id, 4);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Make a shape disappear after a number of seconds

`shapeSetDuration(id, seconds)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `seconds` : `Float`<br>how long it lasts, in **seconds**. Not calling this function = shown permanently
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setDuration = ll.import("HologramLib", "shapeSetDuration");
    const id = create(100, 66, 100, "§cGone in 10 seconds");
    setDuration(id, 10);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Set the dimension a shape belongs to

`shapeSetDimension(id, dimId)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `dimId` : `Integer`<br>the dimension number — `0` Overworld, `1` Nether, `2` End
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateLine");
    const setDimension = ll.import("HologramLib", "shapeSetDimension");
    const id = create(100, 65, 100, 110, 65, 110);
    setDimension(id, 0);
    ll.import("HologramLib", "shapeDrawToDimension")(id, 0);   // Overworld players only
    ```

### Fix a shape to a three-axis orientation

`shapeSetRotation(id, pitch, yaw, roll)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `pitch` : `Float`<br>one of the rotation angles about the three axes, in **degrees**
  - `yaw` : `Float`<br>one of the rotation angles about the three axes, in **degrees**
  - `roll` : `Float`<br>one of the rotation angles about the three axes, in **degrees**
- Return value: `true` on success; `false` when `id` does not exist. Once set, a text shape no longer faces the camera
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setRotation = ll.import("HologramLib", "shapeSetRotation");
    const id = create(100, 66, 100, "§eLying flat");
    setRotation(id, 90, 0, 0);             // a text shape turns on double-sided rendering automatically
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Clear the rotation so a shape faces the camera

`shapeClearRotation(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const clearRotation = ll.import("HologramLib", "shapeClearRotation");
    const id = create(100, 66, 100, "§eFacing the camera again");
    clearRotation(id);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Query a shape's current rotation angles

`shapeGetRotation(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `[pitch, yaw, roll]` (degrees); an empty array when no rotation was ever set (it still faces the camera) or when `id` does not exist
- Return type: `Array<Float>`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const getRotation = ll.import("HologramLib", "shapeGetRotation");
    const id = create(100, 66, 100, "§eText");
    const rot = getRotation(id);   // no rotation was set → []
    logger.info(`Rotation entries ${rot.length}`);
    ```

### Query which kind of shape an id is

`shapeGetShapeType(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: the type number: `0` text, `1` line, `2` box, `3` circle, `4` sphere, `5` arrow; `-1` when `id` does not exist
- Return type: `Integer`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateBox");
    const getShapeType = ll.import("HologramLib", "shapeGetShapeType");
    const id = create(100, 64, 100, 110, 70, 110);
    if (getShapeType(id) === 2) logger.info("This is a box");
    ```

### Set the colour of the board behind a text shape

`shapeSetBackgroundColor(id, r, g, b, a)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `r` : `Float`<br>the red component of the board colour, `0.0 ~ 1.0`
  - `g` : `Float`<br>the green component of the board colour, `0.0 ~ 1.0`
  - `b` : `Float`<br>the blue component of the board colour, `0.0 ~ 1.0`
  - `a` : `Float`<br>the alpha component of the board colour, `0.0 ~ 1.0`; leave it unset for the client's default colour
- Return value: `true` on success; `false` when `id` does not exist **or is not a text shape**
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setBackgroundColor = ll.import("HologramLib", "shapeSetBackgroundColor");
    const id = create(100, 66, 100, "§fWhite on black");
    setBackgroundColor(id, 0, 0, 0, 0.6);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Restore the text board to the client's default colour

`shapeClearBackgroundColor(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` on success; `false` when `id` does not exist or is not a text shape
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const clearBackgroundColor = ll.import("HologramLib", "shapeClearBackgroundColor");
    const id = create(100, 66, 100, "§fBack to the default background");
    clearBackgroundColor(id);
    ll.import("HologramLib", "shapeDraw")(id);
    ```

### Decide whether text is occluded by blocks

`shapeSetDepthTest(id, enabled)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `enabled` : `Boolean`<br>`true` = occluded by blocks / entities; `false` = always rendered (visible through walls), which is also the default behaviour
- Return value: `true` on success; `false` when `id` does not exist or is not a text shape
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setDepthTest = ll.import("HologramLib", "shapeSetDepthTest");
    const id = create(100, 66, 100, "§eText hidden behind the wall");
    setDepthTest(id, true);                // invisible once something blocks it
    ll.import("HologramLib", "shapeDraw")(id);
    ```

## Display

### Show a shape to every online player (any dimension)

`shapeDraw(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` once it has been sent; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateArrow");
    const draw = ll.import("HologramLib", "shapeDraw");
    const id = create(100, 66, 100, 110, 66, 100);
    draw(id);
    ```

### Show a shape to one player only

`shapeDrawToPlayer(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `playerName` : `String`<br>the player name (LSE's `realName`). Nothing is displayed while the player is offline
- Return value: `true` once it has been sent; `false` when `id` does not exist or the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const drawToPlayer = ll.import("HologramLib", "shapeDrawToPlayer");
    const id = create(100, 66, 100, "§bOnly you can see this");
    drawToPlayer(id, "Steve");
    ```

### Show a shape to one dimension only

`shapeDrawToDimension(id, dimId)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `dimId` : `Integer`<br>the dimension number — `0` Overworld, `1` Nether, `2` End
- Return value: `true` once it has been sent; `false` when `id` does not exist. The shape's dimension is recorded as that value
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateBox");
    const drawToDimension = ll.import("HologramLib", "shapeDrawToDimension");
    const id = create(100, 64, 100, 110, 70, 110);
    drawToDimension(id, 0);      // only Overworld players can see it
    ```

### Show several shapes at once

`shapeDrawBatch(ids)`

- Parameters:
  - `ids` : `Array<Integer>`<br>an array of shape ids; ids in the array that do not exist are skipped (an empty array also returns `true`)
- Return value: `true` once it has been sent
- Return type: `Boolean`
- Example:

    ```js
    const createText = ll.import("HologramLib", "shapeCreateText");
    const createLine = ll.import("HologramLib", "shapeCreateLine");
    const drawBatch = ll.import("HologramLib", "shapeDrawBatch");
    const a = createText(100, 66, 100, "§eA");
    const b = createLine(100, 65, 100, 110, 65, 110);
    drawBatch([a, b]);
    ```

### Hide a shape from every player but keep its data

`shapeRemove(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` on success; `false` when `id` does not exist. The data stays on the server, so the shape can be drawn again
- Return type: `Boolean`
- Example:

    ```js
    const draw = ll.import("HologramLib", "shapeDraw");
    const remove = ll.import("HologramLib", "shapeRemove");
    remove(id);        // hide it
    draw(id);          // show it again with the same id
    ```

### Make a shape invisible to one player only

`shapeRemoveToPlayer(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `playerName` : `String`<br>the player name; returns `false` when the player is offline
- Return value: `true` on success; `false` when `id` does not exist or the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const removeToPlayer = ll.import("HologramLib", "shapeRemoveToPlayer");
    removeToPlayer(id, "Steve");
    ```

### Make a shape invisible to one dimension

`shapeRemoveToDimension(id, dimId)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `dimId` : `Integer`<br>the dimension number — `0` Overworld, `1` Nether, `2` End
- Return value: `true` on success; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const removeToDimension = ll.import("HologramLib", "shapeRemoveToDimension");
    removeToDimension(id, 0);
    ```

### Re-send changed properties to the viewers

`shapeUpdate(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` when `id` exists; does nothing (and reports no error) when the shape has not been displayed yet, and `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const create = ll.import("HologramLib", "shapeCreateText");
    const setText = ll.import("HologramLib", "shapeSetText");
    const update = ll.import("HologramLib", "shapeUpdate");
    const id = create(100, 66, 100, "§eCountdown: 10");
    ll.import("HologramLib", "shapeDraw")(id);
    setText(id, "§eCountdown: 9");
    update(id);        // changing the text alone does not reach the screen; re-send it
    ```

### Re-send changed properties to one player

`shapeUpdateToPlayer(id, playerName)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `playerName` : `String`<br>the player name; returns `false` when the player is offline
- Return value: `true` when `id` exists (nothing is sent if it was never displayed); `false` when `id` does not exist or the player is offline
- Return type: `Boolean`
- Example:

    ```js
    const updateToPlayer = ll.import("HologramLib", "shapeUpdateToPlayer");
    updateToPlayer(id, "Steve");
    ```

### Re-send changed properties to one dimension

`shapeUpdateToDimension(id, dimId)`

- Parameters:
  - `id` : `Integer`<br>the shape id
  - `dimId` : `Integer`<br>the dimension number — `0` Overworld, `1` Nether, `2` End
- Return value: `true` when `id` exists (nothing is sent if it was never displayed); `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const updateToDimension = ll.import("HologramLib", "shapeUpdateToDimension");
    updateToDimension(id, 0);
    ```

## Queries

### Find text shapes near a point

`shapeFindTextByLocation(x, y, z, radius)`

- Parameters:
  - `x` : `Float`<br>the centre world coordinate X to query
  - `y` : `Float`<br>the centre world coordinate Y to query
  - `z` : `Float`<br>the centre world coordinate Z to query
  - `radius` : `Float`<br>the query radius (bounds included); only text shapes take part in the match, lines / boxes / circles / spheres / arrows are not returned
- Return value: the ids of the text shapes that matched; an empty array when there are none
- Return type: `Array<Integer>`
- Example:

    ```js
    const find = ll.import("HologramLib", "shapeFindTextByLocation");
    const ids = find(100, 65, 100, 10);
    for (const id of ids) logger.info(`Nearby text shape ${id}`);
    ```

### Find a text shape near a point by its content

`shapeFindTextByLocationAndContent(x, y, z, radius, text)`

- Parameters:
  - `x` : `Float`<br>the centre world coordinate X
  - `y` : `Float`<br>the centre world coordinate Y
  - `z` : `Float`<br>the centre world coordinate Z
  - `radius` : `Float`<br>the query radius (bounds included)
  - `text` : `String`<br>the full text to compare (only an exact match counts)
- Return value: the id of the first matching text shape; `-1` when nothing matched
- Return type: `Integer`
- Example:

    ```js
    const find = ll.import("HologramLib", "shapeFindTextByLocationAndContent");
    const id = find(100, 65, 100, 10, "§eSpawn");
    if (id !== -1) ll.import("HologramLib", "shapeDestroy")(id);
    ```

### List the ids of every current shape

`shapeGetAllShapeIds()`

- Return value: every shape id (all types); an empty array when there is no shape at all
- Return type: `Array<Integer>`
- Example:

    ```js
    const getAll = ll.import("HologramLib", "shapeGetAllShapeIds");
    const ids = getAll();
    logger.info(`There are currently ${ids.length} shapes`);
    ```

### Tell whether a shape id still exists

`shapeExists(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` when it exists; `false` when it does not
- Return type: `Boolean`
- Example:

    ```js
    const exists = ll.import("HologramLib", "shapeExists");
    if (!exists(id)) logger.info("This shape has already been destroyed");
    ```

## Destruction

### Delete a shape from the client and the server together

`shapeDestroy(id)`

- Parameters:
  - `id` : `Integer`<br>the shape id
- Return value: `true` when the deletion succeeds; `false` when `id` does not exist
- Return type: `Boolean`
- Example:

    ```js
    const destroy = ll.import("HologramLib", "shapeDestroy");
    destroy(id);
    ```

### Delete every current shape

`shapeDestroyAll()`

- Return value: no return value; any shapes left over on the client disappear along with it
- Return type: `Null`
- Example:

    ```js
    const destroyAll = ll.import("HologramLib", "shapeDestroyAll");
    destroyAll();      // tidy up when the plugin unloads or reloads
    ```

### Delete a batch of shapes at once

`shapeDestroyBatch(ids)`

- Parameters:
  - `ids` : `Array<Integer>`<br>an array of shape ids; ids in the array that do not exist are skipped (an empty array also returns `true`)
- Return value: `true` once it has been handled
- Return type: `Boolean`
- Example:

    ```js
    const destroyBatch = ll.import("HologramLib", "shapeDestroyBatch");
    destroyBatch([a, b, c]);
    ```

## Things to know

- **Pass every parameter the signature asks for**. There are no default values on the LSE side: give too few or too many and the call hands you the failure value straight away (`Boolean` gives `false`, `Integer` gives `0`, `String` gives an empty string), with no exception to warn you.
- **Re-send after changing a property**: `shapeSetText` / `shapeSetLocation` / `shapeSetColor` / `shapeSetScale` / `shapeSetRotation` / `shapeSetBackgroundColor` / `shapeSetDepthTest` / `shapeSetDimension` only change the value recorded on the server, so a shape that is already on screen needs another `shapeUpdate` (or `shapeUpdateToPlayer` / `shapeUpdateToDimension`), or simply another `shapeDraw`.
- **The three things only a text shape has**: the background box colour (`shapeSetBackgroundColor`), the through-walls switch (`shapeSetDepthTest`) and the three-axis orientation switch. Lines / boxes / circles / spheres / arrows return `false` straight away when you call the first two.
- **Colours are stored at 8 bits**: after writing `0.5`, `shapeGetColor` may read back `0.498` or `0.502`, so do not compare colours with a strict equality.
- **`shapeSetDuration` is a countdown deletion**: once it is set the client deletes the shape on its own when the time runs out, so do not call it if you want the shape shown permanently.
- **The display function decides who can see it**: `shapeDraw` sends to every online player (any dimension), `shapeDrawToDimension` sends only to that dimension, `shapeDrawToPlayer` sends only to that player; the automatic resend when a player joins later uses the last setting as well.
- **You do not have to resend when a player joins**: these shapes live in client memory only, and the library resends them automatically after a player joins (once 1 second and once 5 seconds after joining, then a background resend every 15 seconds).
- **Calling from the main thread is recommended** (inside a server tick or an event callback).
- When the library is not installed or not loaded, `ll.import` returns `null` and calling it directly throws; you can check for null first, or call `ll.hasExported("HologramLib", "shapeDraw")` first as a capability check.
