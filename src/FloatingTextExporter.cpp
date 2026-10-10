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
    
    // create(x, y, z) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "holoCreate",
        [&mgr](float x, float y, float z) -> int64_t {
            return mgr.create(x, y, z);
        });
    
    // destroy(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoDestroy",
        [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });
    
    // destroyAll() -> void
    hologramlib::lse::exportAs(NAMESPACE, "holoDestroyAll",
        [&mgr]() -> void {
            mgr.destroyAll();
        });
}

void FloatingTextExporter::exportLineFunctions() {
    auto& mgr = FloatingTextManager::getInstance();
    
    // addLine(id, text) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoAddLine",
        [&mgr](int64_t id, std::string const& text) -> bool {
            return mgr.addLine(id, text);
        });
    
    // setLineText(id, lineIndex, text) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineText",
        [&mgr](int64_t id, int lineIndex, std::string const& text) -> bool {
            return mgr.setLineText(id, lineIndex, text);
        });

    // setLinePool(id, lineIndex, content: [s], intervalMs) -> bool（动态行: 内容池轮播, 重构自 Phantom, LGPL-3.0）
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLinePool",
        [&mgr](int64_t id, int lineIndex, std::vector<std::string> content, int intervalMs) -> bool {
            return mgr.setLinePool(id, lineIndex, content, intervalMs);
        });

    // setLineParseVariables(id, lineIndex, enabled) -> bool（该行是否解析变量; 默认 true）
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineParseVariables",
        [&mgr](int64_t id, int lineIndex, bool enabled) -> bool {
            return mgr.setLineParseVariables(id, lineIndex, enabled);
        });
    
    // removeLine(id, lineIndex) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoRemoveLine",
        [&mgr](int64_t id, int lineIndex) -> bool {
            return mgr.removeLine(id, lineIndex);
        });
    
    // clearLines(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoClearLines",
        [&mgr](int64_t id) -> bool {
            return mgr.clearLines(id);
        });
    
    // getLineCount(id) -> int
    hologramlib::lse::exportAs(NAMESPACE, "holoGetLineCount",
        [&mgr](int64_t id) -> int {
            return mgr.getLineCount(id);
        });
}

void FloatingTextExporter::exportColorFunctions() {
    auto& mgr = FloatingTextManager::getInstance();
    
    // setColor(id, r, g, b, a) -> bool - 整块文字颜色
    hologramlib::lse::exportAs(NAMESPACE, "holoSetColor",
        [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
            return mgr.setColor(id, r, g, b, a);
        });
    
    // setBackgroundColor(id, r, g, b, a) -> bool - 背景框颜色（不设 = 客户端默认）
    hologramlib::lse::exportAs(NAMESPACE, "holoSetBackgroundColor",
        [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
            return mgr.setBackgroundColor(id, r, g, b, a);
        });
    
    // clearBackgroundColor(id) -> bool - 清除背景框颜色（回客户端默认色）
    hologramlib::lse::exportAs(NAMESPACE, "holoClearBackgroundColor",
        [&mgr](int64_t id) -> bool {
            return mgr.clearBackgroundColor(id);
        });
}

void FloatingTextExporter::exportStyleFunctions() {
    auto& mgr = FloatingTextManager::getInstance();
    
    // setScale(id, scale) -> bool - 整块缩放
    hologramlib::lse::exportAs(NAMESPACE, "holoSetScale",
        [&mgr](int64_t id, float scale) -> bool {
            return mgr.setScale(id, scale);
        });
    
    // setDepthTest(id, enabled) -> bool - 穿墙可见性（true = 被方块遮挡, false = 始终渲染）
    hologramlib::lse::exportAs(NAMESPACE, "holoSetDepthTest",
        [&mgr](int64_t id, bool enabled) -> bool {
            return mgr.setDepthTest(id, enabled);
        });
    
    // setRotation(id, pitch, yaw, roll) -> bool - 三轴固定朝向（度; 不再面向相机）
    hologramlib::lse::exportAs(NAMESPACE, "holoSetRotation",
        [&mgr](int64_t id, float pitch, float yaw, float roll) -> bool {
            return mgr.setRotation(id, pitch, yaw, roll);
        });
    
    // clearRotation(id) -> bool - 恢复面向相机
    hologramlib::lse::exportAs(NAMESPACE, "holoClearRotation",
        [&mgr](int64_t id) -> bool {
            return mgr.clearRotation(id);
        });
    
    // setLocation(id, x, y, z) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLocation",
        [&mgr](int64_t id, float x, float y, float z) -> bool {
            return mgr.setLocation(id, x, y, z);
        });

    // setDimension(id, dimId) -> bool（1.12.0: 迁移维度; 已绘制时原地重发无闪烁）
    hologramlib::lse::exportAs(NAMESPACE, "holoSetDimension",
        [&mgr](int64_t id, int dimId) -> bool {
            return mgr.setDimension(id, dimId);
        });
    
    // setFollowPlayer(id, playerName, offsetY) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoSetFollowPlayer",
        [&mgr](int64_t id, std::string const& playerName, float offsetY) -> bool {
            return mgr.setFollowPlayer(id, playerName, offsetY);
        });
    
    // clearFollowPlayer(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoClearFollowPlayer",
        [&mgr](int64_t id) -> bool {
            return mgr.clearFollowPlayer(id);
        });
}

void FloatingTextExporter::exportDisplayFunctions() {
    auto& mgr = FloatingTextManager::getInstance();
    
    // draw(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoDraw",
        [&mgr](int64_t id) -> bool {
            return mgr.draw(id);
        });
    
    // drawToDimension(id, dimId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoDrawToDimension",
        [&mgr](int64_t id, int dimId) -> bool {
            return mgr.drawToDimension(id, dimId);
        });
    
    // drawToPlayer(id, playerName) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoDrawToPlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.drawToPlayer(id, playerName);
        });
    
    // remove(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoRemove",
        [&mgr](int64_t id) -> bool {
            return mgr.remove(id);
        });
    
    // refresh(id) -> bool - 重解析变量/跟随坐标并原地重发
    hologramlib::lse::exportAs(NAMESPACE, "holoRefresh",
        [&mgr](int64_t id) -> bool {
            return mgr.refresh(id);
        });
}

} // namespace debugshape_export
