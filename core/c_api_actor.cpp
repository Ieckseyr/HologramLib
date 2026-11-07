#include "core/c_api.h"

#include "core/c_api_internal.h"

#include "BlockPackets.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "core/ItemNbt.h"
#include "core/ProtocolShape.h"
#include "core/ShapeWire.h"


namespace {

using namespace debugshape_export;

} // namespace

namespace {

using namespace debugshape_export;





} // namespace

extern "C" {

int32_t holo_core_encode_mob_effect(
    uint64_t  runtime_id,
    uint8_t   event_type,
    int32_t   effect_id,
    int32_t   amplifier,
    uint8_t   show_particles,
    int32_t   duration_ticks,
    uint64_t  tick,
    uint8_t   ambient,
    uint8_t** out_body,
    size_t*   out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::MobEffectPacket packet;
    packet.runtime_id            = proto::bp::ActorRuntimeID{runtime_id};
    packet.event_id              = static_cast<decltype(packet.event_id)>(event_type);
    packet.effect_id             = effect_id;
    packet.effect_amplifier      = amplifier;
    packet.show_particles        = show_particles != 0;
    packet.effect_duration_ticks = duration_ticks;
    packet.tick                  = proto::bp::PlayerInputTick{tick};
    packet.ambient               = ambient != 0;
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_actor_event(
    uint64_t  runtime_id,
    int32_t   event_id,
    int32_t   data,
    uint8_t** out_body,
    size_t*   out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::ActorEventPacket packet;
    packet.runtime_id = proto::bp::ActorRuntimeID{runtime_id};
    packet.event_id   = static_cast<decltype(packet.event_id)>(event_id);
    packet.data       = data;
    packet.fire_at_position.reset();
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_animate_entity(
    const char*     animation,
    const char*     next_state,
    const char*     stop_expression,
    int32_t         molang_version,
    const char*     controller,
    float           blend_out_time,
    const uint64_t* runtime_ids,
    size_t          count,
    uint8_t**       out_body,
    size_t*         out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::AnimateEntityPacket packet;
    packet.animation               = animation != nullptr ? animation : "";
    packet.next_state              = next_state != nullptr ? next_state : "";
    packet.stop_expression         = stop_expression != nullptr ? stop_expression : "";
    packet.stop_expression_version = molang_version;
    packet.controller              = controller != nullptr ? controller : "";
    packet.blend_out_time          = blend_out_time;
    packet.runtime_ids.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        packet.runtime_ids.push_back(proto::bp::ActorRuntimeID{runtime_ids[i]});
    }
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_set_actor_data(
    uint64_t               runtime_id,
    uint64_t               tick,
    const holo_meta_entry* entries,
    size_t                 count,
    uint8_t**              out_body,
    size_t*                out_len) {
    namespace proto = debugshape_export::core::proto;
    namespace bp    = debugshape_export::core::proto::bp;

    proto::SetActorDataPacket packet;
    packet.id        = bp::ActorRuntimeID{runtime_id};
    packet.tick      = bp::PlayerInputTick{tick};
    packet.synched_properties = {};

    using EntryT   = std::remove_cvref_t<decltype(packet.packed_items.data)>::value_type;
    using Payloads = decltype(EntryT::payload);
    using ByteT    = std::variant_alternative_t<0, Payloads>;
    using ShortT   = std::variant_alternative_t<1, Payloads>;
    using IntT     = std::variant_alternative_t<2, Payloads>;
    using FloatT   = std::variant_alternative_t<3, Payloads>;
    using Int64T   = std::variant_alternative_t<7, Payloads>;
    using Vec3T    = std::variant_alternative_t<8, Payloads>;

    for (size_t i = 0; i < count; ++i) {
        holo_meta_entry const& e = entries[i];
        EntryT item;
        item.id = static_cast<std::uint32_t>(e.id);
        switch (e.kind) {
        case 0:
            item.payload = ByteT{static_cast<std::int8_t>(e.ival)};
            break;
        case 1:
            item.payload = ShortT{static_cast<std::int16_t>(e.ival)};
            break;
        case 2:
            item.payload = IntT{static_cast<std::int32_t>(e.ival)};
            break;
        case 3:
            item.payload = FloatT{e.fval};
            break;
        case 4:
            item.payload = Int64T{e.ival};
            break;
        case 5:
            item.payload = Vec3T{bp::Vec3{e.x, e.y, e.z}};
            break;
        default:
            item.payload = IntT{static_cast<std::int32_t>(e.ival)};
            break;
        }
        packet.packed_items.data.push_back(std::move(item));
    }

    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_respawn(
    float     x,
    float     y,
    float     z,
    uint8_t   state,
    uint64_t  runtime_id,
    uint8_t** out_body,
    size_t*   out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::RespawnPacket packet;
    packet.pos        = proto::bp::Vec3{x, y, z};
    packet.state      = static_cast<decltype(packet.state)>(state > 2 ? 2 : state);
    packet.runtime_id = proto::bp::ActorRuntimeID{runtime_id};
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_remove_actor(
    int64_t   unique_id,
    uint8_t** out_body,
    size_t*   out_len) {
    debugshape_export::core::proto::RemoveActorPacket packet;
    packet.entity_id = debugshape_export::core::proto::ActorUniqueID{unique_id};
    return debugshape_export::copyOut(debugshape_export::core::proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_move_actor_absolute(
    uint64_t runtime_id,
    uint8_t  flags,
    float x, float y, float z,
    uint8_t   rotation_x,
    uint8_t   rotation_y,
    uint8_t   rotation_y_head,
    uint8_t** out_body,
    size_t*   out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::MoveActorAbsolutePacket packet;
    packet.move_data.runtime_id  = proto::bp::ActorRuntimeID{runtime_id};
    packet.move_data.header      = flags;
    packet.move_data.pos         = proto::bp::Vec3{x, y, z};
    packet.move_data.rot_x       = rotation_x;
    packet.move_data.rot_y       = rotation_y;
    packet.move_data.rot_y_head  = rotation_y_head;
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_set_actor_motion(
    uint64_t runtime_id,
    float vx, float vy, float vz,
    uint64_t  tick,
    uint8_t** out_body,
    size_t*   out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::SetActorMotionPacket packet;
    packet.runtime_id = proto::bp::ActorRuntimeID{runtime_id};
    packet.motion     = proto::bp::Vec3{vx, vy, vz};
    packet.tick       = proto::bp::PlayerInputTick{tick};
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_add_actor_ex(
    const char* identifier,
    const char* name,
    uint64_t    runtime_id,
    int64_t     unique_id,
    float x, float y, float z,
    float yaw, float pitch,
    float     hitbox_width,
    float     hitbox_height,
    uint8_t** out_body,
    size_t*   out_len) {
    namespace proto = debugshape_export::core::proto;
    namespace bp    = debugshape_export::core::proto::bp;

    proto::AddActorPacket packet;
    packet.entity_id  = proto::ActorUniqueID{unique_id};
    packet.runtime_id = bp::ActorRuntimeID{runtime_id};
    packet.actor_type = identifier != nullptr ? identifier : "minecraft:fox";
    packet.pos        = bp::Vec3{x, y, z};
    packet.velocity   = bp::Vec3{0, 0, 0};
    packet.rot        = bp::Vec2{pitch, yaw};
    packet.y_head_rotation = 0.0f;
    packet.y_body_rotation = 0.0f;

    auto addAttr = [&](char const* attrName, float minValue, float currentValue, float maxValue) {
        bp::SyncedAttribute attr;
        attr.name          = attrName;
        attr.min_value     = minValue;
        attr.current_value = currentValue;
        attr.max_value     = maxValue;
        packet.attributes.push_back(attr);
    };
    addAttr("minecraft:health", 0.0f, 20.0f, 20.0f);
    addAttr("minecraft:movement", 0.0f, 3.4028235e38f, 0.3f);
    addAttr("minecraft:underwater_movement", 0.0f, 3.4028235e38f, 0.02f);
    addAttr("minecraft:lava_movement", 0.0f, 3.4028235e38f, 0.02f);
    addAttr("minecraft:absorption", 0.0f, 16.0f, 0.0f);
    addAttr("minecraft:luck", -1024.0f, 1024.0f, 0.0f);
    addAttr("minecraft:knockback_resistance", 1.0f, 1.0f, 0.0f);

    using EntryT   = std::remove_cvref_t<decltype(packet.data.data)>::value_type;
    using Payloads = decltype(EntryT::payload);
    using Int64T   = std::variant_alternative_t<7, Payloads>;
    using IntT     = std::variant_alternative_t<2, Payloads>;
    using FloatT   = std::variant_alternative_t<3, Payloads>;
    using StringT  = std::variant_alternative_t<4, Payloads>;

    auto addMeta = [&](std::uint32_t id, EntryT item) {
        item.id = id;
        packet.data.data.push_back(std::move(item));
    };
    (void)addMeta;

    EntryT flagsItem;
    flagsItem.payload = Int64T{(std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31)};
    addMeta(0, std::move(flagsItem));

    EntryT hitboxW;
    hitboxW.payload = FloatT{hitbox_width};
    addMeta(53, std::move(hitboxW));

    EntryT hitboxH;
    hitboxH.payload = FloatT{hitbox_height};
    addMeta(54, std::move(hitboxH));

    if (name != nullptr && name[0] != '\0') {
        EntryT nameItem;
        nameItem.payload = StringT{std::string{name}};
        addMeta(4, std::move(nameItem));
    }
    (void)sizeof(IntT);

    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_mob_equipment(
    uint64_t  runtime_id,
    int16_t   item_id,
    uint16_t  stack_size,
    uint32_t  aux,
    uint8_t   slot,
    uint8_t   selected_slot,
    uint8_t   container_id,
    uint8_t** out_body,
    size_t*   out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::MobEquipmentPacket packet;
    packet.runtime_id    = proto::bp::ActorRuntimeID{runtime_id};
    packet.item.id       = item_id;
    packet.item.stack_size = stack_size;
    packet.item.aux_value  = aux;
    packet.slot          = slot;
    packet.selected_slot = selected_slot;
    packet.container_id  = static_cast<decltype(packet.container_id)>(container_id);
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_npc_dialogue(
    uint64_t    npc_id,
    uint8_t     action,
    const char* dialogue,
    const char* scene_name,
    const char* npc_name,
    const char* action_json,
    uint8_t**   out_body,
    size_t*     out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::NpcDialoguePacket packet;
    packet.npc_id  = proto::ActorUniqueID{static_cast<std::int64_t>(npc_id)};
    packet.npc_dialogue_action_type =
        static_cast<decltype(packet.npc_dialogue_action_type)>(action);
    packet.dialogue    = dialogue != nullptr ? dialogue : "";
    packet.scene_name  = scene_name != nullptr ? scene_name : "";
    packet.npc_name    = npc_name != nullptr ? npc_name : "";
    packet.action_json = action_json != nullptr ? action_json : "[]";
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_player_list_add(
    const uint8_t*        uuid16,
    int64_t               actor_unique_id,
    const char*           name,
    const holo_skin_desc* skin,
    uint8_t**             out_body,
    size_t*               out_len) {
    namespace proto = debugshape_export::core::proto;
    namespace bp    = debugshape_export::core::proto::bp;

    proto::PlayerListPacket packet;
    bp::PlayerListPacket::AddEntry entry;
    entry.action = bp::PlayerListPacketType::Add;
    if (uuid16 != nullptr) {
        std::uint64_t msb = 0;
        std::uint64_t lsb = 0;
        std::memcpy(&msb, uuid16, 8);
        std::memcpy(&lsb, uuid16 + 8, 8);
        entry.uuid = bp::UUID{__builtin_bswap64(msb), __builtin_bswap64(lsb)};
    }
    entry.id   = proto::ActorUniqueID{actor_unique_id};
    entry.name = name != nullptr ? name : "";
    entry.build_platform = static_cast<decltype(entry.build_platform)>(7); // Windows

    if (skin != nullptr) {
        auto& out          = entry.skin;
        out.id             = skin->id != nullptr ? skin->id : "";
        out.full_id        = out.id;
        out.geometry_data  = skin->geometry != nullptr ? skin->geometry : "";
        out.image_data.width  = skin->width;
        out.image_data.height = skin->height;
        if (skin->rgba != nullptr && skin->rgba_len > 0) {
            out.image_data.image_bytes.assign(skin->rgba, skin->rgba_len);
        }
        out.arm_size = (skin->arm_size != nullptr && std::string_view{skin->arm_size} == "slim")
                           ? proto::ArmSizeType::Slim
                           : proto::ArmSizeType::Wide;
        out.skin_color       = proto::SkinColorType{0};
        out.is_primary_user  = true;
        out.trusted_skin_flag = proto::TrustedSkinFlag::False;
    }

    packet.entries.push_back(entry);
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_player_list_remove(
    const uint8_t* uuid16,
    uint8_t**      out_body,
    size_t*        out_len) {
    namespace proto = debugshape_export::core::proto;
    namespace bp    = debugshape_export::core::proto::bp;

    proto::PlayerListPacket packet;
    bp::PlayerListPacket::RemoveEntry entry;
    entry.action = bp::PlayerListPacketType::Remove;
    if (uuid16 != nullptr) {
        std::uint64_t msb = 0;
        std::uint64_t lsb = 0;
        std::memcpy(&msb, uuid16, 8);
        std::memcpy(&lsb, uuid16 + 8, 8);
        entry.uuid = bp::UUID{__builtin_bswap64(msb), __builtin_bswap64(lsb)};
    }
    packet.entries.push_back(entry);

    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_add_player(
    uint64_t              runtime_id,
    const char*           name,
    const uint8_t*        uuid16,
    const holo_skin_desc* skin,
    float x, float y, float z,
    float yaw, float pitch,
    float       scale,
    const char* nametag,
    uint8_t**   out_body,
    size_t*     out_len) {
    namespace proto = debugshape_export::core::proto;
    namespace bp    = debugshape_export::core::proto::bp;
    (void)skin; // 皮肤走 PlayerList + 客户端缓存，AddPlayer 不带完整皮肤

    proto::AddPlayerPacket packet;
    if (uuid16 != nullptr) {
        std::uint64_t msb = 0;
        std::uint64_t lsb = 0;
        std::memcpy(&msb, uuid16, 8);
        std::memcpy(&lsb, uuid16 + 8, 8);
        packet.uuid = bp::UUID{__builtin_bswap64(msb), __builtin_bswap64(lsb)};
    }
    packet.name        = name != nullptr ? name : "";
    packet.runtime_id  = bp::ActorRuntimeID{runtime_id};
    packet.platform_online_id = "";
    packet.device_id   = "";
    packet.pos         = bp::Vec3{x, y, z};
    packet.velocity    = bp::Vec3{0, 0, 0};
    packet.rot         = bp::Vec2{pitch, yaw};
    packet.y_head_rot  = yaw;
    packet.player_game_type = static_cast<decltype(packet.player_game_type)>(0); // Survival
    packet.build_platform   = static_cast<decltype(packet.build_platform)>(7);   // Windows

    packet.abilities_data.target_player       = proto::ActorUniqueID{static_cast<std::int64_t>(runtime_id)};
    packet.abilities_data.player_permissions  = static_cast<decltype(packet.abilities_data.player_permissions)>(2);
    packet.abilities_data.command_permissions = static_cast<decltype(packet.abilities_data.command_permissions)>(0);
    {
        decltype(packet.abilities_data.layers)::value_type layer;
        layer.serialized_layer = static_cast<decltype(layer.serialized_layer)>(0);
        layer.abilities_set    = 0x0000FFFFu;
        layer.ability_values   = 0;
        layer.fly_speed          = 0.05f;
        layer.vertical_fly_speed = 1.0f;
        layer.walk_speed       = 0.1f;
        packet.abilities_data.layers.push_back(layer);
    }

    using EntryT   = std::remove_cvref_t<decltype(packet.unpack.data)>::value_type;
    using Payloads = decltype(EntryT::payload);
    using Int64T   = std::variant_alternative_t<7, Payloads>;
    using FloatT   = std::variant_alternative_t<3, Payloads>;
    using StringT  = std::variant_alternative_t<4, Payloads>;

    auto addMeta = [&](std::uint32_t id, EntryT item) {
        item.id = id;
        packet.unpack.data.push_back(std::move(item));
    };

    EntryT flagsItem;
    flagsItem.payload = Int64T{(std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31)};
    addMeta(0, std::move(flagsItem));

    EntryT hitboxW;
    hitboxW.payload = FloatT{0.6f * scale};
    addMeta(53, std::move(hitboxW));

    EntryT hitboxH;
    hitboxH.payload = FloatT{1.8f * scale};
    addMeta(54, std::move(hitboxH));

    if (nametag != nullptr && nametag[0] != '\0') {
        EntryT nameItem;
        nameItem.payload = StringT{std::string{nametag}};
        addMeta(4, std::move(nameItem));
    }

    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_emote(
    uint64_t    runtime_id,
    const char* emote_id,
    uint32_t    length_ticks,
    uint8_t     flags,
    uint8_t**   out_body,
    size_t*     out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::EmotePacket packet;
    packet.runtime_id  = proto::bp::ActorRuntimeID{runtime_id};
    packet.piece_id    = emote_id != nullptr ? emote_id : "";
    packet.emote_ticks = length_ticks;
    packet.xuid        = "";
    packet.platform_id = "";
    packet.flags       = static_cast<decltype(packet.flags)>(flags);
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_set_actor_link(
    uint8_t   type,
    int64_t   target_a_unique_id,
    int64_t   target_b_unique_id,
    uint8_t   immediate,
    uint8_t   passenger_initiated,
    float     vehicle_angular_velocity,
    uint8_t** out_body,
    size_t*   out_len) {
    namespace proto = debugshape_export::core::proto;
    proto::SetActorLinkPacket packet;
    packet.link.a = proto::ActorUniqueID{target_a_unique_id};
    packet.link.b = proto::ActorUniqueID{target_b_unique_id};
    packet.link.type = static_cast<decltype(packet.link.type)>(type);
    packet.link.immediate = immediate != 0;
    packet.link.passenger_initiated = passenger_initiated != 0;
    packet.link.vehicle_angular_velocity = vehicle_angular_velocity;
    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

} // extern "C"
