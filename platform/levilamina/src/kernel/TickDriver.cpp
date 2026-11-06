#include "core/TickDriver.h"
#include "kernel/EventIds.h"
#include "core/HookRegistry.h"

#include <ll/api/mod/NativeMod.h>

#include <atomic>

#include "DiagLog.h"

#include <ll/api/event/world/ServerLevelTickEvent.h>

#include <cstring>

namespace debugshape_export::tick {

TickDriver& TickDriver::getInstance() noexcept {
    static TickDriver instance;
    return instance;
}

bool TickDriver::add(char const* name, Callback cb, Order order) {
    if (name == nullptr || cb == nullptr)
        return false;
    if (mCount >= kMax) {
        HLIB_LOG_ERROR("[tick] registry full, drop '{}'", name);
        return false;
    }
    for (std::size_t i = 0; i < mCount; ++i) {
        if (std::strcmp(mEntries[i].name, name) == 0) {
            HLIB_LOG_ERROR("[tick] duplicate registration '{}'", name);
            return false;
        }
    }

    std::size_t pos = mCount;
    while (pos > 0 && mEntries[pos - 1].order > static_cast<int>(order)) {
        mEntries[pos] = mEntries[pos - 1];
        --pos;
    }
    mEntries[pos] = Entry{name, cb, static_cast<int>(order)};
    ++mCount;
    mPublished.store(mCount, std::memory_order_release);
    return true;
}

void TickDriver::run() {

    std::size_t const n = mPublished.load(std::memory_order_acquire);
    for (std::size_t i = 0; i < n; ++i) {
        mEntries[i].cb();
    }
}

std::size_t TickDriver::size() const noexcept {
    return mPublished.load(std::memory_order_acquire);
}

std::string TickDriver::describe() const {
    std::string       out;
    std::size_t const n = mPublished.load(std::memory_order_acquire);
    for (std::size_t i = 0; i < n; ++i) {
        if (!out.empty())
            out += " -> ";
        out += mEntries[i].name;
        out += '(';
        out += std::to_string(mEntries[i].order);
        out += ')';
    }
    return out;
}

} // namespace debugshape_export::tick

namespace debugshape_export::tick {

void installDriver() {
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;
    core::listen<ll::event::ServerLevelTickEvent>(
        core::kServerLevelTickId,
        [](ll::event::ServerLevelTickEvent const&) {
            TickDriver::getInstance().run();
        },
        core::ListenerScope::Process);
}

} // namespace debugshape_export::tick
