#pragma once

// 形状（PrimitiveShapes）的平台无关模型；编码见 core/ShapeWire.cpp。

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include "core/ShapeIdMap.h"
#include "core/proto/Version.h"

namespace debugshape_export {

namespace bp = ::bedrock::protocol;

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

struct Vec2 {
    float x{};
    float y{};
};

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct ProtoColor {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    ProtoColor() = default;
    ProtoColor(std::int32_t packed) // ARGB
        : r(static_cast<std::uint8_t>((packed >> 16) & 0xFF))
        , g(static_cast<std::uint8_t>((packed >> 8) & 0xFF))
        , b(static_cast<std::uint8_t>(packed & 0xFF))
        , a(static_cast<std::uint8_t>((packed >> 24) & 0xFF)) {}

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
            (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(r) << 16)
            | (static_cast<std::uint32_t>(g) << 8) | static_cast<std::uint32_t>(b)
        );
    }
};

enum class DebugShapeType {
    Text,
    Line,
    Box,
    Circle,
    Sphere,
    Arrow,
    Cylinder,
    Pyramid,
    Ellipsoid,
    Cone
};

using ProtoShapeType = DebugShapeType;

struct DebugText {
    std::string                     mText{};
    bool                            mUseRotation{};
    float                           mLineGapHeight{};
    bool                            mDepthTest{};
    bool                            mShowBackface{};
    bool                            mShowTextBackface{};
    std::optional<std::int32_t>     mBackgroundColor{};
};

struct DebugLine {
    Vec3 mLineEndLocation{};
};

struct DebugBox {
    Vec3 mBoxBound{};
};

struct DebugSphere {
    std::uint8_t mSegments{};
};

struct DebugCylinder {
    Vec2         mRadiusX{};
    Vec2         mRadiusZ{};
    float        mHeight{};
    std::uint8_t mNumSegments{};
};

struct DebugPyramid {
    float mWidth{};
    float mDepth{};
    float mHeight{};
};

struct DebugEllipsoid {
    Vec3         mRadii{};
    std::uint8_t mSegmentsPerAxis{};
};

struct DebugCone {
    Vec2         mRadii{};
    float        mHeight{};
    std::uint8_t mNumSegments{};
};

struct DebugArrow {
    Vec3 mArrowEndLocation{};
};

using DebugShapePayload = std::variant<
    std::monostate,
    DebugText,
    DebugLine,
    DebugBox,
    DebugSphere,
    DebugCylinder,
    DebugPyramid,
    DebugEllipsoid,
    DebugCone,
    DebugArrow>;

struct DebugShape {
    std::uint64_t              mNetworkId{};
    DebugShapeType             mType{DebugShapeType::Box};
    std::optional<Vec3>        mLocation{};
    std::optional<float>       mScale{};
    std::optional<Vec3>        mRotation{};
    std::optional<float>       mTotalTimeLeft{};
    std::optional<float>       mMaxRenderDistance{};
    std::optional<ProtoColor>  mColor{};
    std::optional<std::int32_t> mDimensionId{};
    std::optional<std::int64_t> mAttachedToEntityId{};
    DebugShapePayload          mShape{};
};

using ProtoShape        = DebugShape;
using ProtoVec3         = Vec3;
using ProtoTextPayload  = DebugText;
using ProtoBoxPayload   = DebugBox;
using ProtoLinePayload  = DebugLine;
using ProtoArrowPayload = DebugArrow;
using ProtoSpherePayload = DebugSphere;

using DebugShapeTypeAlias = DebugShapeType;
using DebugTextAlias      = DebugText;
using DebugBoxAlias       = DebugBox;
using DebugLineAlias      = DebugLine;
using DebugArrowAlias     = DebugArrow;

[[nodiscard]] inline DebugShapeType toProtoShapeType(LSEShapeType t) {
    switch (t) {
    case LSEShapeType::Text:
        return DebugShapeType::Text;
    case LSEShapeType::Line:
        return DebugShapeType::Line;
    case LSEShapeType::Box:
        return DebugShapeType::Box;
    case LSEShapeType::Circle:
        return DebugShapeType::Circle;
    case LSEShapeType::Sphere:
        return DebugShapeType::Sphere;
    case LSEShapeType::Arrow:
        return DebugShapeType::Arrow;
    case LSEShapeType::Cylinder:
        return DebugShapeType::Cylinder;
    case LSEShapeType::Pyramid:
        return DebugShapeType::Pyramid;
    case LSEShapeType::Ellipsoid:
        return DebugShapeType::Ellipsoid;
    case LSEShapeType::Cone:
        return DebugShapeType::Cone;
    }
    return DebugShapeType::Box;
}

inline ProtoShape makeRemoveShape(std::uint64_t networkId) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    return shape;
}

constexpr float kTextLineGapHeight = 0.0f;

inline ProtoShape makeTextShape(std::uint64_t networkId, Vec3 const& pos, std::string const& text) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Text;
    shape.mLocation  = pos;
    shape.mShape     = DebugText{.mText = text, .mLineGapHeight = kTextLineGapHeight};
    return shape;
}

inline ProtoShape makeLineShape(std::uint64_t networkId, Vec3 const& start, Vec3 const& end) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Line;
    shape.mLocation  = start;
    shape.mShape     = DebugLine{.mLineEndLocation = end};
    return shape;
}

inline ProtoShape makeBoxShape(std::uint64_t networkId, Vec3 const& center, Vec3 const& bound) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Box;
    shape.mLocation  = center;
    shape.mShape     = DebugBox{.mBoxBound = bound};
    return shape;
}

inline ProtoShape makeSphereShape(std::uint64_t networkId, Vec3 const& pos, float scale, std::uint8_t segments = 16) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Sphere;
    shape.mLocation  = pos;
    shape.mScale     = scale;
    shape.mShape     = DebugSphere{.mSegments = segments};
    return shape;
}

inline ProtoShape makeCircleShape(std::uint64_t networkId, Vec3 const& pos, float scale) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Circle;
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
    std::uint8_t  segments
) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Cylinder;
    shape.mLocation  = pos;
    shape.mShape     = DebugCylinder{
        .mRadiusX     = Vec2{radiusX, radiusX},
        .mRadiusZ     = Vec2{radiusZ, radiusZ},
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
    float         depth
) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Pyramid;
    shape.mLocation  = pos;
    shape.mShape     = DebugPyramid{.mWidth = width, .mDepth = depth, .mHeight = height};
    return shape;
}

inline ProtoShape makeEllipsoidShape(
    std::uint64_t networkId,
    Vec3 const&   pos,
    Vec3 const&   radii,
    std::uint8_t  segmentsPerAxis
) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Ellipsoid;
    shape.mLocation  = pos;
    shape.mShape     = DebugEllipsoid{.mRadii = radii, .mSegmentsPerAxis = segmentsPerAxis};
    return shape;
}

inline ProtoShape makeConeShape(
    std::uint64_t networkId,
    Vec3 const&   pos,
    float         radiusX,
    float         radiusZ,
    float         height,
    std::uint8_t  segments
) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Cone;
    shape.mLocation  = pos;
    shape.mShape = DebugCone{
        .mRadii       = Vec2{radiusX, radiusZ},
        .mHeight      = height,
        .mNumSegments = segments,
    };
    return shape;
}

inline ProtoShape makeArrowShape(std::uint64_t networkId, Vec3 const& start, Vec3 const& end) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Arrow;
    shape.mLocation  = start;
    shape.mShape     = DebugArrow{.mArrowEndLocation = end};
    return shape;
}

} // namespace debugshape_export
