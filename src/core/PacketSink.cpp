#include "core/PacketSink.h"

#include "DiagLog.h"

#include <ll/api/service/Bedrock.h>

#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export::wire {

namespace {

Counters gCounters;

::NetworkPeer* peerFor(::Player& player) noexcept {
    auto networkSystem = ::ll::service::getNetworkSystem();
    if (!networkSystem) return nullptr;
    return networkSystem->getPeerForUser(player.getNetworkIdentifier());
}

constexpr ::NetworkPeer::Reliability toBds(Reliability r) noexcept {
    return r == Reliability::ReliableOrdered ? ::NetworkPeer::Reliability::ReliableOrdered
                                             : ::NetworkPeer::Reliability::Reliable;
}

} // namespace

Counters& counters() { return gCounters; }

void noteValidationFailure(std::string_view packetName) {
    gCounters.droppedInvalid.fetch_add(1, std::memory_order_relaxed);
    HLIB_LOG_WARN("[wire] sculk packet validation failed for {}", packetName);
}

bool sendRaw(::Player& player, std::string_view bytes, Reliability r) {
    auto* peer = peerFor(player);
    if (peer == nullptr) {
        gCounters.droppedNoPeer.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    peer->sendPacket(std::string{bytes}, toBds(r), ::Compressibility::Compressible);
    gCounters.sent.fetch_add(1, std::memory_order_relaxed);
    return true;
}

// 注意: 下面两个 "ToAll/ToDimension" 沿用收敛前的返回值口径 ——
// 原 ProtocolPacketWriter::sendToAll/sendToDimension 在**没有在线玩家**时也返回 true
// （调用方据此判断的是"库这一侧是否可用"，不是"有没有人收到"）。
// 只有 level / networkSystem 缺失才算失败。
bool sendRawToAll(std::string_view bytes, Reliability r) {
    auto level         = ::ll::service::getLevel();
    auto networkSystem = ::ll::service::getNetworkSystem();
    if (!level || !networkSystem) return false;

    std::string const data{bytes};
    level->forEachPlayer([&](::Player& player) -> bool {
        (void)sendRaw(player, data, r);
        return true;
    });
    return true;
}

bool sendRawToDimension(int dimId, std::string_view bytes, Reliability r) {
    auto level         = ::ll::service::getLevel();
    auto networkSystem = ::ll::service::getNetworkSystem();
    if (!level || !networkSystem) return false;

    std::string const data{bytes};
    level->forEachPlayer([&](::Player& player) -> bool {
        if (static_cast<int>(player.getDimensionId()) != dimId) return true;
        (void)sendRaw(player, data, r);
        return true;
    });
    return true;
}

} // namespace debugshape_export::wire
