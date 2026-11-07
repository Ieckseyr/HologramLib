#include "core/c_api.h"

#include "core/c_api_internal.h"

#include "core/ProtocolShape.h"
#include "core/ShapeWire.h"

namespace debugshape_export {
namespace {

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
        text.mText             = d.text != nullptr ? d.text : "";
        text.mUseRotation      = d.text_use_rotation != 0;
        text.mLineGapHeight    = d.text_line_gap_height;
        text.mDepthTest        = d.text_depth_test != 0;
        text.mShowBackface     = d.text_show_backface != 0;
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
        shape.mShape = DebugCylinder{
            .mRadiusX     = Vec2{d.radius_x, d.radius_x},
            .mRadiusZ     = Vec2{d.radius_z, d.radius_z},
            .mHeight      = d.height,
            .mNumSegments = static_cast<std::uint8_t>(d.segments > 0 ? d.segments : 16),
        };
        break;
    case LSEShapeType::Pyramid:
        shape.mShape = DebugPyramid{
            .mWidth  = d.width,
            .mDepth  = d.depth,
            .mHeight = d.height,
        };
        break;
    case LSEShapeType::Ellipsoid:
        shape.mShape = DebugEllipsoid{
            .mRadii           = Vec3{d.ex, d.ey, d.ez},
            .mSegmentsPerAxis = static_cast<std::uint8_t>(d.segments > 0 ? d.segments : 12),
        };
        break;
    case LSEShapeType::Cone:
        shape.mShape = DebugCone{
            .mRadii       = Vec2{d.radius_x, d.radius_z},
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

} // namespace
} // namespace debugshape_export

extern "C" {

int32_t holo_core_encode_primitive_shapes(
    holo_shape_desc const* shapes,
    size_t                 count,
    uint8_t**              out_body,
    size_t*                out_len) {
    if (shapes == nullptr || count == 0) {
        return -1;
    }
    auto const encoded = debugshape_export::core::buildPrimitiveShapesBody(debugshape_export::toShapes(shapes, count));
    return debugshape_export::copyOut(encoded, out_body, out_len);
}

int32_t holo_core_encode_primitive_shapes_packet(
    holo_shape_desc const* shapes,
    size_t                 count,
    uint8_t**              out_packet,
    size_t*                out_len) {
    if (shapes == nullptr || count == 0) {
        return -1;
    }
    auto const encoded = debugshape_export::core::buildPrimitiveShapesPacket(debugshape_export::toShapes(shapes, count));
    return debugshape_export::copyOut(encoded, out_packet, out_len);
}

} // extern "C"
