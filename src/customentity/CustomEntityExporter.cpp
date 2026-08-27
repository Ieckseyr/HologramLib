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

    // ── 1.12.0 追加 ──

    // entitySetPose(id, pose) -> bool（PoseIndex 0..13; 盔甲架坐姿/睡姿/跳舞等, 经 respawn 生效）
    hologramlib::lse::exportAs(
        NAMESPACE, "entitySetPose", [&mgr](int64_t id, int pose) -> bool { return mgr.setPose(id, pose); });

    // entitySetEquipmentSlot(id, slot, name, aux, nbt) -> bool
    // 槽位: 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet; name 空清空槽位; nbt 为 SNBT 字符串
    hologramlib::lse::exportAs(
        NAMESPACE,
        "entitySetEquipmentSlot",
        [&mgr](int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) -> bool {
            return mgr.setEquipmentSlot(id, slot, name, aux, nbt);
        });

    // entityScaleBy(id, factor) -> bool（相对缩放: 现有 scale × factor, 自动钳制 0.0625~10）
    hologramlib::lse::exportAs(
        NAMESPACE, "entityScaleBy", [&mgr](int64_t id, float factor) -> bool {
            return mgr.scaleBy(id, factor);
        });

    // entitySetVisiblePlayers(id, playerNames: [s]) -> bool（空列表 = 清除限制 = 全员可见）
    hologramlib::lse::exportAs(NAMESPACE, "entitySetVisiblePlayers",
        [&mgr](int64_t id, std::vector<std::string> playerNames) -> bool {
            return mgr.setVisiblePlayers(id, playerNames);
        });
    // entityClearVisiblePlayers(id) -> bool（恢复全员可见）
    hologramlib::lse::exportAs(NAMESPACE, "entityClearVisiblePlayers",
        [&mgr](int64_t id) -> bool { return mgr.clearVisiblePlayers(id); });
    // entitySetVisiblePlayer(id, playerName: s) -> bool（单玩家白名单; JS 侧推荐标量版）
    hologramlib::lse::exportAs(NAMESPACE, "entitySetVisiblePlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.setVisiblePlayer(id, playerName);
        });

    // entityGetInfo(id) -> s（诊断探针; 找不到返回 "not_found"）
    hologramlib::lse::exportAs(
        NAMESPACE, "entityGetInfo", [&mgr](int64_t id) -> std::string { return mgr.getDebugInfo(id); });

    // entitySetRidePlayer(id, playerName) -> bool（实体骑到指定玩家头上; 空名清除; 玩家须在线）
    hologramlib::lse::exportAs(NAMESPACE, "entitySetRidePlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.setRidePlayer(id, playerName);
        });
    // entitySetRideEntity(id, vehicleEntityId) -> bool（实体骑到另一自定义实体上; 0 清除）
    hologramlib::lse::exportAs(NAMESPACE, "entitySetRideEntity",
        [&mgr](int64_t id, int64_t vehicleEntityId) -> bool {
            return mgr.setRideEntity(id, vehicleEntityId);
        });
    // entityClearRide(id) -> bool（解除骑乘链接）
    hologramlib::lse::exportAs(
        NAMESPACE, "entityClearRide", [&mgr](int64_t id) -> bool { return mgr.clearRide(id); });

    // entityPlayAnimation(id, animation, stopExpression, durationTicks) -> bool
    // 播放原版动画（如 "animation.humanoid.base_pose"）; stopExpression 空串 = 常驻;
    // durationTicks>0 到期自动停止; controller 名库内自动唯一化
    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityPlayAnimation",
        [&mgr](int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks)
            -> bool { return mgr.playAnimation(id, animation, stopExpression, durationTicks); });
}

} // namespace debugshape_export