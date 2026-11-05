#include "core/HookRegistry.h"

#include <ll/api/mod/NativeMod.h>

#include <array>
#include <atomic>

namespace debugshape_export {
namespace core {

namespace {

constexpr std::size_t kMaxHooks = 24;

struct Registry {
    std::array<HookEntry, kMaxHooks> entries{};
    std::size_t                      count{};
    std::size_t                      released{}; // 0 = 全部挂着
};

Registry& registry() {
    static Registry r;
    return r;
}

} // namespace

namespace {
std::atomic<bool> gLive{true};
}

bool live() {
    return gLive.load(std::memory_order_acquire);
}

void setLive(bool value) {
    gLive.store(value, std::memory_order_release);
}

void registerHook(HookEntry const& entry) {
    Registry& r = registry();
    if (r.count >= kMaxHooks) {
        return;
    }
    r.entries[r.count++] = entry;
}

void hookAll() {
    Registry& r = registry();
    if (r.released == 0) {
        return;
    }
    for (std::size_t i = 0; i < r.count; ++i) {
        r.entries[i].enable();
    }
    if (auto mod = ll::mod::NativeMod::current(); mod) {
        mod->getLogger().info("[hooks] 已重新挂载 {} 个 detour", r.count);
    }
    r.released = 0;
}

void unhookAll() {
    Registry& r = registry();
    if (r.released != 0) {
        return;
    }
    for (std::size_t i = 0; i < r.count; ++i) {
        r.entries[i].disable();
    }
    if (auto mod = ll::mod::NativeMod::current(); mod) {
        mod->getLogger().info("[hooks] 已摘下 {} 个 detour", r.count);
    }
    r.released = r.count;
}

std::size_t hookCount() {
    return registry().count;
}

std::size_t releasedCount() {
    return registry().released;
}

} // namespace core
} // namespace debugshape_export
