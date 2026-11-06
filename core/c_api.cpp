#include "core/c_api.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "core/ItemNbt.h"
#include "core/ProtocolShape.h"
#include "core/ShapeWire.h"

#include "sculk/protocol/codec/actor/MetaData.hpp"
#include "sculk/protocol/codec/actor/player/LocatorBar.hpp"
#include "sculk/protocol/codec/packet/LocatorBarPacket.hpp"
#include "sculk/protocol/codec/actor/player/PlayerListEntry.hpp"
#include "sculk/protocol/codec/actor/player/SerializedSkin.hpp"
#include "sculk/protocol/codec/packet/AddActorPacket.hpp"
#include "sculk/protocol/codec/packet/AddPlayerPacket.hpp"
#include "sculk/protocol/codec/packet/PlayerListPacket.hpp"
#include "sculk/protocol/codec/packet/ContainerClosePacket.hpp"
#include "sculk/protocol/codec/packet/ContainerOpenPacket.hpp"
#include "sculk/protocol/codec/packet/InventoryContentPacket.hpp"
#include "sculk/protocol/codec/packet/UpdateTradePacket.hpp"
#include "sculk/protocol/codec/packet/AddItemActorPacket.hpp"
#include "sculk/protocol/codec/packet/EmotePacket.hpp"
#include "sculk/protocol/codec/packet/MobEquipmentPacket.hpp"
#include "sculk/protocol/codec/packet/SetActorLinkPacket.hpp"
#include "sculk/protocol/codec/packet/NpcDialoguePacket.hpp"
#include "sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp"
#include "sculk/protocol/codec/packet/RemoveActorPacket.hpp"
#include "sculk/protocol/codec/packet/RespawnPacket.hpp"
#include "sculk/protocol/codec/packet/SetActorMotionPacket.hpp"
#include "sculk/protocol/codec/packet/TakeItemActorPacket.hpp"
#include "sculk/protocol/codec/packet/ActorEventPacket.hpp"
#include "sculk/protocol/codec/packet/SetActorDataPacket.hpp"
#include "sculk/protocol/codec/packet/UpdateBlockPacket.hpp"
#include "sculk/protocol/codec/packet/AnimateEntityPacket.hpp"
#include "sculk/protocol/codec/packet/BossEventPacket.hpp"
#include "sculk/protocol/codec/packet/MobEffectPacket.hpp"
#include "sculk/protocol/codec/packet/RemoveObjectivePacket.hpp"
#include "sculk/protocol/codec/packet/SetDisplayObjectivePacket.hpp"
#include "sculk/protocol/codec/packet/SetHudPacket.hpp"
#include "sculk/protocol/codec/packet/SetScorePacket.hpp"
#include "sculk/protocol/codec/packet/SpawnParticleEffectPacket.hpp"

