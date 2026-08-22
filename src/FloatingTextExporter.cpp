#include "FloatingTextExporter.h"
#include "FloatingTextManager.h"
#include "ModEntry.h"

#include "lse/LseBridge.h"

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void FloatingTextExporter::exportAll() {
    auto& logger = ModEntry::getInstance().getSelf().getLogger();
    logger.info("Exporting FloatingText functions to LegacyRemoteCall...");
    
    exportCreateFunctions();
    exportLineFunctions();
    exportColorFunctions();
    exportAnimationFunctions();
    exportDisplayFunctions();
    
    logger.info("FloatingText functions exported successfully.");
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
    
    // setLineScale(id, lineIndex, scale) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineScale",
        [&mgr](int64_t id, int lineIndex, float scale) -> bool {
            return mgr.setLineScale(id, lineIndex, scale);
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
    
    // setColor(id, r, g, b, a) -> bool - 设置整体颜色
    hologramlib::lse::exportAs(NAMESPACE, "holoSetColor",
        [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
            return mgr.setColor(id, r, g, b, a);
        });
    
    // setLineColor(id, lineIndex, r, g, b, a) -> bool - 设置单行纯色
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineColor",
        [&mgr](int64_t id, int lineIndex, float r, float g, float b, float a) -> bool {
            return mgr.setLineColor(id, lineIndex, r, g, b, a);
        });
    
    // setLineGradient(id, lineIndex, r1, g1, b1, r2, g2, b2) -> bool - 设置单行渐变
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineGradient",
        [&mgr](int64_t id, int lineIndex, 
               float r1, float g1, float b1,
               float r2, float g2, float b2) -> bool {
            return mgr.setLineGradient(id, lineIndex, r1, g1, b1, r2, g2, b2);
        });
    
    // setLineRainbow(id, lineIndex, speed) -> bool - 设置单行彩虹效果
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineRainbow",
        [&mgr](int64_t id, int lineIndex, float speed) -> bool {
            return mgr.setLineRainbow(id, lineIndex, speed);
        });
}

void FloatingTextExporter::exportAnimationFunctions() {
    auto& mgr = FloatingTextManager::getInstance();
    
    // setLineScroll(id, lineIndex, direction, speed) -> bool
    // direction: 0=无, 1=左, 2=右
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineScroll",
        [&mgr](int64_t id, int lineIndex, int direction, float speed) -> bool {
            return mgr.setLineScroll(id, lineIndex, direction, speed);
        });
    
    // setVerticalAnimation(id, type, speed, range) -> bool
    // type: 0=无, 1=弹跳, 2=滚动
    hologramlib::lse::exportAs(NAMESPACE, "holoSetVerticalAnimation",
        [&mgr](int64_t id, int type, float speed, float range) -> bool {
            return mgr.setVerticalAnimation(id, type, speed, range);
        });
    
    // setLineSpacing(id, spacing) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLineSpacing",
        [&mgr](int64_t id, float spacing) -> bool {
            return mgr.setLineSpacing(id, spacing);
        });
    
    // setLocation(id, x, y, z) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "holoSetLocation",
        [&mgr](int64_t id, float x, float y, float z) -> bool {
            return mgr.setLocation(id, x, y, z);
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
    
    // tick(deltaTime) -> void - 更新动画 (应在游戏tick中调用)
    hologramlib::lse::exportAs(NAMESPACE, "holoTick",
        [&mgr](float deltaTime) -> void {
            mgr.tick(deltaTime);
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
    
    // refresh(id) -> bool - 刷新显示
    hologramlib::lse::exportAs(NAMESPACE, "holoRefresh",
        [&mgr](int64_t id) -> bool {
            return mgr.refresh(id);
        });
}

} // namespace debugshape_export
