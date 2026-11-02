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

// 手写 SetHudPacket（308）包体：
//   uvarint 元素个数 | 每个元素 uvarint | uvarint 可见性
// 注意：协议文档写的是 varint32（普通 varint），而协议库的 writeVarInt 是 **zigzag**，
// 非零元素会被读成两倍的值（1 → 2、12 → 24），客户端直接不认这颗包。
bool send(::Player& player, std::vector<ProtoElement> const& elements, ProtoVisibility visibility) {
    ::BinaryStream body;
    body.writeUnsignedVarInt(static_cast<uint>(elements.size()), nullptr, nullptr);
    for (auto element : elements) {
        body.writeUnsignedVarInt(static_cast<uint>(static_cast<int>(element)), nullptr, nullptr);
    }
    body.writeUnsignedVarInt(static_cast<uint>(static_cast<int>(visibility)), nullptr, nullptr);

    ::BinaryStream out;
    out.writeUnsignedVarInt((308 & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12), nullptr, nullptr);
    out.mBuffer.append(body.mBuffer.data(), body.mBuffer.size());

    return wire::sendRaw(player, out.mBuffer);
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
