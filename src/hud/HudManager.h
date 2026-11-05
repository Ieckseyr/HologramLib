#pragma once

#include "hologramlib/HologramLib.h"

#include <string>

namespace debugshape_export {

// HUD 显隐
class HudManager {
public:
    static HudManager& getInstance();

    bool setElement(std::string const& playerName, hologramlib::HudElement element, bool visible);

    int  setElements(std::string const& playerName, int mask, bool visible);
    bool resetAll(std::string const& playerName);

    std::string probeWire() const;

    bool setRaw(std::string const& playerName, int mask, int visibility);

private:
    HudManager() = default;
};

} // namespace debugshape_export
