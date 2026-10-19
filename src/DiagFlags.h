#pragma once

// 诊断开关，默认关（26.40）。
#include <cstdlib>

namespace debugshape_export {

inline bool hlibEnvFlag(char const* name) {
    char const* v = std::getenv(name);
    return v != nullptr && v[0] == '1';
}

inline bool hlibDomainDisabled(char const* name) {
    static bool const safeMode = hlibEnvFlag("HLIB_SAFE_MODE");
    return safeMode || hlibEnvFlag(name);
}

} // namespace debugshape_export
