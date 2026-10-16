![HologramLib — unified protocol-layer hologram & display library for Bedrock servers](assets/HologramLib.png){ width="100%" }

# HologramLib

HologramLib is a **protocol-layer display library** for Minecraft Bedrock dedicated servers (LeviLamina / BDS): one set of APIs that puts shapes, holograms, items, entities and menus in front of the client — with no real entities spawned, nothing written to the save, and no server-side world state touched.

The same capabilities are reachable two ways: from **C++ plugins** (frozen pure-virtual interfaces) and from **LSE scripts** (`ll.import`).

| | |
|---|---|
| Plugin version | `26.51.3` |
| API version | `1.29.0` (`HOLOGLIB_API_VERSION 0x012300`) |
| Platform | LeviLamina 26.51 · BDS 1.26.51 (protocol 2193) |
| Script prerequisite | `lrca` (LegacyRemoteCall) |

---

## Where to start

| You are | Start here | Then |
|---|---|---|
| New to the library | [Getting started](getting-started.md) — build, deploy, first feature | Pick a domain in the [domain guides](domains/README.md) |
| Writing LSE scripts | [LSE scripting API](lse/index.md) — how to import, call and troubleshoot | [Look up functions by domain](lse/index.md) |
| Writing a C++ plugin | [C++ API reference](reference/cpp-api.md) — full signatures for every method | [Domain guides](domains/README.md) |
| Wondering what a domain can and cannot do | [Domain guides](domains/README.md) | The C++ / LSE pages for that domain |
| Looking for what changed in a release | [Changelog](versions/changelog.md) | — |

---

## What it can do

| Need | Entry point |
|---|---|
| Lines, boxes, spheres, circles, arrows, text shapes | [Shapes `shape*`](lse/shapes.md) |
| Multi-line holograms, rotating content pools, gradient lines | [Holograms & gradient lines `holo*` `gradient*`](lse/holograms.md) |
| "Diamond x64" item detail, 3D item display | [Item display `itemDetail*` `itemDisplay*`](lse/items.md) |
| Protocol-level fake players (NPCs) with skin registration / capture / import | [Fake-player NPCs `playerNpc*`](lse/player-npc.md) |
| Custom entities, particle shapes, interaction events | [Entities, particles & interaction `entity*` `particle*` `ghost*`](lse/entities.md) |
| Trade menu, virtual container, NPC dialogue | [Interfaces & containers `trade*` `container*` `npcDialog*`](lse/interfaces.md) |
| Faking your own inventory, sulfur-cube display | [Inventory & sulfur cube `fakeInv*` `sulfur*`](lse/inventory.md) |
| Reskinning / hiding mobs, reading the client device | [View override & sensing `view*` `sensing*`](lse/view.md) |

All **270 script functions** are listed in the [LSE scripting API](lse/index.md); the matching C++ interfaces (`IShapeDrawer`, `IHologramText`, `ICustomEntity`, `IViewOverride` …) are in the [C++ API reference](reference/cpp-api.md).

---

## Glossary

| Term | Meaning |
|---|---|
| Capability domain / domain | A group of related APIs. One C++ interface, one LSE function prefix (e.g. `view*`) |
| Consumer | A plugin calling this library (C++ or LSE), e.g. MeowHolographicRenderer, MSkinventory |
| Prerequisite library | The library's role: mechanisms only, no business logic. Consumers grab the interface via `getInstance()` in `enable()`; no declared dependency |
| Suppress-only | The view-override discipline: the outbound hook only decides send / drop, it never mutates engine packets |
| Timing notification | A callback the library sends to consumers (e.g. an entity finished spawning for a player); what to resend is the consumer's call |
| Carrier entity | The invisible entity interface domains use (trade menu, NPC dialogue); deleted when the interface closes, never persisted |

---

## Versions

- The three numbering schemes (plugin version / API version / `HOLOGLIB_API_VERSION`) and the per-release capability list are in the [changelog](versions/changelog.md).
- The only version source in code is `HOLOGLIB_API_VERSION` in `include/hologramlib/HologramLib.h`: gated at compile time with `static_assert`, handshaken at runtime via `IHologramLib::getInstance().version()`.
