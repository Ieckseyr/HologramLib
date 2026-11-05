#include "core/PacketSink.h"

#include "DiagLog.h"

#include <ll/api/service/Bedrock.h>

#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <array>
#include <atomic>
#include <cstdlib>

namespace debugshape_export::wire {

namespace {

Counters gCounters;

::NetworkPeer* peerFor(::Player& player) noexcept {
    auto networkSystem = ::ll::service::getNetworkSystem();
    if (!networkSystem) {
        return nullptr;
    }
    return networkSystem->getPeerForUser(player.getNetworkIdentifier());
}

constexpr ::NetworkPeer::Reliability toBds(Reliability r) noexcept {
    switch (r) {
    case Reliability::ReliableOrdered:
        return ::NetworkPeer::Reliability::ReliableOrdered;
    case Reliability::UnreliableSequenced:
        return ::NetworkPeer::Reliability::UnreliableSequenced;
    default:
        return ::NetworkPeer::Reliability::Reliable;
    }
}

constexpr std::uint32_t headerValue(std::uint32_t pid) noexcept {
    return (pid & 0x3FF) | ((0u & 3u) << 10) | ((0u & 3u) << 12);
}

enum class ValidatePolicy { Once,
                            All,
                            Off };

ValidatePolicy validatePolicy() noexcept {
    static ValidatePolicy const policy = [] {
        if (char const* v = std::getenv("HOLO_WIRE_VALIDATE"); v != nullptr) {
            if (v[0] == 'a' || v[0] == 'A')
                return ValidatePolicy::All;
            if (v[0] == 'o' || v[0] == 'O')
                return ValidatePolicy::Off;
        }
        return ValidatePolicy::Once;
    }();
    return policy;
}

} // namespace

namespace {

constexpr std::size_t kMaxTrackedPacketId = 512;

enum class ValidationState : std::uint8_t { Unknown = 0,
                                            Ok      = 1,
                                            Bad     = 2 };

std::atomic<std::uint8_t>& validationSlot(std::uint32_t packetId) noexcept {
    static std::array<std::atomic<std::uint8_t>, kMaxTrackedPacketId> states{};
    return states[packetId];
}

} // namespace

// 每个包型号第一次校验；失败过的型号每次都校验
bool shouldValidate(std::uint32_t packetId) noexcept {
    switch (validatePolicy()) {
    case ValidatePolicy::All:
        return true;
    case ValidatePolicy::Off:
        return false;
    case ValidatePolicy::Once:
    default:
        break;
    }
    if (packetId >= kMaxTrackedPacketId) {
        return true;
    }
    auto const state = static_cast<ValidationState>(validationSlot(packetId).load(std::memory_order_relaxed));
    return state != ValidationState::Ok;
}

void noteValidationResult(std::uint32_t packetId, bool ok) noexcept {
    if (packetId >= kMaxTrackedPacketId) {
        return;
    }

    if (ok) {
        validationSlot(packetId).store(static_cast<std::uint8_t>(ValidationState::Ok), std::memory_order_relaxed);
    } else {
        validationSlot(packetId).store(static_cast<std::uint8_t>(ValidationState::Bad), std::memory_order_relaxed);
    }
}

std::size_t headerLength(std::uint32_t packetId) noexcept {
    std::uint32_t value = headerValue(packetId);
    std::size_t   len   = 1;
    while (value & ~0x7Fu) {
        ++len;
        value >>= 7;
    }
    return len;
}

std::string withHeader(std::uint32_t packetId, std::string&& body) {
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

Counters& counters() {
    return gCounters;
}

void noteValidationFailure(std::string_view packetName) {
    gCounters.droppedInvalid.fetch_add(1, std::memory_order_relaxed);
    HLIB_LOG_WARN("[wire] sculk packet validation failed for {}", packetName);
}

bool validateBody(std::uint32_t packetId, std::string const& body) {

    ReadOnlyBinaryStream stream(body, false);
    auto                 packet = ::MinecraftPackets::createPacket(static_cast<::MinecraftPacketIds>(packetId));
    if (!packet || !packet->read(stream)) {
        return false;
    }
    if (stream.ensureReadCompleted().has_value()) {
        return true;
    }

    static std::atomic<bool> warned{false};
    if (!warned.exchange(true, std::memory_order_relaxed)) {
        HLIB_LOG_WARN("[wire] packet id {} has trailing bytes after read (线格式与 BDS 读端不一致)", packetId);
    }
    static bool const strict = [] {
        char const* v = std::getenv("HOLO_WIRE_STRICT");
        return v != nullptr && v[0] == '1';
    }();
    return !strict;
}

bool sendBody(
    ::Player&        player,
    std::uint32_t    packetId,
    std::string      body,
    Reliability      r,
    std::string_view packetName,
    bool             validate) {
    if (body.empty()) {
        return false;
    }

    if (validate && shouldValidate(packetId)) {
        bool const ok = validateBody(packetId, body);
        noteValidationResult(packetId, ok);
        if (!ok) {
            noteValidationFailure(packetName);
            return false;
        }
    }

    counters().encoded.fetch_add(1, std::memory_order_relaxed);
    return sendRaw(player, withHeader(packetId, std::move(body)), r, packetName, packetId);
}

namespace {

void deliver(::NetworkPeer& peer, ::Player& player, std::string const& bytes, Reliability r,
             std::string_view name, std::uint32_t id) {
    peer.sendPacket(bytes, toBds(r), ::Compressibility::Compressible);
    gCounters.sent.fetch_add(1, std::memory_order_relaxed);
    noteSent(name, id, player, bytes.size());
}

} // namespace

bool sendRaw(::Player& player, std::string const& bytes, Reliability r, std::string_view name, std::uint32_t id) {
    auto* peer = peerFor(player);
    if (peer == nullptr) {
        gCounters.droppedNoPeer.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    deliver(*peer, player, bytes, r, name, id);
    return true;
}

bool sendRawToPeer(::NetworkPeer& peer, ::Player& player, std::string const& bytes, Reliability r,
                   std::string_view name, std::uint32_t id) {
    deliver(peer, player, bytes, r, name, id);
    return true;
}

// 没有在线玩家也算成功（调用方用返回值判断"库这一侧可用"）
bool sendRawToAll(std::string const& bytes, Reliability r) {
    auto level = ::ll::service::getLevel();
    if (!level) {
        return false;
    }

    level->forEachPlayer([&](::Player& player) -> bool {
        (void)sendRaw(player, bytes, r);
        return true;
    });
    return true;
}

bool sendRawToDimension(int dimId, std::string const& bytes, Reliability r) {
    auto level = ::ll::service::getLevel();
    if (!level) {
        return false;
    }
    level->forEachPlayer([&](::Player& player) -> bool {
        if (static_cast<int>(player.getDimensionId()) == dimId) {
            (void)sendRaw(player, bytes, r);
        }
        return true;
    });
    return true;
}

} // namespace debugshape_export::wire
