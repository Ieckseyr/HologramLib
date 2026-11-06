#include "FormControlManager.h"

#include <ll/api/service/Bedrock.h>

#include <mc/network/packet/ClientboundCloseFormPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include "kernel/PacketSink.h"

namespace debugshape_export {

FormControlManager& FormControlManager::getInstance() {
    static FormControlManager instance;
    return instance;
}

bool FormControlManager::closeForm(std::string const& playerName) {
    auto level = ll::service::getLevel();
    if (!level)
        return false;
    ::Player* player = level->getPlayer(playerName);
    if (player == nullptr)
        return false;

    // 走库统一发包口：该包无负载，只需包头
    auto const id = static_cast<std::uint32_t>(::MinecraftPacketIds::ClientboundCloseScreen);
    return wire::sendRaw(*player, wire::withHeader(id, std::string{}), wire::Reliability::Reliable,
                         "ClientboundCloseScreen", id);
}

int FormControlManager::closeAllForms() {
    auto level = ll::service::getLevel();
    if (!level)
        return 0;

    int count = 0;
    level->forEachPlayer([&](::Player& player) -> bool {
        auto const id = static_cast<std::uint32_t>(::MinecraftPacketIds::ClientboundCloseScreen);
        if (wire::sendRaw(player, wire::withHeader(id, std::string{}), wire::Reliability::Reliable,
                          "ClientboundCloseScreen", id))
            ++count;
        return true;
    });
    return count;
}

} // namespace debugshape_export
