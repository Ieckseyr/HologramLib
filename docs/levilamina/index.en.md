# LeviLamina edition

The LeviLamina build of HologramLib: a C++ plugin (`HologramLib.dll`) with **two calling conventions** over the same
in-engine implementation.

| You are | Use | Entry point |
|---|---|---|
| Writing LSE scripts | `ll.import("HologramLib", "functionName")` | [LSE scripting API](../lse/index.md) |
| Writing C++ plugins | link `HologramLib.lib`, call `getInstance()` | [C++ API reference](../reference/cpp-api.md) |

Both entry points drive the same managers (shapes, holograms, entities, containers…), so a capability behaves
identically either way: same packets, same state.

## 30-second start (LSE)

```js
const create = ll.import("HologramLib", "holoCreate");
const addLine = ll.import("HologramLib", "holoAddLine");
const draw = ll.import("HologramLib", "holoDraw");

const id = create(0, 101, 0);
addLine(id, "§6online §f{online}");
draw(id);
```

## 30-second start (C++)

```cpp
auto& lib = hologramlib::HologramLib::getInstance();
auto* shapes = lib.getShapeDrawer();
if (shapes) {
    shapes->createText({0, 100, 0}, "hello");
}
```

## How this section is organised

1. **LSE scripting API** — per domain, one section per function: parameters, return value, examples, pitfalls.
   Start with [importing](../lse/importing.md).
2. **C++ API reference** — full signatures, threading and lifetime rules.

## Relation to the Endstone edition

| | LeviLamina | Endstone |
|---|---|---|
| Plugin form | C++ DLL (`HologramLib.zip`) | Python wheel + `holo_core.dll` |
| Calling | C++ interfaces / LSE `ll.import` | Python attributes |
| Packet encoding | `core/` compiled into the DLL | `core/` built as `holo_core.dll` |

Both are **byte-identical**. See the [Endstone edition](../endstone/index.md) for the other side.
