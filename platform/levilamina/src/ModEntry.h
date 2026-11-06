#pragma once

#include "core/ServiceHost.h"

#include <ll/api/mod/NativeMod.h>

namespace debugshape_export {

class ModEntry {
public:
    static ModEntry& getInstance();

    ModEntry()
        : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();

    bool enable();

    bool disable();

private:
    ll::mod::NativeMod& mSelf;

    core::ServiceHost mHost;
};

} // namespace debugshape_export
