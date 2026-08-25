// SculkPacketSend.h - sculk 协议包通用发送原语（跨域共享）
//
// 流程: sculk 包 write 序列化 -> BDS createPacket 同型包回读验证 ->
// 手动补 VarUInt 包头 -> NetworkPeer::sendPacket 原始字节流发送。
// 与 ItemDisplayManager.cpp 内的同名实现同源（1.10.0 提取共享,
// ItemDisplay 侧暂保持原副本, 验证稳定后统一迁移）。
#pragma once

#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkSystem.h>
#include <mc/world/actor/player/Player.h>

#include <sculk/protocol/codec/packet/IPacket.hpp>
#include <sculk/protocol/codec/utility/deps/BinaryStream.hpp>

#include <string>
#include <vector>

namespace debugshape_export {

// 发送 sculk 构造的协议包到指定玩家（验证失败仅告警丢弃, 不抛出）
template <typename PacketT>
void sendSculkPacketToPlayer(::Player& player, PacketT const& packet) {
    std::vector<std::byte>        bodyBuffer;
    sculk::protocol::BinaryStream bodyStream(bodyBuffer);
    packet.write(bodyStream);

    std::string          checkBuffer(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());
    ReadOnlyBinaryStream checkStream(checkBuffer, true);
    auto                 checkPacket = MinecraftPackets::createPacket(static_cast<MinecraftPacketIds>(packet.getId()));
    if (!checkPacket || !checkPacket->read(checkStream)) {
        extern void logSculkPacketSendFailure(char const* name);
        logSculkPacketSendFailure(std::string(packet.getName()).c_str());
        return;
    }

    BinaryStream sendStream;
    sendStream.writeUnsignedVarInt(
        (static_cast<int>(packet.getId()) & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12),
        nullptr,
        nullptr
    );
    sendStream.mBuffer.append(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());

    auto networkSystem = ll::service::getNetworkSystem();
    if (networkSystem) {
        auto const& networkId = player.getNetworkIdentifier();
        auto*       peer      = networkSystem->getPeerForUser(networkId);
        if (peer != nullptr) {
            peer->sendPacket(sendStream.mBuffer, NetworkPeer::Reliability::Reliable, Compressibility::Compressible);
        }
    }
}

} // namespace debugshape_export