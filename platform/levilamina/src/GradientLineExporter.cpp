#include "GradientLineExporter.h"

#include "DiagLog.h"
#include "GradientLineManager.h"
#include "ModEntry.h"

#include "lse/LseBridge.h"

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void GradientLineExporter::exportAll() {
    HLIB_LOG_INFO("Exporting GradientLine functions to LegacyRemoteCall...");

    auto& mgr = GradientLineManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "gradientCreate",
                               [&mgr](float x1, float y1, float z1,
                                      float x2, float y2, float z2,
                                      int segments) -> int64_t {
                                   return mgr.create(x1, y1, z1, x2, y2, z2, segments);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientSetGradient",
                               [&mgr](int64_t id,
                                      float r1, float g1, float b1,
                                      float r2, float g2, float b2) -> bool {
                                   return mgr.setGradient(id, r1, g1, b1, r2, g2, b2);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientSetRainbow",
                               [&mgr](int64_t id, float speed) -> bool {
                                   return mgr.setRainbow(id, speed);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientSetColor",
                               [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
                                   return mgr.setColor(id, r, g, b, a);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientSetEndpoints",
                               [&mgr](int64_t id,
                                      float x1, float y1, float z1,
                                      float x2, float y2, float z2) -> bool {
                                   return mgr.setEndpoints(id, x1, y1, z1, x2, y2, z2);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientDraw",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.draw(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientDrawToDimension",
                               [&mgr](int64_t id, int dimId) -> bool {
                                   return mgr.drawToDimension(id, dimId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientRemove",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.remove(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientDestroy",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.destroy(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientDestroyAll",
                               [&mgr]() -> void {
                                   mgr.destroyAll();
                               });

    hologramlib::lse::exportAs(NAMESPACE, "gradientTick",
                               [&mgr](float deltaTime) -> void {
                                   mgr.tick(deltaTime);
                               });

    HLIB_LOG_INFO("GradientLine functions exported successfully.");
}

} // namespace debugshape_export