namespace {

using namespace debugshape_export;

DebugShape toShape(holo_shape_desc const& d) {
    DebugShape shape;
    shape.mNetworkId = d.network_id;

    // type < 0：只带 networkId 的删除条目（不写形状体，也不带 type）
    if (d.type < 0) {
        return shape;
    }

    auto const type = static_cast<LSEShapeType>(d.type);
    shape.mType     = toProtoShapeType(type);

    if (d.has_location) {
        shape.mLocation = Vec3{d.x, d.y, d.z};
    }
    if (d.has_scale) {
        shape.mScale = d.scale;
    }
    if (d.has_rotation) {
        shape.mRotation = Vec3{d.rx, d.ry, d.rz};
    }
    if (d.has_total_time_left) {
        shape.mTotalTimeLeft = d.total_time_left;
    }
    if (d.has_max_render_distance) {
        shape.mMaxRenderDistance = d.max_render_distance;
    }
    if (d.has_color) {
        shape.mColor = d.color;
    }
    if (d.has_attached_entity) {
        shape.mAttachedToEntityId = d.attached_entity_id;
    }
    if (d.has_dimension) {
        shape.mDimensionId = d.dimension_id;
    }

    Vec3 const start{d.x, d.y, d.z};
    Vec3 const end{d.ex, d.ey, d.ez};

    switch (type) {
    case LSEShapeType::Text: {
        ProtoTextPayload text{};
        text.mText          = d.text != nullptr ? d.text : "";
        text.mUseRotation   = d.text_use_rotation != 0;
        text.mLineGapHeight = d.text_line_gap_height;
        text.mDepthTest     = d.text_depth_test != 0;
        text.mShowBackface  = d.text_show_backface != 0;
        text.mShowTextBackface = d.text_show_text_backface != 0;
        if (d.has_text_background) {
            text.mBackgroundColor = d.text_background_color;
        }
        shape.mShape = text;
        break;
    }
    case LSEShapeType::Line:
        shape.mShape = ProtoLinePayload{.mLineEndLocation = end};
        break;
    case LSEShapeType::Box:
        shape.mShape = ProtoBoxPayload{.mBoxBound = end};
        break;
    case LSEShapeType::Arrow:
        shape.mShape = ProtoArrowPayload{.mArrowEndLocation = end};
        break;
    case LSEShapeType::Sphere:
        shape.mShape = ProtoSpherePayload{.mSegments = static_cast<std::uint8_t>(d.segments > 0 ? d.segments : 16)};
        break;
    case LSEShapeType::Circle:
        // 圆只用 location + scale
        break;
    case LSEShapeType::Cylinder:
        shape.mShape = sculk::protocol::CylinderDataPayload{
            .mRadiusX     = sculk::protocol::Vec2{d.radius_x, d.radius_x},
            .mRadiusZ     = sculk::protocol::Vec2{d.radius_z, d.radius_z},
            .mHeight      = d.height,
            .mNumSegments = static_cast<std::uint8_t>(d.segments > 0 ? d.segments : 16),
        };
        break;
    case LSEShapeType::Pyramid:
        shape.mShape = sculk::protocol::PyramidDataPayload{
            .mWidth  = d.width,
            .mDepth  = d.depth,
            .mHeight = d.height,
        };
        break;
    case LSEShapeType::Ellipsoid:
        shape.mShape = sculk::protocol::EllipsoidDataPayload{
            .mRadii           = Vec3{d.ex, d.ey, d.ez},
            .mSegmentsPerAxis = static_cast<std::uint8_t>(d.segments > 0 ? d.segments : 12),
        };
        break;
    case LSEShapeType::Cone:
        shape.mShape = sculk::protocol::ConeDataPayload{
            .mRadii       = sculk::protocol::Vec2{d.radius_x, d.radius_z},
            .mHeight      = d.height,
            .mNumSegments = static_cast<std::uint8_t>(d.segments > 0 ? d.segments : 16),
        };
        break;
    default:
        break;
    }

    (void)start;
    return shape;
}

std::vector<DebugShape> toShapes(holo_shape_desc const* shapes, size_t count) {
    std::vector<DebugShape> out;
    if (shapes == nullptr || count == 0) {
        return out;
    }
    out.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        out.push_back(toShape(shapes[i]));
    }
    return out;
}

int32_t copyOut(std::string const& bytes, uint8_t** out, size_t* outLen) {
    if (out == nullptr || outLen == nullptr) {
        return -2;
    }
    auto* buffer = static_cast<std::uint8_t*>(std::malloc(bytes.size() == 0 ? 1 : bytes.size()));
    if (buffer == nullptr) {
        return -3;
    }
    std::memcpy(buffer, bytes.data(), bytes.size());
    *out    = buffer;
    *outLen = bytes.size();
    return 0;
}

} // namespace

extern "C" {

uint32_t holo_core_api_version(void) { return HOLO_CORE_API_VERSION; }

uint32_t holo_core_packet_id_primitive_shapes(void) { return debugshape_export::core::kPrimitiveShapesPacketId; }

const char* holo_core_version(void) { return "26.60.1"; }

int32_t holo_core_encode_primitive_shapes(
    holo_shape_desc const* shapes,
    size_t                 count,
    uint8_t**              out_body,
    size_t*                out_len) {
    if (shapes == nullptr || count == 0) {
        return -1;
    }
    auto const encoded = debugshape_export::core::buildPrimitiveShapesBody(toShapes(shapes, count));
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_primitive_shapes_packet(
    holo_shape_desc const* shapes,
    size_t                 count,
    uint8_t**              out_packet,
    size_t*                out_len) {
    if (shapes == nullptr || count == 0) {
        return -1;
    }
    auto const encoded = debugshape_export::core::buildPrimitiveShapesPacket(toShapes(shapes, count));
    return copyOut(encoded, out_packet, out_len);
}

uint32_t holo_core_packet_id_spawn_particle(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::SpawnParticleEffect);
}

int32_t holo_core_encode_spawn_particle(
    uint8_t     dimension_id,
    int64_t     actor_unique_id,
    float       x,
    float       y,
    float       z,
    const char* effect,
    uint8_t**   out_body,
    size_t*     out_len) {
    sculk::protocol::SpawnParticleEffectPacket packet;
    packet.mDimensionId    = dimension_id;
    packet.mActorUniqueId  = actor_unique_id;
    packet.mPosition       = Vec3{x, y, z};
    packet.mEffectName     = effect != nullptr ? effect : "";
    packet.mMolangVariableMap = std::nullopt;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_boss_event(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::BossEvent);
}

int32_t holo_core_encode_boss_event(
    int64_t     target_actor_id,
    int64_t     player_id,
    uint8_t     type,
    const char* name,
    const char* filtered_name,
    float       percentage,
    uint32_t    color,
    uint32_t    overlay,
    uint8_t**   out_body,
    size_t*     out_len) {
    sculk::protocol::BossEventPacket packet;
    packet.mTargetActorID = target_actor_id;
    packet.mPlayerID      = player_id;
    packet.mType          = static_cast<sculk::protocol::BossEventPacket::EventType>(type);
    packet.mName          = name != nullptr ? name : "";
    packet.mFilteredName  = filtered_name != nullptr ? filtered_name : "";
    packet.mPercentage    = percentage;
    packet.mColor         = color;
    packet.mOverlay       = overlay;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_set_score(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::SetScore);
}

uint32_t holo_core_packet_id_set_display_objective(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::SetDisplayObjective);
}

