#pragma once

#include "hologramlib/HologramLib.h"

namespace debugshape_export {

class FormControlExporter {
public:
    static void exportAll();
};

class LocatorBarExporter {
public:
    static void exportAll();
};

class PlayerVisibilityExporter {
public:
    static void exportAll();
};

hologramlib::IFormControl&      formControlAdapter();
hologramlib::IPlayerVisibility& playerVisibilityAdapter();
hologramlib::ILocatorBar&       locatorBarAdapter();

} // namespace debugshape_export
