#pragma once

#include "core/proto/Version.h"
#include "MetaWire.h"

#include <cstdint>
#include <string>

namespace debugshape_export::npcdialog {

namespace proto = ::debugshape_export::core::proto;

inline constexpr float kCarrierY = -66.0f;

inline std::string const& npcDataJson() {
    static std::string const json =
        R"JSON({"picker_offsets":{"scale":[1.70,1.70,1.70],"translate":[0,20,0]},"portrait_offsets":{"scale":[1.750,1.750,1.750],"translate":[-7,50,0]},"skin_list":[{"variant":0},{"variant":1},{"variant":2},{"variant":3},{"variant":4},{"variant":5},{"variant":6},{"variant":7},{"variant":8},{"variant":9},{"variant":10},{"variant":11},{"variant":12},{"variant":13},{"variant":14},{"variant":15},{"variant":16},{"variant":17},{"variant":18},{"variant":19},{"variant":25},{"variant":26},{"variant":27},{"variant":28},{"variant":29},{"variant":30},{"variant":31},{"variant":32},{"variant":33},{"variant":34},{"variant":20},{"variant":21},{"variant":22},{"variant":23},{"variant":24},{"variant":35},{"variant":36},{"variant":37},{"variant":38},{"variant":39},{"variant":40},{"variant":41},{"variant":42},{"variant":43},{"variant":44},{"variant":50},{"variant":51},{"variant":52},{"variant":53},{"variant":54},{"variant":45},{"variant":46},{"variant":47},{"variant":48},{"variant":49},{"variant":55},{"variant":56},{"variant":57},{"variant":58},{"variant":59}]})JSON";
    return json;
}

inline proto::AddActorPacket buildCarrierAddActor(
    std::uint64_t        uniqueId,
    std::uint64_t        runtimeId,
    std::string const&   identifier,
    std::string const&   npcName,
    std::string const&   actionJson,
    proto::Vec3 const    position,
    float                yaw,
    int                  skinVariant = -1) {
    using ids = proto::ActorDataIDs;

    proto::AddActorPacket packet;
    packet.entity_id        = proto::ActorUniqueID{static_cast<std::int64_t>(uniqueId)};
    packet.runtime_id       = proto::bp::ActorRuntimeID{runtimeId};
    packet.actor_type       = identifier.empty() ? std::string{"minecraft:npc"} : identifier;
    packet.pos              = position;
    packet.velocity         = proto::Vec3{0.0f, 0.0f, 0.0f};
    packet.rot              = proto::Vec2{yaw, 0.0f};
    packet.y_head_rotation  = 0.0f;
    packet.y_body_rotation  = 0.0f;

    auto& items = packet.data.data;
    items.reserve(6);
    items.push_back(meta_wire::string(ids::Name, std::string(npcName)));
    items.push_back(meta_wire::byte(ids::HasNpc, static_cast<std::uint8_t>(1)));
    items.push_back(meta_wire::string(ids::NpcData, npcDataJson()));
    items.push_back(meta_wire::string(ids::Actions, actionJson));
    items.push_back(meta_wire::string(ids::InteractText, std::string(npcName)));
    if (skinVariant >= 0) {
        items.push_back(meta_wire::int32(ids::SkinId, static_cast<std::int32_t>(skinVariant)));
    }
    return packet;
}

} // namespace debugshape_export::npcdialog
