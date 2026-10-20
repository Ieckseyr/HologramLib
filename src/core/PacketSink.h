// PacketSink.h - 全库唯一的发包出口
//
// 收敛前有三份等价实现，且已开始分叉:
//   1. src/SculkPacketSend.h                    回读校验 + 共享失败计数
//   2. src/itemdisplay/ItemDisplayManager.cpp   匿名 namespace 内的同名副本（**不计数**）
//   3. src/ProtocolPackets.cpp                  writeWithHeader, 各自 forEachPlayer 循环
// 现在: 编码 / 回读校验 / 补包头 / peer 解析 / 可靠性 / 计数 全部在本文件，
// 调用点只表达"发给谁、发什么"。新增协议包只需要 include 这个头。
//
// 线程模型: 与库内其余部分一致 —— 只在主线程网络路径上调用。
// 计数用 relaxed 原子，因为诊断读数可能来自别的线程（视图覆盖域的 describeFor）。
#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/network/MinecraftPackets.h>

#include <sculk/protocol/codec/packet/IPacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

class Player;

namespace debugshape_export {
namespace wire {

// 可靠性口径（收敛前两条路径用的不是同一个值，这里显式化，默认保持旧行为）
enum class Reliability {
    Reliable,        // 原 sendSculkPacketToPlayer 的口径
    ReliableOrdered, // 原 ProtocolPacketWriter 的口径（形状列表）
};

struct Counters {
    std::atomic<std::uint64_t> encoded{0};        // 编码次数（含校验失败）
    std::atomic<std::uint64_t> sent{0};           // 真正写进 peer 的次数
    std::atomic<std::uint64_t> droppedNoPeer{0};  // 找不到 peer（玩家刚掉线等）
    std::atomic<std::uint64_t> droppedInvalid{0}; // 回读校验失败 → 静默丢弃
};

Counters& counters();

// 校验失败的唯一落点（计数 + 诊断日志）。模板内不能直接打日志: 日志开关是编译期的，
// 而这里要保证"关诊断时失败依然被计数"，所以放到 .cpp。
void noteValidationFailure(std::string_view packetName);

// ── 传输: 只认字节流 ──────────────────────────────────────────────────────────
// bytes 必须是**含包头**的完整包体（两个 encode 的产物都满足）。
bool sendRaw(::Player& player, std::string_view bytes, Reliability r = Reliability::Reliable);
bool sendRawToAll(std::string_view bytes, Reliability r = Reliability::Reliable);
bool sendRawToDimension(int dimId, std::string_view bytes, Reliability r = Reliability::Reliable);

// ── 编码 ─────────────────────────────────────────────────────────────────────
// checked（默认）: body 写出 → 用 BDS 同型包回读校验 → 手动补 VarUInt 包头。
//   校验失败返回空串并计入 droppedInvalid，绝不把错位字节发给客户端。
// Unchecked: writeWithHeader 一次成型，不做回读。
//   用于形状列表这条热路径（每 tick 可能多次），与收敛前的行为逐字一致。
template <class PacketT>
std::string encodeChecked(PacketT const& packet) {
    std::vector<std::byte>        bodyBuffer;
    sculk::protocol::BinaryStream bodyStream(bodyBuffer);
    packet.write(bodyStream);

    std::string          checkBuffer(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());
    ReadOnlyBinaryStream checkStream(checkBuffer, true);
    auto                 checkPacket = ::MinecraftPackets::createPacket(static_cast<::MinecraftPacketIds>(packet.getId()));
    if (!checkPacket || !checkPacket->read(checkStream)) {
        noteValidationFailure(packet.getName());
        return {};
    }

    ::BinaryStream sendStream;
    sendStream.writeUnsignedVarInt(
        (static_cast<int>(packet.getId()) & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12),
        nullptr,
        nullptr
    );
    sendStream.mBuffer.append(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());

    counters().encoded.fetch_add(1, std::memory_order_relaxed);
    return sendStream.mBuffer;
}

template <class PacketT>
std::string encodeUnchecked(PacketT const& packet) {
    std::vector<std::byte>        buffer;
    sculk::protocol::BinaryStream stream{buffer};
    buffer.reserve(256);
    packet.writeWithHeader(stream);

    counters().encoded.fetch_add(1, std::memory_order_relaxed);
    return std::string{stream.asStringView()};
}

// 编码 + 发送（校验不过则丢弃，不算发送成功）
template <class PacketT>
bool send(::Player& player, PacketT const& packet, Reliability r = Reliability::Reliable) {
    auto bytes = encodeChecked(packet);
    if (bytes.empty()) return false;
    return sendRaw(player, bytes, r);
}

} // namespace wire
} // namespace debugshape_export
