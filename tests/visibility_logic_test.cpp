// visibility_logic_test.cpp —— 可见性判定的语义契约（纯标准库，CI 用 g++ 直接跑）
//
// 背景：判定收敛到 core::decideVisibility，迟滞带 / 禁用 / 换维度 / 白名单的行为一改就红。
#include "core/VisibilityLogic.h"

#include <cstdio>
#include <string>
#include <unordered_set>

namespace {

int g_pass = 0;
int g_fail = 0;

void check(bool cond, char const* what) {
    if (cond) {
        ++g_pass;
        std::printf("  PASS  %s\n", what);
    } else {
        ++g_fail;
        std::printf("  FAIL  %s\n", what);
    }
}

using debugshape_export::core::decideVisibility;
using debugshape_export::core::SpawnBudget;
using debugshape_export::core::VisibilityDecision;
using debugshape_export::core::VisibilityRule;

VisibilityRule baseRule(double viewDistance, double hysteresis) {
    VisibilityRule rule;
    rule.enabled      = true;
    rule.dimension    = 0;
    rule.x            = 0.0f;
    rule.y            = 64.0f;
    rule.z            = 0.0f;
    rule.viewDistance = viewDistance;
    rule.hysteresis   = hysteresis;
    return rule;
}

void testDistanceBand() {
    std::printf("[1] 视距与迟滞带（R=16, +4）\n");
    auto const  rule = baseRule(16.0, 4.0);
    std::string name = "Steve";

    check(decideVisibility(rule, 0, 0.0f, 64.0f, 0.0f, name, false) == VisibilityDecision::Spawn,
          "距离 0, 未展示 -> 生成");
    check(decideVisibility(rule, 0, 0.0f, 64.0f, 0.0f, name, true) == VisibilityDecision::None,
          "距离 0, 已展示 -> 保持");
    check(decideVisibility(rule, 0, 15.9f, 64.0f, 0.0f, name, false) == VisibilityDecision::Spawn,
          "距离 15.9 <= R, 未展示 -> 生成");
    check(decideVisibility(rule, 0, 16.0f, 64.0f, 0.0f, name, true) == VisibilityDecision::None,
          "距离恰为 R, 已展示 -> 保持");
    check(decideVisibility(rule, 0, 18.0f, 64.0f, 0.0f, name, true) == VisibilityDecision::None,
          "迟滞带内 18, 已展示 -> 保持（不移除）");
    check(decideVisibility(rule, 0, 19.9f, 64.0f, 0.0f, name, true) == VisibilityDecision::None,
          "迟滞带内 19.9, 已展示 -> 保持");
    check(decideVisibility(rule, 0, 20.1f, 64.0f, 0.0f, name, true) == VisibilityDecision::Remove,
          "超出 R+4, 已展示 -> 移除");
    check(decideVisibility(rule, 0, 18.0f, 64.0f, 0.0f, name, false) == VisibilityDecision::None,
          "迟滞带内, 未展示 -> 不生成也不移除");
    check(decideVisibility(rule, 0, 100.0f, 64.0f, 0.0f, name, false) == VisibilityDecision::None,
          "远处, 未展示 -> 什么都不做");
    check(decideVisibility(rule, 0, 0.0f, 200.0f, 0.0f, name, true) == VisibilityDecision::Remove,
          "Y 轴 136 格, 已展示 -> 移除（三维距离）");
}

void testNoHysteresis() {
    std::printf("[2] hysteresis == 0：出视距立刻移除\n");
    auto const  rule = baseRule(16.0, 0.0);
    std::string name = "Steve";

    check(decideVisibility(rule, 0, 15.9f, 64.0f, 0.0f, name, true) == VisibilityDecision::None,
          "距离 15.9, 已展示 -> 保持");
    check(decideVisibility(rule, 0, 16.1f, 64.0f, 0.0f, name, true) == VisibilityDecision::Remove,
          "距离 16.1, 已展示 -> 立刻移除");
    check(decideVisibility(rule, 0, 16.1f, 64.0f, 0.0f, name, false) == VisibilityDecision::None,
          "距离 16.1, 未展示 -> 不生成");
}

void testGatesBypassHysteresis() {
    std::printf("[3] 禁用 / 换维度 / 白名单不享受迟滞\n");
    auto        rule = baseRule(16.0, 4.0);
    std::string name = "Steve";

    rule.enabled = false;
    check(decideVisibility(rule, 0, 0.0f, 64.0f, 0.0f, name, true) == VisibilityDecision::Remove,
          "禁用, 距离 0 且在视距内 -> 立刻移除");
    check(decideVisibility(rule, 0, 0.0f, 64.0f, 0.0f, name, false) == VisibilityDecision::None,
          "禁用, 未展示 -> 什么都不做");
    rule.enabled = true;

    check(decideVisibility(rule, 1, 0.0f, 64.0f, 0.0f, name, true) == VisibilityDecision::Remove,
          "换到其它维度 -> 立刻移除（迟滞带内也一样）");
    check(decideVisibility(rule, 1, 0.0f, 64.0f, 0.0f, name, false) == VisibilityDecision::None,
          "其它维度, 未展示 -> 不生成");

    std::unordered_set<std::string> allow{"Alex"};
    rule.allowList = &allow;
    check(decideVisibility(rule, 0, 0.0f, 64.0f, 0.0f, name, true) == VisibilityDecision::Remove,
          "白名单外, 距离 0 -> 立刻移除");
    check(decideVisibility(rule, 0, 0.0f, 64.0f, 0.0f, std::string{"Alex"}, false) == VisibilityDecision::Spawn,
          "白名单内, 距离 0 -> 生成");
}

void testUnlimited() {
    std::printf("[4] viewDistance <= 0：同维度全可见\n");
    auto const  rule = baseRule(0.0, 4.0);
    std::string name = "Steve";

    check(decideVisibility(rule, 0, 10000.0f, 64.0f, 0.0f, name, false) == VisibilityDecision::Spawn,
          "无限视距, 远处未展示 -> 生成");
    check(decideVisibility(rule, 0, 10000.0f, 64.0f, 0.0f, name, true) == VisibilityDecision::None,
          "无限视距, 已展示 -> 保持");
    check(decideVisibility(rule, 1, 0.0f, 64.0f, 0.0f, name, true) == VisibilityDecision::Remove,
          "无限视距也只限同维度 -> 换维度仍移除");
}

void testBudget() {
    std::printf("[5] 生成预算\n");
    SpawnBudget budget{2};

    check(budget.available(), "预算 2 -> 可用");
    budget.consume();
    budget.consume();
    check(!budget.available(), "连用两次 -> 用尽");
    budget.consume();
    check(budget.remaining == 0, "用尽后继续消费不会变负数");

    SpawnBudget zero{0};
    check(!zero.available(), "预算 0 -> 不可用");
}

} // namespace

int main() {
    testDistanceBand();
    testNoHysteresis();
    testGatesBypassHysteresis();
    testUnlimited();
    testBudget();

    std::printf("\n结果: PASS %d / FAIL %d\n", g_pass, g_fail);
    if (g_fail != 0) {
        std::printf("VISIBILITY-LOGIC-TEST: 失败\n");
        return 1;
    }
    std::printf("VISIBILITY-LOGIC-TEST: 全部通过\n");
    return 0;
}
