# C++ API reference

## 1.1 Entry point

```cpp
namespace hologramlib {
    class IHologramLib {
    public:
        static IHologramLib& getInstance();          // the only dllexport factory
        IShapeDrawer&   shapes();          // shape rendering
        IHologramText&  holograms();       // holographic text
        IItemDetail&    itemDetails();     // item detail
        IItemDisplay&    itemDisplays();   // FMBE item display (1.6.0)
        ICustomEntity&  customEntities();  // custom entities (1.10.0)
        IParticleShape& particleShapes();  // generic particle shapes (1.14.0)
        IPlayerNpc&     playerNpcs();      // fake-player NPCs (1.16.0)
        ITradeMenu&     tradeMenus();      // villager trade menu (1.21.0)
        INpcDialogue&   npcDialogs();      // NPC dialogue (1.21.0)
        IContainerMenu& containerMenus();  // virtual container / list UI (1.21.0)
        IPlayerSensing& playerSensing();   // sensing (1.21.0)
        IFakeInventory& fakeInventories(); // fake inventory (1.23.0)
        ISulfurDisplay& sulfurDisplays();  // sulfur-cube display (1.23.0)
        IViewOverride&  viewOverrides();   // client view override (1.25.0)
        bool     isLseAvailable();        // whether the LSE compatibility layer is mounted
        uint32_t version();               // 0x012300 = 1.29.0
        int64_t  findNearestItemDisplay(float x, float y, float z, int dim, double maxDist); // 1.6.0
        void     setGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.12.0
        void     clearGhostInteractListener();                                             // 1.12.0
        std::vector<std::string> pollGhostInteractions();                                  // 1.12.0 LSE polling version
        uint64_t addGhostInteractListener(std::function<void(GhostInteractEvent const&)> listener); // 1.19.1 multicast
        bool     removeGhostInteractListener(uint64_t token);                               // 1.19.1
        uint64_t addActorInteractListener(std::function<void(ActorInteractEvent const&)> listener); // 1.27.0 real-entity interaction (multicast)
        bool     removeActorInteractListener(uint64_t token);                               // 1.27.0
    };
}
```

General conventions:

- Every `id` is an `int64_t`; a failed creation returns `< 0`
- Colour components are `0.0 ~ 1.0` (RGBA); coordinates are world coordinates; `setDuration` is in seconds
- Methods are mutex-guarded internally (thread-safe); packets are sent on the calling thread, so calling from the main thread is recommended

## 1.2 IShapeDrawer (shape rendering)

