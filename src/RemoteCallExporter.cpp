#include "RemoteCallExporter.h"
#include "PacketDebugRenderer.h"
#include "ModEntry.h"

#include "lse/LseBridge.h"
#include <fmt/format.h>

namespace debugshape_export {

// 命名空间常量
static constexpr const char* NAMESPACE = "HologramLib";

void RemoteCallExporter::exportAll() {
    auto& logger = ModEntry::getInstance().getSelf().getLogger();
    logger.info("Exporting DebugShape functions to LegacyRemoteCall (Protocol Layer)...");
    
    fmt::print("[DebugShape] Starting exportAll...\n");
    
    exportCreateFunctions();
    exportPropertyFunctions();
    exportDisplayFunctions();
    exportLifecycleFunctions();
    
    fmt::print("[DebugShape] All functions exported successfully.\n");
    
    logger.info("All functions exported successfully.");
}

void RemoteCallExporter::exportCreateFunctions() {
    auto& mgr = PacketDebugRenderer::getInstance();
    
    fmt::print("[DebugShape] Exporting createText function...\n");
    
    // createText(x, y, z, text) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateText",
        [&mgr](float x, float y, float z, std::string const& text) -> int64_t {
            return mgr.createText(x, y, z, text);
        });
    
    // createLine(x1, y1, z1, x2, y2, z2) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateLine",
        [&mgr](float x1, float y1, float z1, float x2, float y2, float z2) -> int64_t {
            return mgr.createLine(x1, y1, z1, x2, y2, z2);
        });
    
    // createBox(x1, y1, z1, x2, y2, z2) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateBox",
        [&mgr](float x1, float y1, float z1, float x2, float y2, float z2) -> int64_t {
            return mgr.createBox(x1, y1, z1, x2, y2, z2);
        });
    
    // createCircle(x, y, z, scale) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateCircle",
        [&mgr](float x, float y, float z, float scale) -> int64_t {
            return mgr.createCircle(x, y, z, scale);
        });
    
    // createSphere(x, y, z, scale) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateSphere",
        [&mgr](float x, float y, float z, float scale) -> int64_t {
            return mgr.createSphere(x, y, z, scale);
        });
    
    // createArrow(x1, y1, z1, x2, y2, z2) -> int64
    hologramlib::lse::exportAs(NAMESPACE, "shapeCreateArrow",
        [&mgr](float x1, float y1, float z1, float x2, float y2, float z2) -> int64_t {
            return mgr.createArrow(x1, y1, z1, x2, y2, z2);
        });

    // 注: createFilledQuad / createFilledQuadBatch（极薄 Box 模拟填充面,
    // 批量版每像素一个 Box）已按需求移除 —— 请使用 Box 组合或客户端渲染方案
}


void RemoteCallExporter::exportPropertyFunctions() {
    auto& mgr = PacketDebugRenderer::getInstance();
    
    // setText(id, text) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeSetText",
        [&mgr](int64_t id, std::string const& text) -> bool {
            return mgr.setText(id, text);
        });
    
    // getText(id) -> string
    hologramlib::lse::exportAs(NAMESPACE, "shapeGetText",
        [&mgr](int64_t id) -> std::string {
            return mgr.getText(id);
        });
    
    // setLocation(id, x, y, z) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeSetLocation",
        [&mgr](int64_t id, float x, float y, float z) -> bool {
            return mgr.setLocation(id, x, y, z);
        });
    
    // getLocation(id) -> [x, y, z]
    hologramlib::lse::exportAs(NAMESPACE, "shapeGetLocation",
        [&mgr](int64_t id) -> std::vector<float> {
            return mgr.getLocation(id);
        });
    
    // setColor(id, r, g, b, a) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeSetColor",
        [&mgr](int64_t id, float r, float g, float b, float a) -> bool {
            return mgr.setColor(id, r, g, b, a);
        });
    
    // getColor(id) -> [r, g, b, a]
    hologramlib::lse::exportAs(NAMESPACE, "shapeGetColor",
        [&mgr](int64_t id) -> std::vector<float> {
            return mgr.getColor(id);
        });
    
    // setScale(id, scale) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeSetScale",
        [&mgr](int64_t id, float scale) -> bool {
            return mgr.setScale(id, scale);
        });
    
    // setDuration(id, seconds) -> bool - 设置持续时间
    hologramlib::lse::exportAs(NAMESPACE, "shapeSetDuration",
        [&mgr](int64_t id, float seconds) -> bool {
            return mgr.setDuration(id, seconds);
        });

    // setDimension(id, dimId) -> bool - 设置形状所在维度（跨维度显示必需）
    hologramlib::lse::exportAs(NAMESPACE, "shapeSetDimension",
        [&mgr](int64_t id, int dimId) -> bool {
            return mgr.setDimension(id, dimId);
        });
    
    // setRotation(id, pitch, yaw, roll) -> bool - 设置固定朝向（弧度）
    hologramlib::lse::exportAs(NAMESPACE, "shapeSetRotation",
        [&mgr](int64_t id, float pitch, float yaw, float roll) -> bool {
            return mgr.setRotation(id, pitch, yaw, roll);
        });
    
    // clearRotation(id) -> bool - 清除旋转，恢复 billboard 模式
    hologramlib::lse::exportAs(NAMESPACE, "shapeClearRotation",
        [&mgr](int64_t id) -> bool {
            return mgr.clearRotation(id);
        });
    
    // getRotation(id) -> [pitch, yaw, roll] - 获取旋转角度，billboard 模式返回空数组
    hologramlib::lse::exportAs(NAMESPACE, "shapeGetRotation",
        [&mgr](int64_t id) -> std::vector<float> {
            return mgr.getRotation(id);
        });
    
    // getShapeType(id) -> int
    hologramlib::lse::exportAs(NAMESPACE, "shapeGetShapeType",
        [&mgr](int64_t id) -> int {
            return mgr.getShapeType(id);
        });
}


