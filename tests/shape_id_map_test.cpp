// tests/shape_id_map_test.cpp —— 形状编号映射（不依赖协议库/BDS）
//
//   g++ -std=c++20 -I src tests/shape_id_map_test.cpp -o shape_id_map_test && ./shape_id_map_test
//
// 盯的是两套编号不许混：脚本 0..5 固定、6 是空洞、新类型 7..10 对应协议 6..9。
#include "core/ShapeIdMap.h"

#include <cstdio>
#include <string>

namespace ids = debugshape_export::shape_id;

static int gPass = 0;
static int gFail = 0;

static void check(bool cond, std::string const& what) {
    if (cond) {
        ++gPass;
    } else {
        ++gFail;
        std::printf("  FAIL  %s\n", what.c_str());
    }
}

int main() {
    // 1. 原有 6 种：两套编号的对应关系是交叉的（脚本 Text=0 / 协议 Text=4）
    check(ids::toProtocol(0) == 4, "脚本 Text(0) → 协议 Text(4)");
    check(ids::toProtocol(1) == 0, "脚本 Line(1) → 协议 Line(0)");
    check(ids::toProtocol(2) == 1, "脚本 Box(2) → 协议 Box(1)");
    check(ids::toProtocol(3) == 3, "脚本 Circle(3) → 协议 Circle(3)");
    check(ids::toProtocol(4) == 2, "脚本 Sphere(4) → 协议 Sphere(2)");
    check(ids::toProtocol(5) == 5, "脚本 Arrow(5) → 协议 Arrow(5)");

    // 2. 新增 4 种：脚本 7..10 → 协议 6..9
    check(ids::toProtocol(7) == 6, "脚本 Cylinder(7) → 协议 6");
    check(ids::toProtocol(8) == 7, "脚本 Pyramid(8) → 协议 7");
    check(ids::toProtocol(9) == 8, "脚本 Ellipsoid(9) → 协议 8");
    check(ids::toProtocol(10) == 9, "脚本 Cone(10) → 协议 9");

    // 3. 6 是历史空洞，不能当有效类型
    check(ids::toProtocol(6) == -1, "脚本 6 是空洞 → -1");
    check(!ids::isSupportedScriptType(6), "isSupportedScriptType(6) = false");
    check(ids::isSupportedScriptType(0) && ids::isSupportedScriptType(5), "0..5 受支持");
    check(ids::isSupportedScriptType(7) && ids::isSupportedScriptType(10), "7..10 受支持");
    check(!ids::isSupportedScriptType(11), "11 越界 → 不受支持");
    check(!ids::isSupportedScriptType(-1), "-1 不受支持");

    // 4. 反向映射自洽
    for (int lse = 0; lse <= 10; ++lse) {
        int const proto = ids::toProtocol(lse);
        if (proto < 0)
            continue;
        check(ids::toScript(proto) == lse, "往返一致: 脚本 " + std::to_string(lse));
    }

    // 5. 协议侧 10 种类型都能映回脚本编号
    for (int proto = 0; proto <= 9; ++proto) {
        check(ids::toScript(proto) >= 0, "协议 " + std::to_string(proto) + " 有对应脚本编号");
    }
    check(ids::toScript(10) == -1, "协议 10 越界 → -1");

    std::printf("\nSHAPE-ID-MAP-TEST: %d PASS / %d FAIL\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
