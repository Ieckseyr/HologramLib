// SculkPacketSend.h - 兼容层（实现已迁至 core/PacketSink.h）
//
// 历史: 本文件曾是"全库共享的发包原语"，但 ItemDisplayManager.cpp 与 ProtocolPackets.cpp
// 各自留了一份副本，且副本已经不计数、不告警。收敛见 core/PacketSink.h 文件头。
//
// 保留本头只为不制造一次无意义的 25 处调用点 diff: 它现在只是两行转发。
// **新代码请直接用 debugshape_export::wire::**，本文件在下一个大版本删除。
#pragma once

#include "core/PacketSink.h"

namespace debugshape_export {

// 旧名保留（视图覆盖域与 diag 输出在用）: 指的就是 wire 的 droppedInvalid 计数
inline std::atomic<std::uint64_t>& sculkSendFailureCount() { return wire::counters().droppedInvalid; }

template <typename PacketT>
void sendSculkPacketToPlayer(::Player& player, PacketT const& packet) {
    (void)wire::send(player, packet);
}

} // namespace debugshape_export