uint32_t holo_core_packet_id_remove_objective(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::RemoveObjective);
}

uint32_t holo_core_packet_id_set_hud(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::SetHud);
}

int32_t holo_core_encode_set_display_objective(
    const char* slot,
    const char* objective,
    const char* display_name,
    const char* criteria,
    uint8_t     sort_order,
    uint8_t**   out_body,
    size_t*     out_len) {
    sculk::protocol::SetDisplayObjectivePacket packet;
    packet.mDisplaySlotName      = slot != nullptr ? slot : "sidebar";
    packet.mObjectiveName        = objective != nullptr ? objective : "";
    packet.mObjectiveDisplayName = display_name != nullptr ? display_name : "";
    packet.mCriteriaName         = criteria != nullptr ? criteria : "dummy";
    packet.mSortOrder = static_cast<sculk::protocol::SetDisplayObjectivePacket::ObjectiveSortOrder>(sort_order);

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_remove_objective(
    const char* objective,
    uint8_t**   out_body,
    size_t*     out_len) {
    sculk::protocol::RemoveObjectivePacket packet;
    packet.mObjectiveName = objective != nullptr ? objective : "";

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_set_score(
    uint8_t                 packet_type,
    const holo_score_entry* entries,
    size_t                  count,
    uint8_t**               out_body,
    size_t*                 out_len) {
    sculk::protocol::SetScorePacket packet;
    packet.mPacketType = static_cast<sculk::protocol::SetScorePacket::PacketType>(packet_type);
    packet.mScoresInfo.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        sculk::protocol::SetScorePacket::ScoreInfo info;
        info.mScoreboardId  = entries[i].scoreboard_id;
        info.mObjectiveName = entries[i].objective != nullptr ? entries[i].objective : "";
        info.mScoreValue    = entries[i].score_value;
        info.mIdentityType =
            static_cast<sculk::protocol::SetScorePacket::IdentityType>(entries[i].identity_type);
        info.mActorUniqueId  = entries[i].actor_unique_id;
        info.mFakePlayerName = entries[i].fake_player_name != nullptr ? entries[i].fake_player_name : "";
        packet.mScoresInfo.push_back(std::move(info));
    }

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_set_hud(
    const uint8_t* elements,
    size_t         count,
    uint8_t        visibility,
    uint8_t**      out_body,
    size_t*        out_len) {
    sculk::protocol::SetHudPacket packet;
    packet.mHudElements.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        packet.mHudElements.push_back(static_cast<sculk::protocol::SetHudPacket::HudElement>(elements[i]));
    }
    packet.mHudVisibility = static_cast<sculk::protocol::SetHudPacket::HudVisibility>(visibility);

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_mob_effect(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::MobEffect);
}

uint32_t holo_core_packet_id_actor_event(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::ActorEvent);
}

uint32_t holo_core_packet_id_animate_entity(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::AnimateEntity);
}

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
    sculk::protocol::MobEffectPacket packet;
    packet.mActorRuntimeId      = runtime_id;
    packet.mEventId             = static_cast<sculk::protocol::MobEffectPacket::Event>(event_type);
    packet.mEffectId            = effect_id;
    packet.mEffectAmplifier     = amplifier;
    packet.mShowParticles       = show_particles != 0;
    packet.mEffectDurationTicks = duration_ticks;
    packet.mTick                = tick;
    packet.mAmbient             = ambient != 0;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_actor_event(
    uint64_t  runtime_id,
    int32_t   event_id,
    int32_t   data,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::ActorEventPacket packet;
    packet.mRuntimeId = runtime_id;
    packet.mEventId   = static_cast<sculk::protocol::ActorEvent>(event_id);
    packet.mData      = data;
    packet.mFireAtPosition.reset();

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
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
    sculk::protocol::AnimateEntityPacket packet;
    packet.mAnimation                   = animation != nullptr ? animation : "";
    packet.mNextState                   = next_state != nullptr ? next_state : "";
    packet.mStopExpression              = stop_expression != nullptr ? stop_expression : "";
    packet.mStopExpressionMolangVersion = static_cast<sculk::protocol::MolangVersion>(molang_version);
    packet.mController                  = controller != nullptr ? controller : "";
    packet.mBlendOutTime                = blend_out_time;
    packet.mRuntimeIds.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        packet.mRuntimeIds.push_back(runtime_ids[i]);
    }

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_update_block(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::UpdateBlock);
}

