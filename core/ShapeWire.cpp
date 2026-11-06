#include "core/ShapeWire.h"

#include "sculk/protocol/codec/packet/PrimitiveShapesPacket.hpp"

namespace debugshape_export::core {

std::string buildPrimitiveShapesBody(std::vector<DebugShape> const& shapes) {
    sculk::protocol::PrimitiveShapesPacket packet;
    packet.mShapes = shapes;
    return codec::body(packet);
}

std::string buildPrimitiveShapesPacket(std::vector<DebugShape> const& shapes) {
    return codec::withHeader(kPrimitiveShapesPacketId, buildPrimitiveShapesBody(shapes));
}

} // namespace debugshape_export::core
