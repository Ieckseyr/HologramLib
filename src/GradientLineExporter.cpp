#include "GradientLineExporter.h"
#include "GradientLineManager.h"
#include "ModEntry.h"

#include "lse/LseBridge.h"

namespace debugshape_export {

static constexpr const char* NAMESPACE = "GradientLine";

void GradientLineExporter::exportAll() {
    auto& logger = ModEntry::getInstance().getSelf().getLogger();
    logger.info("Exporting GradientLine functions to LegacyRemoteCall...");
    
    auto& mgr = GradientLineManager::getInstance();
    
    // create(x1, y1, z1, x2, y2, z2, segments) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "create",
        [&mgr](float x1, float y1, float z1, 
               float x2, float y2, float z2,
               int segments) -> int64_t {
            return mgr.create(x1, y1, z1, x2, y2, z2, segments);
        });
    
    // setGradient(id, r1, g1, b1, r2, g2, b2) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "setGradient",
        [&mgr](int64_t id,
               float r1, float g1, float b1,
               float r2, float g2, float b2) -> bool {
            return mgr.setGradient(id, r1, g1, b1, r2, g2, b2);
        });
    
    // setRainbow(id, speed) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "setRainbow",
        [&mgr](int64_t id, float speed) -> bool {
            return mgr.setRainbow(id, speed);
        });
    
    // setColor(id, r, g, b, a) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "setColor",
        [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
            return mgr.setColor(id, r, g, b, a);
        });
    
    // setEndpoints(id, x1, y1, z1, x2, y2, z2) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "setEndpoints",
        [&mgr](int64_t id,
               float x1, float y1, float z1,
               float x2, float y2, float z2) -> bool {
            return mgr.setEndpoints(id, x1, y1, z1, x2, y2, z2);
        });
    
    // draw(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "draw",
        [&mgr](int64_t id) -> bool {
            return mgr.draw(id);
        });
    
    // drawToDimension(id, dimId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "drawToDimension",
        [&mgr](int64_t id, int dimId) -> bool {
            return mgr.drawToDimension(id, dimId);
        });
    
    // remove(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "remove",
        [&mgr](int64_t id) -> bool {
            return mgr.remove(id);
        });
    
    // destroy(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "destroy",
        [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });
    
    // destroyAll() -> void
    hologramlib::lse::exportAs(NAMESPACE, "destroyAll",
        [&mgr]() -> void {
            mgr.destroyAll();
        });
    
    // tick(deltaTime) -> void - 更新动画 (应在游戏tick中调用)
    hologramlib::lse::exportAs(NAMESPACE, "tick",
        [&mgr](float deltaTime) -> void {
            mgr.tick(deltaTime);
        });
    
    logger.info("GradientLine functions exported successfully.");
}

} // namespace debugshape_export