int32_t holo_core_encode_update_block(
    int32_t   x,
    int32_t   y,
    int32_t   z,
    uint32_t  runtime_id,
    uint32_t  flag,
    uint32_t  layer,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::UpdateBlockPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mRuntimeId     = runtime_id;
    packet.mFlag          = flag;
    packet.mLayer         = layer;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_set_actor_data(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::SetActorData);
}

int32_t holo_core_encode_set_actor_data(
    uint64_t               runtime_id,
    uint64_t               tick,
    const holo_meta_entry* entries,
    size_t                 count,
    uint8_t**              out_body,
    size_t*                out_len) {
    using sculk::protocol::ActorDataIDs;
    using sculk::protocol::MetaData;
    using sculk::protocol::Vec3;

    sculk::protocol::SetActorDataPacket packet;
    packet.mActorRuntimeId = runtime_id;
    packet.mTick           = tick;
    packet.mSynchedProperties = {};

    for (size_t i = 0; i < count; ++i) {
        holo_meta_entry const& e = entries[i];
        MetaData::DataItem item;
        item.mId = static_cast<ActorDataIDs>(e.id);
        switch (e.kind) {
        case 0:
            item.mData = static_cast<std::uint8_t>(e.ival);
            break;
        case 1:
            item.mData = static_cast<short>(e.ival);
            break;
        case 2:
            item.mData = static_cast<int>(e.ival);
            break;
        case 3:
            item.mData = e.fval;
            break;
        case 4:
            item.mData = static_cast<std::int64_t>(e.ival);
            break;
        case 5:
            item.mData = Vec3{e.x, e.y, e.z};
            break;
        default:
            item.mData = static_cast<int>(e.ival);
            break;
        }
        packet.mMetaData.mDataItems.push_back(std::move(item));
    }

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_locator_bar(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::LocatorBar);
}

int32_t holo_core_encode_locator_bar(
    const holo_waypoint* waypoints,
    size_t               count,
    uint8_t**            out_body,
    size_t*              out_len) {
    using namespace sculk::protocol;

    LocatorBarPacket packet;
    packet.mWaypoints.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        holo_waypoint const& w = waypoints[i];
        LocatorBarWaypointPayload payload;
        payload.mGroupHandle.mUUID.mMostSignificantBits  = w.msb;
        payload.mGroupHandle.mUUID.mLeastSignificantBits = w.lsb;
        payload.mActionFlag = static_cast<LocatorBarAction>(w.action);

        auto& server = payload.mServerWaypointPayload;
        server.mUpdateFlag = static_cast<LocatorBarServerWaypointPayload::UpdateFlags>(w.flags);
        if (w.has_visible) {
            server.mIsVisible = w.visible != 0;
        }
        if (w.has_position) {
            server.mWorldPosition = LocatorBarWorldPosition{Vec3{w.x, w.y, w.z}, w.dimension};
        }
        if (w.has_texture) {
            server.mTexturePath = w.texture != nullptr ? w.texture : "";
        }
        if (w.has_icon_size) {
            server.mIconSize = Vec2{w.icon_w, w.icon_h};
        }
        if (w.has_color) {
            server.mColor = w.color;
        }
        if (w.has_authority) {
            server.mClientPositionAuthority = w.authority != 0;
        }
        if (w.has_actor) {
            server.mActorUniqueId = w.actor_uid;
        }
        packet.mWaypoints.push_back(std::move(payload));
    }

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_respawn(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::Respawn);
}

int32_t holo_core_encode_respawn(
    float     x,
    float     y,
    float     z,
    uint8_t   state,
    uint64_t  runtime_id,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::RespawnPacket packet;
    packet.mPosition       = Vec3{x, y, z};
    packet.mState          = static_cast<sculk::protocol::RespawnPacket::PlayerRespawnState>(state > 2 ? 2 : state);
    packet.mActorRuntimeId = runtime_id;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_add_item_actor(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::AddItemActor);
}

uint32_t holo_core_packet_id_remove_actor(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::RemoveActor);
}

uint32_t holo_core_packet_id_move_actor_absolute(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::MoveActorAbsolute);
}

uint32_t holo_core_packet_id_set_actor_motion(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::SetActorMotion);
}

uint32_t holo_core_packet_id_take_item_actor(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::TakeItemActor);
}

