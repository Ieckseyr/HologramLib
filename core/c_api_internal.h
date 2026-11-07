#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

namespace debugshape_export {

inline int32_t copyOut(std::string const& bytes, std::uint8_t** out, std::size_t* outLen) {
    if (out == nullptr || outLen == nullptr) {
        return -2;
    }
    auto* buffer = static_cast<std::uint8_t*>(std::malloc(bytes.size() == 0 ? 1 : bytes.size()));
    if (buffer == nullptr) {
        return -3;
    }
    std::memcpy(buffer, bytes.data(), bytes.size());
    *out    = buffer;
    *outLen = bytes.size();
    return 0;
}

} // namespace debugshape_export
