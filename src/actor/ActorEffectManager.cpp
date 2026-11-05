#include "ActorEffectManager.h"

#include "core/PacketSink.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/actor/ActorEvent.hpp>
#include <sculk/protocol/codec/packet/ActorEventPacket.hpp>

namespace debugshape_export {

namespace {

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

bool sendTo(::Player& player, int64_t runtimeId, int eventId, int data) {
    sculk::protocol::ActorEventPacket packet;
    packet.mRuntimeId = static_cast<std::uint64_t>(runtimeId);
    packet.mEventId   = static_cast<sculk::protocol::ActorEvent>(eventId);
    packet.mData      = data;

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    wire::sendRaw(player, bytes);
    return true;
}

} // namespace

ActorEffectManager& ActorEffectManager::getInstance() {
    static ActorEffectManager instance;
    return instance;
}

bool ActorEffectManager::dispatch(std::string const& playerName, int64_t runtimeId, int eventId, int data) {
    if (runtimeId <= 0)
        return false;

    if (!playerName.empty()) {
        ::Player* player = findPlayer(playerName);
        if (player == nullptr)
            return false;
        return sendTo(*player, runtimeId, eventId, data);
    }

    auto level = ::ll::service::getLevel();
    if (!level)
        return false;
    bool sent = false;
    level->forEachPlayer([&](::Player& player) -> bool {
        sent = sendTo(player, runtimeId, eventId, data) || sent;
        return true;
    });
    return sent;
}

bool ActorEffectManager::playHurt(int64_t entityRuntimeId) {
    return dispatch("", entityRuntimeId, static_cast<int>(sculk::protocol::ActorEvent::Hurt), 0);
}

bool ActorEffectManager::playDeath(int64_t entityRuntimeId) {
    return dispatch("", entityRuntimeId, static_cast<int>(sculk::protocol::ActorEvent::Death), 0);
}

bool ActorEffectManager::playInstantDeath(int64_t entityRuntimeId) {
    return dispatch("", entityRuntimeId, static_cast<int>(sculk::protocol::ActorEvent::InstantDeath), 0);
}

bool ActorEffectManager::play(int64_t entityRuntimeId, int eventId, int data) {
    return dispatch("", entityRuntimeId, eventId, data);
}

bool ActorEffectManager::playFor(
    std::string const& playerName,
    int64_t            entityRuntimeId,
    int                eventId,
    int                data) {
    return dispatch(playerName, entityRuntimeId, eventId, data);
}

} // namespace debugshape_export
