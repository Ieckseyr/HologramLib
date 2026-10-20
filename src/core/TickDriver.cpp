#include "core/TickDriver.h"

#include "DiagLog.h"

#include <ll/api/memory/Hook.h>

#include <mc/world/level/Level.h>

#include <cstring>

namespace debugshape_export::tick {

TickDriver& TickDriver::getInstance() noexcept {
    static TickDriver instance;
    return instance;
}

bool TickDriver::add(char const* name, Callback cb, Order order) {
    if (name == nullptr || cb == nullptr) return false;
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
    // 插入排序: 注册是一次性的, 数量 <= kMax, 图的是 run() 里顺序天然正确
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
    // 热路径: 一次原子读, 无锁
    std::size_t const n = mPublished.load(std::memory_order_acquire);
    for (std::size_t i = 0; i < n; ++i) {
        mEntries[i].cb();
    }
}

std::size_t TickDriver::size() const noexcept { return mPublished.load(std::memory_order_acquire); }

std::string TickDriver::describe() const {
    std::string out;
    std::size_t const n = mPublished.load(std::memory_order_acquire);
    for (std::size_t i = 0; i < n; ++i) {
        if (!out.empty()) out += " -> ";
        out += mEntries[i].name;
        out += '(';
        out += std::to_string(mEntries[i].order);
        out += ')';
    }
    return out;
}

} // namespace debugshape_export::tick

namespace debugshape_export {

// ── 全库唯一的 Level::tick hook ────────────────────────────────────────────────
// origin() 在这里调用**一次**，各域的回调只做自己的事（原 4 个 hook 各自 origin() 的那份
// 已合并到这里）。注册表为空时只剩这一次转发, 成本与一个空 detour 相同。
LL_TYPE_INSTANCE_HOOK(LevelTickDriverHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();
    tick::TickDriver::getInstance().run();
}

static ll::memory::HookRegistrar<LevelTickDriverHook> gLevelTickDriverHookRegistrar;

} // namespace debugshape_export
