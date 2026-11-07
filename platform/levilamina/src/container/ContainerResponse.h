#pragma once

#include "ProtoPacketSend.h"

#include <cstdint>
#include <vector>

#include <mc/world/actor/player/Player.h>

#include "core/proto/Version.h"

namespace debugshape_export::container {

namespace proto = ::debugshape_export::core::proto;

inline void sendRequestSuccess(::Player& player, std::vector<std::int32_t> const& requestIds) {
    if (requestIds.empty())
        return;
    proto::ItemStackResponsePacket packet;
    for (auto const id : requestIds) {
        proto::ItemStackResponseInfo info;
        info.result            = proto::ItemStackNetResult::Success;
        info.client_request_id = proto::bp::ItemStackRequestId{id};
        packet.responses.push_back(std::move(info));
    }
    sendProtoPacketToPlayer(player, packet);
}

} // namespace debugshape_export::container
