# LeviLamina 版

HologramLib 的 LeviLamina 版本：一个 C++ 插件（`HologramLib.dll`），**两种调用方式**共用同一套引擎内实现。

| 你是 | 用 | 入口 |
|---|---|---|
| 写 LSE 脚本 | `ll.import("HologramLib", "函数名")` | [LSE 脚本接口](../lse/index.md) |
| 写 C++ 插件 | 链接 `HologramLib.lib`，`getInstance()` 拿接口 | [C++ 接口参考](../reference/cpp-api.md) |

两种入口背后是同一组管理器（形状、悬浮字、实体、容器…），所以同一个能力无论从哪边走，
发出的包和产生的状态完全一致；C++ 侧多了直接的接口强类型，LSE 侧多了动态调用与热重载。

## 30 秒上手（LSE）

```js
const create = ll.import("HologramLib", "holoCreate");
const addLine = ll.import("HologramLib", "holoAddLine");
const draw = ll.import("HologramLib", "holoDraw");

const id = create(0, 101, 0);
addLine(id, "§6在线 §f{online}");
draw(id);
```

## 30 秒上手（C++）

```cpp
auto& lib = hologramlib::HologramLib::getInstance();
auto* shapes = lib.getShapeDrawer();
if (shapes) {
    shapes->createText({0, 100, 0}, "hello");
}
```

## 这一节怎么读

1. **LSE 脚本接口** —— 按域分组，每个函数一节：参数、返回值、示例、坑。
   脚本导入（lrca 直连 / 统一导入）先看 [脚本导入](../lse/importing.md)。
2. **C++ 接口参考** —— 每个类的完整签名、线程与生命周期约定。

## 和 Endstone 版的关系

| | LeviLamina | Endstone |
|---|---|---|
| 插件形态 | C++ DLL（`HologramLib.zip`） | Python wheel + `holo_core.dll` |
| 调用方式 | C++ 接口 / LSE `ll.import` | Python 属性 |
| 协议编码 | `core/` 编进 DLL | `core/` 编成 `holo_core.dll` |

两边**字节同源**：同一个形状、同一个悬浮字，发出的包体完全一致。endstone 侧的写法见 [Endstone 版](../endstone/index.md)。
