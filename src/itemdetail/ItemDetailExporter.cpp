// ItemDetailExporter.cpp - 物品详情 LSE 导出实现
#include "ItemDetailExporter.h"
#include "ItemDetailManager.h"

#include "lse/LseBridge.h"

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void ItemDetailExporter::exportAll() {
    auto& mgr = ItemDetailManager::getInstance();

    // itemDetailShow(dimId, x, y, z, itemId, aux, count, customText) -> int64
    // customText 传空串 "" 表示使用自动文本（"本地化物品名 xN"）
    hologramlib::lse::exportAs(NAMESPACE, "itemDetailShow",
        [&mgr](int dimId, float x, float y, float z,
               std::string const& itemId, int aux, int count,
               std::string const& customText) -> int64_t {
            return mgr.show(dimId, x, y, z, itemId, aux, count, customText);
        });

    // itemDetailHide(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "itemDetailHide",
        [&mgr](int64_t id) -> bool {
            return mgr.hide(id);
        });
}

} // namespace debugshape_export
