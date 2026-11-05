#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/Packet.h>

#include <sculk/protocol/codec/packet/IPacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include "compat/Wire26.h"
#include "core/PacketTrace.h"

class Player;
class NetworkPeer;

namespace debugshape_export {
namespace wire {

enum class Reliability {
    Reliable,
    ReliableOrdered,
    UnreliableSequenced,
};

struct Counters {
    std::atomic<std::uint64_t> encoded{0};
    std::atomic<std::uint64_t> sent{0};
    std::atomic<std::uint64_t> droppedNoPeer{0};
    std::atomic<std::uint64_t> droppedInvalid{0};
};

Counters& counters();

void noteValidationFailure(std::string_view packetName);

std::size_t headerLength(std::uint32_t packetId) noexcept;

std::string withHeader(std::uint32_t packetId, std::string&& body);

bool shouldValidate(std::uint32_t packetId) noexcept;

bool validateBody(std::uint32_t packetId, std::string const& body);

bool sendRaw(
    ::Player&          player,
    std::string const& bytes,
    Reliability        r    = Reliability::Reliable,
    std::string_view   name = {},
    std::uint32_t      id   = 0);

bool sendRawToPeer(
    ::NetworkPeer&     peer,
    ::Player&          player,
    std::string const& bytes,
    Reliability        r    = Reliability::Reliable,
    std::string_view   name = {},
    std::uint32_t      id   = 0);

bool sendRawToAll(std::string const& bytes, Reliability r = Reliability::Reliable);

bool sendRawToDimension(int dimId, std::string const& bytes, Reliability r = Reliability::Reliable);

// sculk 协议包走 body26，引擎 Packet 走 BDS 自己的 write；两条都在这里加包头
template <class PacketT>
std::string encode(PacketT const& packet, bool validate = true) {
    constexpr bool isEngine = std::is_base_of_v<::Packet, PacketT>;

    auto const  id = static_cast<std::uint32_t>(packet.getId());
    std::string body;
    if constexpr (isEngine) {
        ::BinaryStream stream;
        packet.write(stream);
        body = std::string{stream.mBuffer};
    } else {
        body = wire26::body26(packet);
    }
    if (body.empty()) {
        return {};
    }

    if constexpr (!isEngine) {
        if (validate && shouldValidate(id) && !validateBody(id, body)) {
            noteValidationFailure(packet.getName());
            return {};
        }
    }

    counters().encoded.fetch_add(1, std::memory_order_relaxed);
    return withHeader(id, std::move(body));
}

template <class PacketT>
bool send(::Player& player, PacketT const& packet, Reliability r = Reliability::Reliable) {
    auto bytes = encode(packet);
    if (bytes.empty()) {
        return false;
    }
    return sendRaw(player, bytes, r, packet.getName(), static_cast<std::uint32_t>(packet.getId()));
}

bool sendBody(
    ::Player&        player,
    std::uint32_t    packetId,
    std::string      body,
    Reliability      r        = Reliability::Reliable,
    std::string_view name     = {},
    bool             validate = true);

} // namespace wire
} // namespace debugshape_export
