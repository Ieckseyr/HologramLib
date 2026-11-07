#include "HudManager.h"

#include "kernel/PacketSink.h"

#include <format>

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include "core/proto/Version.h"

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

namespace {

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

using ProtoElement    = proto::HudElement;
using ProtoVisibility = proto::HudVisibility;

bool send(::Player& player, std::vector<ProtoElement> const& elements, ProtoVisibility visibility) {
    proto::SetHudPacket packet;
    packet.hud_element  = elements;
    packet.hud_visible  = visibility;

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    wire::sendRaw(player, bytes);
    return true;
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

    if (visible) {
        std::vector<ProtoElement> all;
        for (int bit = 0; bit <= 12; ++bit)
            all.push_back(static_cast<ProtoElement>(bit));
        return send(*player, all, ProtoVisibility::Reset);
    }

    std::vector<ProtoElement> keep;
    for (int bit = 0; bit <= 12; ++bit) {
        if (bit != value)
            keep.push_back(static_cast<ProtoElement>(bit));
    }
    return send(*player, keep, ProtoVisibility::Hide);
}

int HudManager::setElements(std::string const& playerName, int mask, bool visible) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return 0;

    std::vector<ProtoElement> all;
    for (int bit = 0; bit <= 12; ++bit)
        all.push_back(static_cast<ProtoElement>(bit));

    if (visible) {
        if (!send(*player, all, ProtoVisibility::Reset))
            return 0;
        return static_cast<int>(all.size());
    }

    std::vector<ProtoElement> keep;
    for (int bit = 0; bit <= 12; ++bit) {
        if ((mask & (1 << bit)) != 0)
            keep.push_back(static_cast<ProtoElement>(bit));
    }
    if (!send(*player, keep, ProtoVisibility::Hide))
        return 0;
    return 13 - static_cast<int>(keep.size());
}

bool HudManager::setRaw(std::string const& playerName, int mask, int visibility) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    std::vector<ProtoElement> elements;
    for (int bit = 0; bit <= 12; ++bit) {
        if ((mask & (1 << bit)) != 0)
            elements.push_back(static_cast<ProtoElement>(bit));
    }
    return send(*player, elements, static_cast<ProtoVisibility>(visibility));
}

std::string HudManager::probeWire() const {

    ::BinaryStream   body;
    std::vector<int> keep;
    for (int bit = 1; bit <= 12; ++bit)
        keep.push_back(bit);
    body.writeUnsignedVarInt(static_cast<uint>(keep.size()), nullptr, nullptr);
    for (int value : keep)
        body.writeUnsignedVarInt(static_cast<uint>(value), nullptr, nullptr);
    body.writeUnsignedVarInt(0, nullptr, nullptr);

    std::string const bytes = body.mBuffer;
    std::string       hex;
    hex.reserve(bytes.size() * 2);
    for (unsigned char c : bytes)
        hex += std::format("{:02x}", c);

    return std::format(
        "valid={} bytes={} body_hex={} (Hide + 保留 12 项，普通 varint)",
        wire::validateBody(308, bytes) ? 1 : 0,
        bytes.size(),
        hex);
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
