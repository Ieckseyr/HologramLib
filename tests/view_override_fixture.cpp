// view_override_fixture.cpp - 客户端视图覆盖的纯逻辑离线自检
//
// 测的是**库内同一份** src/view/ViewOverrideLogic.h（不是重写一遍的副本）:
//   1. packPos / unpackX|Y|Z        —— 坐标打包（x 26 位 / z 26 位 / y 12 位, 各自补码）
//   2. inChunk / dimensionMatches    —— 区块归属与维度过滤
//   3. needsRewrite                  —— 幂等判断（服务端发的就是覆盖方块时不再动）
//   4. metaPlanOf                    —— 名字牌两项的改与不改
//   5. normaliseType                 —— 类型名清洗（短名补命名空间）
//   6. 包 id 常量                    —— 与引擎枚举对齐（管理器里有 static_assert 兜底）
//
// 这个头不依赖 BDS 类型, 所以本 fixture 只吃 src / include 目录, 不需要 Protocol.lib。
// 用法: view_override_fixture（失败时返回非 0）
#include "view/ViewOverrideLogic.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace debugshape_export::view::logic;

namespace {

int gChecks = 0;
int gFailed = 0;

void check(bool ok, std::string const& what) {
    ++gChecks;
    if (!ok) {
        ++gFailed;
        std::printf("FAIL  %s\n", what.c_str());
    } else {
        std::printf("ok    %s\n", what.c_str());
    }
}

// 打包 → 解包必须回到原值（含负数与 26 位边界）
void checkRoundTrip(int x, int y, int z) {
    auto const key = packPos(x, y, z);
    check(
        unpackX(key) == x && unpackY(key) == y && unpackZ(key) == z,
        "round-trip (" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z) + ")"
    );
}

void checkPositions() {
    std::printf("-- 坐标打包 --\n");
    checkRoundTrip(0, 0, 0);
    checkRoundTrip(1, 2, 3);
    checkRoundTrip(-1, -1, -1);
    checkRoundTrip(-1, 0, -1);
    checkRoundTrip(16, 64, -16);
    checkRoundTrip(-64, 0, 320);
    checkRoundTrip(-30000000, -64, 29999999);
    checkRoundTrip(29999999, 320, -30000000);
    checkRoundTrip(-2048, -64, 2047);
    checkRoundTrip(1234567, 255, -7654321);

    // 同坐标同键（覆盖表靠它做一次比对）
    check(packPos(10, 20, 30) == packPos(10, 20, 30), "same pos -> same key");
    check(packPos(10, 20, 30) != packPos(10, 20, 31), "different pos -> different key");
    // 三个轴不互相串（打包信息位不重叠）
    check(packPos(1, 0, 0) != packPos(0, 1, 0), "x/y not aliased");
    check(packPos(1, 0, 0) != packPos(0, 0, 1), "x/z not aliased");
    check(packPos(0, 1, 0) != packPos(0, 0, 1), "y/z not aliased");
}

void checkChunk() {
    std::printf("-- 区块归属 --\n");
    check(inChunk(packPos(0, 64, 0), 0, 0), "(0,64,0) in chunk (0,0)");
    check(inChunk(packPos(15, 64, 15), 0, 0), "(15,64,15) in chunk (0,0)");
    check(!inChunk(packPos(16, 64, 15), 0, 0), "(16,64,15) not in chunk (0,0)");
    check(inChunk(packPos(16, 64, 15), 1, 0), "(16,64,15) in chunk (1,0)");
    check(inChunk(packPos(-1, 0, -1), -1, -1), "(-1,0,-1) in chunk (-1,-1)");
    check(inChunk(packPos(-16, 0, -16), -1, -1), "(-16,0,-16) in chunk (-1,-1)");
    check(inChunk(packPos(-17, 0, -17), -2, -2), "(-17,0,-17) in chunk (-2,-2)");
    // y 不参与区块归属
    check(inChunk(packPos(-16, -64, -16), -1, -1), "y ignored for chunk");

    check(dimensionMatches(kAnyDimension, 0), "any-dim matches overworld");
    check(dimensionMatches(kAnyDimension, 1), "any-dim matches nether");
    check(dimensionMatches(1, 1), "dim 1 matches dim 1");
    check(!dimensionMatches(1, 0), "dim 1 does not match dim 0");
}

