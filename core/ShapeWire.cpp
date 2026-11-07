#include "core/ShapeWire.h"

#include "core/proto/Version.h"

#include <variant>

namespace debugshape_export::core {

namespace bp = ::bedrock::protocol;

namespace {

bp::Vec3 toVec3(Vec3 const& v) {
    return bp::Vec3{v.x, v.y, v.z};
}

bp::v1001::ScriptPrimitiveShapeType toProtoType(DebugShapeType type) {
    switch (type) {
    case DebugShapeType::Text:
        return bp::v1001::ScriptPrimitiveShapeType::Text;
    case DebugShapeType::Line:
        return bp::v1001::ScriptPrimitiveShapeType::Line;
    case DebugShapeType::Box:
        return bp::v1001::ScriptPrimitiveShapeType::Box;
    case DebugShapeType::Circle:
        return bp::v1001::ScriptPrimitiveShapeType::Circle;
    case DebugShapeType::Sphere:
        return bp::v1001::ScriptPrimitiveShapeType::Sphere;
    case DebugShapeType::Arrow:
        return bp::v1001::ScriptPrimitiveShapeType::Arrow;
    case DebugShapeType::Cylinder:
        return bp::v1001::ScriptPrimitiveShapeType::Cylinder;
    case DebugShapeType::Pyramid:
        return bp::v1001::ScriptPrimitiveShapeType::Pyramid;
    case DebugShapeType::Ellipsoid:
        return bp::v1001::ScriptPrimitiveShapeType::Ellipsoid;
    case DebugShapeType::Cone:
        return bp::v1001::ScriptPrimitiveShapeType::Cone;
    }
    return bp::v1001::ScriptPrimitiveShapeType::Box;
}

bp::v2193::PrimitiveShapeDataPayload toPayload(DebugShape const& shape) {
    bp::v2193::PrimitiveShapeDataPayload out;
    out.network_id = shape.mNetworkId;
    out.shape_type = toProtoType(shape.mType);
    if (shape.mLocation) {
        out.location = toVec3(*shape.mLocation);
    }
    if (shape.mScale) {
        out.scale = *shape.mScale;
    }
    if (shape.mRotation) {
        out.rotation = toVec3(*shape.mRotation);
    }
    if (shape.mTotalTimeLeft) {
        out.time_left_total_sec = *shape.mTotalTimeLeft;
    }
    if (shape.mMaxRenderDistance) {
        out.max_render_distance = *shape.mMaxRenderDistance;
    }
    if (shape.mColor) {
        out.color = bp::Color{shape.mColor->toPacked()};
    }
    if (shape.mDimensionId) {
        out.dimension_id = static_cast<decltype(out.dimension_id)::value_type>(*shape.mDimensionId);
    }
    if (shape.mAttachedToEntityId) {
        out.attached_to_id = bp::ActorUniqueID{*shape.mAttachedToEntityId};
    }

    std::visit(
        [&](auto const& value) {
            using T = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::is_same_v<T, DebugText>) {
                bp::v2193::TextDataPayload payload;
                payload.text            = value.mText;
                payload.line_gap_height = value.mLineGapHeight;
                out.extra_data_payload  = payload;
            } else if constexpr (std::is_same_v<T, DebugLine>) {
                bp::LineDataPayload payload;
                payload.end_location  = toVec3(value.mLineEndLocation);
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugBox>) {
                bp::BoxDataPayload payload;
                payload.box_bound      = toVec3(value.mBoxBound);
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugSphere>) {
                bp::SphereDataPayload payload;
                payload.num_segments  = value.mSegments;
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugCylinder>) {
                bp::v1001::CylinderDataPayload payload;
                payload.radius_x     = bp::Vec2{value.mRadiusX.x, value.mRadiusX.y};
                payload.radius_z     = bp::Vec2{value.mRadiusZ.x, value.mRadiusZ.y};
                payload.height       = value.mHeight;
                payload.num_segments = value.mNumSegments;
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugPyramid>) {
                bp::v1001::PyramidDataPayload payload;
                payload.width  = value.mWidth;
                payload.depth  = value.mDepth;
                payload.height = value.mHeight;
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugEllipsoid>) {
                bp::v1001::EllipsoidDataPayload payload;
                payload.radii             = toVec3(value.mRadii);
                payload.segments_per_axis = value.mSegmentsPerAxis;
                out.extra_data_payload    = payload;
            } else if constexpr (std::is_same_v<T, DebugCone>) {
                bp::v1001::ConeDataPayload payload;
                payload.radii        = bp::Vec2{value.mRadii.x, value.mRadii.y};
                payload.height       = value.mHeight;
                payload.num_segments = value.mNumSegments;
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugArrow>) {
                bp::ArrowDataPayload payload;
                payload.end_location  = toVec3(value.mArrowEndLocation);
                out.extra_data_payload = payload;
            }
        },
        shape.mShape
    );
    return out;
}

} // namespace

std::string buildPrimitiveShapesBody(std::vector<DebugShape> const& shapes) {
    bp::v2193::PrimitiveShapesPacket packet;
    packet.shapes.reserve(shapes.size());
    for (auto const& shape : shapes) {
        packet.shapes.push_back(toPayload(shape));
    }

    std::string buffer;
    bp::BinaryWriter writer{buffer};
    bp::serialize(writer, packet);
    return buffer;
}

std::string buildPrimitiveShapesPacket(std::vector<DebugShape> const& shapes) {
    return codec::withHeader(kPrimitiveShapesPacketId, buildPrimitiveShapesBody(shapes));
}

} // namespace debugshape_export::core
