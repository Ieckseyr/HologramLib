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
        return false;

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
