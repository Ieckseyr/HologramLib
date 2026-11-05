#include "ParticleShapeExporter.h"
#include "ParticleShapeManager.h"

#include "lse/LseBridge.h"

#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void ParticleShapeExporter::exportAll() {
    auto& mgr = ParticleShapeManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreatePoint",
        [&mgr](
            std::string const& playerName, int dimId,
            float x, float y, float z,
            std::string const& effect, int interval, int lifetime) -> int64_t {
            return mgr.createPoint(playerName, dimId, x, y, z, effect, interval, lifetime);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreateLine",
        [&mgr](
            std::string const& playerName, int dimId,
            float x1, float y1, float z1, float x2, float y2, float z2, float step,
            std::string const& effect, int interval, int lifetime) -> int64_t {
            return mgr.createLine(playerName, dimId, x1, y1, z1, x2, y2, z2, step, effect, interval, lifetime);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreateRect",
        [&mgr](
            std::string const& playerName, int dimId,
            float cx, float cy, float cz, float w, float h, int axis, float step,
            std::string const& effect, int interval, int lifetime) -> int64_t {
            return mgr.createRect(playerName, dimId, cx, cy, cz, w, h, axis, step, effect, interval, lifetime);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreatePlane",
        [&mgr](
            std::string const& playerName, int dimId,
            float cx, float cy, float cz, float w, float h, int axis, float step,
            std::string const& effect, int interval, int lifetime) -> int64_t {
            return mgr.createPlane(playerName, dimId, cx, cy, cz, w, h, axis, step, effect, interval, lifetime);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreateBox",
        [&mgr](
            std::string const& playerName, int dimId,
            float cx, float cy, float cz, float hx, float hy, float hz, float step,
            std::string const& effect, int interval, int lifetime) -> int64_t {
            return mgr.createBox(playerName, dimId, cx, cy, cz, hx, hy, hz, step, effect, interval, lifetime);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreateBoxFaces",
        [&mgr](
            std::string const& playerName, int dimId,
            float cx, float cy, float cz, float hx, float hy, float hz, float step,
            std::string const& effect, int interval, int lifetime) -> int64_t {
            return mgr.createBoxFaces(playerName, dimId, cx, cy, cz, hx, hy, hz, step, effect, interval, lifetime);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleCreatePoly",
        [&mgr](
            std::string const& playerName, int dimId,
            std::string const& verts, std::string const& edges, float step,
            std::string const& effect, int interval, int lifetime) -> int64_t {
            return mgr.createPoly(playerName, dimId, verts, edges, step, effect, interval, lifetime);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetPos",
        [&mgr](int64_t id, float x, float y, float z) -> bool {
            return mgr.setPos(id, x, y, z);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleMoveBy",
        [&mgr](int64_t id, float dx, float dy, float dz) -> bool {
            return mgr.moveBy(id, dx, dy, dz);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleMoveTo",
        [&mgr](int64_t id, float x, float y, float z, int durationTicks) -> bool {
            return mgr.moveTo(id, x, y, z, durationTicks);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetRot",
        [&mgr](int64_t id, float rx, float ry, float rz) -> bool {
            return mgr.setRot(id, rx, ry, rz);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSpin",
        [&mgr](int64_t id, float sx, float sy, float sz) -> bool {
            return mgr.spin(id, sx, sy, sz);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetScale",
        [&mgr](int64_t id, float s) -> bool {
            return mgr.setScale(id, s);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleFollow",
        [&mgr](int64_t id, std::string const& uuid, float offX, float offY, float offZ) -> bool {
            return mgr.follow(id, uuid, offX, offY, offZ);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleUnfollow", [&mgr](int64_t id) -> bool {
            return mgr.unfollow(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetEffect",
        [&mgr](int64_t id, std::string const& effect) -> bool {
            return mgr.setEffect(id, effect);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetVisible",
        [&mgr](int64_t id, std::string const& playersCsv) -> bool {
            return mgr.setVisible(id, playersCsv);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetInterval",
        [&mgr](int64_t id, int ticks) -> bool {
            return mgr.setInterval(id, ticks);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetViewDistance",
        [&mgr](int64_t id, int blocks) -> bool {
            return mgr.setViewDistance(id, blocks);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetLifetime",
        [&mgr](int64_t id, int ticks) -> bool {
            return mgr.setLifetime(id, ticks);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleDestroy", [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });

    hologramlib::lse::exportAs(NAMESPACE, "particleDestroyAll", [&mgr]() -> void {
        mgr.destroyAll();
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleExists", [&mgr](int64_t id) -> bool {
            return mgr.exists(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleGetAllIds", [&mgr]() -> std::vector<int64_t> {
            return mgr.getAllIds();
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleSetVisiblePlayers",
        [&mgr](int64_t id, std::vector<std::string> playerUuids) -> bool {
            return mgr.setVisiblePlayers(id, playerUuids);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleClearVisiblePlayers", [&mgr](int64_t id) -> bool {
            return mgr.clearVisiblePlayers(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "particleGetInfo",
        [&mgr](int64_t id) -> std::string {
            return mgr.getInfo(id);
        });
}

} // namespace debugshape_export
