#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>

#include "core/proto/Version.h"

namespace debugshape_export::npc_protocol {

namespace proto = ::debugshape_export::core::proto;

inline constexpr std::uint8_t kSkinTrustedUnset = 0;
inline constexpr std::uint8_t kSkinTrustedFalse = 1;
inline constexpr std::uint8_t kSkinTrustedTrue  = 2;

inline void finalizeSkinIds(proto::SerializedSkinRef& skin, std::string_view registryId) {
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : registryId) {
        h ^= static_cast<std::uint64_t>(c);
        h *= 1099511628211ULL;
    }
    auto const a = static_cast<std::uint32_t>(h >> 32);
    auto const b = static_cast<std::uint32_t>(h);
    char       uuid[80]{};
    std::snprintf(
        uuid,
        sizeof(uuid),
        "%08x-%04x-4%03x-%04x-%08x%04x",
        a,
        (b >> 16) & 0xFFFF,
        b & 0x0FFF,
        static_cast<unsigned>((b >> 28) & 0x3) | 0x8000,
        a,
        b & 0xFFFF);
    skin.id      = std::string(uuid) + "." + std::string(registryId);
    skin.full_id = skin.id;
}

inline proto::UUID npcUuid(std::int64_t id) {
    return {0xF0B3'4E50'4300'0000ULL, static_cast<std::uint64_t>(id)};
}

// PlayerList 的 AddEntry。skin 的 trusted_skin_flag 就是线上信任三态，调用方传入的值会被覆盖成 True。
inline proto::PlayerListAddEntry playerListEntry(
    proto::UUID const&              uuid,
    std::int64_t                    uniqueId,
    std::string const&              name,
    proto::SerializedSkinRef const& skin) {
    proto::PlayerListAddEntry entry;
    entry.action             = proto::PlayerListPacketType::Add;
    entry.uuid               = uuid;
    entry.id                 = proto::ActorUniqueID{uniqueId};
    entry.name               = name;
    entry.xuid               = "0";
    entry.play_fab_id        = {};
    entry.platform_online_id = {};
    entry.build_platform     = static_cast<decltype(entry.build_platform)>(1);
    entry.skin               = skin;
    entry.skin.trusted_skin_flag =
        static_cast<decltype(entry.skin.trusted_skin_flag)>(kSkinTrustedTrue);
    entry.is_teacher     = false;
    entry.is_host        = false;
    entry.is_sub_client  = false;
    entry.color          = proto::Color{0};
    return entry;
}

inline proto::PlayerListRemoveEntry playerListRemoveEntry(proto::UUID const& uuid) {
    proto::PlayerListRemoveEntry entry;
    entry.action = proto::PlayerListPacketType::Remove;
    entry.uuid   = uuid;
    return entry;
}

inline std::string bodyPrefixHex(std::span<std::byte const> body) {
    constexpr char digits[] = "0123456789abcdef";
    auto const     count    = std::min<std::size_t>(body.size(), 24);
    std::string    result;
    result.reserve(count * 3);
    for (std::size_t index = 0; index < count; ++index) {
        if (index != 0)
            result.push_back(' ');
        auto const value = std::to_integer<unsigned>(body[index]);
        result.push_back(digits[value >> 4]);
        result.push_back(digits[value & 0x0f]);
    }
    return result;
}

} // namespace debugshape_export::npc_protocol
