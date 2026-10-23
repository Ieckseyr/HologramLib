// click_event_test.cpp —— 点击回传行的格式 / 解析契约（纯标准库，CI 用 g++ 直接跑）
//
// 背景：26.51.6 的 container 域漏了回填 Menu::menuId，点击回传的 menuId 恒为 0，
// 按 id 路由的调用方全部失效。这类"回传字段"的契约以前没有任何离线检查，
// 这里把版式钉住，格式或字段顺序一改就红。
#include "core/ClickEvent.h"

#include <cstdio>
#include <string>

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

void checkEq(std::string const& actual, std::string const& expect, char const* what) {
    if (actual == expect) {
        ++g_pass;
        std::printf("  PASS  %s\n", what);
    } else {
        ++g_fail;
        std::printf("  FAIL  %s\n    期望 [%s]\n    实际 [%s]\n", what, expect.c_str(), actual.c_str());
    }
}

using debugshape_export::core::ClickEventLine;
using debugshape_export::core::formatClickLine;
using debugshape_export::core::parseClickLine;

void testFormat() {
    std::printf("[1] 既有版式不能变（脚本按 player=/menuId=/slot=/closed= 抓）\n");
    ClickEventLine ev;
    ev.player = "Steve";
    ev.menuId = 3;
    ev.slot   = 4;
    ev.closed = false;
    checkEq(formatClickLine(ev), "player=Steve menuId=3 slot=4 closed=0", "普通点击");

    ev.closed = true;
    ev.slot   = -1;
    checkEq(formatClickLine(ev), "player=Steve menuId=3 slot=-1 closed=1", "关闭事件（slot=-1）");

    ev.player = "玩家_01";
    ev.menuId = 1234567;
    ev.slot   = 53;
    checkEq(formatClickLine(ev), "player=玩家_01 menuId=1234567 slot=53 closed=1", "非 ASCII 名 + 大 id + 末槽位");
}

void testRoundTrip() {
    std::printf("[2] 格式化 → 解析 往返一致\n");
    char const* cases[] = {
        "player=Steve menuId=1 slot=0 closed=0",
        "player=Alex menuId=-1 slot=26 closed=0",
        "player=名字带空格不走这条路 menuId=0 slot=-1 closed=1",
        "player=P menuId=9223372036854775807 slot=4095 closed=0",
    };
    for (auto const* text : cases) {
        ClickEventLine parsed;
        bool const     ok = parseClickLine(text, parsed);
        std::string    name(&text[7], 0);
        check(ok, "能解析");
        if (ok)
            checkEq(formatClickLine(parsed), text, "往返后逐字节一致");
    }
}

void testReject() {
    std::printf("[3] 版式不对必须拒绝（不能猜）\n");
    ClickEventLine out;
    check(!parseClickLine("", out), "空串");
    check(!parseClickLine("player=", out), "只有前缀");
    check(!parseClickLine("player=Steve", out), "缺 menuId");
    check(!parseClickLine("player=Steve menuId=x slot=1 closed=0", out), "menuId 不是数");
    check(!parseClickLine("player=Steve menuId=1 slot=1 closed=2", out), "closed 只能是 0/1");
    check(!parseClickLine("player= menuId=1 slot=1 closed=0", out), "空玩家名");
    check(!parseClickLine("player=Steve menuId=1 slot=-5 closed=0", out), "槽位越界（负数 < -1）");
    check(!parseClickLine("player=Steve menuId=1 slot=1 closed=0 尾巴", out), "尾部多余内容");
    check(!parseClickLine("Player=Steve menuId=1 slot=1 closed=0", out), "前缀大小写不同");
}

void testKeyContract() {
    std::printf("[4] 契约：menuId 必须等于打开时拿到的 id（不能是 0）\n");
    // 这条对应 26.51.6 的 bug：库忘了回填 Menu::menuId，回传里恒为 0。
    ClickEventLine ev;
    ev.player           = "Steve";
    ev.menuId           = 7; // 打开容器时 containerOpen 返回的值
    ev.slot             = 4;
    auto const     line = formatClickLine(ev);
    ClickEventLine parsed;
    check(parseClickLine(line, parsed), "回传行可解析");
    check(parsed.menuId == 7, "回传的 menuId 与打开时的 id 相等");
    check(parsed.menuId != 0, "回传的 menuId 不是 0（未回填的特征）");
}

} // namespace

int main() {
    std::printf("=== 点击回传行契约 ===\n");
    testFormat();
    testRoundTrip();
    testReject();
    testKeyContract();
    std::printf("\n结果: PASS %d / FAIL %d\n", g_pass, g_fail);
    if (g_fail != 0) {
        std::printf("CLICK-EVENT-TEST: 有失败\n");
        return 1;
    }
    std::printf("CLICK-EVENT-TEST: 全部通过\n");
    return 0;
}