void checkIdempotence() {
    std::printf("-- 幂等判断 --\n");
    // 服务端发的是真实方块（≠ 覆盖）→ 要改, 且这次的真实值可以记下来
    check(needsRewrite(42u, 7u), "rewrite when packet carries real block");
    check(shouldRecordReal(42u, 7u), "record real id when it differs");
    // 服务端发的就是覆盖方块 → 不用动（同一条包被两条发包路径各套用一次也不会重复改）
    check(!needsRewrite(7u, 7u), "no rewrite when already overridden");
    check(!shouldRecordReal(7u, 7u), "never mistake override id for the real one");
    check(!needsRewrite(0u, 0u), "air stays air");
}

void checkMetaPlan() {
    std::printf("-- 名字牌计划 --\n");
    auto const untouched = metaPlanOf(false, "ignored", true);
    check(untouched.empty(), "hasNametag=false -> nothing to do");
    check(!untouched.setName && !untouched.setAlwaysShow, "untouched plan sets nothing");

    auto const named = metaPlanOf(true, std::string("\xc2\xa7" "c" "cow"), true);
    check(named.setName, "hasNametag=true -> set Name");
    check(named.name.size() > 3, "nametag text carried through");
    check(named.setAlwaysShow && named.alwaysShow, "always-show follows the flag");

    auto const hiddenName = metaPlanOf(true, std::string{}, false);
    check(hiddenName.setName && hiddenName.name.empty(), "empty string = clear the nametag");
    check(hiddenName.setAlwaysShow && !hiddenName.alwaysShow, "always-show can be turned off");
    check(!hiddenName.empty(), "clearing a nametag is still work to do");
}

void checkTypeNames() {
    std::printf("-- 类型名清洗 --\n");
    check(normaliseType("cow") == "minecraft:cow", "short name gets the vanilla namespace");
    check(normaliseType("  cow  ") == "minecraft:cow", "surrounding whitespace trimmed");
    check(normaliseType("minecraft:stone") == "minecraft:stone", "namespaced name untouched");
    check(normaliseType("my_pack:thing") == "my_pack:thing", "custom namespace untouched");
    check(normaliseType("") == "", "empty means do not change");
    check(normaliseType("   ") == "", "whitespace-only means do not change");
    check(normaliseType("\tdiamond_block\n") == "minecraft:diamond_block", "tabs and newlines trimmed");
}

void checkPacketIds() {
    std::printf("-- 包 id 常量 --\n");
    // 这几个值必须与 mc/network/MinecraftPacketIds.h 一致（管理器里有 static_assert,
    // 这里再钉一遍, 免得改常量时只改一处）
    check(kAddPlayer == 12, "AddPlayer = 12");
    check(kAddActor == 13, "AddActor = 13");
    check(kRemoveActor == 14, "RemoveActor = 14");
    // 掉落物的出生包: 引擎对 item 实体发的是 AddItemActor(id 15), 不是 AddActor
    check(kAddItemActor == 15, "AddItemActor = 15");
    check(kUpdateBlock == 21, "UpdateBlock = 21");
    check(kSetActorData == 39, "SetActorData = 39");
    check(kLevelChunk == 58, "FullChunkData = 58");
    check(kUpdateBlockSynced == 110, "UpdateBlockSynced = 110");
}

void checkItemSpawnPlan() {
    std::printf("-- 掉落物出生包处理计划 --\n");
    // 1.28.0: 掉落物的出生包(AddItemActor)也纳入拦截。隐藏优先（丢包）, 其次换类型（丢原包 +
    // 同 id 手写一只 AddActor）, 两样都没有才放行（例如只改名字牌的覆盖）。
    check(planItemSpawn(true, true) == ItemSpawnPlan::Drop, "hidden + identifier -> drop");
    check(planItemSpawn(true, false) == ItemSpawnPlan::Drop, "hidden -> drop");
    check(planItemSpawn(false, true) == ItemSpawnPlan::Rewrite, "identifier -> rewrite");
    check(planItemSpawn(false, false) == ItemSpawnPlan::Pass, "neither -> pass (nametag-only override)");
}

} // namespace

int main() {
    std::printf("view-override offline checks\n");
    checkPositions();
    checkChunk();
    checkIdempotence();
    checkMetaPlan();
    checkTypeNames();
    checkPacketIds();
    checkItemSpawnPlan();
    std::printf("\n%d checks, %d failed\n", gChecks, gFailed);
    return gFailed == 0 ? 0 : 1;
}
