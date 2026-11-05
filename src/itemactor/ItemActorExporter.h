#pragma once

#include "hologramlib/HologramLib.h"

namespace debugshape_export {

class ItemActorExporter {
public:
    static void exportAll();
};

hologramlib::IItemActor& itemActorAdapter();

} // namespace debugshape_export
