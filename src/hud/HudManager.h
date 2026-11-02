// HudManager.h - HUD 元素显隐（1.34.0）
//
// 协议: SetHudPacket(308)。字段: HudElements（元素列表，重复 uvarint32）+ HudVisibility（0=Hide / 1=Reset）。
//
// 语义按协议: "Hide" 是隐藏，"Reset" 是恢复默认（协议没有单独的 "Show"）。所以 setElement(visible=true)
// 发 Reset。服务端不记状态，纯客户端表现。
#pragma once

#include "hologramlib/HologramLib.h" // HudElement

#include <string>

namespace debugshape_export {

class HudManager {
public:
    static HudManager& getInstance();

    bool setElement(std::string const& playerName, hologramlib::HudElement element, bool visible);
    // 位掩码: bit0=PaperDoll … bit12=ItemText；返回实际生效的个数
    int  setElements(std::string const& playerName, int mask, bool visible);
    bool resetAll(std::string const& playerName);

private:
    HudManager() = default;
};

} // namespace debugshape_export