| Method | Signature | Description |
|------|------|------|
| createText | `(float x, float y, float z, std::string const& text) -> int64_t` | Text shape |
| createLine | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | Line segment |
| createBox | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | Box (two opposite corners) |
| createCircle | `(x, y, z, scale: float) -> int64_t` | Circle |
| createSphere | `(x, y, z, scale: float) -> int64_t` | Sphere |
| createArrow | `(x1,y1,z1, x2,y2,z2: float) -> int64_t` | Arrow |
| setColor | `(int64_t id, float r, float g, float b, float a) -> bool` | RGBA |
| setScale | `(int64_t id, float scale) -> bool` | Scale |
| setDuration | `(int64_t id, float seconds) -> bool` | Lifetime |
| setDimension | `(int64_t id, int dimId) -> bool` | Dimension |
| setLocation | `(int64_t id, float x, float y, float z) -> bool` | Position |
| setText | `(int64_t id, std::string const& text) -> bool` | Text content |
| setRotation | `(int64_t id, float pitch, float yaw, float roll) -> bool` | Three-axis rotation (degrees; text shapes enable useRotation + double-sided rendering automatically) |
| clearRotation | `(int64_t id) -> bool` | Clear the rotation (back to facing the camera) |
| draw | `(int64_t id) -> bool` | Draw for everyone in the dimension who can see it |
| drawToPlayer | `(int64_t id, std::string const& playerName) -> bool` | For one player |
| drawToDimension | `(int64_t id, int dimId) -> bool` | For one dimension |
| remove | `(int64_t id) -> bool` | Hide (the data is kept) |
| update | `(int64_t id) -> bool` | While visible, re-send in place (same networkId overwrite, no flicker) |
| destroy | `(int64_t id) -> bool` | Destroy |
| destroyAll | `() -> void` | Destroy all |
| exists | `(int64_t id) -> bool` | Existence |
| type | `(int64_t id) -> ShapeType` | Shape type |
| setBackgroundColor | `(int64_t id, float r, float g, float b, float a) -> bool` | Background box colour (1.26.0; text shapes only, unset = the client's default colour) |
| clearBackgroundColor | `(int64_t id) -> bool` | Back to the client's default background colour (1.26.0) |
| setDepthTest | `(int64_t id, bool enabled) -> bool` | Through-wall visibility (1.26.0; true = occluded by blocks/entities, false = always rendered) |

```cpp
enum class ShapeType : int { Text=0, Line=1, Box=2, Circle=3, Sphere=4, Arrow=5 };
```

## 1.3 IHologramText (holographic text)

The whole multi-line block renders as one background box (all lines are joined with `\n`); after a change, call `refresh` to redraw in place (same networkId overwrite, no flicker). Line indices start at 0.
**The rule after the 1.26.0 shrink**: lines carry text only; colour / scale / background box / depth test / rotation are all **whole-block** properties — for per-line style differences, embed § colour codes in the text.

| Category | Method | Signature | Description |
|------|------|------|------|
| Lifecycle | create | `(float x, float y, float z) -> int64_t` | Create |
| | destroy | `(int64_t id) -> bool` | Destroy |
| | destroyAll | `() -> void` | Destroy all |
| Line management | addLine | `(int64_t id, std::string const& text) -> bool` | Append a line |
| | setLineText | `(int64_t id, int line, std::string const& text) -> bool` | Line text |
| | setLinePool | `(int64_t id, int line, std::vector<std::string> const& content, int intervalMs) -> bool` | Dynamic line (1.26.0): the content pool **rotates by time modulo** `intervalMs` (stateless, several holograms sharing one pool stay in phase); a pool of ≤1 entry / an interval of ≤0 = no rotation; empty = clear the pool |
| | setLineParseVariables | `(int64_t id, int line, bool enabled) -> bool` | Whether this line parses variables (default true; off = `{}` is shown literally) (1.26.0) |
| | removeLine | `(int64_t id, int line) -> bool` | Remove a line |
| | clearLines | `(int64_t id) -> bool` | Clear the lines |
| | getLineCount | `(int64_t id) -> int` | Line count |
| Colour | setColor | `(int64_t id, r, g, b, a: float) -> bool` | Whole-block text colour |
| | setBackgroundColor | `(int64_t id, r, g, b, a: float) -> bool` | Background box colour (unset = the client's default colour; 1.26.0) |
| | clearBackgroundColor | `(int64_t id) -> bool` | Clear the background box colour (1.26.0) |
| Style | setScale | `(int64_t id, float scale) -> bool` | Whole-block scale (1.26.0; replaces the old per-line setLineScale) |
| | setDepthTest | `(int64_t id, bool enabled) -> bool` | Through-wall visibility (1.26.0; true = occluded by blocks/entities) |
| | setRotation | `(int64_t id, pitch, yaw, roll: float) -> bool` | Three-axis fixed orientation (degrees; no longer faces the camera; 1.26.0) |
| | clearRotation | `(int64_t id) -> bool` | Back to facing the camera (1.26.0) |
| Position | setLocation | `(int64_t id, float x, float y, float z) -> bool` | Position |
| | setFollowPlayer | `(int64_t id, std::string const& playerName, float offsetY) -> bool` | Follow a player (the position is resolved in place at call time, no self-driving) |
| | clearFollowPlayer | `(int64_t id) -> bool` | Stop following |
| | setDimension | `(int64_t id, int dimId) -> bool` | Move dimension: when it is already drawn, the underlying shape's dimension is synced and it is re-sent in place to the original draw targets (no flicker) |
| Display | draw / drawToDimension / drawToPlayer / remove | Same semantics as shapes | |
| | refresh | `(int64_t id) -> bool` | Re-resolve variables / follow coordinates and re-send in place |

> **1.26.0 shrink (on the record)**: `setLineScale` / `setLineColor` / `setLineGradient` / `setLineRainbow` /
> `setLineScroll` / `setVerticalAnimation` / `setLineSpacing` and `tick` were removed — under the "one whole block,
> one shape" model they cannot be implemented, or they spin idle. The reasons and the replacements (§ colour codes / setScale)
> are in the [changelog](../versions/changelog.md) and the [version & ABI matrix](../versions/api-version-matrix.md).
> **C++ ABI**: they are kept as **deprecated empty slots** (non-pure + inline empty body + `[[deprecated]]`; calling one returns `false` / does nothing),
> with vtable slots unchanged from 1.25.0 — a consumer binary that was not recompiled calls through the old slots without misaligning; on the LSE side the exports were deleted as usual.

> **Dynamic lines (1.26.0; reworked from [Phantom](https://github.com/GroupMountain/Phantom), LGPL-3.0, source credited as its licence requires)**: `setLinePool` = the content pool **rotates by time modulo** `intervalMs`
> (stateless, several holograms sharing one pool stay in phase); `setLineParseVariables` = a per-line variable switch (on by default). Rotation / variable refresh is driven by a
> **0.5 s throttle** inside the library and **only resends when the content changed** (unlike the self-driven "offset" removed in this version — that one moved on every beat, but nothing it drove
> had a consumer). Variable set: `{player}` `{online}` `{time}` `{tps}` `{dimension}` `{x}` `{y}` `{z}` +
> external placeholders through MeowPAPI. **Text containing `{var}` gets one shape per viewer** (resolved per player), and its refresh re-sends
> in place per player (same networkId, no flicker).

Built-in variables (resolved as a fallback after PAPI translation): `{time}` `{online}` `{tps}` `{player}`.
External placeholders `%name%` / `{name}` go through LseBridge to `MeowPAPI::translateString(WithPlayer)` (optional; when absent they are kept as they are).

## 1.4 IItemDetail (item detail)

```cpp
// Shows "localised item name xN" at (x,y,z) (translated automatically; count<=1 has no count suffix)
// A non-empty customText fully replaces the automatic text (supports § colour codes and {variables})
// Returns the hologram-text id (keep refining it with holograms())
int64_t show(int dimId, float x, float y, float z,
             std::string const            std::string const& itemId, int aux, int count, itemId, int aux, int count,
             std::string const            std::string const& customText = ""); customText = "");
bool hide(int64_t id);
```

Item-name resolution chain: `ItemRegistry (HashedString) → ItemStack → I18n(getDescriptionId)` → `getName/getDescriptionName` → fallback to showing the id as it is (non-blocking).

## 1.5 IItemDisplay (FMBE item display, 1.6.0)

Configuration-driven item/block display (FMBE fox + packets). Every transform field is a **constant number or a Molang expression string**; persistence is the consumer's job (the library only renders).

```cpp
struct ItemDisplayConfig {
    std::string item{"minecraft:diamond"}; // item identifier
    int         itemAux{0};                // aux value
    float       x{0}, y{64}, z{0};         // world coordinates
    int         dimension{0};
    std::string offsetX{"0"}, offsetY{"-4"}, offsetZ{"0"};           // v.xpos/ypos/zpos translation in model units
    std::string baseOffsetX/Y/Z{"0"};                                 // v.xbasepos... render-pixel offset
    std::string rotX{"180"}, rotY{"0"}, rotZ{"180"};                 // three-axis rotation (degrees)
    std::string scale{"0.375"};                                       // scale (0.5 is suggested for block mode)
    std::string extendScale{"1"}, extendRotX{"-90"}, extendRotY{"0"}; // second-stage transform for block mode
    int    mode{0};                        // 0=auto 1=item 2=block
    double viewDistance{64.0};             // <=0 = unlimited
    bool   enabled{true};
    std::string itemNbt{};                 // extra item data (SNBT; custom name and the like)
    bool   itemGlint{false};               // enchantment glint (BDS's native path injects Sharpness I)
    float  hitboxWidth{0.0f};              // AABB hitbox width (R53; 0 = no hitbox, 1.17.0)
    float  hitboxHeight{0.0f};             // AABB hitbox height (R54; 0 = no hitbox, 1.17.0)
};
```

| Method | Signature | Description |
|------|------|------|
| create | `(ItemDisplayConfig const&) -> int64_t` | Create (<0 on failure) |
| destroy / destroyAll | `(int64_t) -> bool` / `() -> void` | Destroy |
| exists | `(int64_t) -> bool` | Existence |
| get | `(int64_t, ItemDisplayConfig&) -> bool` | Copy the current configuration out |
| setItem | `(int64_t, std::string const&, int) -> bool` | Change the item |
| setPosition | `(int64_t, float, float, float, int) -> bool` | dim<0 changes the coordinates only |
| setOffset / setBaseOffset | `(int64_t, ox, oy, oz: string) -> bool` | Translation (Molang) |
| setRotation | `(int64_t, rx, ry, rz: string) -> bool` | Three-axis rotation (Molang) |
| setScale | `(int64_t, scale: string) -> bool` | Scale (Molang) |
| setExtend | `(int64_t, scale, rx, ry: string) -> bool` | Second-stage transform for block mode |
| setMode / setEnabled / setViewDistance | — | Behaviour |
| rotateY | `(int64_t, float) -> bool` | Yaw delta added on top (adapts to a constant or an expression) |
| getAllIds | `() -> std::vector<int64_t>` | All ids |
| createRandom | `(ItemDisplayConfig const&) -> int64_t` | Auto-generates a unique id from the random range `[0x10000000, 0x7FFFFFFF)`; on success returns the generated id, <0 on failure |
| createWithId | `(ItemDisplayConfig const&, int64_t desiredId) -> int64_t` | Create with a chosen id (for restoring persisted state); desiredId<=0 or already taken returns -2 |
| isIdUsed | `(int64_t) -> bool` | Whether an id is in use |
| scaleBy | `(int64_t, double factor) -> bool` | Relative scale: current scale × factor; a constant is multiplied directly, an expression is wrapped as `(expr)*factor`; factor<=0 returns false |
| setItemWithNbt | `(int64_t, std::string const& item, int aux, std::string const& nbt) -> bool` | Change the item (with extra data): nbt is SNBT; an empty string = clear; a parse failure is treated as no NBT and warns |
| setGlint | `(int64_t, bool on) -> bool` | Enchantment-glint switch: on = BDS's native `saveEnchantsToUserData` injects Sharpness I; idempotent |
| follow | `(int64_t, std::string const& playerName, float offX, float offY, float offZ) -> bool` | 1.17.0 display follows a player: every tick it syncs the target's live coordinates (including an automatic respawn across dimensions) and sends MoveActorAbsolute to players who already see it (not a teleport, so the client interpolates) for smooth movement with no respawn anywhere; the player logging off releases it automatically (the block stays where it is); a manual `setPosition` releases the follow |
| unfollow | `(int64_t) -> bool` | 1.17.0 release the follow; returns true when there is no follow relationship |
| setHitbox | `(int64_t, float width, float height) -> bool` | 1.17.0 AABB hitbox: broadcasts SetActorData(R53/R54) to players who already see it, effective immediately with no respawn; the values persist into the configuration (a respawn sends packets with the current values automatically); 0/0 restores "cannot be hit" |

Visibility is synced automatically by the in-library Level tick hook (every 20 ticks: players in the same dimension and inside the view distance are spawned/removed automatically; a player disconnecting is cleaned up automatically); property changes take effect immediately (an atomic despawn→respawn for players who already see it). `setMode` / `setViewDistance` are idempotent (the same value is not marked dirty and does not respawn).

Seamless creation (`createSeamless`), one-shot jump scaling (`scaleTo`) and the visibility whitelist are currently LSE exports only (§2.5) and are not on the C++ interface. Gradient animation is driven by the consumer stepping `scaleTo` (for example 5 steps of easeOutCubic).

## 1.6 ICustomEntity (custom entities, 1.10.0)

AddActorPacket goes straight to the client to create a **purely visual entity** that occupies no server-side entity system. Suited to NPC shells / decorative mobs / armour-stand props and the like (no collision, no server-side logic; interaction is routed through ghost events, see 1.7).

```cpp
struct CustomEntityEquipment {
    std::string name;    // item name (empty = empty slot)
    int         aux{0};
    std::string nbt{};   // SNBT form; empty = no extra NBT
};

struct CustomEntityConfig {
    std::string identifier{"minecraft:armor_stand"}; // entity type identifier (a short name gets minecraft: prefixed automatically)
    float       x{0}, y{64}, z{0};                   // world coordinates
    int         dimension{0};
    float       yaw{0}, pitch{0};                    // orientation (degrees)
    std::string nametag{};                           // overhead name (supports § colour codes; empty = none)
    bool        nametagAlwaysShow{false};
    float       scale{1.0f};                         // valid client-side range 0.0625~10, clamped automatically
    int         variant{0};                          // variant (skin / subspecies)
    int         markVariant{0};                      // mark variant
    int         colorIndex{0};                       // colour index (for dyeable entities such as sheep / collars)
    std::int64_t flags{0};                           // raw flags bitmask (0x01=on fire 0x20=invisible, etc.)
    bool        invisible{false};                    // convenience invisible switch (composes 0x20 when sending)
    double      viewDistance{64.0};                  // <=0 = unlimited
    bool        enabled{true};
    int         pose{0};                             // PoseIndex 0..13 (0=Standing, 3..13 sitting/sleeping/dancing and the like)
    CustomEntityEquipment equipment[6];              // slots: 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet
    std::string ridePlayerName{};                    // ride on a given player's head (SetActorLinkPacket; must be online)
    int64_t     rideEntityId{0};                     // ride another custom entity (that entity's library id is the vehicle)
};
```

| Category | Method | Signature | Description |
|------|------|------|------|
| Lifecycle | create / createRandom / createWithId | `(CustomEntityConfig const&[, int64_t desiredId]) -> int64_t` | Same semantics as itemDisplay (random range `[0x10000000, 0x7FFFFFFF)`; desiredId<=0 or taken returns -2) |
| | destroy / destroyAll / exists / isIdUsed / getAllIds / get | — | Same semantics as itemDisplay |
| Properties | setIdentifier | `(int64_t, std::string const&) -> bool` | Change the entity type |
| | setPosition | `(int64_t, float, float, float, int) -> bool` | dim<0 changes the coordinates only |
| | setRotation | `(int64_t, float yaw, float pitch) -> bool` | Orientation (degrees) |
| | setNametag | `(int64_t, std::string const&) -> bool` | An empty string clears it |
| | setScale | `(int64_t, float) -> bool` | <=0 is rejected |
| | setVariant / setMarkVariant / setColorIndex | `(int64_t, int) -> bool` | Variant / colouring |
| | setFlags / setInvisible / setEnabled / setViewDistance | — | Behaviour |
| | setPose | `(int64_t, int pose) -> bool` | PoseIndex 0..13 |
| | setEquipmentSlot | `(int64_t, int slot, std::string const& name, int aux, std::string const& nbt) -> bool` | Six-slot equipment; an empty name clears the slot; nbt is SNBT |
| | findNearest | `(float x, float y, float z, int dim, double maxDist) -> int64_t` | Nearest lookup; -1 when nothing matches |
| Scale | scaleBy | `(int64_t, double factor) -> bool` | Relative scale: current scale × factor, with the result clamped to 0.0625~10 automatically; factor<=0 returns false |
| Visibility | setVisiblePlayers | `(int64_t, std::vector<std::string> const&) -> bool` | Whitelist (matched by realName); an empty list = clear the restriction |
| | setVisiblePlayer / clearVisiblePlayers | `(int64_t, std::string const&) / (int64_t) -> bool` | Scalar version / clear (back to visible to everyone) |
| Per-client orientation (1.20.0, C++ interface only; LSE exports do not cover it yet) | setPlayerRotation | `(int64_t, std::string const& playerName, float yaw, float pitch) -> bool` | **1.20.0**: overrides the orientation of this entity as received by a given player (both the AddActor spawn packet and the delta packets are sent with the override value); players without an override still use the config orientation; returns false when the player is offline / has not seen this entity; a change goes through the light-dirty delta path (no flicker) and takes effect on the next tick |
| | clearPlayerRotation | `(int64_t, std::string const& playerName) -> bool` | Clear one player's orientation override |
| | clearPlayerRotations | `(int64_t) -> bool` | Clear the orientation overrides of every player for this entity |
| Diagnostics | getDebugInfo | `(int64_t) -> std::string` | Runtime summary; returns `not_found` when not found |
| Riding | setRidePlayer | `(int64_t, std::string const& playerName) -> bool` | Ride on a player's head (an empty name clears; must be online) |
| | setRideEntity | `(int64_t, int64_t vehicleEntityId) -> bool` | Ride another custom entity; 0 clears |
| | clearRide | `(int64_t) -> bool` | Break the riding link |
| Animation | playAnimation | `(int64_t, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | Play a vanilla animation (AnimateEntityPacket, e.g. `animation.humanoid.base_pose`); an empty stopExpression = permanent; durationTicks>0 stops it automatically on expiry |

Property changes merge into a single respawn through the tick dirty refresh (no flicker, no cross-talk); visibility is synced automatically by the in-library Level tick hook (the same pattern as itemDisplay).

## 1.7 GhostInteractEvent (ghost interaction event, 1.12.0)

When the client clicks/attacks an entity that "exists on the protocol", the event arrives through the ItemUseOnActor transaction embedded in InventoryTransactionPacket (since protocol 944 InteractPacket no longer carries entity interaction); after receiving the packet the library hook maps the target runtimeId back to a library id and dispatches it, which is what makes **clickable NPCs / hologram menus** possible.

```cpp
struct GhostInteractEvent {
    std::string playerName;   // the clicker (realName)
    int         action{0};    // 1=right-click interact 2=left-click attack (see the note below)
    std::string domain;       // "entity" / "itemDisplay" / "npc"
    int64_t     id{-1};       // the library id inside the corresponding domain
    bool        hasPos{false};
    float       x{0}, y{0}, z{0};
};
```

- `action`: 1=right-click interact, 2=left-click attack (ItemUseOnActor's Interact/ItemInteract map to 1 and Attack maps to 2; aligned with the old InteractPacket semantics, so the consumer does not have to tell the sources apart)
- Two ways to consume it, pick one (they can coexist): C++ push via `setGhostInteractListener`; LSE polling via `ghostPollInteractions` (takes and empties the queue; the queue holds at most 256 entries and drops the oldest when full)
- Always calls origin, so BDS's vanilla behaviour for interaction packets with an unknown runtimeId is unchanged; with no listener and nobody polling it only discriminates the runtimeId range, at near-zero cost

## 1.8 IParticleShape (generic protocol-layer particle shapes, 1.14.0)

Points / lines / rectangular rings / filled planes / box frames / six faces / polyhedra + translation / smooth movement / rotation / spin / scale / follow. Batch concurrent sending: a per-player vanilla `SpawnParticleEffectPacket` is queued through `NetworkSystem`, and BDS's tick flush aggregates and compresses it into a single Batch datagram automatically (the same path as the vanilla particle broadcast); sampling / view-distance culling / periodic resend are self-driven by an in-library tick, so the consumer only creates and controls.

| Category | Method | Signature | Description |
|------|------|------|------|
| Creation | createPoint | `(std::string const& owner, int dimId, float x, float y, float z, std::string const& effect, int intervalTicks, int lifetimeTicks) -> int64_t` | Single point |
| | createLine | `(owner, dimId, x1,y1,z1, x2,y2,z2: float, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | Line segment |
| | createRect | `(owner, dimId, float cx, cy, cz, w, h, int axis, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | Rectangular ring (axis: 0=XY 1=YZ 2=XZ) |
| | createPlane | Same as createRect | Filled plane mesh |
| | createBox | `(owner, dimId, cx, cy, cz, hx, hy, hz: float, step, effect, intervalTicks, lifetimeTicks) -> int64_t` | The 12-edge box wireframe (h* are half sizes) |
| | createBoxFaces | Same as createBox | Six filled faces |
| | createPoly | `(owner, dimId, std::vector<float> const& verts, std::vector<std::int32_t> const& edges, float step, effect, intervalTicks, lifetimeTicks) -> int64_t` | Polyhedron (verts=(x,y,z)×N; edges=(i,j)×M; anchor=centroid) |
| Control | setPos | `(int64_t id, float x, float y, float z) -> bool` | Translate the anchor (= moving the particles; also releases the follow) |
| | moveBy | `(int64_t id, float dx, float dy, float dz) -> bool` | Relative translation |
| | moveTo | `(int64_t id, float x, float y, float z, int durationTicks) -> bool` | Smooth point-to-point movement (easeOutCubic; releases the follow; duration<=0 arrives immediately) |
| | setRot | `(int64_t id, float rx, float ry, float rz) -> bool` | Euler angles (degrees, ZYX order, about the anchor) |
| | spin | `(int64_t id, float sx, float sy, float sz) -> bool` | Spin (degrees/tick; 0,0,0 stops it) |
| | setScale | `(int64_t id, float scale) -> bool` | Isotropic scale |
| | follow | `(int64_t id, std::string const& playerUuid, float offX, float offY, float offZ) -> bool` | The anchor follows a player + offset (updated every tick, across dimensions) |
| | unfollow | `(int64_t id) -> bool` | Release the follow |
| Rendering | setEffect | `(int64_t id, std::string const& effect) -> bool` | Particle effect name |
| | setVisiblePlayers | `(int64_t id, std::vector<std::string> const& playerUuids) -> bool` | Whitelist (UUIDs; empty = everyone in the dimension) |
| | clearVisiblePlayers | `(int64_t id) -> bool` | Clear the whitelist |
| | setInterval | `(int64_t id, int ticks) -> bool` | Whole-batch resend interval |
| | setViewDistance | `(int64_t id, int blocks) -> bool` | Per-player 3D culling; 0 = no culling |
| | setLifetime | `(int64_t id, int ticks) -> bool` | From now on; 0 = permanent |
| Lifecycle | destroy / destroyAll / exists / getAllIds | Same as the other domains | — |
| Diagnostics | getDebugInfo | `(int64_t id) const -> std::string` | Runtime summary (returns "not_found" when not found) |

Conventions:

- `intervalTicks` = the whole-batch resend interval (particles are transient, so the resend is what keeps the visual alive); `lifetimeTicks` 0 = permanent
- Sample points are converted to local coordinates at construction time (the anchor = the shape's geometric reference point); translation / rotation / scale act on the local points and take effect on the next emission automatically
- Shares one Manager with the LSE `particle*` functions (§2.8); the id spaces are interchangeable

C++ consumer example:

```cpp
auto& ps = hologramlib::IHologramLib::getInstance().particleShapes();
// a 16×16 particle wall under the player (XZ plane fill, follows the player)
auto id = ps.createPlane("MeowLand", 0, 0, 64, 0, 16, 16, 2, 1.0f,
                         "minecraft:endrod", 4, 0);
if (id > 0) {
    ps.follow(id, playerUuid, 0, -2, 0);
    ps.setVisiblePlayers(id, {playerUuid}); // visible to that player only
}
```

## 1.9 IPlayerNpc (fake-player NPCs, 1.16.0)

> **NPC skins now render correctly (fixed in 26.40.2)**. Every interface in this section is usable and the whole data path is complete: skin registration (PNG / captured online / directory import), export-restore through `getSkinBlob` / `registerSkinFromBlob`, NPC creation/movement/orientation/scale/view distance/visibility and click interaction all work. The earlier symptom (the client not rendering the skin, falling back to the default model) had its root cause in the PlayerList skin entry's `Id` / `FullId` being empty or retaining a cached key from the original player; that is fixed.

Pure-protocol fake player: `PlayerListPacket(Add, carrying the skin) → AddPlayerPacket → [20 ticks] PlayerListPacket(Remove)` (the fake player appears in the Tab list briefly and is then removed; the entity keeps rendering because the skin is cached). It occupies no server-side entity system; click interaction is dispatched through the ghost pipeline with `domain="npc"` (§1.7).

One skin registry, shared globally: register a PNG file (decoded with GDI+ at 64×64/128×128, custom geometry/armSize), bulk-import a directory (one subfolder = a PNG + an optional `.json` geometry model; the default is the standard player model) or capture from an online player (`Player::mSkin → SerializedSkinImpl` with every field copied: texture/cape/animation texture/geometry/Persona parts/tinting → registered as a runtime snapshot under a skinId, so a later skin change by that player does not affect it). Decoding/capturing happens once and any number of NPCs reuse it at zero repeated cost. **The library never writes to disk** — persistence is the consumer's job: after a successful registration/capture/import, export the full-field binary snapshot with `getSkinBlob` and store it in your own directory, then restore it on restart with `registerSkinFromBlob`, with no dependency on the source PNG or the player being online.

| Category | Method | Signature | Description |
|------|------|------|------|
| Skins | registerSkin | `(PlayerNpcSkin const& skin) -> bool` | PNG registration (an empty skinId = the file name; a duplicate overwrites) |
| | importSkins | `(std::string const& dirPath) -> int` | Bulk directory import (skinId = the subfolder name; the PNG is required, `.json` geometry optional; returns the number imported, -1 for an invalid directory) |
| | captureSkin | `(std::string const& skinId, std::string const& playerName) -> bool` | Capture from an online player (returns false when offline; a duplicate overwrites) |
| | getSkinBlob | `(std::string const& skinId, std::string& out) const -> bool` | Full-field serialised export (for the consumer to persist; false when not registered) |
| | registerSkinFromBlob | `(std::string const& blob) -> bool` | Register by deserialising a blob (pairs with getSkinBlob; false for an invalid format) |
| | hasSkin / unregisterSkin / getSkinIds | — | unregister is refused while an NPC references the skin |
| Lifecycle | create / createRandom / createWithId | `(PlayerNpcConfig const&) -> int64_t` | Failures: -1 general / -2 id taken / -3 skin not registered; persistence is the consumer's job |
| | destroy / destroyAll / exists / get / isIdUsed / getAllIds | — | Same as the other domains |
| Properties | setPosition | `(int64_t id, float x, float y, float z, int dim) -> bool` | dim<0 changes the coordinates only |
| | setRotation / setNametag / setSkin / setViewDistance / setEnabled | — | setSkin returns false when the skin is not registered; changes merge into a single respawn through the tick dirty refresh |
| Orientation (1.20.0, C++ interface only; LSE exports do not cover it yet) | setRotationLight | `(int64_t id, float yaw) -> bool` | Lightweight orientation: sends only a MoveActorAbsolute delta (no entity rebuild, no skin resend, no flicker), suited to per-tick tracking |
| | setPlayerRotation | `(int64_t id, std::string const& playerName, float yaw) -> bool` | Overrides the orientation a given player receives (both the spawn packet and the deltas use the override value); players without an override use the config orientation |
| | clearPlayerRotation / clearPlayerRotations | `(int64_t[, std::string const&]) -> bool` | Clear one / every player's orientation override |
| Visibility | setVisiblePlayers / clearVisiblePlayers / setVisiblePlayer | — | Player-name whitelist (empty = everyone) |
| Lightweight position (26.40.8) | setPositionLight | `(int64_t id, float x, float y, float z, int dim) -> bool` | **Sends only a `MoveActorAbsolute` delta** (no entity rebuild, no skin resend, no flicker); the same light-dirty channel as setRotationLight, merged into one packet in the same tick; returns false when `dim` differs from the current dimension (cross-dimension still goes through the setPosition rebuild) |
| Skin injection (26.40.8; C++ only) | injectSkin | `(std::string const& viewerName, std::string const& targetName, std::string const& skinId) -> bool` | Sends `PlayerList(Add)` with **the target's own UUID / uniqueId** to update the skin entry in place (the real xuid + the three-state trust + the skin object's `OverridesPlayerAppearance=true` — the precondition for overriding a skin the client already has equipped); an empty `viewerName` = every online player (**target included**); the entry is sent exactly as it sits in the registry; **it does not resend automatically** (the consumer calls it once more if a frame is dropped) |
| | injectSkinAll | `(std::string const& targetName, std::string const& skinId) -> bool` | Everyone version (equivalent to `injectSkin("", targetName, skinId)`) |
| Animation (26.40.8; C++ only) | playAnimation | `(int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | Sends `AnimateEntityPacket` to "players who have already seen this NPC" (the controller name is uniquified internally from the NPC id automatically, so the resource pack needs no controller of the same name); an empty `stopExpression` = permanent, `query.any_animation` = stop at once, `durationTicks>0` sends the stop packet automatically on expiry; the animation name has to be resolvable by the client (vanilla, or shipped with the resource pack); returns false with no observers |
| | playAnimationTo | `(int64_t id, std::string const& playerName, std::string const& animation, std::string const& stopExpression, int durationTicks) -> bool` | Same as above, sent to that player only (for re-sends; returns false when the player has not seen this NPC) |
| Spawn callback (26.40.8; C++ only) | setEntitySpawnCallback | `(std::function<void(int64_t id, std::string const& playerName)>) -> void` | Callback for an NPC having **finished spawning** for a given player (for re-sending animations/state; EntitySpawnCallback semantics); the same queue as the customentity domain (fired +2 ticks later) |
| Diagnostics | getDebugInfo | `(int64_t id) const -> std::string` | Runtime summary |

```cpp
struct PlayerNpcSkin {
    std::string pngPath;                                        // PNG path (64/128)
    std::string skinId;                                         // empty = use the file name
    std::string geometry{"geometry.humanoid.custom"};          // model (resourcePatch)
    std::string armSize{"wide"};                                // "wide" / "slim"
    std::string geometryData;                                   // 1.18.0 full geometry JSON (optional; providing it enables a custom model)
};

struct PlayerNpcConfig {
    std::string name{"NPC"};        // display name (kept in sync with the nametag)
    std::string skinId{"default"};   // must already be registered, otherwise creation fails with -3
    float x{0}, y{64}, z{0};
    int   dimension{0};
    float yaw{0};
    double viewDistance{96.0};      // <=0 = unlimited; hysteresis: leaving requires going beyond viewDistance+4
    bool  enabled{true};
};
```

---

## 1.10 ITradeMenu (villager trade menu, 1.21.0; display-only since 1.22.0; **real settlement at the protocol level since 1.24.0**)

`IHologramLib::tradeMenus()`. A **hand-written packet** opens the trade screen: the screen is driven entirely by the `UpdateTradePacket` this library constructs, and the payload is byte-identical to BDS 26.40's native villager trade (`tests/check-trade-packet.bat` byte-compares the whole packet, `check-trade-offers.bat` only the Offers section). This packet is the only thing that opens the trade screen — the native flow has no `ContainerOpen` with `ContainerType=Trade`.

> **By default it reads no clicks at all**; only with `settleLocally` on does it read **item actions on the trade slots** and `CraftRecipe` actions carrying a self-built recipe id.
>
> **Real settlement (1.24.0)**: with `TradeMenuSpec::settleLocally = true` (and `usePacketOffers = true`) the screen is still the self-built `UpdateTrade` (the server publishes no trade table), but the library catches the client's pay-in / take-back / deal requests and **settles them itself**: it takes the payment out of the real inventory and holds it, checks the ledger on a deal and writes the outputs into the inventory per entry, and refunds unspent payment when the screen closes; the "pick it up with one click" part is hosted by the library too (the cursor). Results come back through `TradeSettlementEvent` (a C++ listener / LSE `tradePollSettlements`). The decision logic is isolated in `src/trade/TradeSettlementLogic.h` with an offline self-check, `tests/check-trade-settlement.bat` (43 assertions). **Known limitation**: the client requires the server to echo slot corrections for the trade slots in its reply, and in this protocol version that section does not match the protocol library's wire format (writing it by hand as the protocol library suggests crashes the client) — the server accepts the payment, but the client screen may roll it back. **For reliable settlement use `usePacketOffers=false` (BDS settlement) or a container UI.**
>
> Field-tested boundaries: touch cannot reach settlement at the pure protocol layer (payment cannot be dragged into a trade slot) — use the real-trade-table path or a container UI for touch; a client that **cannot afford it sends no packet**; the ledger is deducted by "type + count" (trade slots are not distinguished); `numCrafts` is not expanded (one deal sends one set of outputs).

Two paths (`TradeMenuSpec::usePacketOffers`):

- **`true` + `false` (default, display-only)**: only the self-built `UpdateTrade` goes out and the server publishes no trade table. A request to put something into a payment slot is rejected by BDS, the item bounces back and the screen stays at "cannot trade" (the same route as the reference implementation GMLIB `ChestUI`).
- **`true` + `true` (1.24.0, real settlement at the protocol level)**: see the note above.
- **`false` (real trade table)**: give the carrier entity a real trade table and call BDS's `openTrading`; **the player is genuinely trading** (items are genuinely consumed; touch can walk the whole flow). When `carrierUniqueIdOverride != 0` that real entity is the trade target directly; on this path `settleLocally` has no effect.

Carrier entity: the trade screen needs an `EntityUniqueId`, so opening it spawns a **hidden, this-player-only** fake villager 5 blocks behind the player (`minecraft:villager_v2`) and deletes it on close. The carrier reaches the client asynchronously, so both `UpdateTrade` and the experience-bar metadata are sent a few ticks later (sent back to back, the client does not know the entity yet and the screen cannot bind to it). The experience bar is not inside the packet; it is driven by the carrier entity's `TradeTier` / `MaxTradeTier` (always 4) / `TradeExperience` metadata.

| Category | Method | Signature | Description |
|------|------|------|------|
| Lifecycle | open | `(std::string const& playerName, TradeMenuSpec const&) -> int64_t` | A repeated call for the same player closes the old menu first; -1 on failure |
| | update / close / closeAll / isOpen / getAllIds | — | `update` resends the trade table (on the real-table path it reopens the screen) |
| Content (added in 1.22.0) | addOffer | `(int64_t menuId, TradeMenuOffer const&) -> bool` | Append one entry and **re-send the trade table in place** (no screen reopen, no waiting for a delay) |
| | setTier | `(int64_t menuId, int tier, int experience) -> bool` | Change the displayed tier value / experience bar, also re-sent in place |
| Real settlement (1.24.0) | setSettleLocally / isSettleLocally | `(int64_t, bool) -> bool` / `(int64_t) const -> bool` | Toggle real settlement at any time after opening (the initial value comes from `TradeMenuSpec::settleLocally`) |
| | addSettlementListener / removeSettlementListener | `(std::function<void(TradeSettlementEvent const&)>) -> uint64_t` / `(uint64_t) -> bool` | Deal / failure callback (C++ side) |
| | pollSettlements | `() -> std::vector<std::string>` | LSE polling (takes and empties); entries look like `"player=Steve menuId=3 offer=1 ok=1 reason="` |

```cpp
struct TradeMenuOffer {
    TradeMenuItem buyA, buyB, sell;   // an empty buyB.type = no second payment item
    int  tier{kTradeTierNovice};      // 1-based: 1=novice … 5=master; above spec.tier it shows as locked
    int  maxUses{16};
    int  traderExp{0};
    bool locked{false};               // true = shows as locked no matter how high spec.tier is
};

struct TradeMenuSpec {
    std::string tradeType{"entity.villager.butcher"}; // free translation key
    int         tier{kTradeTierNovice};               // 1-based, clamped when out of range
    int         experience{0};                        // current experience on the experience bar
    bool        usePacketOffers{true};                // see above: protocol-layer display / real trade table
    std::int64_t carrierUniqueIdOverride{0};          // non-zero = use that real entity (no self-built offers are published)
    std::vector<TradeMenuOffer> offers;
    std::string carrierIdentifier{"minecraft:villager_v2"}; // or "minecraft:wandering_trader"
};
```

```cpp
// Carrier entity type (appended at the tail of TradeMenuSpec): villager by default; wandering trader = "minecraft:wandering_trader"
// (the screen's look / header image follows the type; it applies to both paths, and the real-trade-table path installs its table the same way)
spec.carrierIdentifier = "minecraft:wandering_trader";
```

## 1.11 INpcDialogue (NPC dialogue, 1.21.0)

`IHologramLib::npcDialogs()`. `NpcDialoguePacket` (scene name + body + NPC name + button JSON) goes out, `NpcRequestPacket` comes back (which button was clicked / the player closed it).

- The button table is generated entirely server-side, per player: deciding whether a button appears from permissions / tags / scoreboard / quest state simply means working out `buttons` yourself before calling this interface; the client does not have to cooperate.
- Multi-level dialogue = the click callback carries `sceneName` + `buttonIndex` / `actionId` back, and the caller sends the next level from that.
- The carrier entity is a **purely protocol-synthesised** `minecraft:npc` (it never enters BDS's entity system), positioned below the world at `y=-66` — the client cannot see the entity, but the avatar in the screen still renders as usual (switching to the invisible flag would make the avatar disappear with it). It is sent to that player only; the library sends `RemoveActor` to delete it when a button is clicked / the screen is closed / the player goes offline.
- When `NpcDialogSpec::rawActionJson` is non-empty it is sent verbatim as `mActionJSON` (the vanilla button structure is not publicly documented; this is for field-testing the field format in game); `carrierIdentifier` has to be in the NPC family, and swapping it for a vanilla entity such as a villager does not work (a field-tested requirement).
- **The avatar/model shown in the chat screen is customisable (26.40.8)**: `NpcDialogSpec::avatarSkinVariant` (an NPC **built-in skin variant** 0..59, i.e. a variant value from `NpcData.skin_list`) — written into the carrier's ActorData as `SkinId(104)`, which switches the avatar/model in the screen to a built-in skin; `-1` (the default) omits the entry (byte-identical to the old behaviour). `NpcDialogSpec::avatarViewSpec` — **layers IViewOverride semantics on top of the carrier** (the spec syntax is the same as `viewEntity`): `type=` changes the carrier type (**field-tested: a non-NPC family such as a zombie/chicken still brings the screen up**), `name=` changes the screen title / interaction text. **The `skin=` branch has been removed** (a shrink inside an unreleased batch, on the record): it turned the carrier into a "player model" (PlayerList+AddPlayer), and in field tests the client closes the screen when you click into the menu on that carrier, so it is currently unusable; the "make the client believe it is a player" route is left for the consumer to build (`playerNpcs()` + `npcUniqueIdOverride`). For "the whole model fully custom" there is also the `npcUniqueIdOverride` route: the view domain / a custom entity creates a display entity and the dialogue hangs off its id (1.27.0's `EntityView::skinId` same-id model swap can be used on that display entity too). For in-place modification on the LSE side use `npcDialogSetAvatar`.

```cpp
struct NpcDialogButton {
    std::string label;
    std::vector<std::string> commands; // shown on the button; a synthesised NPC does not get BDS to run them, the commands come back to the caller
    std::string actionId;              // server-side identifier, carried back verbatim by the click callback (for wiring up a dialogue tree/quest)
    int         mode{0};               // 0=normal button 1=close 2=open
};

struct NpcDialogClickEvent {
    std::string playerName;
    int64_t     dialogId{-1};
    std::string sceneName;
    int         buttonIndex{-1};  // -1 = not a button event
    std::string actionId;
    bool        closed{false};
    std::vector<std::string> commands; // the commands attached to the clicked button (filled in from buttonIndex)
};
```

## 1.12 IContainerMenu (virtual container / list UI, 1.21.0)

`IHologramLib::containerMenus()`. The approach reproduces the reference implementation GMLIB's `ChestUI` item by item:

1. `UpdateBlockPacket` places a **client-side chest block** above the player's head (sent to that player only; it does not exist in the server's world)
2. `BlockActorDataPacket` places that block's block-entity NBT: `id=Chest` / `CustomName=title` / `x,y,z` / `pairx,pairz,pairlead` (double chest) / `Items=[entry items]` — the entry items travel in this NBT, **not** through per-slot `InventorySlot`
3. wait 10 ticks, then send `ContainerOpen` (`ContainerType::Container(0)` + block coordinates + target entity `-1`)
**For per-slot dynamic refresh use `setItem(menuId, slot, item)`**: it sends one `InventorySlotPacket` (the same packet a real chest uses to sync its contents), and the
client swaps that one slot in place — no block-entity resend, no `ContainerOpen` resend, **no delay, no flicker**; an empty `item.type` clears the slot.
Only a change to the whole page uses `update(menuId, spec)`.

**For paging / entering a submenu / going back use `update(menuId, spec)`**: it reuses the same carrier block, does not tear the screen down, does not replace the block and does not wait,
it only re-sends the block-entity NBT (new entries/title) + `ContainerOpen` for the client to re-read. Reopening through `open()` pays the open delay every time
(it closes the old menu first, restores the real block, then places it again and waits `openDelayTicks` once more).

4. The player clicks → the client sends `ItemStackRequest(147)` → the slot hits this container → the callback fires (the slot number = the entry index). An action that hits the container is **only reported back, never intercepted**: the server does not have this container, so once it is let through BDS fails on its own and the client rolls back its prediction (the item flashes back to where it was, with no error message), while the callback is still received — the same as the reference implementation GMLIB. A field-tested lesson: if the library answers with an `ItemStackNetResult` 3 (request not allowed) itself, the client **pops up an error message**
5. Close → `ContainerClosePacket` → the real block is changed back → the callback fires with `closed = true`

**There is no such container on the server at all**: the items are merely "sitting there", and taking or moving them changes nothing at all (read-only by nature). Suited to screens that "just display + fire a click callback", such as a quest list, an achievement list or a shop preview.

Small and large containers: `rows = 3` → a single chest, **27 slots** (the block entity carries no pairing keys); `rows = 6` → a double chest, **54 slots** (two adjacent chest blocks + the `pairx`/`pairz`/`pairlead` pairing, with the first 27 slots going into the lead half and the last 27 into the other half, each half numbering its `Slot` 0..26). The block position is taken 5 blocks above the player's feet, preferring an air position (so it does not cover a real block), and on close the real block at that position is changed back by its network id. Container ids use the 101..199 display range, which does not clash with the dynamic ids BDS uses for real containers (1..100).

```cpp
struct ContainerMenuSpec {
    int rows{3};                       // 3 = single chest, 27 slots; 6 = double chest, 54 slots
    std::vector<ContainerMenuItem> items; // the index is the slot number (double chest 0..53)
    bool useMinecart{false};           // false (default) = the GMLIB block approach; true = the old minecart path (the client does not accept it, for comparison only)
    std::string title{"虚拟容器"};      // chest screen title (the block entity's CustomName)
    int openDelayTicks{7};             // how many ticks to wait after placing the block + block entity before sending ContainerOpen
                                       // (GMLIB: 10, then 4 more to fill the slots ≈700ms; this library's entries live in the block entity's NBT,
                                       //  so it only needs 7 ≈350ms. Field-tested: 7 opens the screen, 6 does not; only affects that one "open")
};

struct ContainerClickEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         slot{-1};       // the clicked slot (= the index into items)
    bool        closed{false};  // true = the screen was closed (slot = -1 in that case)
};
```

## 1.13 Sensing / per-viewer overrides / per-viewer variable holograms (1.21.0)

**Sensing `IHologramLib::playerSensing()`**: hooks `PlayerAuthInputPacket` (per player, per tick) and captures `InputMode` packet by packet.
`inputDeviceOf(playerName)` returns a `ClientInputDevice` (`Unknown/KeyboardMouse/Touch/Gamepad/MotionController`), and
`isTouch(playerName)` is the convenience check. A player going offline clears it; nothing reported = `Unknown`. Typical use: routing the UI by device
(touch goes to a container list, keyboard-mouse/gamepad go to the trade screen — that touch cannot reach settlement in a pure protocol-layer trade is a field-tested conclusion).

**Per-viewer overrides**, added to `ICustomEntity`: `setPlayerNametag(id, player, text)` (empty = clear the override),
`setPlayerScale(id, player, scale)` (<=0 = clear the override), `setPlayerEquipmentSlot(id, player, slot, name, aux, nbt)`
(slot 0..5; an empty name = that slot falls back to the config, sent immediately as a single packet with no flicker), `clearPlayerAppearance(id, player)`.
The mechanism is the same as `setPlayerRotation`: overrides are stored per player uuid, both the spawn packet and the delta packets use the override value, and players without an override use the config value.

**Per-viewer variable holograms**: when holographic text contains `{var}` placeholders and the draw target is not a single player, the library switches to **per-viewer shapes**
automatically — every online player receives separate text resolved against their own name (`{player}` is correct for each of them); a viewer who goes offline or leaves the dimension has their shape
destroyed in place. With no variables it is still one shared shape (performance unchanged). Text animation (scrolling/bouncing) is self-driven by an in-library `ServerLevelTickEvent`
listener, with no tick call needed from the consumer.

> **Per-client sounds** and **short-lived floating text** were once implemented briefly and were removed wholesale after discussion (see the README changelog).

---

## 1.14 IFakeInventory (fake inventory, 1.23.0)

`IHologramLib::fakeInventories()`. **Rewrites the player's inventory at the protocol layer, as the client sees it**: one `InventoryContentPacket(49)`
(`ContainerId = Inventory(0)`, slots 0..35) replaces the whole content with the copy the caller supplies; a single-slot change goes through `InventorySlotPacket(50)`.
The server-side inventory is not touched at all — the items "appear" to be there.

> **Item serialisation is handed to BDS**: an item descriptor's User Data uses the NBT dialect with a u16 name length / 4-byte integers,
> which differs from sculk `CompoundTag`'s varint dialect; writing it by hand gets it wrong (in field tests it froze the client). So this domain builds its packet bodies with BDS's
> `InventoryContentPacket` / `InventorySlotPacket` (container-name byte 0 + an empty storage descriptor, matching the packet capture).

The reference implementation GMLIB's `ChestUI` fills the player's inventory along the same route (it writes `InventorySlot` slot by slot, with the same container ids
`Inventory(0)` + `InventoryContainer(29)`); this domain does the whole-content push with a single `InventoryContent`.

**Behaviour matches the virtual container**: the player clicks a fake item in their own inventory → the callback reports the slot number and the item is not actually taken —
the client sends the request according to the fake content, the real server-side slot does not match → BDS rules it a failure → the client rolls back its prediction (the same behaviour
as the virtual container, and the library does not have to answer on its behalf); the library then re-sends that one slot so the fake content is not washed away by the rollback.

**Coexisting with the trade menu / virtual container**: when the trade screen or a chest screen opens, BDS re-sends the player's inventory content (which overwrites the fake content),
so this domain re-sends on the `refreshIntervalTicks` cycle by default (20 ticks = 1s by default), and `refresh()` can be called at any time. The inventory area shown on those
screens therefore stays the fake content, and clicks still fire the callback.

| Category | Method | Signature | Description |
|------|------|------|------|
| Push | apply | `(std::string const& playerName, FakeInventorySpec const&) -> bool` | The whole fake content (overwrites that player's previous one); false when the player is offline |
| | setSlot | `(std::string const& playerName, int slot, ContainerMenuItem const&) -> bool` | A single-slot change (one `InventorySlot`, no delay); returns false when `apply` has never run |
| | refresh | `(std::string const& playerName) -> bool` | Re-send the current fake content at once |
| | clear / clearAll | `(std::string const& playerName) -> bool` / `() -> void` | Undo the fake content + stop the periodic resend, and re-send the **real** inventory once |
| Query | isActive / getActivePlayers | `(std::string const&) -> bool` / `() -> std::vector<std::string>` | |
| Callback | addClickListener / removeClickListener | `(std::function<void(FakeInventoryClickEvent const&)>) -> uint64_t` | Multicast; the callback only fires when a cell faked to non-empty is clicked |

```cpp
inline constexpr int kFakeInventorySlots = 36; // 0..8 hotbar, 9..35 main inventory

struct FakeInventorySpec {
    std::vector<ContainerMenuItem> items;   // the index is the slot number; an empty type = leave that cell empty
    int refreshIntervalTicks{20};           // 0 = no periodic resend
};

struct FakeInventoryClickEvent {
    std::string playerName;
    int         slot{-1};                   // the clicked fake slot
};
```

**Boundaries (written down in advance so they are not misused)**:

- Faking only affects what the client displays. What the player can "actually use/eat" is still the real server-side item.
- If a cell's real item happens to match the fake one, that one action is carried out for real by the server — if you want a cell to be display-only, do not fake it as something identical to the real item.
- When the real item in that cell changes for some other reason (picking something up / using an item / another plugin editing the inventory), the periodic resend paints the fake content back over it.
- Armour (`ArmorContainer` 6) / offhand (`OffhandContainer` 34) are outside this domain; they each have their own container enum.

## 1.15 ISulfurDisplay (sulfur-cube display, 1.23.0)

`IHologramLib::sulfurDisplays()`. The second way to place a display (alongside `IItemDisplay`'s "fox + item"):
spawn a `minecraft:sulfur_cube` and **put the block/item to display into its main hand** — in the behaviour pack the cube holds the block it "swallowed" through
`slot.weapon.mainhand` + `behavior.equip_item` (the client renders it as equipment).

The appearance tier `minecraft:sulfur_cube_archetype` is a `client_sync: true` enum property in the behaviour pack, sent through a **hand-written
`ChangeMobProperty`(182)** (26.40 wire format: Actor Id (compressed int64) → Property Name (string) → Bool →
String → Int32 (compressed) → Float). `AddActor`'s `PropertySyncData` only has room for int/float values by index and no room for an
enum, so the property has to be sent after the client already knows the entity — the library re-sends it automatically after a spawn and replays it after a respawn.

This domain **delegates entirely to `ICustomEntity`** (the cube is just a custom entity), so scale / view distance / visible-player whitelist / per-client
orientation and the like are inherited directly; the only new protocol content is `ChangeMobProperty`.

| Category | Method | Signature | Description |
|------|------|------|------|
| Creation | create | `(SulfurDisplaySpec const&) -> int64_t` | Returns the display id (<0 = failure) |
| **Swallowing a block** | setBlock | `(int64_t id, ContainerMenuItem const&) -> bool` | Change the block it "swallowed" (changes the main-hand equipment → rebuilds the entity once) |
| Appearance | setArchetype | `(int64_t id, std::string const&) -> bool` | Change the appearance tier (`ChangeMobProperty`; an empty string = clear the property; **does not rebuild the entity**) |
| **Invisibility** | setInvisible | `(int64_t id, bool) -> bool` | Invisibility switch (same meaning as the `invisible` field on the spec) |
| | setScale | `(int64_t id, float) -> bool` | Scale (0.0625~10) |
| Lifecycle | destroy / destroyAll / exists / getAllIds / entityIdOf | — | `entityIdOf` returns the custom entity id behind it (for diagnostics) |

```cpp
struct SulfurDisplaySpec {
    float       x{0}, y{64}, z{0};
    int         dim{0};
    ContainerMenuItem block;                    // **the swallowed block** (the core parameter; goes through the main-hand equipment)
    std::string archetype{"regular"};           // none/regular/bouncy/sticky/hot/explosive/light/...
    int         variant{2};                     // 1=small / 2=medium (the tier that holds the block)
    bool        invisible{true};                // **the invisibility field** (**on by default**: in field tests the block still renders while hidden → only the content is left)
    float       scale{1.0f};
    double      viewDistance{0.0};
    std::vector<std::string> visiblePlayers;    // empty = visible to everyone
};
```

**Invisibility is on by default (field-tested)**: confirmed on a local 26.40 client — while the cube is invisible, the "swallowed block" in its main hand still renders,
so `invisible` defaults to `true` and the default look is "a block floating there" (the cube itself is invisible); set it to `false` to see the cube itself.
(An NPC carrier's avatar is wiped out along with invisibility; the two entities behave differently, so do not generalise.)

**"Swallowing mobs" was not done**: protocol-layer entities have no AI, so genuinely swallowing/digesting is not possible; letting another entity ride on the cube was tried as an approximation,
but neither the riding position nor the collision could be tuned into a "being swallowed" look (it did not hold up in field tests), so it was removed wholesale — this domain does blocks and invisibility only.

**Also**: `ICustomEntity::setMobProperty(id, name, value)` / `clearMobProperties(id)` were opened up in the same batch —
any `client_sync` entity property can be sent by name (that is how the sulfur cube's tier is implemented).

## 1.16 IViewOverride (client view override, 1.25.0)

> **Completed in 1.29.0 (26.40.10, and on the 26.51 line with `26.51.3`): `AddItemActor` is intercepted too.** A dropped item (item entity) spawns through
> `AddItemActor`(id 15) rather than `AddActor`(id 13), and only the latter used to be intercepted → "change a dropped item's type / hide it" was only half done:
> setting an override could swap it for players already looking at it, but the `AddItemActor` the engine then sent itself pushed the item icon back into the client
> (packet-capture proof from 2026-10-16: 13 ms after `RemoveActor` + `AddActor(minecraft:cod)`, a packet with the same uniqueId
> arrives, an `AddItemActor`). Now:
> * `hidden = true` → the dropped item's `AddItemActor` is **dropped** (the client never sees it at all);
> * `identifier = "<mob>"` → the original packet is dropped and a hand-written `AddActor` goes out with **the same runtimeId/uniqueId** (its properties use
>   the verified recipe `health 20 / scale 1` — do **not** copy ItemActor's `maxHealth = 0`, which is another point that renders incorrectly);
> * Capability probe: the LSE side gains `viewItemSpawnHooked() -> bool` (an older library has no such name, so a script's `ll.hasExported` judges false).
>
> Mob behaviour is unchanged in every character; for item entities this is a **fix** (previously half-broken).

```cpp
auto& view = lib.viewOverrides();

// Entity: change the type / change the skin (our model) / change the nametag / hide it from that player (an empty playerName = every player)
hologramlib::EntityView cow;
cow.identifier = "minecraft:cow";   // a short name gets the minecraft: prefix added automatically
cow.hasNametag = true;
cow.nametag    = "§c奶牛";          // the nametag is only changed when hasNametag is present (an empty value = clear the name); 奶牛 = "cow"
cow.nametagAlwaysShow = true;
view.overrideEntity("Steve", uniqueId, cow);

hologramlib::EntityView gone;
gone.hidden = true;                 // make Steve unable to see this entity (blocks its spawn and update packets)
view.overrideEntity("Steve", uniqueId, gone);

// Non-player entity: swap in "our model" (a registered skin, custom geometry included) — **the same runtimeId/uniqueId**
hologramlib::EntityView skinned;
skinned.skinId = "ms_demo";         // a skin id from the IPlayerNpc registry
skinned.scale  = 0.8f;              // optional: model scale (the client hitbox scales with it; 0 = no scaling)
view.overrideEntity("", mobUniqueId, skinned);
// The client sees our model; server-side it is still that mob — clicking it / hitting it / aiming at it all hit the real thing.
// Undo: view.clearEntity("", mobUniqueId); (the library re-sends the real mob's spawn packet and it is back to normal at once)

// Two kinds of "per-viewer override" that only apply to player entities (seen by Steve alone)
hologramlib::EntityView skin;
skin.asPlayer = "Alex";             // make Steve see this player looking like Alex (skin)
view.overrideEntity("Steve", playerUniqueId, skin);

hologramlib::EntityView asMob;
asMob.identifier = "minecraft:cow"; // a player model cannot render a mob → the library goes for "replacement": it sends a cow with the same runtimeId/uniqueId
view.overrideEntity("Steve", playerUniqueId, asMob);

// Block: show (100,64,100) as a diamond block on his client
hologramlib::BlockView block;
block.type = "minecraft:diamond_block";
view.overrideBlock("Steve", 100, 64, 100, block);

view.clearEntity("Steve", uniqueId);   // undo
view.clearBlock("Steve", 100, 64, 100);
view.clearAll("Steve");                // undo all of this player's overrides (an empty string = every player)
std::string s = view.describeFor("Steve"); // "entities=1 blocks=2"
```

| Method | Return | Description |
|------|--------|------|
| `overrideEntity(playerName, entityUniqueId, view)` | bool | false = the player is offline. It can also be set before the entity spawns (the library applies it when it spawns). None of the five fields (`identifier` / `asPlayer` / `skinId` / `hidden` / `hasNametag`) set = treated as an undo |
| `overrideBlock(playerName, x, y, z, view)` | bool | false = the player is offline / the type name is not in the block registry. An empty type = undo |
| `clearEntity(playerName, entityUniqueId)` | bool | Undo one entity (returns whether an override was there before) |
| `clearBlock(playerName, x, y, z)` | bool | Undo one block and push the **real block** to that player |
| `clearAll(playerName)` | void | Clear that player's override table (an empty string = all players) |
| `describeFor(playerName)` | string | A summary of how many overrides are active for that player (including the global one); carries the intercept-hook counters and `sendFail` |
| `startControl(controllerName, targetName, follow)` | bool | Lets the controller's movement/turn input **drive the target's real movement** (server-side displacement); with `follow=true` the controller is locked onto the target and travels with it (a possession-like feel) |
| `stopControl(controllerName)` | bool | Stop controlling (it also stops automatically when either side goes offline) |

**Mechanism (suppress-only)**: the library intercepts **the packets the server was about to send to this player**, but **only decides pass or drop** —
it **never modifies engine packet fields**. Anything that has to "change" always takes this route: **drop the original packet + write the protocol packet itself**
to send instead (sculk construction → read-back check → raw bytes out). A type change = drop the original spawn packet and send one itself with the same `runtimeId`/`uniqueId`
(the other fields are copied from the original); a nametag = the original packet goes out as it is and the library's own `SetActorData` follows immediately to override it;
a block override = drop the original `UpdateBlock` and send one itself with the overridden network id. The server's world / save file / collision /
what other plugins see are all the real content. The interception hangs off `NetworkSystem::send` / `sendToMultiple` (the point where BDS converges structured packets sent per
recipient; entity spawn packets do not go through `LoopbackPacketSender`, which is why it did not work before), **plus a heartbeat layer**: the server checks itself every tick and
re-pushes the overrides when an entity "re-enters view" or a player changes chunk — that is what catches the cases interception misses.

**Swapping in "our model" (`skinId`, 1.27.0; non-player entities only)**: the library swallows this entity's spawn packet and, with **the same
`runtimeId`/`uniqueId`**, sends `PlayerList(Add)` (carrying that skin) + `AddPlayer` — the client renders it as a player entity
(custom geometry/textures go through the player skin channel, so no resource pack is needed); because **the id did not change**, the server still considers it that mob, and
**clicking it / hitting it / aiming at it all hit the real thing** (damage, drops and what other plugins see are all the real body). Two things go with it: ① this entity's
`MoveActor*`(18/111) is swallowed for those viewers, and the library pushes position/orientation every tick with `MoveActorAbsolute` (the same light-push channel as
`IPlayerNpc::setPositionLight`, sent only when it moved); ② the `PlayerList` entry is removed roughly 20 ticks later (it does not stay in the Tab list forever; the entity stays).
Undo = `clearEntity` (which re-sends the real mob's spawn packet).
The skin must already be in the `IPlayerNpc` registry (`registerSkin` / `captureSkin` / `registerSkinFromBlob`).

**A player never sees their own override**: when the override target is a player, **that player's own client takes no part in the override** — no packets about their own entity
are sent or swallowed (`RemoveActor` / an `AddActor` with the same runtimeId / displacement sent back to the local player makes the client go haywire or even freeze; proven on 2026-10-09).
**Exception: skin swaps (`asPlayer`) include the player themselves** — sending `PlayerList(Add)` to the player is the proper way to "see yourself reskinned"
(third-person and first-person self-views both change; the same convention as `IPlayerNpc::injectSkin`).

**Per-player isolation and idempotence**: the override tables are one per player name plus one global table; paths that "send the same packet to several people" such as
`sendToClients` / broadcast are expanded into a per-recipient decision — purely so that "pass / drop" applies per person; because fields are never modified, there is no cross-talk
where "a later recipient sees an earlier override". The same packet judged once by each of several send paths gives the same result as judging it once. When the library holds
no override at all, the hook passes everything through without parsing anything.

**Player entities (supported since 1.25.0)**:
- `asPlayer = "<online player name>"` → re-sends one `PlayerList(Add)` to that viewer (using the entity's own UUID + the source player's skin)
  — the same mechanism as the vanilla "skin update", **sent to that viewer only**; undo = send it once more with his own skin and he is back to normal at once.
  Among *any* entities this only works on player entities (skins are recognised by UUID).
- `identifier = "<mob>"` → a player model cannot render a mob, so the library goes for **replacement** (not proxying): it swallows his `AddPlayer` and sends an `AddActor` with **the same `runtimeId`/`uniqueId`**; his `MovePlayer` is swallowed for this viewer too, and the library pushes position/orientation from his input packets with `MoveActorAbsolute`. **The id did not change → the server still considers him that real player, and hitting him hits the real person.** On undo the replacement entity is moved away and **the real body is re-sent to that viewer**: `RemoveActor` → a `PlayerList(Add)` with his own identity (the skin goes through the player skin channel) → `AddPlayer` (position/orientation taken from the engine's current values, game mode from the PlayMode in his input packets). So an undo takes effect **immediately**, with no relog or dimension change. When the skin data cannot be read the library leaves the client alone (it would rather keep the status quo than do a "delete first and fail to restore").

**What 26.40 cannot do**: per-player **scale / glowing / invisibility** are not in the API — the metadata table has no `scale`,
and no `glowing` / `invisible` flags either (in Bedrock invisibility is an effect and goes through `MobEffectPacket`).
For per-client scale / equipment slots on custom entities, use `ICustomEntity`'s per-viewer overrides (1.21.0).

**Restrictions (written down so they are not misused)**:
- **A type change** only works on non-player mobs (a player's spawn packet `AddPlayer` carries no type field; a player entity's nametag / hidden state can still be overridden as usual).
- **A type change / hiding / an undo all take effect immediately**: the library performs one "remove + re-send one in the target state" on this entity (the same
  `runtimeId` and `uniqueId`, so the server's later movement/state updates still land on it as usual). The cost is that the client's copy of the entity gets
  rebuilt — details that only ever arrived with the spawn packet (variant, initial equipment state) wait for the server's next sync to come back.
- On undo, an entity that is too far away (> 64 blocks) or already gone is not rebuilt (the client did not have it anyway). Player entities take the "re-send the real body" route (see above); the re-sent real body has **an empty held item and default abilities**, which the engine then corrects on its own (switching items pushes `MobEquipment`, other state changes push `SetActorData`).
- Block overrides are re-sent automatically by the library after a chunk resend.
- Overrides live in memory only (a server restart loses them); an override keyed by player name still applies after that player rejoins.
- The nametag's always-show state has no read path: undoing a nametag override pushes the real name back as "not always shown", and the next time the server re-sends metadata for some other reason it brings the real value back.

**Diagnostics**: the string from `describeFor()` carries the intercept-hook counters (`hooks(calls=… spawn=… meta=… block=… chunk=…)`)
and the sculk send-failure count (`sendFail=`) — when the symptom is "I set it and nothing happens", look here first: a `calls` that stays 0 means the convergence point was never reached
(the heartbeat is covering the feature at that point, so it still works, just not instantly); a `sendFail` above 0 means some packet was constructed illegally (report it).

The decision logic (coordinate packing / chunk ownership / idempotence checks / the nametag plan / type-name sanitisation) is isolated in
`src/view/ViewOverrideLogic.h` and can be tested offline: `tests\check-view-override.bat` (59 assertions).
