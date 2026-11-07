#pragma once

#include "core/DiagFlags.h"
#include "core/PacketCodec.h"
#include "kernel/PacketSink.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <cstdint>
#include <string>

namespace debugshape_export::container_util {

template <typename PacketT>
bool sendPacket(::Player& player, PacketT const& packet) {
    if (hlibDomainDisabled("HLIB_NO_CONTAINER"))
        return false;

    auto bytes = wire::encode(packet, /*validate=*/false);
    if (bytes.empty())
        return false;
    return wire::sendRaw(
        player,
        bytes,
        wire::Reliability::Reliable,
        wire::detail::packetName(packet),
        wire::detail::packetId(packet));
}

inline ::Player* findPlayerByName(std::string const& name) {
    auto level = ::ll::service::getLevel();
    if (!level)
        return nullptr;
    ::Player* found = nullptr;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (player.getRealName() == name) {
            found = &player;
            return false;
        }
        return true;
    });
    return found;
}

inline std::uint64_t nowTick() {
    auto level = ::ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : std::uint64_t{0};
}

} // namespace debugshape_export::container_util
