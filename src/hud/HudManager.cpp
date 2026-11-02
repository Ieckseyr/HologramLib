// HudManager.cpp - HUD 元素显隐
#include "HudManager.h"

#include "core/PacketSink.h"

#include <format>

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

    // 语义（与 BDS 的 ServerPlayer::hideAllExcept 一致）:
    //   Hide 时，包里的元素列表是"**要保留**"的那些，其余全隐藏；
    //   Reset 时列表被忽略（一律恢复默认）。
    // 所以"只藏纸娃娃"= Hide + [除纸娃娃外的 12 项]，而不是 Hide + [纸娃娃]。
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

    // mask 里的元素**要保留**，其余隐藏（同 setElement 的语义）
    std::vector<ProtoElement> keep;
    for (int bit = 0; bit <= 12; ++bit) {
        if ((mask & (1 << bit)) != 0)
            keep.push_back(static_cast<ProtoElement>(bit));
    }
    if (!send(*player, keep, ProtoVisibility::Hide))
        return 0;
    return 13 - static_cast<int>(keep.size()); // 返回被隐藏的个数
}

std::string HudManager::probeWire() const {
    // 与 setElement(隐藏) 同一套语义: Hide + "除目标外全部保留"
    std::vector<ProtoElement> keep;
    for (int bit = 0; bit <= 12; ++bit) {
        if (bit != 0)
            keep.push_back(static_cast<ProtoElement>(bit));
    }

    sculk::protocol::SetHudPacket packet;
    packet.mHudElements   = keep;
    packet.mHudVisibility = ProtoVisibility::Hide;

    std::vector<std::byte>        buffer;
    sculk::protocol::BinaryStream stream(buffer);
    packet.write(stream);
    std::string const body(reinterpret_cast<char const*>(buffer.data()), buffer.size());

    std::string head;
    head.reserve(6 * 2);
    for (std::size_t i = 0; i < body.size() && i < 6; ++i)
        head += std::format("{:02x}", static_cast<unsigned char>(body[i]));

    return std::format(
        "valid={} bytes={} head_hex={} (Hide + 保留 12 项)",
        wire::validateBody(308, body) ? 1 : 0,
        body.size(),
        head);
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
