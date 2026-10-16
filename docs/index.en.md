<div align="center">
  <img src="assets/HologramLib.svg" alt="HologramLib — unified protocol-layer hologram & display library for Bedrock servers" width="100%">
</div>

# HologramLib documentation

[中文](index.md) · [English](index.en.md) · [Project home](../README.en.md)

HologramLib is a protocol-layer display library for Minecraft Bedrock dedicated servers (LeviLamina / BDS). It folds 18 capability domains into one prerequisite-only plugin, usable from both C++ plugins (frozen virtual interfaces) and LSE scripts (`ll.import`).

Current: plugin `26.51.3` · API `1.29.0` (`0x012300`) · LeviLamina 26.51 / BDS 1.26.51 / protocol 2193.

> Partial translation. The English tree covers the entry layer: this hub, [getting started](getting-started.en.md), the [domain overview](domains/README.en.md) and the [architecture overview](architecture/overview.en.md). Every other page falls back to the Chinese original (the site's language switcher jumps straight to it). The C++ and LSE API references are Chinese-only for now; they are large, change with every release, and would otherwise need two maintained copies.

---

## Find your path

| You are… | Start here | Then |
|---|---|---|
| New to this library | [Getting started](getting-started.en.md) — build, deploy, first feature | [Domain overview](domains/README.en.md) to pick a capability |
| Wiring a capability into your plugin | [Domain guides](domains/README.en.md) — how each works, plus field-tested pitfalls | [C++ API](reference/cpp-api.md) / [LSE API](reference/lse-api.md) for signatures *(zh)* |
| Writing LSE scripts | [LSE API reference](reference/lse-api.md) — all 16 prefix domains *(zh)* | Script-side examples in the [domain guides](domains/README.en.md) |
| Debugging a packet or render problem | [Protocol notes](architecture/protocol-notes.md) — wire format, tick, packet captures *(zh)* | The "field-tested" section of that domain |
| Chasing a version / ABI crash | [Version & ABI matrix](versions/api-version-matrix.md) *(zh)* | ABI rules in the [architecture overview](architecture/overview.en.md#abi-why-append-only-is-load-bearing) |
| Writing or reviewing library code | [Design notes](design-contracts.en.md) — what may live in the library, and the boundary cases already settled | How those boundaries appear at runtime in the [architecture overview](architecture/overview.en.md) |
| Maintaining / contributing | [README · Design notes](../README.en.md#design-notes) | Architecture & protocol notes |

---

## Layout

```
docs/
├── index.md                       This hub (zh)
├── index.en.md                    This hub (en)
├── getting-started.md             Build / deploy / first feature
├── design-contracts.md            Design notes: what may live in the library, what stays in the consumer
├── domains/                       Per-domain how-to + pitfalls
├── reference/                     Signatures: the API surface, method by method
├── architecture/                  Internals: why it is built this way
├── versions/                      Changelog and the version/ABI matrix
├── internal/                    ⚠ Maintainer-local docs — NOT published, NOT committed
└── assets/HologramLib.svg         Header image
```

> ⚠ `docs/internal/` is local-only and forbidden from GitHub. It is excluded by `.gitignore`, kept out of the site by `mkdocs.yml`'s `exclude_docs`, and a CI guard in `.github/workflows/docs.yml` fails the build if it ever gets committed. That is why nothing above links into it.

---

## Three things to know before reading

1. A "capability domain" is one independently usable surface. On the C++ side it is an interface (`IShapeDrawer`, `IPlayerNpc`, …); on the LSE side it is a function-name prefix (`shape*`, `playerNpc*`, …). Domains are independent; only a few relate (the sulfur-cube domain delegates entirely to `ICustomEntity`).

2. Version numbers in the docs are the minimum API version a capability appeared in. Use them in `static_assert` gates. The three numbering schemes (release version / API version / `HOLOGLIB_API_VERSION`) are explained in the [version & ABI matrix](versions/api-version-matrix.md) *(zh)*.

3. A "consumer" is a plugin that calls this library. HologramLib stores nothing, implements no business logic, and never decides when a consumer should resend something. Persistence, commands, permissions and forms all live in the consumer, which is the answer to "why doesn't the library store this for me?"

---

## Glossary

| Term | Meaning |
|---|---|
| Capability domain / domain | A set of related APIs, e.g. the "view override" domain. One C++ interface, one LSE prefix |
| Consumer | A plugin calling HologramLib (C++ or LSE): MeowHolographicRenderer, MSkinventory, ZXPanel |
| Prerequisite library | This library's role: mechanisms only, no business logic. Consumers call `getInstance()` in `enable()`; no declared dependency |
| Intercept-only | The view-override restriction: the outbound hook only decides send or drop, and never mutates engine packet fields |
| Timing notification | A callback from the library (e.g. "entity spawned for this player"); the consumer decides what to resend |
| Interface compatibility | `IHologramLib` and every domain interface gain virtuals only at the end of the class; existing signatures and semantics never change |
| Vtable slot | A pure virtual's position in the C++ vtable. Inserting or deleting in the middle silently misaligns pre-compiled consumers |
| ABI layout stamp | A vtable-layout descriptor in the header plus a consumer-side compile-time check; the library rejects mismatched consumers at runtime |
| Documented shrink | Removal of already-released API (normally a major-version change), recorded per release with the reason and migration path |
| Carrier entity | The invisible entity behind an interface domain (NPC dialogue / trade menu). Deleted when the UI closes; never saved |

---

## Where the numbers come from

Version numbers and function counts in these docs are taken from code, never from prose written afterwards:

| Fact | Source of truth |
|---|---|
| Plugin release version | `version` in `manifest.json` |
| API version / `HOLOGLIB_API_VERSION` | `include/hologramlib/HologramLib.h` |
| LSE export list and count | The `exportAs(NAMESPACE, ...)` calls in `src/` |
| C++ interface signatures | `include/hologramlib/HologramLib.h` |

If the docs disagree with the code, the code wins — please open an issue. This documentation pass already corrected several stale numbers (the API version had been stuck at 1.27.0, `view*` was undercounted by 2, and the `trade*` / `npcDialog*` lists were missing entries). See the [changelog](versions/changelog.md) *(zh)*.
