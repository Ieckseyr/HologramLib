#include "ProtocolPackets.h"

#include <ll/api/service/Bedrock.h>
#include <mc/network/NetworkSystem.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include "sculk/protocol/codec/utility/deps/BinaryStream.hpp"

namespace debugshape_export {

//   VarUInt(header) + VarUInt(shapeCount) + [shape...]
std::string ProtocolPacketWriter::buildDebugDrawerPacket(std::vector<DebugShape> const& shapes) {
    using sculk::protocol::abi_v944::DebugDrawerPacket;
    using sculk::protocol::abi_v944::BinaryStream;

    DebugDrawerPacket packet;
    packet.mShapes = shapes;

    std::vector<std::byte> buffer;
    buffer.reserve(256);
    BinaryStream stream{buffer};
    packet.writeWithHeader(stream);

    return std::string{stream.asStringView()};
}


// NetworkPeer获取并发送
static NetworkPeer* getPlayerPeer(::Player& player) {
    auto networkSystem = ll::service::getNetworkSystem();
    if (!networkSystem.has_value()) return nullptr;
    auto& nid = player.getNetworkIdentifier();
    return networkSystem->getPeerForUser(nid);
}

bool ProtocolPacketWriter::sendToPlayer(::Player& player, std::vector<DebugShape> const& shapes) {
    if (shapes.empty()) return true;
    auto* peer = getPlayerPeer(player);
    if (!peer) return false;
    std::string packetData = buildDebugDrawerPacket(shapes);
    peer->sendPacket(
        packetData,
        NetworkPeer::Reliability::ReliableOrdered,
        Compressibility::Compressible
    );
    return true;
}

bool ProtocolPacketWriter::sendToAll(std::vector<DebugShape> const& shapes) {
    if (shapes.empty()) return true;
    std::string packetData = buildDebugDrawerPacket(shapes);

    auto level = ll::service::getLevel();
    if (!level.has_value()) return false;

    auto networkSystem = ll::service::getNetworkSystem();
    if (!networkSystem.has_value()) return false;

    level->forEachPlayer([&](::Player& player) -> bool {
        auto& nid = player.getNetworkIdentifier();
        auto* peer = networkSystem->getPeerForUser(nid);
        if (peer) {
            peer->sendPacket(
                packetData,
                NetworkPeer::Reliability::ReliableOrdered,
                Compressibility::Compressible
            );
        }
        return true;
    });
    return true;
}

bool ProtocolPacketWriter::sendToDimension(int dimId, std::vector<DebugShape> const& shapes) {
    if (shapes.empty()) return true;
    std::string packetData = buildDebugDrawerPacket(shapes);

    auto level = ll::service::getLevel();
    if (!level.has_value()) return false;

    auto networkSystem = ll::service::getNetworkSystem();
    if (!networkSystem.has_value()) return false;

    level->forEachPlayer([&](::Player& player) -> bool {
        if (player.getDimensionId().id != dimId) return true;
        auto& nid = player.getNetworkIdentifier();
        auto* peer = networkSystem->getPeerForUser(nid);
        if (peer) {
            peer->sendPacket(
                packetData,
                NetworkPeer::Reliability::ReliableOrdered,
                Compressibility::Compressible
            );
        }
        return true;
    });
    return true;
}

} // namespace debugshape_export
