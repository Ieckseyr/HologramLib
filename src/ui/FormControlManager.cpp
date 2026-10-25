// FormControlManager.cpp - 客户端表单控制
#include "FormControlManager.h"

#include <ll/api/service/Bedrock.h>

#include <mc/network/packet/ClientboundCloseFormPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

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

    ::ClientboundCloseFormPacket packet;
    player->sendNetworkPacket(packet);
    return true;
}

int FormControlManager::closeAllForms() {
    auto level = ll::service::getLevel();
    if (!level)
        return 0;

    int count = 0;
    level->forEachPlayer([&](::Player& player) -> bool {
        ::ClientboundCloseFormPacket packet;
        player.sendNetworkPacket(packet);
        ++count;
        return true;
    });
    return count;
}

} // namespace debugshape_export
