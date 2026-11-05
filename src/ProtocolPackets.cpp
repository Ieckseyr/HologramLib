#include "ProtocolPackets.h"
#include "DiagFlags.h"

#include "core/PacketSink.h"

namespace debugshape_export {

static bool shapePacketsEnabled() {
    return !hlibDomainDisabled("HLIB_NO_SHAPE");
}

std::string ProtocolPacketWriter::buildDebugDrawerPacket(std::vector<DebugShape> const& shapes) {

    using sculk::protocol::PrimitiveShapesPacket;

    PrimitiveShapesPacket packet;
    packet.mShapes = shapes;
    return wire::encode(packet, false);
}

bool ProtocolPacketWriter::sendToPlayer(::Player& player, std::vector<DebugShape> const& shapes) {
    if (shapes.empty())
        return true;
    if (!shapePacketsEnabled())
        return true;
    return wire::sendRaw(player, buildDebugDrawerPacket(shapes), wire::Reliability::ReliableOrdered);
}

bool ProtocolPacketWriter::sendToAll(std::vector<DebugShape> const& shapes) {
    if (shapes.empty())
        return true;
    if (!shapePacketsEnabled())
        return true;
    return wire::sendRawToAll(buildDebugDrawerPacket(shapes), wire::Reliability::ReliableOrdered);
}

bool ProtocolPacketWriter::sendToDimension(int dimId, std::vector<DebugShape> const& shapes) {
    if (shapes.empty())
        return true;
    if (!shapePacketsEnabled())
        return true;
    return wire::sendRawToDimension(dimId, buildDebugDrawerPacket(shapes), wire::Reliability::ReliableOrdered);
}

} // namespace debugshape_export
