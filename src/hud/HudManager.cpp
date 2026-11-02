// HudManager.cpp - HUD 元素显隐
#include "HudManager.h"

#include "core/PacketSink.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/packet/SetHudPacket.hpp>

namespace debugshape_export {

namespace {

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

// 协议: 0 = Hide, 1 = Reset（Reset 即"恢复默认显示"）
using ProtoElement    = sculk::protocol::SetHudPacket::HudElement;
using ProtoVisibility = sculk::protocol::SetHudPacket::HudVisibility;

bool send(::Player& player, std::vector<ProtoElement> const& elements, ProtoVisibility visibility) {
    if (elements.empty())
        return true;

    sculk::protocol::SetHudPacket packet;
    packet.mHudElements   = elements;
    packet.mHudVisibility = visibility;

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return false;

    wire::sendRaw(player, bytes);
    return true;
}

ProtoVisibility visibilityOf(bool visible) {
    return visible ? ProtoVisibility::Reset : ProtoVisibility::Hide;
}

} // namespace

HudManager& HudManager::getInstance() {
    static HudManager instance;
    return instance;
}

bool HudManager::setElement(std::string const& playerName, hologramlib::HudElement element, bool visible) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    auto const value = static_cast<int>(element);
    if (value < 0 || value > 12)
        return false;

    return send(*player, {static_cast<ProtoElement>(value)}, visibilityOf(visible));
}

int HudManager::setElements(std::string const& playerName, int mask, bool visible) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return 0;

    std::vector<ProtoElement> elements;
    for (int bit = 0; bit <= 12; ++bit) {
        if ((mask & (1 << bit)) != 0)
            elements.push_back(static_cast<ProtoElement>(bit));
    }
    if (elements.empty())
        return 0;

    if (!send(*player, elements, visibilityOf(visible)))
        return 0;
    return static_cast<int>(elements.size());
}

bool HudManager::resetAll(std::string const& playerName) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    std::vector<ProtoElement> all;
    for (int bit = 0; bit <= 12; ++bit)
        all.push_back(static_cast<ProtoElement>(bit));
    return send(*player, all, ProtoVisibility::Reset);
}

} // namespace debugshape_export
