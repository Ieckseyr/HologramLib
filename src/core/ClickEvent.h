// ClickEvent.h - 点击回传行的格式与解析（内核层，只依赖标准库）
//
// 版式固定为 `player=<名字> menuId=<id> slot=<槽位> closed=<0|1>`：
//   · C++ 侧 formatClick() 出这个版式（LSE 的 containerPollClicks / npcDialogPollClicks 原样透传）
//   · 消费方（脚本、外部工具）按这个版式解析
// 两边共用一份实现，格式改动只需要动这里；tests/click_event_test.cpp 对拍往返。
#pragma once

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

namespace debugshape_export {
namespace core {

struct ClickEventLine {
    std::string  player;
    std::int64_t menuId{0};
    int          slot{-1};
    bool         closed{false};
};

/// 序列化成一行：`player=Steve menuId=3 slot=4 closed=0`
inline std::string formatClickLine(ClickEventLine const& event) {
    std::string out;
    out.reserve(64);
    out += "player=";
    out += event.player;
    out += " menuId=";
    out += std::to_string(event.menuId);
    out += " slot=";
    out += std::to_string(event.slot);
    out += " closed=";
    out += event.closed ? '1' : '0';
    return out;
}

namespace detail {

inline bool readInt(std::string_view text, std::int64_t& out) {
    if (text.empty())
        return false;
    auto const* first = text.data();
    auto const* last  = text.data() + text.size();
    auto        res   = std::from_chars(first, last, out);
    return res.ec == std::errc{} && res.ptr == last;
}

} // namespace detail

/// 解析一行。成功返回 true 并写 out；版式不对返回 false（out 不改）。
/// 玩家名可以是任意不含空格的字符串（基岩版名字就是这规则）。
inline bool parseClickLine(std::string_view line, ClickEventLine& out) {
    constexpr std::string_view kPlayer = "player=";
    constexpr std::string_view kMenu   = " menuId=";
    constexpr std::string_view kSlot   = " slot=";
    constexpr std::string_view kClosed = " closed=";

    if (line.rfind(kPlayer, 0) != 0)
        return false;
    auto const menuPos = line.find(kMenu);
    if (menuPos == std::string_view::npos)
        return false;
    auto const slotPos = line.find(kSlot, menuPos + kMenu.size());
    if (slotPos == std::string_view::npos)
        return false;
    auto const closedPos = line.find(kClosed, slotPos + kSlot.size());
    if (closedPos == std::string_view::npos)
        return false;

    ClickEventLine parsed;
    parsed.player = std::string{line.substr(kPlayer.size(), menuPos - kPlayer.size())};
    if (parsed.player.empty())
        return false;

    std::int64_t menuId = 0;
    if (!detail::readInt(line.substr(menuPos + kMenu.size(), slotPos - menuPos - kMenu.size()), menuId)) {
        return false;
    }
    std::int64_t slot = 0;
    if (!detail::readInt(line.substr(slotPos + kSlot.size(), closedPos - slotPos - kSlot.size()), slot)) {
        return false;
    }
    if (slot < -1 || slot > 4096)
        return false; // 槽位只在 [-1, 4096) 有意义（-1 = 关闭事件）

    auto const closedText = line.substr(closedPos + kClosed.size());
    if (closedText != "0" && closedText != "1")
        return false;

    parsed.menuId = menuId;
    parsed.slot   = static_cast<int>(slot);
    parsed.closed = closedText == "1";
    out           = std::move(parsed);
    return true;
}

} // namespace core
} // namespace debugshape_export
