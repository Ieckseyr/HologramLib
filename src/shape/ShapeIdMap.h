// ShapeIdMap.h —— 形状编号的双向映射（纯常量，不依赖协议库/BDS）
//
// 两套编号不是一回事：
//   · LSE 脚本侧（公开 API）：0..5 已固定，6 是历史空洞（FilledQuad 废弃后留空），
//     新增类型只能从 7 开始往后排；
//   · 协议层 PrimitiveShapesType：0..9 连续，其中 6..9 是 Cylinder/Pyramid/Ellipsoid/Cone。
//
// 所以 7..10（脚本）↔ 6..9（协议）。这个映射只有一处，ProtocolShape.h 与导出层都从它取，
// 避免两边各写一份。
#pragma once

#include <cstdint>

namespace debugshape_export::shape_id {

// LSE 脚本侧编号
inline constexpr int kLseText      = 0;
inline constexpr int kLseLine      = 1;
inline constexpr int kLseBox       = 2;
inline constexpr int kLseCircle    = 3;
inline constexpr int kLseSphere    = 4;
inline constexpr int kLseArrow     = 5;
inline constexpr int kLseHole      = 6; // 历史空洞：不要复用
inline constexpr int kLseCylinder  = 7;
inline constexpr int kLsePyramid   = 8;
inline constexpr int kLseEllipsoid = 9;
inline constexpr int kLseCone      = 10;

// 协议层编号（sculk::protocol::PrimitiveShapesType）
inline constexpr int kProtoLine      = 0;
inline constexpr int kProtoBox       = 1;
inline constexpr int kProtoSphere    = 2;
inline constexpr int kProtoCircle    = 3;
inline constexpr int kProtoText      = 4;
inline constexpr int kProtoArrow     = 5;
inline constexpr int kProtoCylinder  = 6;
inline constexpr int kProtoPyramid   = 7;
inline constexpr int kProtoEllipsoid = 8;
inline constexpr int kProtocone      = 9;

// 脚本编号 → 协议编号；未知编号返回 -1
[[nodiscard]] constexpr int toProtocol(int lseType) {
    switch (lseType) {
    case kLseText:
        return kProtoText;
    case kLseLine:
        return kProtoLine;
    case kLseBox:
        return kProtoBox;
    case kLseCircle:
        return kProtoCircle;
    case kLseSphere:
        return kProtoSphere;
    case kLseArrow:
        return kProtoArrow;
    case kLseCylinder:
        return kProtoCylinder;
    case kLsePyramid:
        return kProtoPyramid;
    case kLseEllipsoid:
        return kProtoEllipsoid;
    case kLseCone:
        return kProtocone;
    default:
        return -1; // 含 6（历史空洞）
    }
}

// 协议编号 → 脚本编号；未知编号返回 -1
[[nodiscard]] constexpr int toScript(int protocolType) {
    switch (protocolType) {
    case kProtoText:
        return kLseText;
    case kProtoLine:
        return kLseLine;
    case kProtoBox:
        return kLseBox;
    case kProtoCircle:
        return kLseCircle;
    case kProtoSphere:
        return kLseSphere;
    case kProtoArrow:
        return kLseArrow;
    case kProtoCylinder:
        return kLseCylinder;
    case kProtoPyramid:
        return kLsePyramid;
    case kProtoEllipsoid:
        return kLseEllipsoid;
    case kProtocone:
        return kLseCone;
    default:
        return -1;
    }
}

// 这个脚本编号是否是库支持的类型（6 是空洞，返回 false）
[[nodiscard]] constexpr bool isSupportedScriptType(int lseType) {
    return toProtocol(lseType) >= 0;
}

} // namespace debugshape_export::shape_id
