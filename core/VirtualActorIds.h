#pragma once

#include <cstdint>

namespace debugshape_export {
namespace core {
namespace actorIds {

struct Range {
    std::uint64_t first;
    std::uint64_t last;

    constexpr bool          contains(std::uint64_t value) const noexcept { return value >= first && value < last; }
    constexpr std::uint64_t size() const noexcept { return last - first; }
};

inline constexpr Range itemDisplay{0x6D000000ULL, 0x6E000000ULL};
inline constexpr Range customEntity{0x6E000000ULL, 0x6F000000ULL};
inline constexpr Range playerNpc{0x6F000000ULL, 0x70000000ULL};

inline constexpr Range itemActor{0x70000000ULL, 0x71000000ULL};

inline constexpr Range bossBar{0x71000000ULL, 0x72000000ULL};

inline constexpr std::int64_t randomIdBase = 0x10000000;
inline constexpr std::int64_t randomIdSpan = 0x70000000;

static_assert(itemDisplay.last == customEntity.first, "itemDisplay 与 customEntity 段必须首尾相接");
static_assert(customEntity.last == playerNpc.first, "customEntity 与 playerNpc 段必须首尾相接");
static_assert(playerNpc.last == itemActor.first, "playerNpc 与 itemActor 段必须首尾相接");
static_assert(itemActor.last == bossBar.first, "itemActor 与 bossBar 段必须首尾相接");
static_assert(bossBar.last == 0x72000000ULL, "bossBar 段的终点决定整块号段的上界");

static_assert(itemDisplay.size() == customEntity.size(), "各段大小必须一致，否则 0x6D/0x6E/0x6F 前缀不再对应");
static_assert(customEntity.size() == playerNpc.size(), "各段大小必须一致");
static_assert(playerNpc.size() == itemActor.size(), "各段大小必须一致");
static_assert(itemActor.size() == bossBar.size(), "各段大小必须一致");

static_assert(itemDisplay.first >= 0x60000000ULL, "虚拟号段必须避开真实实体的 runtimeId 区");
static_assert(bossBar.last <= 0x72000000ULL, "虚拟号段上界不得越过 0x72000000");

constexpr bool anyContains(std::uint64_t value) noexcept {
    return itemDisplay.contains(value) || customEntity.contains(value) || playerNpc.contains(value);
}

constexpr char const* domainOf(std::uint64_t value) noexcept {
    if (itemDisplay.contains(value))
        return "itemDisplay";
    if (customEntity.contains(value))
        return "customEntity";
    if (playerNpc.contains(value))
        return "playerNpc";
    return nullptr;
}

} // namespace actorIds
} // namespace core
} // namespace debugshape_export
