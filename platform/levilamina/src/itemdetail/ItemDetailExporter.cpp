#include "ItemDetailExporter.h"
#include "ItemDetailManager.h"

#include "lse/LseBridge.h"

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void ItemDetailExporter::exportAll() {
    auto& mgr = ItemDetailManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "itemDetailShow",
                               [&mgr](int dimId, float x, float y, float z,
                                      std::string const& itemId, int aux, int count,
                                      std::string const& customText) -> int64_t {
                                   return mgr.show(dimId, x, y, z, itemId, aux, count, customText);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDetailHide",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.hide(id);
                               });
}

} // namespace debugshape_export
