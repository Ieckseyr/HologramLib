#pragma once

#include <hologramlib/HologramLib.h>

namespace debugshape_export {

class ContainerMenuExporter {
public:
    static void exportAll();
};

hologramlib::IContainerPreview& containerPreviewAdapter();

} // namespace debugshape_export
