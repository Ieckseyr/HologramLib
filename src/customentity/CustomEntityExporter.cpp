// CustomEntityExporter.cpp - 自定义实体协议层生成 LSE 导出实现
#include "CustomEntityExporter.h"
#include "CustomEntityManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void CustomEntityExporter::exportAll() {
    auto& mgr = CustomEntityManager::getInstance();

    // 创建/生命周期
    // entityCreate(identifier, x, y, z, dim) -> int64（id; <0 失败）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityCreate",
        [&mgr](std::string const& identifier, float x, float y, float z, int dim) -> int64_t {
            CustomEntityConfig cfg;
            cfg.identifier = identifier;
            cfg.x           = x;
            cfg.y           = y;
            cfg.z           = z;
            cfg.dimension   = dim;
            return mgr.create(cfg);
        });

    // entityCreateAdvanced(identifier, x, y, z, dim, yaw, pitch, scale, nametag) -> int64
    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityCreateAdvanced",
        [&mgr](
            std::string const& identifier,
            float              x,
            float              y,
            float              z,
            int                dim,
            float              yaw,
            float              pitch,
            float              scale,
            std::string const& nametag
        ) -> int64_t {
            CustomEntityConfig cfg;
            cfg.identifier = identifier;
            cfg.x           = x;
            cfg.y           = y;
            cfg.z           = z;
            cfg.dimension   = dim;
            cfg.yaw         = yaw;
            cfg.pitch       = pitch;
            cfg.scale       = scale;
            cfg.nametag     = nametag;
            return mgr.create(cfg);
        });

    // entityCreateRandom(identifier, x, y, z, dim) -> int64（随机段 ID; <0 失败）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityCreateRandom",
        [&mgr](std::string const& identifier, float x, float y, float z, int dim) -> int64_t {
            CustomEntityConfig cfg;
            cfg.identifier = identifier;
            cfg.x           = x;
            cfg.y           = y;
            cfg.z           = z;
            cfg.dimension   = dim;
            return mgr.createRandom(cfg);
        });

    // entityCreateWithId(identifier, x, y, z, dim, desiredId) -> int64（持久化恢复用; <=0 或占用返回 -2）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityCreateWithId",
        [&mgr](std::string const& identifier, float x, float y, float z, int dim, int64_t desiredId) -> int64_t {
            CustomEntityConfig cfg;
            cfg.identifier = identifier;
            cfg.x           = x;
            cfg.y           = y;
            cfg.z           = z;
            cfg.dimension   = dim;
            return mgr.createWithId(cfg, desiredId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "entityDestroy", [&mgr](int64_t id) -> bool { return mgr.destroy(id); });
    hologramlib::lse::exportAs(
        NAMESPACE, "entityDestroyAll", [&mgr]() -> void { mgr.destroyAll(); });
    hologramlib::lse::exportAs(
        NAMESPACE, "entityExists", [&mgr](int64_t id) -> bool { return mgr.exists(id); });
    hologramlib::lse::exportAs(
        NAMESPACE, "entityIsIdUsed", [&mgr](int64_t id) -> bool { return mgr.isIdUsed(id); });
    hologramlib::lse::exportAs(
        NAMESPACE, "entityGetAllIds", [&mgr]() -> std::vector<int64_t> { return mgr.getAllIds(); });

    // 属性
    hologramlib::lse::exportAs(NAMESPACE, "entitySetIdentifier",
        [&mgr](int64_t id, std::string const& identifier) -> bool {
            return mgr.setIdentifier(id, identifier);
        });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetPosition",
        [&mgr](int64_t id, float x, float y, float z, int dim) -> bool {
            return mgr.setPosition(id, x, y, z, dim);
        });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetRotation",
        [&mgr](int64_t id, float yaw, float pitch) -> bool { return mgr.setRotation(id, yaw, pitch); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetNametag",
        [&mgr](int64_t id, std::string const& text) -> bool { return mgr.setNametag(id, text); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetScale",
        [&mgr](int64_t id, float scale) -> bool { return mgr.setScale(id, scale); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetVariant",
        [&mgr](int64_t id, int variant) -> bool { return mgr.setVariant(id, variant); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetMarkVariant",
        [&mgr](int64_t id, int markVariant) -> bool { return mgr.setMarkVariant(id, markVariant); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetColorIndex",
        [&mgr](int64_t id, int colorIndex) -> bool { return mgr.setColorIndex(id, colorIndex); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetFlags",
        [&mgr](int64_t id, int64_t flags) -> bool { return mgr.setFlags(id, flags); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetInvisible",
        [&mgr](int64_t id, bool on) -> bool { return mgr.setInvisible(id, on); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetEnabled",
        [&mgr](int64_t id, bool enabled) -> bool { return mgr.setEnabled(id, enabled); });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetViewDistance",
        [&mgr](int64_t id, float dist) -> bool { return mgr.setViewDistance(id, dist); });

    // entityFindNearest(x, y, z, dim, maxDist) -> int64（最近查找; 无匹配 -1）
    hologramlib::lse::exportAs(NAMESPACE, "entityFindNearest",
        [&mgr](float x, float y, float z, int dim, float maxDist) -> int64_t {
            return mgr.findNearest(x, y, z, dim, maxDist);
        });
}

} // namespace debugshape_export