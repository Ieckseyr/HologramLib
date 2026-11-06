#include "core/PacketCodec.h"

namespace debugshape_export::codec {

std::size_t headerLength(std::uint32_t packetId) noexcept {
    std::uint32_t value = headerValue(packetId);
    std::size_t   len   = 1;
    while (value & ~0x7Fu) {
        ++len;
        value >>= 7;
    }
    return len;
}

std::string withHeader(std::uint32_t packetId, std::string body) {
    std::uint32_t value = headerValue(packetId);

    std::string out;
    out.reserve(body.size() + 5);
    while (value & ~0x7Fu) {
        out.push_back(static_cast<char>((value & 0x7F) | 0x80));
        value >>= 7;
    }
    out.push_back(static_cast<char>(value));
    out.append(body);
    return out;
}

} // namespace debugshape_export::codec
