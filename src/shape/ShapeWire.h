// ShapeWire.h - 形状包的手写线格式（26.51）
//
// 为什么不直接用协议库的 PrimitiveShapesPacket::write：
//   协议库的 CylinderDataPayload 是**两个** Vec2（mRadiusX / mRadiusZ），而 26.51 的
//   BDS/客户端只有一个 Vec2（LeviLamina 头: CylinderDataPayload{mRadii; mHeight; mNumSegments}）。
//   多写 8 个字节 → 整个 PrimitiveShapesPacket 被客户端判成错位 → 一个形状都不画。
//   其余 9 种载荷与协议库一致，这里按同样的字段顺序手写，只把圆柱改成单个 Vec2。
//
// 字段顺序与可选标记都照协议库的写法（旧 6 种形状已在实机上验证可用），
// 唯一的有意差异就是圆柱载荷。
#pragma once

#include "ProtocolShape.h"
#include "core/PacketSink.h"

#include <mc/deps/core/utility/BinaryStream.h>

#include <cstdint>
#include <string>
#include <vector>

namespace debugshape_export::shapewire {

namespace detail {

inline void writeVec3(::BinaryStream& s, ProtoVec3 const& v) {
    s.writeFloat(v.mX, nullptr, nullptr);
    s.writeFloat(v.mY, nullptr, nullptr);
    s.writeFloat(v.mZ, nullptr, nullptr);
}

inline void writeOptVec3(::BinaryStream& s, std::optional<ProtoVec3> const& v) {
    s.writeBool(v.has_value(), nullptr, nullptr);
    if (v)
        writeVec3(s, *v);
}

inline void writeOptFloat(::BinaryStream& s, std::optional<float> const& v) {
    s.writeBool(v.has_value(), nullptr, nullptr);
    if (v)
        s.writeFloat(*v, nullptr, nullptr);
}

inline void writeOptByte(::BinaryStream& s, std::optional<int> const& v) {
    s.writeBool(v.has_value(), nullptr, nullptr);
    if (v)
        s.writeByte(static_cast<uchar>(*v), nullptr, nullptr);
}

inline void writeOptInt32(::BinaryStream& s, std::optional<std::int32_t> const& v) {
    s.writeBool(v.has_value(), nullptr, nullptr);
    if (v)
        s.writeSignedInt(*v, nullptr, nullptr);
}

inline void writeOptVarInt(::BinaryStream& s, std::optional<std::int32_t> const& v) {
    s.writeBool(v.has_value(), nullptr, nullptr);
    if (v)
        s.writeVarInt(*v, nullptr, nullptr);
}

inline void writeOptVarInt64(::BinaryStream& s, std::optional<std::int64_t> const& v) {
    s.writeBool(v.has_value(), nullptr, nullptr);
    if (v)
        s.writeVarInt64(*v, nullptr, nullptr);
}

// 变体下标（与协议库 PrimitiveShapeDataVariant 的次序一致）
constexpr std::uint32_t kIdxNull      = 0;
constexpr std::uint32_t kIdxArrow     = 1;
constexpr std::uint32_t kIdxText      = 2;
constexpr std::uint32_t kIdxBox       = 3;
constexpr std::uint32_t kIdxLine      = 4;
constexpr std::uint32_t kIdxSphere    = 5;
constexpr std::uint32_t kIdxCylinder  = 6;
constexpr std::uint32_t kIdxPyramid   = 7;
constexpr std::uint32_t kIdxEllipsoid = 8;
constexpr std::uint32_t kIdxCone      = 9;

inline void writePayload(::BinaryStream& s, ProtoShape const& shape) {
    using namespace sculk::protocol;
    std::visit(
        [&](auto const& payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                s.writeUnsignedVarInt(kIdxNull, nullptr, nullptr);
            } else if constexpr (std::is_same_v<T, ArrowDataPayload>) {
                s.writeUnsignedVarInt(kIdxArrow, nullptr, nullptr);
                writeOptVec3(s, payload.mArrowEndLocation);
                writeOptFloat(s, payload.mArrowHeadLength);
                writeOptFloat(s, payload.mArrowHeadRadius);
                writeOptByte(s, payload.mArrowSegments);
            } else if constexpr (std::is_same_v<T, TextDataPayload>) {
                s.writeUnsignedVarInt(kIdxText, nullptr, nullptr);
                s.writeString(payload.mText, nullptr, nullptr);
                s.writeBool(payload.mUseRotation, nullptr, nullptr);
                writeOptInt32(s, payload.mBackgroundColor);
                s.writeFloat(payload.mLineGapHeight, nullptr, nullptr);
                s.writeBool(payload.mDepthTest, nullptr, nullptr);
                s.writeBool(payload.mShowBackface, nullptr, nullptr);
                s.writeBool(payload.mShowTextBackface, nullptr, nullptr);
            } else if constexpr (std::is_same_v<T, BoxDataPayload>) {
                s.writeUnsignedVarInt(kIdxBox, nullptr, nullptr);
                writeVec3(s, payload.mBoxBound);
            } else if constexpr (std::is_same_v<T, LineDataPayload>) {
                s.writeUnsignedVarInt(kIdxLine, nullptr, nullptr);
                writeVec3(s, payload.mLineEndLocation);
            } else if constexpr (std::is_same_v<T, SphereDataPayload>) {
                s.writeUnsignedVarInt(kIdxSphere, nullptr, nullptr);
                s.writeByte(payload.mSegments, nullptr, nullptr);
            } else if constexpr (std::is_same_v<T, CylinderDataPayload>) {
                // 26.51: 只有**一个** Vec2 半径（协议库写成两个，会多 8 字节）
                s.writeUnsignedVarInt(kIdxCylinder, nullptr, nullptr);
                s.writeFloat(payload.mRadiusX.mX, nullptr, nullptr);
                s.writeFloat(payload.mRadiusZ.mX, nullptr, nullptr);
                s.writeFloat(payload.mHeight, nullptr, nullptr);
                s.writeByte(payload.mNumSegments, nullptr, nullptr);
            } else if constexpr (std::is_same_v<T, PyramidDataPayload>) {
                s.writeUnsignedVarInt(kIdxPyramid, nullptr, nullptr);
                s.writeFloat(payload.mWidth, nullptr, nullptr);
                writeOptFloat(s, payload.mDepth);
                s.writeFloat(payload.mHeight, nullptr, nullptr);
            } else if constexpr (std::is_same_v<T, EllipsoidDataPayload>) {
                s.writeUnsignedVarInt(kIdxEllipsoid, nullptr, nullptr);
                writeVec3(s, payload.mRadii);
                s.writeByte(payload.mSegmentsPerAxis, nullptr, nullptr);
            } else if constexpr (std::is_same_v<T, ConeDataPayload>) {
                s.writeUnsignedVarInt(kIdxCone, nullptr, nullptr);
                s.writeFloat(payload.mRadii.mX, nullptr, nullptr);
                s.writeFloat(payload.mRadii.mY, nullptr, nullptr);
                s.writeFloat(payload.mHeight, nullptr, nullptr);
                s.writeByte(payload.mNumSegments, nullptr, nullptr);
            }
        },
        shape.mShape);
}

} // namespace detail

