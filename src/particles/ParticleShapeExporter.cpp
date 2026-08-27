// ParticleShapeExporter.cpp - 通用粒子形状系统 LSE 导出实现

#include "ParticleShapeExporter.h"
#include "ParticleShapeManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void ParticleShapeExporter::exportAll() {
    auto& mgr = ParticleShapeManager::getInstance();

    // ── 创建（世界坐标; interval/lifetime 单位 = tick; lifetime 0 = 永久）──
    // particleCreatePoint(x, y, z, effect, interval, lifetime) -> id
    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreatePoint",
        [&mgr](
            std::string const& playerName, int dimId,
            float x, float y, float z,
            std::string const& effect, int interval, int lifetime
        ) -> int64_t {
            return mgr.createPoint(playerName, dimId, x, y, z, effect, interval, lifetime);
        });

    // particleCreateLine(x1,y,z1, x2,y2,z2, step, effect, interval, lifetime) -> id
    // 锚点 = 线中点（旋转绕中点）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreateLine",
        [&mgr](
            std::string const& playerName, int dimId,
            float x1, float y1, float z1, float x2, float y2, float z2, float step,
            std::string const& effect, int interval, int lifetime
        ) -> int64_t {
            return mgr.createLine(playerName, dimId, x1, y1, z1, x2, y2, z2, step, effect, interval, lifetime);
        });

    // particleCreateRect(cx,cy,cz, w, h, axis, step, effect, interval, lifetime) -> id
    // 矩形环线; axis: 0=XY 1=YZ 2=XZ（w/h 沿平面两轴）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreateRect",
        [&mgr](
            std::string const& playerName, int dimId,
            float cx, float cy, float cz, float w, float h, int axis, float step,
            std::string const& effect, int interval, int lifetime
        ) -> int64_t {
            return mgr.createRect(playerName, dimId, cx, cy, cz, w, h, axis, step, effect, interval, lifetime);
        });

    // particleCreatePlane(同 rect 参数) -> id（填充平面网格）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreatePlane",
        [&mgr](
            std::string const& playerName, int dimId,
            float cx, float cy, float cz, float w, float h, int axis, float step,
            std::string const& effect, int interval, int lifetime
        ) -> int64_t {
            return mgr.createPlane(playerName, dimId, cx, cy, cz, w, h, axis, step, effect, interval, lifetime);
        });

    // particleCreateBox(cx,cy,cz, hx,hy,hz, step, effect, interval, lifetime) -> id
    // 长方体 12 边线框; hx/hy/hz = 半尺寸
    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreateBox",
        [&mgr](
            std::string const& playerName, int dimId,
            float cx, float cy, float cz, float hx, float hy, float hz, float step,
            std::string const& effect, int interval, int lifetime
        ) -> int64_t {
            return mgr.createBox(playerName, dimId, cx, cy, cz, hx, hy, hz, step, effect, interval, lifetime);
        });

    // particleCreateBoxFaces(同 box 参数) -> id（六面填充）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreateBoxFaces",
        [&mgr](
            std::string const& playerName, int dimId,
            float cx, float cy, float cz, float hx, float hy, float hz, float step,
            std::string const& effect, int interval, int lifetime
        ) -> int64_t {
            return mgr.createBoxFaces(playerName, dimId, cx, cy, cz, hx, hy, hz, step, effect, interval, lifetime);
        });

    // particleCreatePoly(verts, edges, step, effect, interval, lifetime) -> id
    // verts: "x,y,z;x,y,z;..."（世界坐标, 锚点=质心）; edges: "i-j;i-j"（顶点索引对）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreatePoly",
        [&mgr](
            std::string const& playerName, int dimId,
            std::string const& verts, std::string const& edges, float step,
            std::string const& effect, int interval, int lifetime
        ) -> int64_t {
            return mgr.createPoly(playerName, dimId, verts, edges, step, effect, interval, lifetime);
        });

    // ── 智能控制 ──

    // particleSetPos(id, x, y, z) -> b —— 平移锚点（粒子移动; 解除跟随）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetPos",
        [&mgr](int64_t id, float x, float y, float z) -> bool {
            return mgr.setPos(id, x, y, z);
        });

    // particleMoveBy(id, dx, dy, dz) -> b —— 相对平移
    hologramlib::lse::exportAs(
        NAMESPACE, "particleMoveBy",
        [&mgr](int64_t id, float dx, float dy, float dz) -> bool {
            return mgr.moveBy(id, dx, dy, dz);
        });

    // particleMoveTo(id, x, y, z, durationTicks) -> b —— 平滑点对点移动
    // 锚点 easeOutCubic 插值逼近目标（单点/整面/整体形状均随锚点移动）; 解除跟随
    hologramlib::lse::exportAs(
        NAMESPACE, "particleMoveTo",
        [&mgr](int64_t id, float x, float y, float z, int durationTicks) -> bool {
            return mgr.moveTo(id, x, y, z, durationTicks);
        });

    // particleSetRot(id, rx, ry, rz) -> b —— 欧拉角（度, ZYX 序, 绕锚点）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetRot",
        [&mgr](int64_t id, float rx, float ry, float rz) -> bool {
            return mgr.setRot(id, rx, ry, rz);
        });

    // particleSpin(id, sx, sy, sz) -> b —— 自旋速率（度/tick; 0,0,0 停止）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSpin",
        [&mgr](int64_t id, float sx, float sy, float sz) -> bool { return mgr.spin(id, sx, sy, sz); });

    // particleSetScale(id, s) -> b
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetScale",
        [&mgr](int64_t id, float s) -> bool { return mgr.setScale(id, s); });

    // particleFollow(id, uuid, offX, offY, offZ) -> b —— 锚点跟随玩家位置+偏移（每 tick）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleFollow",
        [&mgr](int64_t id, std::string const& uuid, float offX, float offY, float offZ) -> bool {
            return mgr.follow(id, uuid, offX, offY, offZ);
        });

    // particleUnfollow(id) -> b
    hologramlib::lse::exportAs(
        NAMESPACE, "particleUnfollow", [&mgr](int64_t id) -> bool { return mgr.unfollow(id); });

    // ── 渲染 / 可见性 / 生命周期 ──

    // particleSetEffect(id, effect) -> b
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetEffect",
        [&mgr](int64_t id, std::string const& effect) -> bool {
            return mgr.setEffect(id, effect);
        });

    // particleSetVisible(id, playersCsv) -> b —— "uuid1,uuid2" 白名单; 空串 = 维度全员
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetVisible",
        [&mgr](int64_t id, std::string const& playersCsv) -> bool {
            return mgr.setVisible(id, playersCsv);
        });

    // particleSetInterval(id, ticks) -> b —— 重发周期
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetInterval",
        [&mgr](int64_t id, int ticks) -> bool { return mgr.setInterval(id, ticks); });

    // particleSetViewDistance(id, blocks) -> b —— 逐玩家 3D 裁剪半径; 0 = 不裁剪
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetViewDistance",
        [&mgr](int64_t id, int blocks) -> bool { return mgr.setViewDistance(id, blocks); });

    // particleSetLifetime(id, ticks) -> b —— 从现在起重新计时; 0 = 永久
    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetLifetime",
        [&mgr](int64_t id, int ticks) -> bool { return mgr.setLifetime(id, ticks); });

    // particleDestroy(id) -> b
    hologramlib::lse::exportAs(
        NAMESPACE, "particleDestroy", [&mgr](int64_t id) -> bool { return mgr.destroy(id); });

    // particleGetInfo(id) -> str（诊断探针）
    hologramlib::lse::exportAs(
        NAMESPACE, "particleGetInfo",
        [&mgr](int64_t id) -> std::string { return mgr.getInfo(id); });
}

} // namespace debugshape_export
