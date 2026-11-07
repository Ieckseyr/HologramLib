#pragma once

#include <cstdint>
#include <string>

#include "core/proto/Version.h"

namespace debugshape_export::sulfur {

namespace proto = ::debugshape_export::core::proto;

inline proto::ChangeMobPropertyPacket makeMobProperty(
    std::int64_t actorUniqueId,
    std::string  name,
    std::string  value,
    bool         boolValue  = true,
    std::int32_t intValue   = 0,
    float        floatValue = 0.0f) {
    proto::ChangeMobPropertyPacket packet;
    packet.actor_id             = proto::ActorUniqueID{actorUniqueId};
    packet.prop_name            = std::move(name);
    packet.bool_component_val   = boolValue;
    packet.string_component_val = std::move(value);
    packet.int_component_val    = intValue;
    packet.float_component_val  = floatValue;
    return packet;
}

inline constexpr char const* kArchetypeProperty = "minecraft:sulfur_cube_archetype";

} // namespace debugshape_export::sulfur
