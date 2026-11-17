#pragma once

// 1.9.7 与 26.60 的引擎侧小差异：
//   1) 一部分包不是 ll::PayloadPacket，字段直接在包类上 → asPacket<T>()
//   2) Actor/Player 没有 distanceTo() → distance()

#include <cmath>

#include <mc/network/Packet.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>

namespace holo_compat {

template <class PacketT>
[[nodiscard]] inline PacketT const& asPacket(::Packet const& packet) {
    return static_cast<PacketT const&>(packet);
}

template <class PacketT>
[[nodiscard]] inline PacketT& asPacket(::Packet& packet) {
    return static_cast<PacketT&>(packet);
}

[[nodiscard]] inline float distance(::Vec3 const& a, ::Vec3 const& b) {
    float const dx = a.x - b.x;
    float const dy = a.y - b.y;
    float const dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

[[nodiscard]] inline float distance(::Actor const& a, ::Actor const& b) {
    return distance(a.getPosition(), b.getPosition());
}

} // namespace holo_compat
