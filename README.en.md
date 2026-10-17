<div align="center">
  <img src="docs/assets/HologramLib.png" alt="HologramLib — unified protocol-layer hologram & display library for Bedrock servers" width="100%">
</div>

<!-- Docs site: https://ieckseyr.github.io/HologramLib/ — built and published from this repo's docs/ by .github/workflows/docs.yml -->
[中文](README.md) · [English](README.en.md) · [Docs](https://ieckseyr.github.io/HologramLib/en/)

HologramLib is a **protocol-layer display library** for Bedrock servers (LeviLamina / BDS): one API that pushes shapes, holograms, items, entities and menus straight to the client — no real entities spawned, nothing written to the save, no server-side world state touched.

The same capabilities are reachable two ways: **C++ plugins** (frozen pure-virtual interfaces) and **LSE scripts** (`ll.import`).

| | |
|---|---|
| Plugin release | `26.51.4` |
| API version | `1.30.0` (`HOLOGLIB_API_VERSION 0x012400`) |
| Platform | LeviLamina 26.51 · BDS 1.26.51 (protocol 2193) |
| Public surface | One public header, `include/hologramlib/HologramLib.h`, plus the LSE namespace `HologramLib` (278 exports) |
| Script prerequisite | `lrca` (LegacyRemoteCall); when absent every export call is a safe no-op and the C++ interface is unaffected |
| License | MIT (the floating-text "dynamic lines" feature is derived from Phantom, LGPL-3.0) |

This is a **prerequisite library, not a feature plugin**: it provides rendering primitives, protocol wrappers and timing notifications only. Commands, forms, permissions, placeholders and persistence belong to the consumer.

## Changelog
The complete changelog lives in
[the changelog](docs/versions/changelog.en.md).

# <font size=24>Full documentation</font>
<font size=16>[HologramLib docs site](https://ieckseyr.github.io/HologramLib/en/)</font>

## How to write

### C++ plugin

```cp0
#include "hologramlib/HologramLib.h"

// Gate the capability at compile time; handshake at runtime in case an older build is installed
static_assert(HOLOGLIB_API_VERSION >= 0x011B00, "requires 1.21.0 (ITradeMenu)");

auto& lib = hologramlib::IHologramLib::getInstance();

// Holographic text: one text shape per block, lines joined with \n, inline colours via § codes
auto holo = lib.holograms().create(0, 70, 0);
lib.holograms().addLine(holo, "§eWelcome to spawn");
lib.holograms().addLine(holo, "Online: {online}");         // {var} resolves per viewer
lib.holograms().setDepthTest(holo, true);                  // true = occluded by blocks (default is see-through)
lib.holograms().draw(holo);

// Shapes: line + colour + lifetime
auto line = lib.shapes().createLine(0, 64, 0, 10, 64, 10);
lib.shapes().setColor(line, 1.0f, 0.2f, 0.2f, 1.0f);
lib.shapes().draw(line);

// Virtual container: a list UI any input device can click (a click is an item pickup)
hologramlib::ContainerMenuSpec menu;
menu.title = "§8Quest list";
menu.rows  = 3;                                            // 3 = 27 slots; 6 = 54 slots
menu.items.resize(27);
menu.items[0].type = "minecraft:diamond_sword";
menu.items[0].name = "§bTrial";
auto menuId = lib.containerMenus().open("Steve", menu);
lib.containerMenus().setItem(menuId, 0, {"minecraft:barrier", 1, 0, "§cDone", {}});
```

xmake integration:

```lua
add_includedirs("../HologramLib/include")
add_linkdirs("../HologramLib/build/windows/x64/release")
add_links("HologramLib")
```

### LSE script

> The standard LSE interfaces (`mc`, `ll`, `logger`, events, NBT, forms, databases …) are covered by the [LSE docs](https://lse.levimc.org/apis/); this section only covers HologramLib's own namespace.

```js
// One namespace, "HologramLib", with 16 domain prefixes
const holoCreate      = ll.import("HologramLib", "holoCreate");
const containerOpen   = ll.import("HologramLib", "containerOpen");
const sensingIsTouch  = ll.import("HologramLib", "sensingIsTouch");

// Route by client device: touch players get the virtual container (clicks always send a packet)
if (sensingIsTouch("Steve")) {
    const id = containerOpen("Steve", "§8Quest list", 3);
    // Drain clicks from a guard/timer loop (reading empties the queue)
    for (const line of ll.import("HologramLib", "containerPollClicks")()) {
        // "player=Steve menuId=1 slot=0 closed=0"
    }
}
```

## Installation

1. Stop the server.
2. Extract the release archive `HologramLib.zip` and drop the resulting `HologramLib` folder into the server's `plugins/`, giving `plugins/HologramLib/` (containing `HologramLib.dll` and `manifest.json`).
3. Start the server. Load order does not matter: the library checks for the LSE compatibility layer during `enable()` and retries on `ServerStarted`.

Consumers only need `getInstance()` in `enable()`; there is no declared dependency to add.

Release builds are completely silent: all diagnostic logging is disabled at compile time and the log strings never enter the binary. To debug, temporarily enable it with `xmake f --holo_diag=y && xmake build -y`, and reset it (`--holo_diag=n`) before shipping.


## Versions

| Number | Where | Purpose |
|---|---|---|
| Plugin release | `version` in `manifest.json` | For deployment; matches the git tag (e.g. `26.51.4`) |
| API version | `1.30.0` above | Human-readable capability version |
| `HOLOGLIB_API_VERSION` | `include/hologramlib/HologramLib.h` | The only version source in code, `0x012400` |

The full mapping, the encoding rule, and the reasons behind the two documented API shrinks are in the [version & ABI matrix](docs/versions/api-version-matrix.en.md).

## License

MIT — see [LICENSE](LICENSE).

The holographic-text "dynamic lines" feature (the `FloatingTextLine` content pool, rotation interval, per-line variable toggle, interval-modulo rotation, 0.5 s throttled refresh with per-player resend, and content deduplication) is derived from [Phantom](https://github.com/GroupMountain/Phantom) (a LeviLamina Bedrock hologram plugin, GNU Lesser General Public License v3.0).

As required by LGPL-3.0, the origin is stated here: that derived part and its derivative works are distributed under LGPL-3.0, copyright held by the Phantom contributors (GroupMountain); the rest of the library remains MIT. Distributions that include that part must also satisfy LGPL-3.0: keep the copyright and licence notices, provide the corresponding source (locations: the file header of `src/FloatingTextManager.h`, the comments above the relevant functions in `src/FloatingTextManager.cpp`, this README and `docs/versions/api-version-matrix.md`), and keep that part replaceable / independently rebuildable.
