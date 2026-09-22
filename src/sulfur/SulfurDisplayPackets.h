// SulfurDisplayPackets.h - 硫磺立方体展示域的协议层包体（手写）
//
// ChangeMobPropertyPacket(182): 按**属性名**同步一个实体属性（布尔/字符串/整数/浮点四个槽位）。
// 26.40 线格式（取自 .cache/schema-1.26.40/ChangeMobPropertyPacketPayload.json 的 x-ordinal-index）:
//   Actor Id(ActorUniqueID: int64 压缩) → Property Name(string) → Bool → String → Int32(压缩) → Float
//
// 用途: minecraft:sulfur_cube 的外观档位 `minecraft:sulfur_cube_archetype` —— 它是行为包里
// `client_sync: true` 的 enum 属性（定义见 vanilla 行为包 entities/sulfur_cube.json 的 properties,
// 取值为 none/regular/bouncy/slow_bouncy/slow_flat/fast_flat/light/fast_sliding/slow_sliding/
// sticky/high_resistance/explosive/hot）。客户端靠它决定立方体的外观与材质，所以按字符串下发。
//
// 注意: AddActor 只带 PropertySyncData（按索引的 int/float），装不下 enum 属性; 所以属性必须在
// 实体已经在客户端存在之后再发 —— 与"装备"同一时机（spawn 之后）。
#pragma once

#include <cstdint>
#include <string>

#include <sculk/protocol/codec/packet/ChangeMobPropertyPacket.hpp>

namespace debugshape_export::sulfur {

// 按属性名同步一个字符串属性（enum 属性走这一支）
inline sculk::protocol::ChangeMobPropertyPacket makeMobProperty(
    std::int64_t       actorUniqueId,
    std::string        name,
    std::string        value,
    bool               boolValue  = true,
    std::int32_t       intValue   = 0,
    float              floatValue = 0.0f
) {
    sculk::protocol::ChangeMobPropertyPacket packet;
    packet.mActorUniqueId = actorUniqueId;
    packet.mPropertyName  = std::move(name);
    packet.mBoolVaue      = boolValue; // sculk 里字段名就拼作 mBoolVaue
    packet.mStringValue   = std::move(value);
    packet.mIntValue      = intValue;
    packet.mFloatValue    = floatValue;
    return packet;
}

// 硫磺立方体的外观档位属性名
inline constexpr char const* kArchetypeProperty = "minecraft:sulfur_cube_archetype";

} // namespace debugshape_export::sulfur
