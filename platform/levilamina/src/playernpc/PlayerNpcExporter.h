#pragma once

#include "hologramlib/HologramLib.h"

namespace debugshape_export {

class PlayerNpcExporter {
public:
    static void exportAll();
};

hologramlib::IPlayerNpc& playerNpcAdapter();

} // namespace debugshape_export
