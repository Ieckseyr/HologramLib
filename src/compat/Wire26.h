#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include <sculk/protocol/codec/actor/player/SerializedSkin.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/AddPlayerPacket.hpp>
#include <sculk/protocol/codec/packet/BossEventPacket.hpp>
#include <sculk/protocol/codec/packet/PlayerListPacket.hpp>
#include <sculk/protocol/codec/packet/PlayerSkinPacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

namespace debugshape_export::wire26 {

inline std::size_t uvLen(std::uint32_t v) {
    std::size_t n = 1;
    while (v >= 0x80) {
        v >>= 7;
        ++n;
    }
    return n;
}

inline void putUv(std::string& b, std::uint32_t v) {
    while (v >= 0x80) {
        b.push_back(static_cast<char>((v & 0x7F) | 0x80));
        v >>= 7;
    }
    b.push_back(static_cast<char>(v));
}

inline void putVi64(std::string& b, std::int64_t v) {
    std::uint64_t z = (static_cast<std::uint64_t>(v) << 1) ^ static_cast<std::uint64_t>(v >> 63);
    while (z >= 0x80) {
        b.push_back(static_cast<char>((z & 0x7F) | 0x80));
        z >>= 7;
    }
    b.push_back(static_cast<char>(z));
}

inline void putU8(std::string& b, std::uint8_t v) {
    b.push_back(static_cast<char>(v));
}

inline void putBool(std::string& b, bool v) {
    b.push_back(v ? 1 : 0);
}

inline void putS32(std::string& b, std::int32_t v) {
    b.append(reinterpret_cast<char const*>(&v), 4);
}

inline void putU32(std::string& b, std::uint32_t v) {
    b.append(reinterpret_cast<char const*>(&v), 4);
}

inline void putU64(std::string& b, std::uint64_t v) {
    b.append(reinterpret_cast<char const*>(&v), 8);
}

inline void putF32(std::string& b, float v) {
    b.append(reinterpret_cast<char const*>(&v), 4);
}

inline void putStr(std::string& b, std::string const& s) {
    putUv(b, static_cast<std::uint32_t>(s.size()));
    b += s;
}

template <class T>
std::string sculkBody(T const& packet) {
    std::vector<std::byte>        buffer;
    sculk::protocol::BinaryStream stream{buffer};
    packet.write(stream);
    return std::string(reinterpret_cast<char const*>(buffer.data()), buffer.size());
}

std::string skinBody(sculk::protocol::SerializedSkin const& skin);
std::string playerListBody(sculk::protocol::PlayerListPacket const& packet);
std::string playerSkinBody(sculk::protocol::PlayerSkinPacket const& packet);
std::string addPlayerBody(sculk::protocol::AddPlayerPacket const& packet);
std::string addActorBody(sculk::protocol::AddActorPacket const& packet);
std::string bossEventBody(sculk::protocol::BossEventPacket const& packet);

template <class T>
std::string body26(T const& packet) {
    return sculkBody(packet);
}

std::string body26(sculk::protocol::AddPlayerPacket const& packet);
std::string body26(sculk::protocol::AddActorPacket const& packet);
std::string body26(sculk::protocol::BossEventPacket const& packet);
std::string body26(sculk::protocol::PlayerListPacket const& packet);
std::string body26(sculk::protocol::PlayerSkinPacket const& packet);

} // namespace debugshape_export::wire26
