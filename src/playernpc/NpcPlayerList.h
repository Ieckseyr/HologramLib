// NPC PlayerList data helpers; no BDS runtime is needed to verify their wire bytes.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include <sculk/protocol/Version.hpp>
#include <sculk/protocol/codec/packet/PlayerListPacket.hpp>

namespace debugshape_export::npc_protocol {

static_assert(SCULK_NETWORK_PROTOCOL_VERSION == 2168, "HologramLib NPCs require Protocol 2168 headers and library");

// 26.40 carries skin trust inside SerializedSkin as a tri-state string on the wire
// (schema TrustedSkinFlag: Unset/False/True), not as a trailing player-list boolean.
// SerializedSkin encodes it as uint8_t: 0 = "unset", 1 = "false", 2 = "true".
inline constexpr std::uint8_t kSkinTrustedUnset = 0;
inline constexpr std::uint8_t kSkinTrustedFalse = 1;
inline constexpr std::uint8_t kSkinTrustedTrue  = 2;

// NPCs use one identity for the skin and its complete cached appearance, as Geyser does.
// Rebuild FullId after capture/Persona rewriting and when restoring older blobs.
inline void finalizeSkinIds(sculk::protocol::SerializedSkin& skin, std::string_view registryId) {
    if (skin.mId.empty()) skin.mId = "HoloLibNpcSkin_" + std::string(registryId);
    skin.mFullId = skin.mId;
}

inline sculk::protocol::UUID npcUuid(std::int64_t id) {
    return {0xF0B3'4E50'4300'0000ULL, static_cast<std::uint64_t>(id)};
}

inline sculk::protocol::PlayerListEntry playerListEntry(
    sculk::protocol::UUID const&           uuid,
    std::int64_t                          uniqueId,
    std::string const&                    name,
    sculk::protocol::SerializedSkin const& skin
) {
    sculk::protocol::PlayerListEntry entry;
    entry.mUUID           = uuid;
    entry.mActorUniqueId  = uniqueId;
    entry.mPlayerName     = name;
    entry.mXuid           = "0";
    entry.mPlatformChatId = "";
    entry.mSerializedSkin = skin;
    // PlayerListEntry::write() copies mSkinTrusted into the skin tri-state, so a stale
    // value carried on the caller's skin must not decide trust here. Set both explicitly.
    entry.mSkinTrusted                     = true;
    entry.mSerializedSkin.mTrustedSkinFlag = kSkinTrustedTrue;
    entry.mBuildPlatform  = 1;
    entry.mColor          = 0;
    return entry;
}

// All NPC call sites send one entry. Check bytes produced by the linked library,
// independently of its version macro: count, variant index, then action.
// This checks framing only; the independent offline test checks the skin payload.
inline bool hasValidPlayerListPrefix(
    sculk::protocol::PlayerListPacket const& packet,
    std::span<std::byte const>              body
) {
    using Action = sculk::protocol::PlayerListPacket::ActionType;
    if (packet.mPlayerEntryList.size() != 1 || body.size() < 3) return false;
    if (packet.mAction != Action::Add && packet.mAction != Action::Remove) return false;
    return body[0] == std::byte{1}
        && body[1] == (packet.mAction == Action::Add ? std::byte{1} : std::byte{0})
        && body[2] == static_cast<std::byte>(packet.mAction);
}

inline std::string bodyPrefixHex(std::span<std::byte const> body) {
    constexpr char digits[] = "0123456789abcdef";
    auto const count = std::min<std::size_t>(body.size(), 24);
    std::string result;
    result.reserve(count * 3);
    for (std::size_t index = 0; index < count; ++index) {
        if (index != 0) result.push_back(' ');
        auto const value = std::to_integer<unsigned>(body[index]);
        result.push_back(digits[value >> 4]);
        result.push_back(digits[value & 0x0f]);
    }
    return result;
}

} // namespace debugshape_export::npc_protocol
