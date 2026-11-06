#pragma once

#include <cstdint>

namespace debugshape_export::shape_id {

inline constexpr int kLseText      = 0;
inline constexpr int kLseLine      = 1;
inline constexpr int kLseBox       = 2;
inline constexpr int kLseCircle    = 3;
inline constexpr int kLseSphere    = 4;
inline constexpr int kLseArrow     = 5;
inline constexpr int kLseHole      = 6;
inline constexpr int kLseCylinder  = 7;
inline constexpr int kLsePyramid   = 8;
inline constexpr int kLseEllipsoid = 9;
inline constexpr int kLseCone      = 10;

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
        return -1;
    }
}

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

[[nodiscard]] constexpr bool isSupportedScriptType(int lseType) {
    return toProtocol(lseType) >= 0;
}

} // namespace debugshape_export::shape_id
