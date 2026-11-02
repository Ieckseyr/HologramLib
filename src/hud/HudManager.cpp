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

// 编码照官方 Primitives 文档：数组长度 uvarint32（普通）、每个元素 varint32（**ZigZag**）、
// 可见性 varint32（ZigZag）。协议库的 writeVarInt 正好是 ZigZag，所以这里直接用协议库编码。
bool send(::Player& player, std::vector<ProtoElement> const& elements, ProtoVisibility visibility) {
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
    // 与 send(隐藏) 同一套：Hide + "除纸娃娃外全部保留"，普通 varint
    ::BinaryStream   body;
    std::vector<int> keep;
    for (int bit = 1; bit <= 12; ++bit)
        keep.push_back(bit);
    body.writeUnsignedVarInt(static_cast<uint>(keep.size()), nullptr, nullptr);
    for (int value : keep)
        body.writeUnsignedVarInt(static_cast<uint>(value), nullptr, nullptr);
    body.writeUnsignedVarInt(0, nullptr, nullptr); // Hide

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
