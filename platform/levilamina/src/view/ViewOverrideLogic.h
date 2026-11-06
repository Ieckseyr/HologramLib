#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace debugshape_export::view::logic {

inline constexpr int kAddPlayer = 12;
inline constexpr int kAddActor  = 13;

inline constexpr int kAddItemActor      = 15;
inline constexpr int kRemoveActor       = 14;
inline constexpr int kUpdateBlock       = 21;
inline constexpr int kSetActorData      = 39;
inline constexpr int kMovePlayer        = 19;
inline constexpr int kMoveAbsoluteActor = 18;
inline constexpr int kMoveDeltaActor    = 111;
inline constexpr int kSetActorMotion    = 40;
inline constexpr int kLevelChunk        = 58;
inline constexpr int kUpdateBlockSynced = 110;

enum class ItemSpawnPlan { Pass,
                           Drop,
                           Rewrite };

inline constexpr ItemSpawnPlan planItemSpawn(bool hidden, bool hasIdentifier) noexcept {
    if (hidden)
        return ItemSpawnPlan::Drop;
    if (hasIdentifier)
        return ItemSpawnPlan::Rewrite;
    return ItemSpawnPlan::Pass;
}

inline constexpr std::int64_t packPos(int x, int y, int z) {
    return (static_cast<std::int64_t>(x & 0x3FFFFFF) << 38) | (static_cast<std::int64_t>(z & 0x3FFFFFF) << 12) | static_cast<std::int64_t>(y & 0xFFF);
}

inline constexpr int signExtend(std::int64_t value, int bits) {
    auto const half = static_cast<std::int64_t>(1) << (bits - 1);
    auto const mask = (static_cast<std::int64_t>(1) << bits) - 1;
    auto const v    = value & mask;
    return static_cast<int>(v >= half ? v - (mask + 1) : v);
}

inline constexpr int unpackX(std::int64_t key) {
    return signExtend(key >> 38, 26);
}
inline constexpr int unpackZ(std::int64_t key) {
    return signExtend((key >> 12) & 0x3FFFFFF, 26);
}
inline constexpr int unpackY(std::int64_t key) {
    return signExtend(key & 0xFFF, 12);
}

inline constexpr int chunkOf(int coord) {
    return coord >> 4;
}

inline constexpr bool inChunk(std::int64_t key, int chunkX, int chunkZ) {
    return chunkOf(unpackX(key)) == chunkX && chunkOf(unpackZ(key)) == chunkZ;
}

inline constexpr bool needsRewrite(std::uint32_t current, std::uint32_t override_) {
    return current != override_;
}
inline constexpr bool shouldRecordReal(std::uint32_t current, std::uint32_t override_) {
    return current != override_;
}

struct MetaPlan {
    bool        setName{false};
    std::string name;
    bool        setAlwaysShow{false};
    bool        alwaysShow{false};
    bool        setPose{false};
    int         pose{0};
    bool        setScale{false};
    float       scale{1.0f};
    bool        setVariant{false};
    int         variant{0};
    bool        setMarkVariant{false};
    int         markVariant{0};

    bool setColorIndex{false};
    int  colorIndex{0};
    bool setColor2{false};
    int  color2{0};

    [[nodiscard]] bool empty() const {
        return !setName && !setAlwaysShow && !setPose && !setScale && !setVariant && !setMarkVariant && !setColorIndex && !setColor2;
    }
};

inline MetaPlan metaPlanOf(bool hasNametag, std::string_view nametag, bool nametagAlwaysShow) {
    MetaPlan plan;
    if (!hasNametag)
        return plan;
    plan.setName       = true;
    plan.name          = std::string{nametag};
    plan.setAlwaysShow = true;
    plan.alwaysShow    = nametagAlwaysShow;
    return plan;
}

