#include "kernel/TickSource.h"

#include "core/HookRegistry.h"

#include <ll/api/memory/Hook.h>

#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>

#include <atomic>
#include <mutex>
#include <utility>
#include <vector>

namespace debugshape_export::core {

namespace {

std::mutex                                               gMutex;
std::vector<std::pair<TickToken, std::function<void()>>> gCallbacks;
std::atomic<TickToken>                                   gNextToken{1};
std::atomic<std::uint64_t>                               gLastTickId{0};

void fireTick() {
    std::vector<std::function<void()>> snapshot;
    {
        std::lock_guard lock(gMutex);
        snapshot.reserve(gCallbacks.size());
        for (auto const& [token, callback] : gCallbacks) {
            snapshot.push_back(callback);
        }
    }
    for (auto& callback : snapshot) {
        if (callback) {
            callback();
        }
    }
}

} // namespace

TickToken addTick(std::function<void()> callback) {
    if (!callback) {
        return 0;
    }
    TickToken const token = gNextToken.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard lock(gMutex);
    gCallbacks.emplace_back(token, std::move(callback));
    return token;
}

void removeTick(TickToken token) {
    if (token == 0) {
        return;
    }
    std::lock_guard lock(gMutex);
    std::erase_if(gCallbacks, [token](auto const& entry) {
        return entry.first == token;
    });
}

namespace {

// Level::tick 每个维度各调一次，用 server tick 计数去重，保证每帧只跑一遍
LL_TYPE_INSTANCE_HOOK(
    LevelTickSourceHook,
    ll::memory::HookPriority::Normal,
    ::Level,
    &::Level::$tick,
    void) {
    origin();
    auto const tickId = static_cast<std::uint64_t>(this->getCurrentServerTick().tickID);
    if (tickId != 0) {
        auto const previous = gLastTickId.exchange(tickId, std::memory_order_relaxed);
        if (previous == tickId) {
            return;
        }
    }
    fireTick();
}

ll::memory::HookRegistrar<LevelTickSourceHook> gLevelTickSourceHook;
HookRegistrar                                  gHookReg_levelTick{
    "kernel-leveltick",
    [] {
        decltype(gLevelTickSourceHook)::hook();
    },
    [] {
        decltype(gLevelTickSourceHook)::unhook();
    },
};

} // namespace

} // namespace debugshape_export::core