// 写一个形状条目
inline void writeShape(::BinaryStream& s, ProtoShape const& shape) {
    s.writeUnsignedVarInt64(shape.mNetworkId, nullptr, nullptr);

    s.writeBool(shape.mType.has_value(), nullptr, nullptr);
    if (shape.mType)
        s.writeByte(static_cast<uchar>(static_cast<std::uint8_t>(*shape.mType)), nullptr, nullptr);

    detail::writeOptVec3(s, shape.mLocation);
    detail::writeOptFloat(s, shape.mScale);
    detail::writeOptVec3(s, shape.mRotation);
    detail::writeOptFloat(s, shape.mTotalTimeLeft);
    detail::writeOptFloat(s, shape.mMaxRenderDistance);
    detail::writeOptInt32(s, shape.mColor);
    detail::writeOptVarInt(s, shape.mDimensionId);
    detail::writeOptVarInt64(s, shape.mAttachedToEntityId);

    detail::writePayload(s, shape);
}

// 形状列表包（含包头，可直接交给 wire::sendRaw）
inline std::string buildPacket(std::vector<ProtoShape> const& shapes) {
    ::BinaryStream body;
    body.writeUnsignedVarInt(static_cast<uint>(shapes.size()), nullptr, nullptr);
    for (auto const& shape : shapes)
        writeShape(body, shape);

    ::BinaryStream out;
    out.writeUnsignedVarInt((328 & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12), nullptr, nullptr);
    out.mBuffer.append(body.mBuffer.data(), body.mBuffer.size());
    return out.mBuffer;
}

} // namespace debugshape_export::shapewire
