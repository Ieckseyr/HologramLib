// ProtocolShape.h - 基于 Protocol v944 的形状数据结构与构造辅助
//
// 本头文件是对 sculk::protocol::abi_v944::DebugShape 的薄封装，
// 提供与旧版 ProtoShape 兼容的接口，使上层 PacketDebugRenderer 无需大改。
// 实际序列化由 Protocol v944 静态库的 DebugShape::write / DebugDrawerPacket::writeWithHeader 完成。
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

#include "sculk/protocol/codec/level/DebugShape.hpp"
#include "sculk/protocol/codec/packet/DebugDrawerPacket.hpp"
#include "sculk/protocol/codec/utility/math/Vec3.hpp"

namespace debugshape_export {

// 形状类型枚举（LSE 兼容，与 PacketDebugRenderer 配套使用）
// 注: FilledQuad(6) 已移除（极薄 Box 模拟填充面方案废弃）; 6 保留空洞防止旧脚本误判
enum class LSEShapeType : int {
    Text = 0,
    Line = 1,
    Box = 2,
    Circle = 3,
    Sphere = 4,
    Arrow = 5
};

// 直接复用 v944 命名空间下的类型
using DebugShape     = sculk::protocol::abi_v944::DebugShape;
using DebugShapeType = sculk::protocol::abi_v944::DebugShapeType;
using Vec3           = sculk::protocol::abi_v944::Vec3;
using DebugText      = sculk::protocol::abi_v944::DebugText;
using DebugBox       = sculk::protocol::abi_v944::DebugBox;
using DebugLine      = sculk::protocol::abi_v944::DebugLine;
using DebugArrow     = sculk::protocol::abi_v944::DebugArrow;
using DebugSegments  = sculk::protocol::abi_v944::DebugSegments;
using DebugShapeVariant = sculk::protocol::abi_v944::DebugShapeVariant;

// 兼容别名：上层代码使用 Proto 前缀
using ProtoShape       = DebugShape;
using ProtoShapeType   = DebugShapeType;
using ProtoVec3        = Vec3;
using ProtoTextPayload = DebugText;
using ProtoBoxPayload  = DebugBox;
using ProtoLinePayload = DebugLine;
using ProtoArrowPayload = DebugArrow;

// ProtoVec3 兼容构造（Vec3 使用 mX/mY/mZ，aggregate init 已支持 {x, y, z}）
// 直接使用 Vec3{x, y, z} 即可。

// 颜色辅助：v944 的 mColor 是 std::optional<std::int32_t>，
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

    // 转换为 v944 使用的 int32 打包颜色（保持与旧实现一致的字节顺序）
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
// 形状构造辅助函数（返回 v944 DebugShape）
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
    shape.mType      = DebugShapeType::Text;
    shape.mLocation  = pos;
    shape.mShape     = DebugText{.mText = text};
    return shape;
}

// MC 没有 Line 类型，使用 Arrow 类型实现（与 v944 DebugShape 行为一致）
inline ProtoShape makeLineShape(std::uint64_t networkId, Vec3 const& start, Vec3 const& end) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Arrow;
    shape.mLocation  = start;
    DebugArrow arrow;
    arrow.mArrowEndLocation = end;
    shape.mShape = std::move(arrow);
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

inline ProtoShape makeSphereShape(std::uint64_t networkId, Vec3 const& pos, float scale, std::uint8_t /*segments*/ = 16) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Sphere;
    shape.mLocation  = pos;
    shape.mScale     = scale;
    // Sphere 使用 monostate（variant 0），与 v944 一致
    return shape;
}

inline ProtoShape makeCircleShape(std::uint64_t networkId, Vec3 const& pos, float scale) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Circle;
    shape.mLocation  = pos;
    shape.mScale     = scale;
    // Circle 使用 monostate（variant 0）
    return shape;
}

inline ProtoShape makeArrowShape(std::uint64_t networkId, Vec3 const& start, Vec3 const& end) {
    ProtoShape shape;
    shape.mNetworkId = networkId;
    shape.mType      = DebugShapeType::Arrow;
    shape.mLocation  = start;
    DebugArrow arrow;
    arrow.mArrowEndLocation = end;
    shape.mShape = std::move(arrow);
    return shape;
}

} // namespace debugshape_export
