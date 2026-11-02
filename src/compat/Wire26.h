// Wire26.h - 1.26.60（协议 2225）与 sculk 2168（26.40）线格式有差异的包，按 26.60 重新编码
//
// 背景：HologramLib 里的 BedrockProtocol（sculk codec）是 26.40 的模型，26.60 有几处字段级变更。
// 这里只对**真变了**的包做 26.60 编码，其余包继续走 sculk。
// 依据：protocol-docs r26_u6（Minecraft 1.26.61.1 / 2225）的 JSON schema，
//       以及同版本服务端上 BDS 自己序列化出来的字节（tools 里跑过对比）。
//
// 已覆盖：
//   AddPlayerPacket   Actor Links 之后、Device Id 之前多了可选字段 PassengerBlockData（无 → 1 字节 0）
//   AddActorPacket    末尾多了同一个可选字段（无 → 1 字节 0）
//   BossEventPacket   去掉 PlayerID；EventType/Color/Overlay 由 varuint 变 uint8
//   PlayerListPacket  条目在 XUID 之后多了 PlayFab ID（空串）；皮肤去掉 PlayFabId
//   PlayerSkinPacket  皮肤去掉 PlayFabId
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

// ── 标量写入（与 sculk BinaryStream 的编码一致）──
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

inline void putU8(std::string& b, std::uint8_t v) { b.push_back(static_cast<char>(v)); }

inline void putBool(std::string& b, bool v) { b.push_back(v ? 1 : 0); }

inline void putS32(std::string& b, std::int32_t v) { b.append(reinterpret_cast<char const*>(&v), 4); }

inline void putU32(std::string& b, std::uint32_t v) { b.append(reinterpret_cast<char const*>(&v), 4); }

inline void putU64(std::string& b, std::uint64_t v) { b.append(reinterpret_cast<char const*>(&v), 8); }

inline void putF32(std::string& b, float v) { b.append(reinterpret_cast<char const*>(&v), 4); }

inline void putStr(std::string& b, std::string const& s) {
    putUv(b, static_cast<std::uint32_t>(s.size()));
    b += s;
}

// 用 sculk 自己的 write() 序列化（未变更的包 / 子结构）
template <class T>
std::string sculkBody(T const& packet) {
    std::vector<std::byte>        buffer;
    sculk::protocol::BinaryStream stream{buffer};
    packet.write(stream);
    return std::string(reinterpret_cast<char const*>(buffer.data()), buffer.size());
}

// ── 26.60 专门编码 ──
std::string skinBody(sculk::protocol::SerializedSkin const& skin);
std::string playerListBody(sculk::protocol::PlayerListPacket const& packet);
std::string playerSkinBody(sculk::protocol::PlayerSkinPacket const& packet);
std::string addPlayerBody(sculk::protocol::AddPlayerPacket const& packet);
std::string addActorBody(sculk::protocol::AddActorPacket const& packet);
std::string bossEventBody(sculk::protocol::BossEventPacket const& packet);

// 统一入口：有专门编码的走专门编码，其余走 sculk
template <class T>
std::string body26(T const& packet) {
    return sculkBody(packet);
}

// 26.60 有字段级变更的包的专门编码（非模板重载，优先于上面的模板）
std::string body26(sculk::protocol::AddPlayerPacket const& packet);
std::string body26(sculk::protocol::AddActorPacket const& packet);
std::string body26(sculk::protocol::BossEventPacket const& packet);
std::string body26(sculk::protocol::PlayerListPacket const& packet);
std::string body26(sculk::protocol::PlayerSkinPacket const& packet);

} // namespace debugshape_export::wire26
