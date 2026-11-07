#pragma once

#include <cstdint>
#include <string>

namespace debugshape_export {

inline constexpr char const* kStopAnimation = "animation.player.attack.positions";

inline std::string npcAnimControllerName(int64_t id) {
    return "controller.animation.holo_npc_" + std::to_string(id);
}

} // namespace debugshape_export
