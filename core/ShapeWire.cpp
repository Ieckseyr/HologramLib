#include "core/ShapeWire.h"

#include "core/proto/Version.h"

#include <type_traits>
#include <variant>

namespace debugshape_export::core {

namespace bp = ::bedrock::protocol;

namespace {

constexpr int kV = proto::kProtocolVersion;

bp::Vec3 toVec3(Vec3 const& v) {
    return bp::Vec3{v.x, v.y, v.z};
}

template <int V>
using KindOf = bp::ScriptPrimitiveShapeType_<V>;

template <int V>
using PayloadOf = bp::PrimitiveShapeDataPayload_<V>;

template <int V>
using TextOf = bp::TextDataPayload_<V>;

// 898 的枚举只有 Line/Box/Sphere/Circle/Text/Arrow 六种，圆柱那四种是 1001 才加的；
// 新版本没有的形状在这里降级成盒子（同时保证 shape_type 与 payload 一致）。
template <int V>
constexpr KindOf<V> toKind(DebugShapeType type) {
    using K = KindOf<V>;
    switch (type) {
    case DebugShapeType::Text:
        return K::Text;
    case DebugShapeType::Line:
        return K::Line;
    case DebugShapeType::Box:
        return K::Box;
    case DebugShapeType::Circle:
        return K::Circle;
    case DebugShapeType::Sphere:
        return K::Sphere;
    case DebugShapeType::Arrow:
        return K::Arrow;
    default:
        break;
    }
    if constexpr (V >= 1001) {
        switch (type) {
        case DebugShapeType::Cylinder:
            return K::Cylinder;
        case DebugShapeType::Pyramid:
            return K::Pyramid;
        case DebugShapeType::Ellipsoid:
            return K::Ellipsoid;
        case DebugShapeType::Cone:
            return K::Cone;
        default:
            break;
        }
    }
    return K::Box;
}

template <int V>
PayloadOf<V> toPayload(DebugShape const& shape) {
    PayloadOf<V> out;
    out.network_id = shape.mNetworkId;
    if (shape.mType) {
        out.shape_type = toKind<V>(*shape.mType);
    }
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
    if constexpr (requires(PayloadOf<V>& p) { p.max_render_distance; }) {
        if (shape.mMaxRenderDistance) {
            out.max_render_distance = *shape.mMaxRenderDistance;
        }
    }
    if (shape.mColor) {
        out.color = bp::Color{shape.mColor->toPacked()};
    }
    if (shape.mDimensionId) {
        out.dimension_id = bp::DimensionType{*shape.mDimensionId};
    }
    if constexpr (requires(PayloadOf<V>& p) { p.attached_to_id; }) {
        if (shape.mAttachedToEntityId) {
            out.attached_to_id = bp::ActorUniqueID{*shape.mAttachedToEntityId};
        }
    }

    std::visit(
        [&](auto const& value) {
            using T = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return;
            } else if constexpr (std::is_same_v<T, DebugText>) {
                TextOf<V> payload;
                payload.text = value.mText;
                if constexpr (requires(TextOf<V>& p) { p.line_gap_height; }) {
                    payload.line_gap_height = value.mLineGapHeight;
                }
                if constexpr (requires(TextOf<V>& p) { p.use_rotation; }) {
                    payload.use_rotation = value.mUseRotation;
                    payload.depth_test   = value.mDepthTest;
                }
                if constexpr (requires(TextOf<V>& p) { p.background_color; }) {
                    if (value.mBackgroundColor) {
                        payload.background_color = bp::Color{*value.mBackgroundColor};
                    }
                }
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugLine>) {
                bp::LineDataPayload payload;
                payload.end_location   = toVec3(value.mLineEndLocation);
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugBox>) {
                bp::BoxDataPayload payload;
                payload.box_bound      = toVec3(value.mBoxBound);
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugSphere>) {
                bp::SphereDataPayload payload;
                payload.num_segments   = value.mSegments;
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugArrow>) {
                bp::ArrowDataPayload payload;
                payload.end_location   = toVec3(value.mArrowEndLocation);
                out.extra_data_payload = payload;
            } else if constexpr (std::is_same_v<T, DebugCylinder>) {
                if constexpr (V >= 1001) {
                    bp::v1001::CylinderDataPayload payload;
                    payload.radius_x     = bp::Vec2{value.mRadiusX.x, value.mRadiusX.y};
                    payload.radius_z     = bp::Vec2{value.mRadiusZ.x, value.mRadiusZ.y};
                    payload.height       = value.mHeight;
                    payload.num_segments = value.mNumSegments;
                    out.extra_data_payload = payload;
                } else {
                    bp::BoxDataPayload payload;
                    payload.box_bound      = bp::Vec3{value.mRadiusX.x, value.mHeight, value.mRadiusZ.x};
                    out.extra_data_payload = payload;
                }
            } else if constexpr (std::is_same_v<T, DebugPyramid>) {
                if constexpr (V >= 1001) {
                    bp::v1001::PyramidDataPayload payload;
                    payload.width          = value.mWidth;
                    payload.depth          = value.mDepth;
                    payload.height         = value.mHeight;
                    out.extra_data_payload = payload;
                } else {
                    bp::BoxDataPayload payload;
                    payload.box_bound      = bp::Vec3{value.mWidth, value.mHeight, value.mDepth};
                    out.extra_data_payload = payload;
                }
            } else if constexpr (std::is_same_v<T, DebugEllipsoid>) {
                if constexpr (V >= 1001) {
                    bp::v1001::EllipsoidDataPayload payload;
                    payload.radii             = toVec3(value.mRadii);
                    payload.segments_per_axis = value.mSegmentsPerAxis;
                    out.extra_data_payload    = payload;
                } else {
                    bp::BoxDataPayload payload;
                    payload.box_bound      = toVec3(value.mRadii);
                    out.extra_data_payload = payload;
                }
            } else if constexpr (std::is_same_v<T, DebugCone>) {
                if constexpr (V >= 1001) {
                    bp::v1001::ConeDataPayload payload;
                    payload.radii          = bp::Vec2{value.mRadii.x, value.mRadii.y};
                    payload.height         = value.mHeight;
                    payload.num_segments   = value.mNumSegments;
                    out.extra_data_payload = payload;
                } else {
                    bp::BoxDataPayload payload;
                    payload.box_bound      = bp::Vec3{value.mRadii.x, value.mHeight, value.mRadii.y};
                    out.extra_data_payload = payload;
                }
            }
        },
        shape.mShape
    );
    return out;
}

} // namespace

std::string buildPrimitiveShapesBody(std::vector<DebugShape> const& shapes) {
    proto::PrimitiveShapesPacket packet;
    packet.shapes.reserve(shapes.size());
    for (auto const& shape : shapes) {
        packet.shapes.push_back(toPayload<kV>(shape));
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
