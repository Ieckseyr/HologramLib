#pragma once

#include "SculkPacketSend.h"

#include <cstdint>
#include <vector>

#include <mc/world/actor/player/Player.h>

#include <sculk/protocol/codec/inventory/item/ItemStackResponse.hpp>
#include <sculk/protocol/codec/packet/ItemStackResponsePacket.hpp>

namespace debugshape_export::container {

inline void sendRequestSuccess(::Player& player, std::vector<std::int32_t> const& requestIds) {
    if (requestIds.empty())
        return;
    sculk::protocol::ItemStackResponsePacket packet;
    for (auto const id : requestIds) {
        sculk::protocol::ItemStackResponseInfo info;
        info.mResult    = sculk::protocol::ItemStackNetResult::Success;
        info.mRequestId = id;
        packet.mResponse.mResponses.push_back(std::move(info));
    }
    sendSculkPacketToPlayer(player, packet);
}

} // namespace debugshape_export::container
