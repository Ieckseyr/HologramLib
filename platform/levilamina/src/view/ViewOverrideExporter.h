#pragma once

#include "hologramlib/HologramLib.h"

#include <string>

namespace debugshape_export {

class ViewOverrideExporter {
public:
    static void exportAll();
};

[[nodiscard]] hologramlib::EntityView parseEntitySpec(std::string const& spec);

} // namespace debugshape_export
