// ProtocolShape.h - 基于 BedrockProtocol v2168 的形状数据结构与构造辅助
//
// 本头文件是对 sculk::protocol::abi_v2168::PrimitiveShapes 的薄封装，
// 提供与旧版 ProtoShape 兼容的接口，使上层 PacketDebugRenderer 无需大改。
// 实际序列化由 Protocol v2168 静态库的 PrimitiveShapes::write /
// PrimitiveShapesPacket::writeWithHeader 完成。
//
// v944 → v2168 迁移说明:
//   DebugDrawerPacket → PrimitiveShapesPacket（包 ID 328 不变）
//   DebugShape        → PrimitiveShapes
//   枚举值重排: v944 Text=0/Line=1/Box=2/Circle=3/Sphere=4/Arrow=5
//              v2168 Line=0/Box=1/Sphere=2/Circle=3/Text=4/Arrow=5
//   新增原生 Line 载荷（v944 用 Arrow 模拟线段, v2168 直接用 LineDataPayload）
//   新增 Sphere segments 载荷（v944 用 monostate）
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

#include "sculk/protocol/codec/level/PrimitiveShapes.hpp"
#include "sculk/protocol/codec/packet/PrimitiveShapesPacket.hpp"
#include "sculk/protocol/codec/math/Vec3.hpp"

namespace debugshape_export {

// 形状类型枚举（LSE 兼容，与 PacketDebugRenderer 配套使用）
// 注意: 数值是 LSE 脚本侧的稳定 API，与协议层 PrimitiveShapesType 的数值不同，
// 映射关系见下方 toProtoShapeType。
// 注: FilledQuad(6) 已移除（极薄 Box 模拟填充面方案废弃）; 6 保留空洞防止旧脚本误判
enum class LSEShapeType : int {
    Text = 0,
    Line = 1,
    Box = 2,
    Circle = 3,
    Sphere = 4,
    Arrow = 5
};

// 直接复用 v2168 命名空间下的类型（保留旧别名使上层代码无需改动）
using ProtoShape          = sculk::protocol::PrimitiveShapes;
using ProtoShapeType      = sculk::protocol::PrimitiveShapesType;
using ProtoVec3           = sculk::protocol::Vec3;
using ProtoTextPayload    = sculk::protocol::TextDataPayload;
using ProtoBoxPayload     = sculk::protocol::BoxDataPayload;
using ProtoLinePayload    = sculk::protocol::LineDataPayload;
using ProtoArrowPayload   = sculk::protocol::ArrowDataPayload;
using ProtoSpherePayload  = sculk::protocol::SphereDataPayload;

// v944 时代的历史别名（上层代码沿用）
using DebugShape          = ProtoShape;
using DebugShapeType      = ProtoShapeType;
using Vec3                = ProtoVec3;
using DebugText           = ProtoTextPayload;
using DebugBox            = ProtoBoxPayload;
using DebugLine           = ProtoLinePayload;
using DebugArrow          = ProtoArrowPayload;

// LSE 形状类型 → v2168 协议形状类型（数值重排后的显式映射）
[[nodiscard]] inline ProtoShapeType toProtoShapeType(LSEShapeType t) {
    switch (t) {
    case LSEShapeType::Text:   return ProtoShapeType::Text;
    case LSEShapeType::Line:   return ProtoShapeType::Line;
    case LSEShapeType::Box:    return ProtoShapeType::Box;
    case LSEShapeType::Circle: return ProtoShapeType::Circle;
    case LSEShapeType::Sphere: return ProtoShapeType::Sphere;
    case LSEShapeType::Arrow:  return ProtoShapeType::Arrow;
    }
    return ProtoShapeType::Box;
}

// 颜色辅助：v2168 的 mColor 是 std::optional<std::int32_t>，
// 按 ARGB 打包（A 在最高字节，B 在最低字节）。
// 旧实现 writeColor 写入字节顺序为 r, g, b, a，
// 对应 LE int32 = r | (g << 8) | (b << 16) | (a << 24)。
struct ProtoColor {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    ProtoColor() = default;
    ProtoColor(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_, std::uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    static ProtoColor fromFloat(float rf, float gf, float bf, float af = 1.0f) {
        return ProtoColor{
            static_cast<std::uint8_t>(std::clamp(rf * 255.0f, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(gf * 255.0f, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(bf * 255.0f, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(af * 255.0f, 0.0f, 255.0f))
        };
    }

    // 转换为 v2168 使用的 int32 打包颜色（保持与旧实现一致的字节顺序）
    [[nodiscard]] std::int32_t toPacked() const {
        return static_cast<std::int32_t>(
            static_cast<std::uint32_t>(r)
            | (static_cast<std::uint32_t>(g) << 8)
            | (static_cast<std::uint32_t>(b) << 16)
            | (static_cast<std::uint32_t>(a) << 24)
        );
    }
};

//================================================================
// 形状构造辅助函数（返回 v2168 PrimitiveShapes）
//================================================================

inline ProtoShape makeRemoveShape(std::uint64_t networkId) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    // 其余字段均为 nullopt / monostate，客户端收到后会移除该 networkId 的形状
    return shape;
}

inline ProtoShape makeTextShape(std::uint64_t networkId, Vec3 const& pos, std::string const& text) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Text;
    shape.mLocation  = pos;
    shape.mShape     = ProtoTextPayload{.mText = text};
    return shape;
}

// v2168 新增原生 Line 载荷（v944 时代用 Arrow 模拟, 现在直接画无箭头线段）
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
    // v2168 Sphere 载荷携带 segments（v944 时代为 monostate, 该参数此前被忽略）
    shape.mShape     = ProtoSpherePayload{.mSegments = segments};
    return shape;
}

inline ProtoShape makeCircleShape(std::uint64_t networkId, Vec3 const& pos, float scale) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = ProtoShapeType::Circle;
    shape.mLocation  = pos;
    shape.mScale     = scale;
    // Circle 无专属载荷（variant 中不存在 Circle 项），使用 monostate（variant 0）
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
