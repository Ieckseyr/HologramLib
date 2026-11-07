#include "actor/PlayerRespawnManager.h"

#include "core/proto/Version.h"
#include "kernel/PacketSink.h"

#include <ll/api/service/Bedrock.h>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>


#include <algorithm>

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

PlayerRespawnManager& PlayerRespawnManager::getInstance() {
    static PlayerRespawnManager instance;
    return instance;
}

bool PlayerRespawnManager::respawn(std::string const& playerName, int state) {
    auto level = ::ll::service::getLevel();
    if (!level) {
        return false;
    }
    ::Player* player = level->getPlayer(playerName);
    if (player == nullptr) {
        return false;
    }
    auto const pos = player->getPosition();
    return respawnAt(playerName, pos.x, pos.y, pos.z, state);
}

bool PlayerRespawnManager::respawnAt(std::string const& playerName, float x, float y, float z, int state) {
    auto level = ::ll::service::getLevel();
    if (!level) {
        return false;
    }
    ::Player* player = level->getPlayer(playerName);
    if (player == nullptr) {
        return false;
    }

    proto::RespawnPacket packet;
    packet.pos        = proto::Vec3{x, y, z};
    packet.state      = static_cast<decltype(packet.state)>(std::clamp(state, 0, 2));
    packet.runtime_id = proto::bp::ActorRuntimeID{player->getRuntimeID().rawID};

    return wire::sendBody(
        *player,
        static_cast<std::uint32_t>(proto::RespawnPacket::Id),
        codec::body(packet),
        wire::Reliability::Reliable,
        "RespawnPacket");
}

} // namespace debugshape_export
