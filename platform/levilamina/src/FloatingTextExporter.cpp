#include "FloatingTextExporter.h"

#include "DiagLog.h"
#include "FloatingTextManager.h"
#include "ModEntry.h"

#include "lse/LseBridge.h"

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void FloatingTextExporter::exportAll() {
    HLIB_LOG_INFO("Exporting FloatingText functions to LegacyRemoteCall...");

    exportCreateFunctions();
    exportLineFunctions();
    exportColorFunctions();
    exportStyleFunctions();
    exportDisplayFunctions();

    HLIB_LOG_INFO("FloatingText functions exported successfully.");
}

void FloatingTextExporter::exportCreateFunctions() {
    auto& mgr = FloatingTextManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "holoCreate",
                               [&mgr](float x, float y, float z) -> int64_t {
                                   return mgr.create(x, y, z);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoDestroy",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.destroy(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoDestroyAll",
                               [&mgr]() -> void {
                                   mgr.destroyAll();
                               });
}

void FloatingTextExporter::exportLineFunctions() {
    auto& mgr = FloatingTextManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "holoAddLine",
                               [&mgr](int64_t id, std::string const& text) -> bool {
                                   return mgr.addLine(id, text);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineText",
                               [&mgr](int64_t id, int lineIndex, std::string const& text) -> bool {
                                   return mgr.setLineText(id, lineIndex, text);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetLinePool",
                               [&mgr](int64_t id, int lineIndex, std::vector<std::string> content, int intervalMs) -> bool {
                                   return mgr.setLinePool(id, lineIndex, content, intervalMs);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineParseVariables",
                               [&mgr](int64_t id, int lineIndex, bool enabled) -> bool {
                                   return mgr.setLineParseVariables(id, lineIndex, enabled);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoRemoveLine",
                               [&mgr](int64_t id, int lineIndex) -> bool {
                                   return mgr.removeLine(id, lineIndex);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoClearLines",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearLines(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoGetLineCount",
                               [&mgr](int64_t id) -> int {
                                   return mgr.getLineCount(id);
                               });
}

void FloatingTextExporter::exportColorFunctions() {
    auto& mgr = FloatingTextManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "holoSetColor",
                               [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
                                   return mgr.setColor(id, r, g, b, a);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetBackgroundColor",
                               [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
                                   return mgr.setBackgroundColor(id, r, g, b, a);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoClearBackgroundColor",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearBackgroundColor(id);
                               });
}

void FloatingTextExporter::exportStyleFunctions() {
    auto& mgr = FloatingTextManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "holoSetScale",
                               [&mgr](int64_t id, float scale) -> bool {
                                   return mgr.setScale(id, scale);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetDepthTest",
                               [&mgr](int64_t id, bool enabled) -> bool {
                                   return mgr.setDepthTest(id, enabled);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetRotation",
                               [&mgr](int64_t id, float pitch, float yaw, float roll) -> bool {
                                   return mgr.setRotation(id, pitch, yaw, roll);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoClearRotation",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearRotation(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetLocation",
                               [&mgr](int64_t id, float x, float y, float z) -> bool {
                                   return mgr.setLocation(id, x, y, z);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetDimension",
                               [&mgr](int64_t id, int dimId) -> bool {
                                   return mgr.setDimension(id, dimId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoSetFollowPlayer",
                               [&mgr](int64_t id, std::string const& playerName, float offsetY) -> bool {
                                   return mgr.setFollowPlayer(id, playerName, offsetY);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoClearFollowPlayer",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearFollowPlayer(id);
                               });
}

void FloatingTextExporter::exportDisplayFunctions() {
    auto& mgr = FloatingTextManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "holoDraw",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.draw(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoDrawToDimension",
                               [&mgr](int64_t id, int dimId) -> bool {
                                   return mgr.drawToDimension(id, dimId);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoDrawToPlayer",
                               [&mgr](int64_t id, std::string const& playerName) -> bool {
                                   return mgr.drawToPlayer(id, playerName);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoRemove",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.remove(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "holoRefresh",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.refresh(id);
                               });
}

} // namespace debugshape_export
