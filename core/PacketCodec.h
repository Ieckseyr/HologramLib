#pragma once

// 平台无关的报文编码：sculk 包 -> body -> 带包头字节串。
// 这一层只做字节，不碰任何引擎对象（Player / Level / NetworkPeer 都在平台层）。

#include <cstdint>
#include <string>

#include "core/Wire26.h"

namespace debugshape_export::codec {

// 包头 = varint(packetId & 0x3FF)，高 4 位保留（当前恒 0）。
constexpr std::uint32_t headerValue(std::uint32_t packetId) noexcept { return packetId & 0x3FFu; }

[[nodiscard]] std::size_t headerLength(std::uint32_t packetId) noexcept;

[[nodiscard]] std::string withHeader(std::uint32_t packetId, std::string body);

// sculk 包的 body（26.60 wire）
template <class PacketT>
[[nodiscard]] std::string body(PacketT const& packet) {
    return wire26::body26(packet);
}

// 完整字节串（包头 + body）
template <class PacketT>
[[nodiscard]] std::string packet(PacketT const& packet) {
    return withHeader(static_cast<std::uint32_t>(packet.getId()), body(packet));
}

} // namespace debugshape_export::codec
