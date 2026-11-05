#include "RemoteCallExporter.h"

#include "DiagLog.h"
#include "ModEntry.h"
#include "PacketDebugRenderer.h"

#include "lse/LseBridge.h"
#include <fmt/format.h>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void RemoteCallExporter::exportAll() {
    HLIB_LOG_INFO("Exporting DebugShape functions to LegacyRemoteCall (Protocol Layer)...");

    fmt::print("[DebugShape] Starting exportAll...\n");

    exportCreateFunctions();
    exportPropertyFunctions();
    exportDisplayFunctions();
    exportLifecycleFunctions();

    fmt::print("[DebugShape] All functions exported successfully.\n");

    HLIB_LOG_INFO("All functions exported successfully.");
}

void RemoteCallExporter::exportCreateFunctions() {
    auto& mgr = PacketDebugRenderer::getInstance();

    fmt::print("[DebugShape] Exporting createText function...\n");

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateText",
                               [&mgr](float x, float y, float z, std::string const& text) -> int64_t {
                                   return mgr.createText(x, y, z, text);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateLine",
                               [&mgr](float x1, float y1, float z1, float x2, float y2, float z2) -> int64_t {
                                   return mgr.createLine(x1, y1, z1, x2, y2, z2);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateBox",
                               [&mgr](float x1, float y1, float z1, float x2, float y2, float z2) -> int64_t {
                                   return mgr.createBox(x1, y1, z1, x2, y2, z2);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateCircle",
                               [&mgr](float x, float y, float z, float scale) -> int64_t {
                                   return mgr.createCircle(x, y, z, scale);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateSphere",
                               [&mgr](float x, float y, float z, float scale) -> int64_t {
                                   return mgr.createSphere(x, y, z, scale);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateArrow",
                               [&mgr](float x1, float y1, float z1, float x2, float y2, float z2) -> int64_t {
                                   return mgr.createArrow(x1, y1, z1, x2, y2, z2);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateCylinder",
                               [&mgr](float x, float y, float z, float radiusX, float radiusZ, float height, int segments) -> int64_t {
                                   return mgr.createCylinder(x, y, z, radiusX, radiusZ, height, segments);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreatePyramid",
                               [&mgr](float x, float y, float z, float width, float height, float depth) -> int64_t {
                                   return mgr.createPyramid(x, y, z, width, height, depth);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateEllipsoid",
                               [&mgr](float x, float y, float z, float radiusX, float radiusY, float radiusZ, int segmentsPerAxis) -> int64_t {
                                   return mgr.createEllipsoid(x, y, z, radiusX, radiusY, radiusZ, segmentsPerAxis);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateCone",
                               [&mgr](float x, float y, float z, float radiusX, float radiusZ, float height, int segments) -> int64_t {
                                   return mgr.createCone(x, y, z, radiusX, radiusZ, height, segments);
                               });
}

void RemoteCallExporter::exportPropertyFunctions() {
    auto& mgr = PacketDebugRenderer::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetText",
                               [&mgr](int64_t id, std::string const& text) -> bool {
                                   return mgr.setText(id, text);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeGetText",
                               [&mgr](int64_t id) -> std::string {
                                   return mgr.getText(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetLocation",
                               [&mgr](int64_t id, float x, float y, float z) -> bool {
                                   return mgr.setLocation(id, x, y, z);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeGetLocation",
                               [&mgr](int64_t id) -> std::vector<float> {
                                   return mgr.getLocation(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetColor",
                               [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
                                   return mgr.setColor(id, r, g, b, a);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeGetColor",
                               [&mgr](int64_t id) -> std::vector<float> {
                                   return mgr.getColor(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetScale",
                               [&mgr](int64_t id, float scale) -> bool {
                                   return mgr.setScale(id, scale);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetMaxRenderDistance",
                               [&mgr](int64_t id, float distance) -> bool {
                                   return mgr.setMaxRenderDistance(id, distance);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeAttachEntity",
                               [&mgr](int64_t id, int64_t entityUniqueId) -> bool {
                                   return mgr.setAttachedEntity(id, entityUniqueId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeClearAttachedEntity",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearAttachedEntity(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetBackgroundColor",
                               [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
                                   return mgr.setBackgroundColor(id, r, g, b, a);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeClearBackgroundColor",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearBackgroundColor(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetDepthTest",
                               [&mgr](int64_t id, bool enabled) -> bool {
                                   return mgr.setDepthTest(id, enabled);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetDuration",
                               [&mgr](int64_t id, float seconds) -> bool {
                                   return mgr.setDuration(id, seconds);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetDimension",
                               [&mgr](int64_t id, int dimId) -> bool {
                                   return mgr.setDimension(id, dimId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeSetRotation",
                               [&mgr](int64_t id, float pitch, float yaw, float roll) -> bool {
                                   return mgr.setRotation(id, pitch, yaw, roll);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeClearRotation",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearRotation(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeGetRotation",
                               [&mgr](int64_t id) -> std::vector<float> {
                                   return mgr.getRotation(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeGetShapeType",
                               [&mgr](int64_t id) -> int {
                                   return mgr.getShapeType(id);
                               });
}

void RemoteCallExporter::exportDisplayFunctions() {
    auto& mgr = PacketDebugRenderer::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "shapeDraw",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.draw(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeDrawToPlayer",
                               [&mgr](int64_t id, std::string const& playerName) -> bool {
                                   return mgr.drawToPlayer(id, playerName);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeDrawToDimension",
                               [&mgr](int64_t id, int dimId) -> bool {
                                   return mgr.drawToDimension(id, dimId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeRemove",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.remove(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeRemoveToPlayer",
                               [&mgr](int64_t id, std::string const& playerName) -> bool {
                                   return mgr.removeToPlayer(id, playerName);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeRemoveToDimension",
                               [&mgr](int64_t id, int dimId) -> bool {
                                   return mgr.removeToDimension(id, dimId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeUpdate",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.update(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeUpdateToPlayer",
                               [&mgr](int64_t id, std::string const& playerName) -> bool {
                                   return mgr.updateToPlayer(id, playerName);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeUpdateToDimension",
                               [&mgr](int64_t id, int dimId) -> bool {
                                   return mgr.updateToDimension(id, dimId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeDrawBatch",
                               [&mgr](std::vector<int64_t> ids) -> bool {
                                   return mgr.drawBatch(ids);
                               });
}

void RemoteCallExporter::exportLifecycleFunctions() {
    auto& mgr = PacketDebugRenderer::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "shapeDestroy",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.destroy(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeDestroyAll",
                               [&mgr]() -> void {
                                   mgr.destroyAll();
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeDestroyBatch",
                               [&mgr](std::vector<int64_t> ids) -> bool {
                                   return mgr.destroyBatch(ids);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeFindTextByLocation",
                               [&mgr](float x, float y, float z, float radius) -> std::vector<int64_t> {
                                   return mgr.findTextByLocation(x, y, z, radius);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeFindTextByLocationAndContent",
                               [&mgr](float x, float y, float z, float radius, std::string const& text) -> int64_t {
                                   return mgr.findTextByLocationAndContent(x, y, z, radius, text);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeGetAllShapeIds",
                               [&mgr]() -> std::vector<int64_t> {
                                   return mgr.getAllShapeIds();
                               });

    hologramlib::lse::exportAs(NAMESPACE, "shapeExists",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.exists(id);
                               });
}

} // namespace debugshape_export
