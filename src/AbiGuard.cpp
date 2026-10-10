// AbiGuard.cpp - 库侧"调用前检查消费方"
//
// 做法: 在库的唯一入口（IHologramLib::getInstance）处认出**调用方模块**, 读它导出的布局戳
//   （HologramLib.h 的 hologramlib_consumerAbiStamp —— 每个包含该头文件的模块都会导出自己那份),
//   与库自己的 HOLOGLIB_ABI_STAMP 比对。结果按模块缓存, 之后每次入口调用只多一次原子读。
// 为什么只记录、不在这里拒绝:
//   · 戳不一致的消费方**可能**没用到错位的那些槽（例如只用 shapes/holograms 的插件）——
//     在这里杀进程会误伤一批本来能跑的插件;
//   · 真正的拒绝放在消费方侧（它能把"重新编译/同批部署"这类人话写进日志）。
// 这里保证: 即使消费方什么自检都没写, 不一致也会被**记录**下来（hologramlib_abiStatus 可查）。
// 热重载同样适用: 新载入的模块在它第一次调用入口时被分类。
#include "hologramlib/HologramLib.h"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <mutex>

namespace debugshape_export::abi {

namespace {

enum : std::uint32_t {
    kOk            = 1, // 戳一致: 这个消费方与库是同一版布局
    kMismatch      = 2, // 戳不一致: 它按别的布局编的（调用会打到别的方法上）
    kNoDeclaration = 3, // 老构建（没带这个导出）/ 宿主进程 —— 无法判定
};

constexpr unsigned kMaxModules = 64;

struct Entry {
    std::atomic<std::uint64_t> mod{0};
    std::atomic<std::uint32_t> verdict{0};
};

Entry                 gEntries[kMaxModules];
std::atomic<unsigned> gCount{0};
std::mutex            gInsertLock;

std::atomic<std::uint64_t> gMismatchHits{0};     // 累计命中次数（每次入口调用都算）
std::atomic<std::uint64_t> gFirstMismatchMod{0}; // 第一个不一致的模块句柄

HMODULE moduleOf(void const* address) noexcept {
    HMODULE mod = nullptr;
    ::GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(address),
        &mod
    );
    return mod;
}

std::uint32_t classify(HMODULE mod) noexcept {
    auto const stamp = reinterpret_cast<std::uint32_t const*>(
        ::GetProcAddress(mod, HOLOGLIB_CONSUMER_STAMP_SYMBOL)
    );
    if (stamp == nullptr) return kNoDeclaration;
    return *stamp == HOLOGLIB_ABI_STAMP ? kOk : kMismatch;
}

std::uint32_t verdictOf(HMODULE mod) noexcept {
    auto const key = reinterpret_cast<std::uint64_t>(mod);

    // 快路径: 已分类过（热路径 —— 每次入口调用都走这里, 只有原子读）
    unsigned const n = gCount.load(std::memory_order_acquire);
    for (unsigned i = 0; i < n; ++i) {
        if (gEntries[i].mod.load(std::memory_order_relaxed) == key) {
            return gEntries[i].verdict.load(std::memory_order_relaxed);
        }
    }

    // 慢路径: 首次见到这个模块 —— 读它的导出戳, 落表
    auto const      verdict = classify(mod);
    std::lock_guard lock(gInsertLock);
    unsigned const  n2 = gCount.load(std::memory_order_relaxed);
    for (unsigned i = 0; i < n2; ++i) { // 可能被别人先插了
        if (gEntries[i].mod.load(std::memory_order_relaxed) == key) return gEntries[i].verdict.load();
    }
    if (n2 < kMaxModules) {
        gEntries[n2].mod.store(key, std::memory_order_relaxed);
        gEntries[n2].verdict.store(verdict, std::memory_order_relaxed);
        gCount.store(n2 + 1, std::memory_order_release);
    }
    return verdict;
}

} // namespace

// 入口处调用（HologramLibImpl 的 getInstance 里传入 _ReturnAddress() —— 即**调用方**模块里的返回地址）。
// 注意: 这个函数**不能**自己调 _ReturnAddress()（那拿到的是库内部的地址）; 必须由调用点传进来。
void checkCaller(void const* returnAddress) noexcept {
    auto const self   = moduleOf(reinterpret_cast<void const*>(&hologramlib_consumerAbiStamp));
    auto const caller = moduleOf(returnAddress);
    if (caller == nullptr || caller == self) return; // 库自身 / 认不出模块 → 不查

    if (verdictOf(caller) != kMismatch) return;

    gMismatchHits.fetch_add(1, std::memory_order_relaxed);
    std::uint64_t expected = 0;
    gFirstMismatchMod.compare_exchange_strong(
        expected,
        reinterpret_cast<std::uint64_t>(caller),
        std::memory_order_relaxed
    );
}

} // namespace debugshape_export::abi

// ── 状态查询导出（纯 C; 库侧不打印任何东西, 由调用方把它变成人话）────────────────
// outMismatchHits         : 命中"布局不一致消费方"的累计次数（0 = 至今没发现）
// outFirstMismatchModule  : 第一个不一致模块的句柄（可用 GetModuleFileNameW 换成路径 → 就知道是哪个
//                           插件; 0 = 没有）
// outMismatchModuleCount  : 目前记录下来的不一致模块数
// 消费方在 enable 时调一次就能把"谁需要重新编译"打进日志。
extern "C" __declspec(dllexport) void __cdecl hologramlib_abiStatus(
    std::uint32_t* outMismatchHits,
    std::uint64_t* outFirstMismatchModule,
    std::uint32_t* outMismatchModuleCount
) {
    using namespace debugshape_export::abi;
    if (outMismatchHits != nullptr) {
        *outMismatchHits = static_cast<std::uint32_t>(gMismatchHits.load(std::memory_order_relaxed));
    }
    if (outFirstMismatchModule != nullptr) {
        *outFirstMismatchModule = gFirstMismatchMod.load(std::memory_order_relaxed);
    }
    if (outMismatchModuleCount != nullptr) {
        std::uint32_t  count = 0;
        unsigned const n     = gCount.load(std::memory_order_acquire);
        for (unsigned i = 0; i < n; ++i) {
            if (gEntries[i].verdict.load(std::memory_order_relaxed) == kMismatch) ++count;
        }
        *outMismatchModuleCount = count;
    }
}
