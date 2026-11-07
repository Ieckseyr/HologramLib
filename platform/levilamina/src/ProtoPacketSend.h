#pragma once

#include "kernel/PacketSink.h"

namespace debugshape_export {

inline std::atomic<std::uint64_t>& protoSendFailureCount() {
    return wire::counters().droppedInvalid;
}

template <typename PacketT>
void sendProtoPacketToPlayer(::Player& player, PacketT const& packet) {
    (void)wire::send(player, packet);
}

} // namespace debugshape_export
