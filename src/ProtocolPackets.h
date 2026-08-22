// ProtocolPackets.h - 基于 Protocol v944 的数据包发送
//
// 使用 sculk::protocol::abi_v944::DebugDrawerPacket 构造完整数据包
// （含包头），再通过 BDS 底层 NetworkPeer::sendPacket 发送原始字节流。
#pragma once

#include <cstdint>
#include <vector>

#include "ProtocolShape.h"

// 仅用于发送的 BDS 类型（不参与序列化）
#include "mc/network/NetworkPeer.h"
#include "mc/network/Compressibility.h"
#include "mc/world/actor/player/Player.h"

namespace debugshape_export {
// 协议层数据包发送器
//
// 使用 v944 DebugDrawerPacket::writeWithHeader 构造完整字节流：
//   VarUInt(header) + VarUInt(shapeCount) + [shape...]
//
// 然后通过 NetworkPeer::sendPacket 发送原始字节。
class ProtocolPacketWriter {
public:
    // 构造完整的 DebugDrawer 数据包字节流（含包头）
    static std::string buildDebugDrawerPacket(std::vector<DebugShape> const& shapes);

    // 发送到指定玩家（通过 NetworkPeer::sendPacket）
    static bool sendToPlayer(::Player& player, std::vector<DebugShape> const& shapes);

    // 发送到所有玩家
    static bool sendToAll(std::vector<DebugShape> const& shapes);

    // 发送到指定维度
    static bool sendToDimension(int dimId, std::vector<DebugShape> const& shapes);
};

} // namespace debugshape_export
