// ItemDisplayExporter.cpp - FMBE 物品悬浮显示 LSE 导出实现
#include "ItemDisplayExporter.h"
#include "ItemDisplayManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void ItemDisplayExporter::exportAll() {
    auto& mgr = ItemDisplayManager::getInstance();

    // 创建/生命周期
    // itemDisplayCreate(x, y, z, dim, item, aux) -> int64（id; <0 失败）
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayCreate",
        [&mgr](float x, float y, float z, int dim, std::string const& item, int aux) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            return mgr.create(cfg);
        });

    // itemDisplayCreateAdvanced(x, y, z, dim, item, aux, offsetX, offsetY, offsetZ, rotX, rotY, rotZ, scale) -> int64
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateAdvanced",
        [&mgr](float               x,
               float               y,
               float               z,
               int                 dim,
               std::string const&  item,
               int                 aux,
               std::string const&  offX,
               std::string const&  offY,
               std::string const&  offZ,
               std::string const&  rotX,
               std::string const&  rotY,
               std::string const&  rotZ,
               std::string const&  scale) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            cfg.offsetX   = offX;
            cfg.offsetY   = offY;
            cfg.offsetZ   = offZ;
            cfg.rotX      = rotX;
            cfg.rotY      = rotY;
            cfg.rotZ      = rotZ;
            cfg.scale     = scale;
            return mgr.create(cfg);
        });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayDestroy",
        [&mgr](int64_t id) -> bool { return mgr.destroy(id); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayDestroyAll",
        [&mgr]() -> void { mgr.destroyAll(); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayExists",
        [&mgr](int64_t id) -> bool { return mgr.exists(id); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayGetAllIds",
        [&mgr]() -> std::vector<int64_t> { return mgr.getAllIds(); });

    // 属性
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetItem",
        [&mgr](int64_t id, std::string const& item, int aux) -> bool {
            return mgr.setItem(id, item, aux);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetPosition",
        [&mgr](int64_t id, float x, float y, float z, int dim) -> bool {
            return mgr.setPosition(id, x, y, z, dim);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetOffset",
        [&mgr](int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) -> bool {
            return mgr.setOffset(id, ox, oy, oz);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetBaseOffset",
        [&mgr](int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) -> bool {
            return mgr.setBaseOffset(id, ox, oy, oz);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetRotation",
        [&mgr](int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) -> bool {
            return mgr.setRotation(id, rx, ry, rz);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetScale",
        [&mgr](int64_t id, std::string const& scale) -> bool { return mgr.setScale(id, scale); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetExtend",
        [&mgr](int64_t id, std::string const& scale, std::string const& rx, std::string const& ry) -> bool {
            return mgr.setExtend(id, scale, rx, ry);
        });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetMode",
        [&mgr](int64_t id, int mode) -> bool { return mgr.setMode(id, mode); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetEnabled",
        [&mgr](int64_t id, bool enabled) -> bool { return mgr.setEnabled(id, enabled); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetViewDistance",
        [&mgr](int64_t id, float dist) -> bool { return mgr.setViewDistance(id, dist); });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayRotateY",
        [&mgr](int64_t id, float delta) -> bool { return mgr.rotateY(id, delta); });
}

} // namespace debugshape_export
