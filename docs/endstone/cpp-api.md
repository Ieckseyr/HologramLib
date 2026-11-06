# Endstone C++ 接口

Endstone 端有两层：日常用 Python（`import hologramlib_endstone as lib`），需要自己拼协议包时可以直接调 **C++ ABI**。两层编的是同一份 `core/` 代码，发出的字节**完全一致**。

这一页给"我知道自己在干什么、想绕过 Python 层"的人看：C++ ABI 长什么样、怎么链、能拿到什么。

## 它是什么

`core/` 里的协议编码器被编成一个纯 C 接口的动态库：

```
hologramlib_endstone/_native/holo_core.dll     # 随 wheel 一起装，无需单独下载
```

- 纯 C 导出（`extern "C"`），没有 C++ 标准库类型跨越边界
- 平台无关：同一个 DLL 给 Endstone 的 Python 层用，也给你的 C++ 插件/工具用
- 不含 Bedrock 服务端代码：只做"把结构体编码成包体字节"，发包由调用方自己决定

## 链接方式

### 方式一：从 C++ 里直接 `LoadLibrary`

```cpp
#include <windows.h>

using encode_fn = int32_t (*)(int32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t,
                              uint8_t**, size_t*);

HINSTANCE dll = LoadLibraryA("holo_core.dll");
auto encodeBlock = reinterpret_cast<encode_fn>(
    GetProcAddress(dll, "holo_core_encode_update_block"));
```

### 方式二：从 Python 里用 ctypes

```python
import ctypes
from hologramlib_endstone.native import NativeCore

core = NativeCore()
core.lib.holo_core_encode_update_block(...)
```

`NativeCore` 已经帮你把路径、参数类型和返回值类型都设好了，所以**在 Python 里调 C++ ABI 不需要写 ctypes 样板**。

## 导出分类

完整符号表见 [ABI 接口](../abi/index.md)，这里只列类别：

| 类别 | 前缀 | 说明 |
|---|---|---|
| 包 id | `holo_core_packet_id_*` | 返回该包在 26.60 协议里的数字 id |
| 编码器 | `holo_core_encode_*` | 把参数编成包体字节（不含包头），返回 `0` 成功 |
| 版本 | `holo_core_api_version` | 与 `HOLOGLIB_API_VERSION` 同值 |

编码器统一签名：

```c
int32_t holo_core_encode_xxx(..., uint8_t** out_body, size_t* out_len);
```

- 成功返回 `0`，`out_body` 是新分配的包体，`out_len` 是长度
- 失败返回负数，`out_body` 为 `NULL`
- 用完必须 `holo_core_free(out_body)`

## 示例：编一个假方块包

```c
uint8_t* body = NULL;
size_t   len  = 0;

if (holo_core_encode_update_block(100, 64, 100, /*runtime_id=*/2150698529, 3, 0, &body, &len) == 0) {
    // body/len 是 UpdateBlock 的包体；包头由调用方按 varint(id) + body 拼
    holo_core_free(body);
}
```

包头规则：`varint(包 id) + 包体`，即 `holo_core_packet_id_update_block()` 的值。

## 什么时候用哪层

| 需求 | 用哪层 |
|---|---|
| 建悬浮字、形状、假方块、菜单 | Python 层（`lib.*`） |
| 自己写协议实验、抓包对比、编一个库里还没封装的包 | C++ ABI |
| 在 Endstone 插件里零拷贝地复用编码器 | C++ ABI + `ctypes`/`LoadLibrary` |
| 写跨平台工具（不依赖 Bedrock 服务端） | C++ ABI |

## 注意

- ABI 只保证**同版本内**稳定；跨版本升级请重新编译或按 `holo_core_api_version` 判断。
- 包体字节与 LeviLamina 端同源：同一组参数，两边编出来的 body 应该逐字节相同，可以直接用抓包对比验证。
- 只给包体，不给包头/压缩/加密：这些由调用方按自己的传输层处理。
