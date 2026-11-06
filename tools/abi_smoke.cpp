// ABI 冒烟测试：用纯 C++（不经过 Python）动态加载 holo_core.dll 并调用 C ABI。
// 编译：clang++ abi_smoke.cpp -o abi_smoke.exe
// 运行：abi_smoke.exe <holo_core.dll 路径>

#include <windows.h>

#include <cstdint>
#include <cstdio>

using fn_version = std::uint32_t (*)();
using fn_packet_id = std::uint32_t (*)();
using fn_encode_hud = std::int32_t (*)(const std::uint8_t*, std::size_t, std::uint8_t, std::uint8_t**, std::size_t*);
using fn_free = void (*)(void*);

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: abi_smoke <holo_core.dll>\n");
        return 2;
    }
    HMODULE module = LoadLibraryA(argv[1]);
    if (module == nullptr) {
        std::printf("LoadLibrary failed: %lu\n", GetLastError());
        return 1;
    }

    auto version = reinterpret_cast<fn_version>(GetProcAddress(module, "holo_core_api_version"));
    auto shapes_id = reinterpret_cast<fn_packet_id>(GetProcAddress(module, "holo_core_packet_id_primitive_shapes"));
    auto free_fn = reinterpret_cast<fn_free>(GetProcAddress(module, "holo_free"));
    if (version == nullptr || shapes_id == nullptr || free_fn == nullptr) {
        std::printf("missing symbol(s)\n");
        return 1;
    }

    std::printf("api_version=0x%08X primitive_shapes=%u\n", version(), shapes_id());

    // 真正走一次编码器：SetHud（mode=0, 全开）
    auto encode_hud = reinterpret_cast<fn_encode_hud>(GetProcAddress(module, "holo_core_encode_set_hud"));
    if (encode_hud == nullptr) {
        std::printf("no holo_core_encode_set_hud\n");
        return 1;
    }
    std::uint8_t* body = nullptr;
    std::size_t len = 0;
    std::uint8_t elements[1] = {0};  // Element::PaperDoll
    std::int32_t rc = encode_hud(elements, 1, 0u, &body, &len);
    std::printf("encode_set_hud rc=%d len=%zu first=%02X%02X\n", rc, len,
                len > 0 ? body[0] : 0u, len > 1 ? body[1] : 0u);
    if (body != nullptr) {
        free_fn(body);
    }
    FreeLibrary(module);
    return rc < 0 ? 1 : 0;
}
