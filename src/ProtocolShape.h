

#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

#include "sculk/protocol/codec/level/PrimitiveShapes.hpp"
#include "sculk/protocol/codec/math/Vec3.hpp"
#include "sculk/protocol/codec/packet/PrimitiveShapesPacket.hpp"
#include "shape/ShapeIdMap.h"

namespace debugshape_export {

enum class LSEShapeType : int {
    Text   = shape_id::kLseText,
    Line   = shape_id::kLseLine,
    Box    = shape_id::kLseBox,
    Circle = shape_id::kLseCircle,
    Sphere = shape_id::kLseSphere,
    Arrow  = shape_id::kLseArrow,

    Cylinder  = shape_id::kLseCylinder,
    Pyramid   = shape_id::kLsePyramid,
    Ellipsoid = shape_id::kLseEllipsoid,
    Cone      = shape_id::kLseCone
};

using ProtoShape         = sculk::protocol::PrimitiveShapes;
using ProtoShapeType     = sculk::protocol::PrimitiveShapesType;
using ProtoVec3          = sculk::protocol::Vec3;
using ProtoTextPayload   = sculk::protocol::TextDataPayload;
using ProtoBoxPayload    = sculk::protocol::BoxDataPayload;
using ProtoLinePayload   = sculk::protocol::LineDataPayload;
using ProtoArrowPayload  = sculk::protocol::ArrowDataPayload;
using ProtoSpherePayload = sculk::protocol::SphereDataPayload;

using DebugShape     = ProtoShape;
using DebugShapeType = ProtoShapeType;
using Vec3           = ProtoVec3;
using DebugText      = ProtoTextPayload;
using DebugBox       = ProtoBoxPayload;
using DebugLine      = ProtoLinePayload;
using DebugArrow     = ProtoArrowPayload;

[[nodiscard]] inline ProtoShapeType toProtoShapeType(LSEShapeType t) {
    switch (t) {
    case LSEShapeType::Text:
        return ProtoShapeType::Text;
    case LSEShapeType::Line:
        return ProtoShapeType::Line;
    case LSEShapeType::Box:
        return ProtoShapeType::Box;
    case LSEShapeType::Circle:
        return ProtoShapeType::Circle;
    case LSEShapeType::Sphere:
        return ProtoShapeType::Sphere;
    case LSEShapeType::Arrow:
        return ProtoShapeType::Arrow;
    case LSEShapeType::Cylinder:
        return ProtoShapeType::Cylinder;
    case LSEShapeType::Pyramid:
        return ProtoShapeType::Pyramid;
    case LSEShapeType::Ellipsoid:
        return ProtoShapeType::Ellipsoid;
    case LSEShapeType::Cone:
        return ProtoShapeType::Cone;
    }
    return ProtoShapeType::Box;
}

struct ProtoColor {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    ProtoColor() = default;
    ProtoColor(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_, std::uint8_t a_ = 255)
        : r(r_)
        , g(g_)
        , b(b_)
        , a(a_) {}

    static ProtoColor fromFloat(float rf, float gf, float bf, float af = 1.0f) {
        return ProtoColor{
            static_cast<std::uint8_t>(std::clamp(rf * 255.0f, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(gf * 255.0f, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(bf * 255.0f, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(af * 255.0f, 0.0f, 255.0f))};
    }

    [[nodiscard]] std::int32_t toPacked() const {
        return static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(r) << 16) | (static_cast<std::uint32_t>(g) << 8) | static_cast<std::uint32_t>(b));
    }
};

inline ProtoShape makeRemoveShape(std::uint64_t networkId) {
    ProtoShape shape;
    shape.mNetworkId = networkId;

    return shape;
}

constexpr float kTextLineGapHeight = 0.0f;

inline ProtoShape makeTextShape(std::uint64_t networkId, Vec3 const& pos, std::string const& text) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Text;
    shape.mLocation  = pos;
    shape.mShape     = ProtoTextPayload{
        .mText          = text,
        .mLineGapHeight = kTextLineGapHeight,
    };
    return shape;
}

inline ProtoShape makeLineShape(std::uint64_t networkId, Vec3 const& start, Vec3 const& end) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Line;
    shape.mLocation  = start;
    shape.mShape     = ProtoLinePayload{.mLineEndLocation = end};
    return shape;
}

inline ProtoShape makeBoxShape(std::uint64_t networkId, Vec3 const& center, Vec3 const& bound) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Box;
    shape.mLocation  = center;
    shape.mShape     = ProtoBoxPayload{.mBoxBound = bound};
    return shape;
}

inline ProtoShape makeSphereShape(std::uint64_t networkId, Vec3 const& pos, float scale, std::uint8_t segments = 16) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Sphere;
    shape.mLocation  = pos;
    shape.mScale     = scale;

    shape.mShape = ProtoSpherePayload{.mSegments = segments};
    return shape;
}

inline ProtoShape makeCircleShape(std::uint64_t networkId, Vec3 const& pos, float scale) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Circle;
    shape.mLocation  = pos;
    shape.mScale     = scale;

    return shape;
}

inline ProtoShape makeCylinderShape(
    std::uint64_t networkId,
    Vec3 const&   pos,
    float         radiusX,
    float         radiusZ,
    float         height,
    std::uint8_t  segments) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Cylinder;
    shape.mLocation  = pos;
    shape.mShape     = sculk::protocol::CylinderDataPayload{
        .mRadiusX     = sculk::protocol::Vec2{radiusX, radiusX},
        .mRadiusZ     = sculk::protocol::Vec2{radiusZ, radiusZ},
        .mHeight      = height,
        .mNumSegments = segments,
    };
    return shape;
}

inline ProtoShape makePyramidShape(
    std::uint64_t networkId,
    Vec3 const&   pos,
    float         width,
    float         height,
    float         depth) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Pyramid;
    shape.mLocation  = pos;
    shape.mShape     = sculk::protocol::PyramidDataPayload{
        .mWidth  = width,
        .mDepth  = depth,
        .mHeight = height,
    };
    return shape;
}

inline ProtoShape makeEllipsoidShape(
    std::uint64_t networkId,
    Vec3 const&   pos,
    Vec3 const&   radii,
    std::uint8_t  segmentsPerAxis) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Ellipsoid;
    shape.mLocation  = pos;
    shape.mShape     = sculk::protocol::EllipsoidDataPayload{
        .mRadii           = radii,
        .mSegmentsPerAxis = segmentsPerAxis,
    };
    return shape;
}

inline ProtoShape makeConeShape(
    std::uint64_t networkId,
    Vec3 const&   pos,
    float         radiusX,
    float         radiusZ,
    float         height,
    std::uint8_t  segments) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Cone;
    shape.mLocation  = pos;
    shape.mShape     = sculk::protocol::ConeDataPayload{
        .mRadii       = sculk::protocol::Vec2{radiusX, radiusZ},
        .mHeight      = height,
        .mNumSegments = segments,
    };
    return shape;
}

inline ProtoShape makeArrowShape(std::uint64_t networkId, Vec3 const& start, Vec3 const& end) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Arrow;
    shape.mLocation  = start;
    shape.mShape     = ProtoArrowPayload{.mArrowEndLocation = end};
    return shape;
}

} // namespace debugshape_export
