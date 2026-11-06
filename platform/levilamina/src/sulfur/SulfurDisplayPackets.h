#pragma once

#include <cstdint>
#include <string>

#include <sculk/protocol/codec/packet/ChangeMobPropertyPacket.hpp>

namespace debugshape_export::sulfur {

inline sculk::protocol::ChangeMobPropertyPacket makeMobProperty(
    std::int64_t actorUniqueId,
    std::string  name,
    std::string  value,
    bool         boolValue  = true,
    std::int32_t intValue   = 0,
    float        floatValue = 0.0f) {
    sculk::protocol::ChangeMobPropertyPacket packet;
    packet.mActorUniqueId = actorUniqueId;
    packet.mPropertyName  = std::move(name);
    packet.mBoolVaue      = boolValue;
    packet.mStringValue   = std::move(value);
    packet.mIntValue      = intValue;
    packet.mFloatValue    = floatValue;
    return packet;
}

inline constexpr char const* kArchetypeProperty = "minecraft:sulfur_cube_archetype";

} // namespace debugshape_export::sulfur