inline MetaPlan metaPlanOf(
    bool             hasNametag,
    std::string_view nametag,
    bool             nametagAlwaysShow,
    bool             hasPose,
    int              pose,
    bool             hasScale,
    float            scale,
    bool             hasVariant,
    int              variant,
    bool             hasMarkVariant,
    int              markVariant) {
    auto plan           = metaPlanOf(hasNametag, nametag, nametagAlwaysShow);
    plan.setPose        = hasPose;
    plan.pose           = pose;
    plan.setScale       = hasScale;
    plan.scale          = scale;
    plan.setVariant     = hasVariant;
    plan.variant        = variant;
    plan.setMarkVariant = hasMarkVariant;
    plan.markVariant    = markVariant;
    return plan;
}

inline constexpr int kColorMin = 0;
inline constexpr int kColorMax = 15;

inline constexpr bool colorAcceptable(int color) noexcept {
    return color >= kColorMin && color <= kColorMax;
}

inline MetaPlan withColors(MetaPlan plan, bool hasColorIndex, int colorIndex, bool hasColor2, int color2) {
    plan.setColorIndex = hasColorIndex;
    plan.colorIndex    = colorIndex;
    plan.setColor2     = hasColor2;
    plan.color2        = color2;
    return plan;
}

inline constexpr int kPoseMin = 0;
inline constexpr int kPoseMax = 13;

inline constexpr int clampPose(int pose) noexcept {
    return pose < kPoseMin ? kPoseMin : (pose > kPoseMax ? kPoseMax : pose);
}

inline constexpr float kScaleMin = 0.0625f;
inline constexpr float kScaleMax = 10.0f;

inline constexpr bool scaleAcceptable(float scale) noexcept {
    return scale >= kScaleMin && scale <= kScaleMax;
}

inline constexpr float kBaseHitboxWidth  = 0.6f;
inline constexpr float kBaseHitboxHeight = 1.8f;

inline constexpr float hitboxWidthOf(float scale) noexcept {
    return kBaseHitboxWidth * scale;
}
inline constexpr float hitboxHeightOf(float scale) noexcept {
    return kBaseHitboxHeight * scale;
}

inline constexpr bool viewHasEffect(
    bool hasIdentifier,
    bool hasAsPlayer,
    bool hasSkin,
    bool hasHidden,
    bool hasNametag,
    bool hasPose        = false,
    bool hasScale       = false,
    bool hasVariant     = false,
    bool hasMarkVariant = false,
    bool hasColor       = false,
    bool hasColor2      = false) {
    return hasIdentifier || hasAsPlayer || hasSkin || hasHidden || hasNametag || hasPose || hasScale || hasVariant || hasMarkVariant || hasColor || hasColor2;
}

inline std::string trimText(std::string_view raw) {

    auto const isSpace = [](char c) {
        return c == ' ' || c == '\x09' || c == '\x0D' || c == '\x0A';
    };
    std::size_t begin = 0;
    std::size_t end   = raw.size();
    while (begin < end && isSpace(raw[begin]))
        ++begin;
    while (end > begin && isSpace(raw[end - 1]))
        --end;
    return std::string{raw.substr(begin, end - begin)};
}

inline std::string normaliseType(std::string_view raw) {
    std::size_t begin   = 0;
    std::size_t end     = raw.size();
    auto const  isSpace = [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };
    while (begin < end && isSpace(raw[begin]))
        ++begin;
    while (end > begin && isSpace(raw[end - 1]))
        --end;
    if (begin == end)
        return {};
    std::string_view trimmed = raw.substr(begin, end - begin);
    if (trimmed.find(':') != std::string_view::npos)
        return std::string{trimmed};
    return "minecraft:" + std::string{trimmed};
}

inline constexpr float kPlayerEyeHeight = 1.62f;

inline constexpr int kAnyDimension = -1;

inline constexpr bool dimensionMatches(int recordDim, int packetDim) {
    return recordDim == kAnyDimension || recordDim == packetDim;
}

} // namespace debugshape_export::view::logic
