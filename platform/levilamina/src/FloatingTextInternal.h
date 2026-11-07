#pragma once

#include <chrono>
#include <cstdint>

namespace debugshape_export {

inline std::uint64_t steadyMs() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

} // namespace debugshape_export
