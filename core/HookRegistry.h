#pragma once

#include <cstddef>

namespace debugshape_export {
namespace core {

// 注册表
struct HookEntry {
    char const* name;
    void (*enable)();
    void (*disable)();
};

void registerHook(HookEntry const& entry);

// 库是否启用
bool live();
void setLive(bool value);

// 静态期注册器
struct HookRegistrar {
    HookEntry entry;
    HookRegistrar(char const* name, void (*enable)(), void (*disable)())
        : entry{name, enable, disable} {
        registerHook(entry);
    }
};
void        hookAll();
void        unhookAll();
std::size_t hookCount();
std::size_t releasedCount();

} // namespace core
} // namespace debugshape_export
