#pragma once

#include "core/PlayerGrid.h"
#include "core/VisibilityLogic.h"

#include <mc/platform/UUID.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace debugshape_export {
namespace core {

template <class RuntimeT, class LookupFn, class SpawnFn, class DespawnFn>
void syncEntryVisibility(
    PlayerGrid const&                    grid,
    std::unordered_set<mce::UUID> const& initialized,
    VisibilityRule const&                rule,
    RuntimeT&                            rt,
    std::vector<mce::UUID>&              seen,
    LookupFn&&                           lookup,
    SpawnFn&&                            spawn,
    DespawnFn&&                          despawn) {
    seen.clear();

    auto const consider = [&](::Player& player) {
        auto const uuid = player.getUuid();
        if (!initialized.contains(uuid))
            return;

        auto const& pos   = player.getPosition();
        bool const  shown = rt.shownPlayers.contains(uuid);

        switch (decideVisibility(
            rule,
            static_cast<int>(player.getDimensionId()),
            pos.x,
            pos.y,
            pos.z,
            player.getRealName(),
            shown)) {
        case VisibilityDecision::Spawn:
            if (spawn(player)) {
                rt.shownPlayers.insert(uuid);
                seen.push_back(uuid);
            }
            break;
        case VisibilityDecision::Remove:
            despawn(player);
            rt.shownPlayers.erase(uuid);
            break;
        case VisibilityDecision::None:
            if (shown)
                seen.push_back(uuid);
            break;
        }
    };

    if (rule.viewDistance <= 0) {
        for (auto* player : grid.inDim(rule.dimension)) {
            if (player != nullptr)
                consider(*player);
        }
    } else {
        auto const radius = static_cast<float>(rule.viewDistance + rule.hysteresis);
        grid.forEachNear(rule.dimension, rule.x, rule.z, radius, consider);
    }

    for (auto it = rt.shownPlayers.begin(); it != rt.shownPlayers.end();) {
        if (std::find(seen.begin(), seen.end(), *it) != seen.end()) {
            ++it;
            continue;
        }
        if (auto* player = lookup(*it); player != nullptr)
            despawn(*player);
        it = rt.shownPlayers.erase(it);
    }
}

} // namespace core
} // namespace debugshape_export
