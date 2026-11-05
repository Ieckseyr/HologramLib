#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

namespace debugshape_export {
namespace core {

using PlayerNameSet = std::unordered_set<std::string>;

struct SpawnBudget {
    int remaining;

    explicit SpawnBudget(int limit)
        : remaining(limit) {}

    [[nodiscard]] bool available() const { return remaining > 0; }

    void consume() {
        if (remaining > 0)
            --remaining;
    }
};

struct VisibilityRule {
    bool                 enabled{false};
    int                  dimension{0};
    float                x{0}, y{0}, z{0};
    double               viewDistance{0};
    double               hysteresis{0};
    PlayerNameSet const* allowList{nullptr};
};

enum class VisibilityDecision { None,
                                Spawn,
                                Remove };

[[nodiscard]] inline VisibilityDecision decideVisibility(
    VisibilityRule const& rule,
    int                   playerDim,
    float                 px,
    float                 py,
    float                 pz,
    std::string const&    playerName,
    bool                  shown) {
    if (!rule.enabled || playerDim != rule.dimension)
        return shown ? VisibilityDecision::Remove : VisibilityDecision::None;
    if (rule.allowList != nullptr && !rule.allowList->contains(playerName))
        return shown ? VisibilityDecision::Remove : VisibilityDecision::None;
    if (rule.viewDistance <= 0)
        return shown ? VisibilityDecision::None : VisibilityDecision::Spawn;

    double const dx     = static_cast<double>(px) - rule.x;
    double const dy     = static_cast<double>(py) - rule.y;
    double const dz     = static_cast<double>(pz) - rule.z;
    double const distSq = dx * dx + dy * dy + dz * dz;
    double const keep   = rule.viewDistance + rule.hysteresis;

    if (distSq <= rule.viewDistance * rule.viewDistance)
        return shown ? VisibilityDecision::None : VisibilityDecision::Spawn;
    if (distSq > keep * keep)
        return shown ? VisibilityDecision::Remove : VisibilityDecision::None;
    return VisibilityDecision::None;
}

} // namespace core
} // namespace debugshape_export
