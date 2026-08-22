#include "GradientLineExporter.h"
#include "GradientLineManager.h"
#include "ModEntry.h"

#include "lse/LseBridge.h"

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void GradientLineExporter::exportAll() {
    auto& logger = ModEntry::getInstance().getSelf().getLogger();
    logger.info("Exporting GradientLine functions to LegacyRemoteCall...");
    
    auto& mgr = GradientLineManager::getInstance();
    
    // create(x1, y1, z1, x2, y2, z2, segments) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "gradientCreate",
        [&mgr](float x1, float y1, float z1, 
               float x2, float y2, float z2,
               int segments) -> int64_t {
            return mgr.create(x1, y1, z1, x2, y2, z2, segments);
        });
    
    // setGradient(id, r1, g1, b1, r2, g2, b2) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "gradientSetGradient",
        [&mgr](int64_t id,
               float r1, float g1, float b1,
               float r2, float g2, float b2) -> bool {
            return mgr.setGradient(id, r1, g1, b1, r2, g2, b2);
        });
    
    // setRainbow(id, speed) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "gradientSetRainbow",
        [&mgr](int64_t id, float speed) -> bool {
            return mgr.setRainbow(id, speed);
        });
    
    // setColor(id, r, g, b, a) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "gradientSetColor",
        [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
            return mgr.setColor(id, r, g, b, a);
        });
    
    // setEndpoints(id, x1, y1, z1, x2, y2, z2) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "gradientSetEndpoints",
        [&mgr](int64_t id,
               float x1, float y1, float z1,
               float x2, float y2, float z2) -> bool {
            return mgr.setEndpoints(id, x1, y1, z1, x2, y2, z2);
        });
    
    // draw(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "gradientDraw",
        [&mgr](int64_t id) -> bool {
            return mgr.draw(id);
        });
    
    // drawToDimension(id, dimId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "gradientDrawToDimension",
        [&mgr](int64_t id, int dimId) -> bool {
            return mgr.drawToDimension(id, dimId);
        });
    
    // remove(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "gradientRemove",
        [&mgr](int64_t id) -> bool {
            return mgr.remove(id);
        });
    
    // destroy(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "gradientDestroy",
        [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });
    
    // destroyAll() -> void
    hologramlib::lse::exportAs(NAMESPACE, "gradientDestroyAll",
        [&mgr]() -> void {
            mgr.destroyAll();
        });
    
    // tick(deltaTime) -> void - 更新动画 (应在游戏tick中调用)
    hologramlib::lse::exportAs(NAMESPACE, "gradientTick",
        [&mgr](float deltaTime) -> void {
            mgr.tick(deltaTime);
        });
    
    logger.info("GradientLine functions exported successfully.");
}

} // namespace debugshape_export
