# ABI 接口

HologramLib 的协议核心（`core/`）编译成 **`holo_core.dll`**，对外是一层**纯 C ABI**：
`extern "C"` + `__declspec(dllexport)`，没有 C++ 名字修饰、没有运行时、不依赖 LeviLamina 或 endstone 的任何头文件。

所以同一个 DLL 能被三种消费方直接吃下：

| 消费方 | 怎么接 | 现在谁在用 |
|---|---|---|
| **endstone C++ 插件** | `LoadLibrary` + `GetProcAddress`（或自建 `.lib`） | 可以，见下面示例 |
| **Python（endstone 插件）** | `ctypes.CDLL` | `platform/endstone` 本体 |
| **LeviLamina 插件** | 直接静态链接 `core/`（同一份源码） | `HologramLib.dll` |

!!! info "为什么单独抽一层 ABI"

    协议报文是「给客户端看的字节」，跟宿主引擎无关。抽成 C ABI 之后，
    形状/悬浮字/实体/容器这些能力只写一次，两个平台共用同一份编码，
    不会再出现「LL 发的是这套字节、endstone 发的是那套」。

| | |
|---|---|
| 导出符号 | **67** 个（30 个包 id + 34 个编码器 + 版本/释放） |
| 头文件 | `core/c_api.h`（可直接 include，也可以照着 `GetProcAddress`） |
| ABI 版本 | `holo_core_api_version()` → `1`；字符串版本用 `holo_core_version()` |
| 内存归属 | core 分配、调用方用 `holo_free(ptr)` 释放 |

完整符号表：[ABI 符号清单](symbols.md)。LeviLamina 侧的 C++ 类包装见 [LeviLamina 的 C++ 包装](levilamina/index.md)。

---

## 1. endstone C++ 插件怎么接

endstone 的 C++ 插件本身就是个普通 DLL（CMake + `endstone_add_plugin`），加载另一个 DLL 是常规操作。
最省事的是运行时加载，不需要 `.lib`：

```cpp
#include <windows.h>
#include <cstdint>
#include <cstdio>

using fn_packet_id = std::uint32_t (*)();
using fn_encode_hud = std::int32_t (*)(const std::uint8_t*, std::size_t, std::uint8_t,
                                       std::uint8_t**, std::size_t*);
using fn_free = void (*)(void*);

void send_hud(endstone::Player& player) {
    static HMODULE core = LoadLibraryA("plugins/hologramlib_endstone/_native/holo_core.dll");
    if (core == nullptr) return;

    auto shapes_id  = reinterpret_cast<fn_packet_id>(GetProcAddress(core, "holo_core_packet_id_primitive_shapes"));
    auto encode_hud = reinterpret_cast<fn_encode_hud>(GetProcAddress(core, "holo_core_encode_set_hud"));
    auto free_fn    = reinterpret_cast<fn_free>(GetProcAddress(core, "holo_free"));
    if (shapes_id == nullptr || encode_hud == nullptr || free_fn == nullptr) return;

    std::uint8_t elements[1] = {0};  // Element::PaperDoll
    std::uint8_t* body = nullptr;
    std::size_t   len  = 0;
    if (encode_hud(elements, 1, 0 /* Hide */, &body, &len) >= 0) {
        player.sendPacket(shapes_id(), std::string_view(reinterpret_cast<char*>(body), len));
        free_fn(body);
    }
}
```

想要链接期解析而不是 `GetProcAddress`，用 LLVM 的 `llvm-dlltool` 从符号表生成导入库：

```bash
llvm-dlltool -m i386:x86-64 -d holo_core.def -l holo_core.lib
# 然后 CMake: target_link_libraries(my_plugin PRIVATE holo_core)
```

!!! warning "DLL 要在搜索路径里"

    endstone 把 wheel 装到 `plugins/.local/Lib/site-packages/hologramlib_endstone/`，
    `holo_core.dll` 在它的 `_native/` 下。用绝对路径 `LoadLibraryA` 最稳；
    也可以先把那个目录 `AddDllDirectory`。

## 2. Python 怎么接（endstone 插件本体就是这么做的）

```python
import ctypes

lib = ctypes.CDLL(r"...\hologramlib_endstone\_native\holo_core.dll")
lib.holo_core_packet_id_primitive_shapes.restype = ctypes.c_uint32
lib.holo_core_encode_set_hud.restype = ctypes.c_int32
lib.holo_core_encode_set_hud.argtypes = [
    ctypes.c_char_p, ctypes.c_size_t, ctypes.c_uint8,
    ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t),
]
```

本仓库的 `platform/endstone/src/hologramlib_endstone/native.py` 就是这层的完整封装（全部 68 个符号）。

## 3. LeviLamina 怎么接

LL 版不走 DLL，而是**把 `core/` 直接静态链接进 `HologramLib.dll`**，所以零加载开销；
C++ 消费方看到的是纯虚接口（`IShapeDrawer`、`IHologramText`…），它们只是这层 ABI 的类型化包装。

```cpp
auto& lib = hologramlib::IHologramLib::getInstance();
lib.shapes().createText({0, 100, 0}, "hello");
```

细节见 [LeviLamina 的 C++ 包装](levilamina/index.md)。

---

## 4. 调用约定与内存

| 约定 | 规则 |
|---|---|
| 调用约定 | 默认 `__cdecl`（x64 下与 Win64 统一调用约定一致） |
| 返回值 | `int32_t`：`>= 0` 是 body 字节数（失败返回负数）；`uint32_t`：包 id |
| 输出缓冲 | 编码器写 `uint8_t** out_body` + `size_t* out_len`，**内存由 core 分配** |
| 释放 | 必须调 `holo_free(ptr)`；不要用调用方的 `free` / `delete` / `CoTaskMemFree` |
| 字符串 | 入参一律 UTF-8 `const char*`，core 不接管所有权 |
| 线程 | 编码器无全局可变状态，可多线程调用；包 id 查询同样无锁 |
| 版本 | 用 `holo_core_api_version()` 校验；不匹配就换 DLL，不要猜字段 |

!!! note "关于 holo_core.dll 的依赖"

    DLL 只依赖 VC++ 运行库与 Windows 系统 DLL，不依赖 BDS、endstone 或 LeviLamina。
    所以同一份 `holo_core.dll` 可以被服务端、独立工具、甚至离线脚本加载（本仓库的
    `tools/abi_smoke.cpp` 就是干这个的：加载 → 取版本 → 编一个 HUD 包 → 打印长度）。
