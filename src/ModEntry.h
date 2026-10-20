#pragma once

#include "core/ServiceHost.h"

#include <ll/api/mod/NativeMod.h>

namespace debugshape_export {

class ModEntry {
public:
    static ModEntry& getInstance();

    ModEntry() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

private:
    ll::mod::NativeMod& mSelf;
    // 域注册与生命周期（core/ServiceHost.h）: 新增域只改 ModEntry.cpp 的 add 一处
    core::ServiceHost mHost;
};

} // namespace debugshape_export
