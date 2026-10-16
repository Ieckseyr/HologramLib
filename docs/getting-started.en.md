# Getting started

From nothing to a running HologramLib: build → deploy → first feature. About 15 minutes.

Just using the library and not building it yourself? Skip ahead to [Deployment](#3-deployment).

[中文](getting-started.md) · [English](getting-started.en.md) · [Docs home](index.en.md)

---

## 1. Prerequisites

| Need | Version | Notes |
|---|---|---|
| Visual Studio 2022 | MSVC x64 | Must include the "Desktop development with C++" workload |
| [xmake](https://xmake.io) | latest | Build system |
| LeviLamina | 26.40.0 | Pulled automatically by xmake |
| [SculkCatalystMC/Protocol](https://github.com/SculkCatalystMC/Protocol) | **v2168** | You must build it with CMake and install it into `plugins/BedrockProtocol-main/install` |
| Python | 3.x | Only for the local offline regression checks under `tests/`; not part of the build |

The `Protocol` static library is a hard dependency: shape rendering, view override, and the hand-written container and trade-menu packet bodies all call its sculk serialisation directly.

---

## 2. Build

```bash
xmake f -c -y
xmake -y
```

Output:

| Path | What it is |
|---|---|
| `build/windows/x64/release/HologramLib.{dll,lib}` | Build output (the `.lib` is what consumers link against) |
| `bin/HologramLib.zip` | Release archive — this is what you deploy; it extracts to a `HologramLib/` folder |

> ⚠ Run xmake from a normal Windows shell (cmd or PowerShell).
> Under Git Bash / MSYS, xmake detects the host as msys and resolves its toolchain and package directories accordingly. It then fails to find MSVC, tries to rebuild base packages such as fmt and gsl, and dies with `cannot get program for cc`.

Release builds also emit PDBs. They are required for symbolicating crash stacks, and the deployment chain already ships them.

---

## 3. Deployment

1. Stop the server.
2. Extract the release archive `HologramLib.zip` and drop the resulting `HologramLib` folder into the server's `plugins/`, giving `plugins/HologramLib/` (containing `HologramLib.dll` and `manifest.json`).
3. Start the server.

Load order does not matter at all. The library checks for the LSE compatibility layer during `enable()` and, if it is not there yet, retries on the `ServerStartedEvent`. That double safety net means it does not matter whether HologramLib or LegacyRemoteCall (`lrca`) loads first.

Consumer plugins do not need to declare a dependency in their `manifest.json`; just call `getInstance()`.

### Confirming it loaded

Release builds are completely silent (see below), so watch LeviLamina's own plugin list:

```
[HologramLib] ...     ← appears in LeviLamina's plugin list means it loaded
```

### A prerequisite library logs nothing in release builds

Every diagnostic message (startup info, per-domain detail, LSE compat-layer probing, protocol validation warnings) is disabled at compile time; the strings never enter the binary. That is deliberate: a prerequisite library has no business writing to the server log.

To debug, enable it temporarily (and rebuild before shipping again):

```bash
xmake f --holo_diag=y
xmake build -y
# Output then goes to the "HologramLib" channel, prefixed like [NpcDialog] / [TradeMenu] / [PlayerNpc]

xmake f --holo_diag=n && xmake build -y   # reset before releasing
```

The switch lives in `src/DiagLog.h`: the four macros `HLIB_LOG_INFO/WARN/ERROR/DEBUG` funnel through `if constexpr`. When disabled they generate no code, but their arguments are still type-checked (a malformed format string still fails the build).

---

## 4. Your first C++ feature

### Version gating — do this first

```cpp
#include "hologramlib/HologramLib.h"

// 1) Compile time: require a capability to exist
static_assert(HOLOGLIB_API_VERSION >= 0x012300, "requires 1.29.0");

// 2) Runtime: handshake, because an older build may be installed on the server
bool onEnable() {
    auto const ver = hologramlib::IHologramLib::getInstance().version();
    if (ver < 0x012300) {
        logger.error("HologramLib too old: 0x{:06X}, need >= 0x012300", ver);
        return false;
    }
    return true;
}
```

The compile-time assertion and the runtime handshake read the same value (`version()` simply returns `HOLOGLIB_API_VERSION`), so if you do both you can never end up with "it compiled but it exploded at runtime".

### Drawing a hologram

```cpp
auto& lib = hologramlib::IHologramLib::getInstance();

auto holo = lib.holograms().create(0, 70, 0);
lib.holograms().addLine(holo, "§eWelcome to spawn");
lib.holograms().addLine(holo, "Online: {online}");
lib.holograms().setBackgroundColor(holo, 0.0f, 0.0f, 0.0f, 0.6f);
lib.holograms().setDepthTest(holo, true);
lib.holograms().draw(holo);
```

Two model-level facts about holographic text will save you a round of trial and error:

- Multiple lines are one text shape. Lines are joined with `\n` and share a single colour / scale / background / depth-test / rotation. For per-line colour, embed `§` codes in the text; there is no per-line API to look for. Protocol 2168's text payload has no per-line fields, and the historical per-line functions were removed in `26.40.8`. The 26.51 line is the same: protocol 2193 only adds a whole-payload `LineGapHeight` (line spacing; the library always sends `0.0f` = no extra gap), while per-line colour / scale still do not exist.
- Never set a `duration` on a permanent hologram. Leaving it unset is what stops the client from running a countdown and deleting it. Lifecycle details are in the [architecture overview](architecture/overview.en.md#the-holographic-text-composition-model).

### xmake integration

```lua
add_includedirs("../HologramLib/include")
add_linkdirs("../HologramLib/build/windows/x64/release")
add_links("HologramLib")
```

---

## 5. Your first LSE script

Everything on the LSE side lives under one namespace, `HologramLib`, with the function-name prefix identifying the domain:

```js
const holoCreate = ll.import("HologramLib", "holoCreate");

const id = holoCreate(0, 70, 0);
ll.import("HologramLib", "holoAddLine")(id, "§eWelcome to spawn");
ll.import("HologramLib", "holoDraw")(id);
```

Probe before you call. The library is mounted at runtime and there is no declarative dependency to rely on, so use `ll.hasExported` for capability negotiation:

```js
if (ll.hasExported("HologramLib", "containerPollClicks")) {
    // capability available
}
```

There are two failure modes, both explicit and neither silent:

| Situation | Symptom |
|---|---|
| Library not loaded / `lrca` not loaded | `ll.import` returns `null` |
| Function name does not exist (older library) | `ll.import` returns `null` |

So "I got `null`" always means "you do not have this capability right now", never "it was called but did nothing".
The complete list of all 16 prefix domains is in the [LSE API reference](reference/lse-api.md) *(zh)*.

---

## 6. Local regression checks (optional, maintainers)

`tests/` holds a set of offline byte-level checks that need no running server. They pin the wire format using real packet captures and the real linked library:

```powershell
./tests/check-no-diagnostics.bat    # release invariant: no diagnostic logging in the shipped build
./tests/check-trade-packet.bat      # whole UpdateTrade packet, byte-compared against a BDS capture
./tests/check-container-packets.bat # all four virtual-container packets, field by field
./tests/check-npc-carrier.bat       # synthetic NPC carrier packets, field by field
./tests/check-npc-playerlist.ps1    # PlayerList 2168 fields and the trusted-skin flag
./tests/check-view-override.bat     # view-override decision logic (59 assertions)
./tests/check-trade-settlement.bat  # trade settlement logic (43 assertions)
./tests/check-fake-inventory.bat    # fake-inventory wire format (10 assertions)
```

Three things to know:

- These scripts are tools from a local working copy and are not shipped with the library (`tests/` is in `.gitignore`). Some need real captures under `logs/fullpkts/`; without a sample a script prints `no captured ... found` and exits non-zero.
- `check-no-diagnostics.bat` verifies a release invariant: after changing `src/DiagLog.h` or any logging call you must re-run it.
- Most checks pin the bytes produced by a specific header, so after changing that header you must re-run its check. The mapping is in the [protocol notes](architecture/protocol-notes.md#offline-regression-checks) *(zh)*.

---

## Next steps

| I want to… | Go to |
|---|---|
| Pick a domain and wire it into my plugin | [Domain overview](domains/README.en.md) |
| Look up a full method signature | [C++ API](reference/cpp-api.md) · [LSE API](reference/lse-api.md) *(zh)* |
| Understand resend, heartbeat and ABI stamping | [Architecture overview](architecture/overview.en.md) |
| Debug a missing packet or a wrong render | [Protocol notes](architecture/protocol-notes.md) *(zh)* |
