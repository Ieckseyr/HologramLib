#include "actor/PlayerRespawnManager.h"

#include "core/Wire26.h"
#include "kernel/PacketSink.h"

#include <ll/api/service/Bedrock.h>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/packet/RespawnPacket.hpp>

#include <algorithm>

namespace debugshape_export {

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

    sculk::protocol::RespawnPacket packet;
    packet.mPosition = sculk::protocol::Vec3{x, y, z};
    packet.mState    = static_cast<sculk::protocol::RespawnPacket::PlayerRespawnState>(
        std::clamp(state, 0, 2));
    packet.mActorRuntimeId = player->getRuntimeID().rawID;

    return wire::sendBody(
        *player,
        static_cast<std::uint32_t>(packet.getId()),
        wire26::body26(packet),
        wire::Reliability::Reliable,
        "RespawnPacket");
}

} // namespace debugshape_export
