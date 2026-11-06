#pragma once

#include "kernel/PacketSink.h"

namespace debugshape_export {

inline std::atomic<std::uint64_t>& sculkSendFailureCount() {
    return wire::counters().droppedInvalid;
}

template <typename PacketT>
void sendSculkPacketToPlayer(::Player& player, PacketT const& packet) {
    (void)wire::send(player, packet);
}

} // namespace debugshape_export