int32_t holo_core_encode_add_item_actor(
    uint64_t  runtime_id,
    int64_t   unique_id,
    int16_t   item_id,
    uint16_t  stack_size,
    uint32_t  aux,
    float     x, float y, float z,
    float     vx, float vy, float vz,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::AddItemActorPacket packet;
    packet.mRuntimeId = runtime_id;
    packet.mUniqueId  = unique_id;
    packet.mItem.mId        = item_id;
    packet.mItem.mStackSize = stack_size;
    packet.mItem.mAux       = aux;
    packet.mPos             = Vec3{x, y, z};
    packet.mVelocity        = Vec3{vx, vy, vz};
    packet.mIsFromFishing   = false;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_remove_actor(
    int64_t   unique_id,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::RemoveActorPacket packet;
    packet.mActorUniqueId = unique_id;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_move_actor_absolute(
    uint64_t  runtime_id,
    uint8_t   flags,
    float     x, float y, float z,
    uint8_t   rotation_x,
    uint8_t   rotation_y,
    uint8_t   rotation_y_head,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::MoveActorAbsolutePacket packet;
    packet.mActorRuntimeId = runtime_id;
    packet.mFlags          = static_cast<sculk::protocol::MoveActorAbsolutePacket::Flags>(flags);
    packet.mPosition       = Vec3{x, y, z};
    packet.mRotationX      = rotation_x;
    packet.mRotationY      = rotation_y;
    packet.mRotationYHead  = rotation_y_head;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_set_actor_motion(
    uint64_t  runtime_id,
    float     vx, float vy, float vz,
    uint64_t  tick,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::SetActorMotionPacket packet;
    packet.mActorRuntimeId = runtime_id;
    packet.mMotion         = Vec3{vx, vy, vz};
    packet.mTick           = tick;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_take_item_actor(
    uint64_t  item_runtime_id,
    uint64_t  actor_runtime_id,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::TakeItemActorPacket packet;
    packet.mItemRuntimeId  = item_runtime_id;
    packet.mActorRuntimeId = actor_runtime_id;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_add_actor(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::AddActor);
}

uint32_t holo_core_packet_id_mob_equipment(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::MobEquipment);
}

int32_t holo_core_encode_add_actor_ex(
    const char* identifier,
    const char* name,
    uint64_t    runtime_id,
    int64_t     unique_id,
    float       x, float y, float z,
    float       yaw, float pitch,
    float       hitbox_width,
    float       hitbox_height,
    uint8_t**   out_body,
    size_t*     out_len) {
    using namespace sculk::protocol;

    AddActorPacket packet;
    packet.mActorRuntimeId = runtime_id;
    packet.mActorUniqueId  = unique_id;
    packet.mIdentifier     = identifier != nullptr ? identifier : "minecraft:fox";
    packet.mPosition       = Vec3{x, y, z};
    packet.mVelocity       = Vec3{0, 0, 0};
    packet.mRotation       = Vec2{pitch, yaw};
    packet.mYHeadRotation  = 0.0f;
    packet.mYBodyRotation  = 0.0f;

    packet.mAttributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:movement", 0.0f, 3.4028235e38f, 0.3f},
        {"minecraft:underwater_movement", 0.0f, 3.4028235e38f, 0.02f},
        {"minecraft:lava_movement", 0.0f, 3.4028235e38f, 0.02f},
        {"minecraft:absorption", 0.0f, 16.0f, 0.0f},
        {"minecraft:luck", -1024.0f, 1024.0f, 0.0f},
        {"minecraft:knockback_resistance", 1.0f, 1.0f, 0.0f},
    };

    std::int64_t flags = (std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31);
    packet.mMetaData.mDataItems = {
        {ActorDataIDs::Reserved0, flags},
        {ActorDataIDs::Reserved53, hitbox_width},
        {ActorDataIDs::Reserved54, hitbox_height},
    };
    if (name != nullptr && name[0] != ' ') {
        packet.mMetaData.mDataItems.push_back({ActorDataIDs::Name, std::string{name}});
    }

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
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
    sculk::protocol::MobEquipmentPacket packet;
    packet.mRuntimeId         = runtime_id;
    packet.mItem.mId          = item_id;
    packet.mItem.mStackSize   = stack_size;
    packet.mItem.mAux         = aux;
    packet.mSlotByte          = slot;
    packet.mSelectedSlotByte  = selected_slot;
    packet.mContainerIdByte   = container_id;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_container_open(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::ContainerOpen);
}

uint32_t holo_core_packet_id_container_close(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::ContainerClose);
}

uint32_t holo_core_packet_id_inventory_content(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::InventoryContent);
}

