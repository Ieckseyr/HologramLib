#include "hologramlib/HologramLib.h"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <mutex>

namespace debugshape_export::abi {

namespace {

enum : std::uint32_t {
    kOk            = 1,
    kMismatch      = 2,
    kNoDeclaration = 3,
};

constexpr unsigned kMaxModules = 64;

struct Entry {
    std::atomic<std::uint64_t> mod{0};
    std::atomic<std::uint32_t> verdict{0};
};

Entry                 gEntries[kMaxModules];
std::atomic<unsigned> gCount{0};
std::mutex            gInsertLock;

std::atomic<std::uint64_t> gMismatchHits{0};
std::atomic<std::uint64_t> gFirstMismatchMod{0};

HMODULE moduleOf(void const* address) noexcept {
    HMODULE mod = nullptr;
    ::GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(address),
        &mod);
    return mod;
}

std::uint32_t classify(HMODULE mod) noexcept {
    auto const stamp = reinterpret_cast<std::uint32_t const*>(
        ::GetProcAddress(mod, HOLOGLIB_CONSUMER_STAMP_SYMBOL));
    if (stamp == nullptr)
        return kNoDeclaration;
    return *stamp == HOLOGLIB_ABI_STAMP ? kOk : kMismatch;
}

std::uint32_t verdictOf(HMODULE mod) noexcept {
    auto const key = reinterpret_cast<std::uint64_t>(mod);

    unsigned const n = gCount.load(std::memory_order_acquire);
    for (unsigned i = 0; i < n; ++i) {
        if (gEntries[i].mod.load(std::memory_order_relaxed) == key) {
            return gEntries[i].verdict.load(std::memory_order_relaxed);
        }
    }

    auto const      verdict = classify(mod);
    std::lock_guard lock(gInsertLock);
    unsigned const  n2 = gCount.load(std::memory_order_relaxed);
    for (unsigned i = 0; i < n2; ++i) {
        if (gEntries[i].mod.load(std::memory_order_relaxed) == key)
            return gEntries[i].verdict.load();
    }
    if (n2 < kMaxModules) {
        gEntries[n2].mod.store(key, std::memory_order_relaxed);
        gEntries[n2].verdict.store(verdict, std::memory_order_relaxed);
        gCount.store(n2 + 1, std::memory_order_release);
    }
    return verdict;
}

} // namespace

void checkCaller(void const* returnAddress) noexcept {
    static auto const self   = moduleOf(reinterpret_cast<void const*>(&hologramlib_consumerAbiStamp));
    auto const        caller = moduleOf(returnAddress);
    if (caller == nullptr || caller == self)
        return;

    if (verdictOf(caller) != kMismatch)
        return;

    gMismatchHits.fetch_add(1, std::memory_order_relaxed);
    std::uint64_t expected = 0;
    gFirstMismatchMod.compare_exchange_strong(
        expected,
        reinterpret_cast<std::uint64_t>(caller),
        std::memory_order_relaxed);
}

} // namespace debugshape_export::abi

extern "C" __declspec(dllexport) void __cdecl hologramlib_abiStatus(
    std::uint32_t* outMismatchHits,
    std::uint64_t* outFirstMismatchModule,
    std::uint32_t* outMismatchModuleCount) {
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
            if (gEntries[i].verdict.load(std::memory_order_relaxed) == kMismatch)
                ++count;
        }
        *outMismatchModuleCount = count;
    }
}