void RemoteCallExporter::exportDisplayFunctions() {
    auto& mgr = PacketDebugRenderer::getInstance();
    
    // draw(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeDraw",
        [&mgr](int64_t id) -> bool {
            return mgr.draw(id);
        });
    
    // drawToPlayer(id, playerName) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeDrawToPlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.drawToPlayer(id, playerName);
        });
    
    // drawToDimension(id, dimId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeDrawToDimension",
        [&mgr](int64_t id, int dimId) -> bool {
            return mgr.drawToDimension(id, dimId);
        });
    
    // remove(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeRemove",
        [&mgr](int64_t id) -> bool {
            return mgr.remove(id);
        });
    
    // removeToPlayer(id, playerName) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeRemoveToPlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.removeToPlayer(id, playerName);
        });
    
    // removeToDimension(id, dimId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeRemoveToDimension",
        [&mgr](int64_t id, int dimId) -> bool {
            return mgr.removeToDimension(id, dimId);
        });
    
    // update(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeUpdate",
        [&mgr](int64_t id) -> bool {
            return mgr.update(id);
        });
    
    // updateToPlayer(id, playerName) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeUpdateToPlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.updateToPlayer(id, playerName);
        });
    
    // updateToDimension(id, dimId) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeUpdateToDimension",
        [&mgr](int64_t id, int dimId) -> bool {
            return mgr.updateToDimension(id, dimId);
        });
    
    // drawBatch(ids) -> bool - 批量绘制
    hologramlib::lse::exportAs(NAMESPACE, "shapeDrawBatch",
        [&mgr](std::vector<int64_t> ids) -> bool {
            return mgr.drawBatch(ids);
        });
}

void RemoteCallExporter::exportLifecycleFunctions() {
    auto& mgr = PacketDebugRenderer::getInstance();
    
    // destroy(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "shapeDestroy",
        [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });
    
    // destroyAll() -> void
    hologramlib::lse::exportAs(NAMESPACE, "shapeDestroyAll",
        [&mgr]() -> void {
            mgr.destroyAll();
        });
    
    // destroyBatch(ids) -> bool - 批量销毁
    hologramlib::lse::exportAs(NAMESPACE, "shapeDestroyBatch",
        [&mgr](std::vector<int64_t> ids) -> bool {
            return mgr.destroyBatch(ids);
        });
    
    // 查询功能
    
    // findTextByLocation(x, y, z, radius) -> [int64...] - 根据位置查找文本形状
    hologramlib::lse::exportAs(NAMESPACE, "shapeFindTextByLocation",
        [&mgr](float x, float y, float z, float radius) -> std::vector<int64_t> {
            return mgr.findTextByLocation(x, y, z, radius);
        });
    
    // findTextByLocationAndContent(x, y, z, radius, text) -> int64 - 根据位置和内容查找
    hologramlib::lse::exportAs(NAMESPACE, "shapeFindTextByLocationAndContent",
        [&mgr](float x, float y, float z, float radius, std::string const& text) -> int64_t {
            return mgr.findTextByLocationAndContent(x, y, z, radius, text);
        });
    
    // getAllShapeIds() -> [int64...] - 获取所有形状ID
    hologramlib::lse::exportAs(NAMESPACE, "shapeGetAllShapeIds",
        [&mgr]() -> std::vector<int64_t> {
            return mgr.getAllShapeIds();
        });
    
    // exists(id) -> bool - 检查形状是否存在
    hologramlib::lse::exportAs(NAMESPACE, "shapeExists",
        [&mgr](int64_t id) -> bool {
            return mgr.exists(id);
        });
}


} // namespace debugshape_export