int32_t holo_core_encode_container_open(
    uint8_t   container_id,
    uint8_t   container_type,
    int32_t   x, int32_t y, int32_t z,
    int64_t   target_actor_id,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::ContainerOpenPacket packet;
    packet.mContainerId   = static_cast<sculk::protocol::ContainerID>(container_id);
    packet.mContainerType = static_cast<sculk::protocol::ContainerType>(container_type);
    packet.mPosition      = sculk::protocol::BlockPos{x, y, z};
    packet.mTargetActorId = target_actor_id;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_container_close(
    uint8_t   container_id,
    uint8_t   container_type,
    uint8_t   server_initiated,
    uint8_t** out_body,
    size_t*   out_len) {
    sculk::protocol::ContainerClosePacket packet;
    packet.mContainerId          = static_cast<sculk::protocol::ContainerID>(container_id);
    packet.mContainerType        = static_cast<sculk::protocol::ContainerType>(container_type);
    packet.mServerInitiatedClose = server_initiated != 0;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_inventory_content(
    uint32_t              container_id,
    const holo_item_slot* slots,
    size_t                count,
    uint8_t**             out_body,
    size_t*               out_len) {
    sculk::protocol::InventoryContentPacket packet;
    packet.mInventoryId = container_id;
    packet.mSlots.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        sculk::protocol::NetworkItemStackDescriptor item;
        item.mId        = slots[i].item_id;
        item.mStackSize = slots[i].stack_size;
        item.mAux       = slots[i].aux;
        if (slots[i].user_data != nullptr && slots[i].user_data_len > 0) {
            item.mUserData.assign(slots[i].user_data, slots[i].user_data_len);
        }
        packet.mSlots.push_back(std::move(item));
    }

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

namespace {

std::vector<std::string> splitCsv(char const* csv, char sep) {
    std::vector<std::string> out;
    if (csv == nullptr) {
        return out;
    }
    std::string current;
    for (char const* p = csv; *p != ' '; ++p) {
        if (*p == sep) {
            if (!current.empty()) {
                out.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(*p);
        }
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

} // namespace

int32_t holo_core_build_item_nbt(
    const char* type,
    int32_t     count,
    int32_t     damage,
    const char* name,
    const char* lore_csv,
    const char* ench_csv,
    int32_t     is_block,
    int32_t     block_version,
    uint8_t**   out_body,
    size_t*     out_len) {
    debugshape_export::trade::TradeItem item;
    item.type    = type != nullptr ? type : "";
    item.count   = count;
    item.damage  = damage;
    item.name    = name != nullptr ? name : "";
    item.lore    = splitCsv(lore_csv, ';');
    item.isBlock = is_block != 0;
    item.blockVersion = block_version;

    for (auto const& entry : splitCsv(ench_csv, ';')) {
        auto const colon = entry.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        try {
            item.enchants.emplace_back(std::stoi(entry.substr(0, colon)), std::stoi(entry.substr(colon + 1)));
        } catch (...) {
        }
    }

    sculk::protocol::CompoundTag tag = debugshape_export::trade::detail::itemToNbt(item);
    std::vector<std::byte> buffer;
    sculk::protocol::BinaryStream stream{buffer};
    tag.serialize(stream);

    std::string encoded;
    encoded.reserve(buffer.size());
    for (auto const byte : buffer) {
        encoded.push_back(static_cast<char>(byte));
    }
    return copyOut(encoded, out_body, out_len);
}

namespace {

std::vector<std::string> splitCsvStr(const char* csv) {
    std::vector<std::string> out;
    if (csv == nullptr) {
        return out;
    }
    std::string current;
    for (char const* p = csv; *p != ' '; ++p) {
        if (*p == ';') {
            if (!current.empty()) {
                out.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(*p);
        }
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

debugshape_export::trade::TradeItem toTradeItem(holo_trade_item const& in) {
    debugshape_export::trade::TradeItem item;
    item.type    = in.type != nullptr ? in.type : "";
    item.count   = in.count;
    item.damage  = in.damage;
    item.name    = in.name != nullptr ? in.name : "";
    item.lore    = splitCsvStr(in.lore_csv);
    item.isBlock = in.is_block != 0;
    item.blockVersion = in.block_version;
    for (auto const& entry : splitCsvStr(in.ench_csv)) {
        auto const colon = entry.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        try {
            item.enchants.emplace_back(std::stoi(entry.substr(0, colon)), std::stoi(entry.substr(colon + 1)));
        } catch (...) {
        }
    }
    return item;
}

std::string serializeTag(sculk::protocol::CompoundTag const& tag) {
    std::vector<std::byte> buffer;
    sculk::protocol::BinaryStream stream{buffer};
    tag.serialize(stream);
    std::string out;
    out.reserve(buffer.size());
    for (auto const byte : buffer) {
        out.push_back(static_cast<char>(byte));
    }
    return out;
}

} // namespace

uint32_t holo_core_packet_id_update_trade(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::UpdateTrade);
}

int32_t holo_core_build_offers(
    const holo_trade_offer* offers,
    size_t                  count,
    const int32_t*          tier_exp,
    size_t                  tier_count,
    int32_t                 base_net_id,
    uint8_t**               out_body,
    size_t*                 out_len) {
    std::vector<debugshape_export::trade::TradeOffer> list;
    list.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        debugshape_export::trade::TradeOffer offer;
        offer.buyA  = toTradeItem(offers[i].buy_a);
        offer.buyB  = toTradeItem(offers[i].buy_b);
        offer.sell  = toTradeItem(offers[i].sell);
        offer.tier  = offers[i].tier;
        offer.maxUses = offers[i].max_uses;
        offer.uses  = offers[i].uses;
        offer.netId = offers[i].net_id;
        offer.traderExp = offers[i].trader_exp;
        offer.demand = offers[i].demand;
        offer.rewardExp = offers[i].reward_exp != 0;
        offer.priceMultiplierA = offers[i].price_multiplier_a;
        offer.priceMultiplierB = offers[i].price_multiplier_b;
        list.push_back(std::move(offer));
    }

    std::vector<int> tierRequirements;
    tierRequirements.reserve(tier_count);
    for (size_t i = 0; i < tier_count; ++i) {
        tierRequirements.push_back(tier_exp[i]);
    }

    auto const tag = debugshape_export::trade::buildOffers(list, tierRequirements, base_net_id);
    return copyOut(serializeTag(tag), out_body, out_len);
}

int32_t holo_core_encode_update_trade(
    uint8_t     container_id,
    uint8_t     container_type,
    int32_t     size,
    int32_t     tier,
    int64_t     entity_unique_id,
    int64_t     last_trading_player,
    const char* display_name,
    uint8_t     use_new_screen,
    uint8_t     use_economy_trade,
    const uint8_t* offers_nbt,
    size_t      offers_nbt_len,
    uint8_t**   out_body,
    size_t*     out_len) {
    sculk::protocol::UpdateTradePacket packet;
    packet.mContainerId       = static_cast<sculk::protocol::ContainerID>(container_id);
    packet.mContainerType     = static_cast<sculk::protocol::ContainerType>(container_type);
    packet.mSize              = size;
    packet.mTier              = tier;
    packet.mEntityUniqueId    = entity_unique_id;
    packet.mLastTradingPlayer = last_trading_player;
    packet.mDisplayName       = display_name != nullptr ? display_name : "";
    packet.mUseNewTradeScreen = use_new_screen != 0;
    packet.mUseEconomyTrade   = use_economy_trade != 0;
    // offers 由调用方给 NBT 字节，这里反序列化回 CompoundTag
    if (offers_nbt != nullptr && offers_nbt_len > 0) {
        std::vector<std::byte> buffer;
        buffer.reserve(offers_nbt_len);
        for (size_t i = 0; i < offers_nbt_len; ++i) {
            buffer.push_back(static_cast<std::byte>(offers_nbt[i]));
        }
        sculk::protocol::ReadOnlyBinaryStream stream{std::string_view{
            reinterpret_cast<char const*>(buffer.data()), buffer.size()}};
        sculk::protocol::CompoundTag tag;
        tag.deserialize(stream);
        packet.mOffers = std::move(tag);
    }

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_npc_dialogue(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::NpcDialogue);
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
    sculk::protocol::NpcDialoguePacket packet;
    packet.mNpcId      = npc_id;
    packet.mActionType = static_cast<sculk::protocol::NpcDialoguePacket::ActionType>(action);
    packet.mDialogue   = dialogue != nullptr ? dialogue : "";
    packet.mSceneName  = scene_name != nullptr ? scene_name : "";
    packet.mNpcName    = npc_name != nullptr ? npc_name : "";
    packet.mActionJSON = action_json != nullptr ? action_json : "[]";

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

namespace {

sculk::protocol::UUID uuidFrom16(uint8_t const* bytes) {
    sculk::protocol::UUID uuid;
    if (bytes == nullptr) {
        return uuid;
    }
    std::memcpy(&uuid.mMostSignificantBits, bytes, 8);
    std::memcpy(&uuid.mLeastSignificantBits, bytes + 8, 8);
    uuid.mMostSignificantBits  = __builtin_bswap64(uuid.mMostSignificantBits);
    uuid.mLeastSignificantBits = __builtin_bswap64(uuid.mLeastSignificantBits);
    return uuid;
}

sculk::protocol::SerializedSkin makeSkin(holo_skin_desc const* desc) {
    sculk::protocol::SerializedSkin skin;
    if (desc == nullptr) {
        return skin;
    }
    skin.mId            = desc->id != nullptr ? desc->id : "";
    skin.mFullId        = skin.mId;
    skin.mPlayFabId     = "";
    skin.mResourcePatch = "";
    skin.mSkinImageWidth  = desc->width;
    skin.mSkinImageHeight = desc->height;
    if (desc->rgba != nullptr && desc->rgba_len > 0) {
        skin.mSkinImageBytes.assign(desc->rgba, desc->rgba_len);
    }
    skin.mGeometryData = desc->geometry != nullptr ? desc->geometry : "";
    skin.mArmSize      = desc->arm_size != nullptr ? desc->arm_size : "wide";
    skin.mSkinColor    = "#0";
    skin.mIsPrimaryUser = true;
    skin.mTrustedSkinFlag = 1;
    return skin;
}

} // namespace

uint32_t holo_core_packet_id_player_list(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::PlayerList);
}

uint32_t holo_core_packet_id_add_player(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::AddPlayer);
}

int32_t holo_core_encode_player_list_add(
    const uint8_t*        uuid16,
    int64_t               actor_unique_id,
    const char*           name,
    const holo_skin_desc* skin,
    uint8_t**             out_body,
    size_t*               out_len) {
    sculk::protocol::PlayerListPacket packet;
    packet.mAction = sculk::protocol::PlayerListPacket::ActionType::Add;

    sculk::protocol::PlayerListEntry entry;
    entry.mUUID          = uuidFrom16(uuid16);
    entry.mActorUniqueId = actor_unique_id;
    entry.mPlayerName    = name != nullptr ? name : "";
    entry.mSerializedSkin = makeSkin(skin);
    entry.mBuildPlatform  = 7; // Windows
    packet.mPlayerEntryList.push_back(std::move(entry));

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_player_list_remove(
    const uint8_t* uuid16,
    uint8_t**      out_body,
    size_t*        out_len) {
    sculk::protocol::PlayerListPacket packet;
    packet.mAction = sculk::protocol::PlayerListPacket::ActionType::Remove;
    sculk::protocol::PlayerListEntry entry;
    entry.mUUID = uuidFrom16(uuid16);
    packet.mPlayerEntryList.push_back(std::move(entry));

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_add_player(
    uint64_t              runtime_id,
    const char*           name,
    const uint8_t*        uuid16,
    const holo_skin_desc* skin,
    float                 x, float y, float z,
    float                 yaw, float pitch,
    float                 scale,
    const char*           nametag,
    uint8_t**             out_body,
    size_t*               out_len) {
    using namespace sculk::protocol;

    AddPlayerPacket packet;
    packet.mActorRuntimeId   = runtime_id;
    packet.mName             = name != nullptr ? name : "";
    packet.mUuid             = uuidFrom16(uuid16);
    packet.mPos              = Vec3{x, y, z};
    packet.mVelocity         = Vec3{0, 0, 0};
    packet.mRot              = Vec2{pitch, yaw};
    packet.mYHeadRot         = yaw;
    packet.mPlatformOnlineId = "";
    packet.mDeviceId         = "";
    packet.mBuildPlatform    = 7;
    packet.mGameType         = GameType::Survival;

    packet.mAbilities.mPlayerRawId       = static_cast<std::int64_t>(runtime_id);
    packet.mAbilities.mPlayerPermission  = 2; // member
    packet.mAbilities.mCommandPermission = 0;
    packet.mAbilities.mLayers.push_back(SerializedAbilitiesData::SerializedLayer{
        .mLayer            = 0,
        .mAbilitiesSet     = 0x000000000000FFFFu,
        .mAbilitiesValue   = 0,
        .mFlySpeed         = 0.05f,
        .mVerticalFlySpeed = 1.0f,
        .mWalkSpeed        = 0.1f,
    });

    std::int64_t flags = (std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31);
    packet.mMetaData.mDataItems = {
        {ActorDataIDs::Reserved0, flags},
        {ActorDataIDs::Reserved53, 0.6f * scale},
        {ActorDataIDs::Reserved54, 1.8f * scale},
    };
    if (nametag != nullptr && nametag[0] != 0) {
        packet.mMetaData.mDataItems.push_back({ActorDataIDs::Name, std::string{nametag}});
    }

    (void)skin; // 皮肤通过 PlayerList + 客户端缓存关联，AddPlayer 里不带完整皮肤

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

uint32_t holo_core_packet_id_emote(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::Emote);
}

uint32_t holo_core_packet_id_set_actor_link(void) {
    return static_cast<uint32_t>(sculk::protocol::MinecraftPacketIds::SetActorLink);
}

int32_t holo_core_encode_emote(
    uint64_t    runtime_id,
    const char* emote_id,
    uint32_t    length_ticks,
    uint8_t     flags,
    uint8_t**   out_body,
    size_t*     out_len) {
    sculk::protocol::EmotePacket packet;
    packet.mActorRuntimeId   = runtime_id;
    packet.mEmoteId          = emote_id != nullptr ? emote_id : "";
    packet.mEmoteLengthTicks = length_ticks;
    packet.mXuid             = "";
    packet.mPlatformId       = "";
    packet.mFlags            = flags;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
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
    sculk::protocol::SetActorLinkPacket packet;
    packet.mLink.mType                     = static_cast<sculk::protocol::ActorLinkType>(type);
    packet.mLink.mTargetAUniqueId          = target_a_unique_id;
    packet.mLink.mTargetBUniqueId          = target_b_unique_id;
    packet.mLink.mImmediate                = immediate != 0;
    packet.mLink.mPassengerInitiated       = passenger_initiated != 0;
    packet.mLink.mVehicleAngularVelocity   = vehicle_angular_velocity;

    auto const encoded = debugshape_export::codec::body(packet);
    return copyOut(encoded, out_body, out_len);
}

void holo_free(void* ptr) { std::free(ptr); }

} // extern "C"
