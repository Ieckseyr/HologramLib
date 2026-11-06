

#pragma once

#include <cstdint>
#include <vector>

#include "core/ProtocolShape.h"

#include "mc/network/Compressibility.h"
#include "mc/network/NetworkPeer.h"
#include "mc/world/actor/player/Player.h"

namespace debugshape_export {

class ProtocolPacketWriter {
public:
    static std::string buildDebugDrawerPacket(std::vector<DebugShape> const& shapes);

    static bool sendToPlayer(::Player& player, std::vector<DebugShape> const& shapes);

    static bool sendToAll(std::vector<DebugShape> const& shapes);

    static bool sendToDimension(int dimId, std::vector<DebugShape> const& shapes);
};

} // namespace debugshape_export
