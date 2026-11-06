#include "ProtocolPackets.h"

#include "core/DiagFlags.h"
#include "core/ShapeWire.h"

#include "kernel/PacketSink.h"

namespace debugshape_export {

static bool shapePacketsEnabled() {
    return !hlibDomainDisabled("HLIB_NO_SHAPE");
}

// 字节由 core 统一产出（与 endstone 侧共用同一份编码），这里只负责发。
std::string ProtocolPacketWriter::buildDebugDrawerPacket(std::vector<DebugShape> const& shapes) {
    auto bytes = core::buildPrimitiveShapesPacket(shapes);
    wire::counters().encoded.fetch_add(1, std::memory_order_relaxed);
    return bytes;
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
