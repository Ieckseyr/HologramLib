# C++ 接口（LeviLamina）

LeviLamina 侧的 C++ 消费方式，底层就是 **ABI 接口**（`holo_core.dll` 的 C ABI）的类型化包装。
本页只做导览，具体内容都在 ABI 大节里。

| 想看 | 去哪 |
|---|---|
| 这层 ABI 是什么、怎么加载、内存怎么释放 | [ABI 接口总览](../abi/index.md) |
| 68 个导出符号的完整清单 | [ABI 符号清单](../abi/symbols.md) |
| 每个纯虚接口的完整签名（`IShapeDrawer`、`IHologramText`…） | [LeviLamina 的 C++ 包装](../abi/levilamina/index.md) |

## 和 LSE 的关系

同一套能力，两种入口：

```cpp
// C++
auto& lib = hologramlib::IHologramLib::getInstance();
lib.shapes().createText({0, 100, 0}, "hello");
```

```js
// LSE
const create = ll.import("HologramLib", "shapeCreateText");
create(0, 100, 0, "hello");
```

两者背后是同一组管理器，发出的包和产生的状态完全一致。LSE 的函数清单见 [LSE 脚本接口](../lse/index.md)。

## 为什么 C++ 页放在 ABI 下面

因为**同一份字节**同时供三个消费方使用：LeviLamina 的 C++ 接口、endstone 的 C++ 插件、
以及 Python（endstone 插件本体）。把协议编码抽成 C ABI 之后，C++ 那层只是薄包装 ——
文档跟着接口走，才不会出现"两个平台各写一套、还写得不一样"。
