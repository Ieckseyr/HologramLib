// ProtocolPackets.cpp - 形状列表（PrimitiveShapesPacket）的编码与投递
//
// 收敛说明（PR1）: 本文件原来自带一份 peer 解析 + 两个 forEachPlayer 循环，
// 与 SculkPacketSend.h、ItemDisplayManager.cpp 两份实现重复。现在:
//   · 编码走 wire::encodeUnchecked（writeWithHeader, 热路径不做回读 —— 与收敛前逐字一致）
//   · 投递走 wire::sendRaw / sendRawToAll / sendRawToDimension（peer 解析全库只此一份）
// 对外 API 与行为不变（含"没有在线玩家也返回 true"的口径），PacketDebugRenderer 无需改动。
#include "DiagFlags.h"
#include "ProtocolPackets.h"

#include "core/PacketSink.h"

namespace debugshape_export {

// 形状/悬浮字发包（26.40）；HLIB_NO_SHAPE=1 可关。
static bool shapePacketsEnabled() { return !hlibDomainDisabled("HLIB_NO_SHAPE"); }

std::string ProtocolPacketWriter::buildDebugDrawerPacket(std::vector<DebugShape> const& shapes) {
    using sculk::protocol::PrimitiveShapesPacket;

    PrimitiveShapesPacket packet;
    packet.mShapes = shapes;
    return wire::encodeUnchecked(packet);
}

bool ProtocolPacketWriter::sendToPlayer(::Player& player, std::vector<DebugShape> const& shapes) {
    if (shapes.empty()) return true;
    if (!shapePacketsEnabled()) return true;
    return wire::sendRaw(player, buildDebugDrawerPacket(shapes), wire::Reliability::ReliableOrdered);
}

bool ProtocolPacketWriter::sendToAll(std::vector<DebugShape> const& shapes) {
    if (shapes.empty()) return true;
    if (!shapePacketsEnabled()) return true;
    return wire::sendRawToAll(buildDebugDrawerPacket(shapes), wire::Reliability::ReliableOrdered);
}

bool ProtocolPacketWriter::sendToDimension(int dimId, std::vector<DebugShape> const& shapes) {
    if (shapes.empty()) return true;
    if (!shapePacketsEnabled()) return true;
    return wire::sendRawToDimension(dimId, buildDebugDrawerPacket(shapes), wire::Reliability::ReliableOrdered);
}

} // namespace debugshape_export
