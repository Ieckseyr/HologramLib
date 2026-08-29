// PlayerNpcExporter.cpp - 假玩家 NPC LSE 导出实现 + IPlayerNpc 适配器
//
// 适配器将 IPlayerNpc 接口（冻结契约）映射到 PlayerNpcManager 单例;
// LSE 导出命名对齐现有风格（itemDisplay* / customEntity* → playerNpc*）。
#include "PlayerNpcExporter.h"
#include "PlayerNpcManager.h"

#include "lse/LseBridge.h"

#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

// ── IPlayerNpc 适配器（IHologramLib::playerNpcs() 返回引用）──

class PlayerNpcAdapter final : public hologramlib::IPlayerNpc {
public:
    bool registerSkin(hologramlib::PlayerNpcSkin const& skin) override {
        return PlayerNpcManager::getInstance().registerSkin(skin);
    }
    bool captureSkin(std::string const& skinId, std::string const& playerName) override {
        return PlayerNpcManager::getInstance().captureSkin(skinId, playerName);
    }
    bool hasSkin(std::string const& skinId) const override {
        return PlayerNpcManager::getInstance().hasSkin(skinId);
    }
    bool unregisterSkin(std::string const& skinId) override {
        return PlayerNpcManager::getInstance().unregisterSkin(skinId);
    }
    std::vector<std::string> getSkinIds() const override {
        return PlayerNpcManager::getInstance().getSkinIds();
    }

    int64_t create(hologramlib::PlayerNpcConfig const& config) override {
        return PlayerNpcManager::getInstance().create(config);
    }
    int64_t createRandom(hologramlib::PlayerNpcConfig const& config) override {
        return PlayerNpcManager::getInstance().createRandom(config);
    }
    int64_t createWithId(hologramlib::PlayerNpcConfig const& config, int64_t desiredId) override {
        return PlayerNpcManager::getInstance().createWithId(config, desiredId);
    }
    bool destroy(int64_t id) override { return PlayerNpcManager::getInstance().destroy(id); }
    void destroyAll() override { PlayerNpcManager::getInstance().destroyAll(); }
    bool exists(int64_t id) const override { return PlayerNpcManager::getInstance().exists(id); }
    bool get(int64_t id, hologramlib::PlayerNpcConfig& out) const override {
        return PlayerNpcManager::getInstance().get(id, out);
    }
    bool isIdUsed(int64_t id) const override { return PlayerNpcManager::getInstance().isIdUsed(id); }
    std::vector<int64_t> getAllIds() const override {
        return PlayerNpcManager::getInstance().getAllIds();
    }

    bool setPosition(int64_t id, float x, float y, float z, int dim) override {
        return PlayerNpcManager::getInstance().setPosition(id, x, y, z, dim);
    }
    bool setRotation(int64_t id, float yaw) override {
        return PlayerNpcManager::getInstance().setRotation(id, yaw);
    }
    bool setNametag(int64_t id, std::string const& text) override {
        return PlayerNpcManager::getInstance().setNametag(id, text);
    }
    bool setSkin(int64_t id, std::string const& skinId) override {
        return PlayerNpcManager::getInstance().setSkin(id, skinId);
    }
    bool setViewDistance(int64_t id, double dist) override {
        return PlayerNpcManager::getInstance().setViewDistance(id, dist);
    }
    bool setEnabled(int64_t id, bool enabled) override {
        return PlayerNpcManager::getInstance().setEnabled(id, enabled);
    }

    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) override {
        return PlayerNpcManager::getInstance().setVisiblePlayers(id, playerNames);
    }
    bool clearVisiblePlayers(int64_t id) override {
        return PlayerNpcManager::getInstance().clearVisiblePlayers(id);
    }
    bool setVisiblePlayer(int64_t id, std::string const& playerName) override {
        return PlayerNpcManager::getInstance().setVisiblePlayer(id, playerName);
    }

    std::string getDebugInfo(int64_t id) const override {
        return PlayerNpcManager::getInstance().getDebugInfo(id);
    }
};

hologramlib::IPlayerNpc& playerNpcAdapter() {
    static PlayerNpcAdapter adapter;
    return adapter;
}

// ── LSE 导出 ──

void PlayerNpcExporter::exportAll() {
    auto& mgr = PlayerNpcManager::getInstance();

    // 皮肤注册表
    // playerNpcRegisterSkin(pngPath, skinId, geometry, armSize) -> bool
    // （skinId 空 = 用文件名; geometry 默认 geometry.humanoid.custom; armSize: wide/slim）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcRegisterSkin",
        [&mgr](std::string const& pngPath, std::string const& skinId, std::string const& geometry, std::string const& armSize) -> bool {
            hologramlib::PlayerNpcSkin skin;
            skin.pngPath = pngPath;
            skin.skinId  = skinId;
            if (!geometry.empty()) skin.geometry = geometry;
            if (!armSize.empty()) skin.armSize = armSize;
            return mgr.registerSkin(skin);
        });

    // playerNpcCaptureSkin(skinId, playerName) -> bool（从在线玩家采集, 永久注册）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCaptureSkin",
        [&mgr](std::string const& skinId, std::string const& playerName) -> bool {
            return mgr.captureSkin(skinId, playerName);
        });

    // playerNpcHasSkin(skinId) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcHasSkin", [&mgr](std::string const& skinId) -> bool { return mgr.hasSkin(skinId); });

    // playerNpcUnregisterSkin(skinId) -> bool（有 NPC 引用时拒绝）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcUnregisterSkin",
        [&mgr](std::string const& skinId) -> bool { return mgr.unregisterSkin(skinId); });

    // playerNpcGetSkinIds() -> [string]
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcGetSkinIds", [&mgr]() -> std::vector<std::string> { return mgr.getSkinIds(); });

    // 创建/生命周期
    // playerNpcCreate(x, y, z, dim, name, skinId) -> int64（id; <0 失败: -2 id 占用, -3 皮肤未注册）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCreate",
        [&mgr](float x, float y, float z, int dim, std::string const& name, std::string const& skinId) -> int64_t {
            PlayerNpcConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            if (!name.empty()) cfg.name = name;
            if (!skinId.empty()) cfg.skinId = skinId;
            return mgr.create(cfg);
        });

    // playerNpcCreateRandom(x, y, z, dim, name, skinId) -> int64（随机段 ID）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCreateRandom",
        [&mgr](float x, float y, float z, int dim, std::string const& name, std::string const& skinId) -> int64_t {
            PlayerNpcConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            if (!name.empty()) cfg.name = name;
            if (!skinId.empty()) cfg.skinId = skinId;
            return mgr.createRandom(cfg);
        });

    // playerNpcCreateWithId(x, y, z, dim, name, skinId, desiredId) -> int64（持久化恢复用）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCreateWithId",
        [&mgr](float               x,
               float               y,
               float               z,
               int                 dim,
               std::string const&  name,
               std::string const&  skinId,
               int64_t             desiredId) -> int64_t {
            PlayerNpcConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            if (!name.empty()) cfg.name = name;
            if (!skinId.empty()) cfg.skinId = skinId;
            return mgr.createWithId(cfg, desiredId);
        });

    // playerNpcDestroy(id) -> bool / playerNpcDestroyAll() / playerNpcExists(id) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcDestroy", [&mgr](int64_t id) -> bool { return mgr.destroy(id); });
    hologramlib::lse::exportAs(NAMESPACE, "playerNpcDestroyAll", [&mgr]() -> void { mgr.destroyAll(); });
    hologramlib::lse::exportAs(NAMESPACE, "playerNpcExists", [&mgr](int64_t id) -> bool { return mgr.exists(id); });
    hologramlib::lse::exportAs(NAMESPACE, "playerNpcIsIdUsed", [&mgr](int64_t id) -> bool { return mgr.isIdUsed(id); });
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcGetAllIds", [&mgr]() -> std::vector<int64_t> { return mgr.getAllIds(); });

    // 属性
    // playerNpcSetPos(id, x, y, z, dim) -> bool（dim<0 仅改坐标不改维度）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetPos",
        [&mgr](int64_t id, float x, float y, float z, int dim) -> bool { return mgr.setPosition(id, x, y, z, dim); });

    // playerNpcSetRotation(id, yaw) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetRotation", [&mgr](int64_t id, float yaw) -> bool { return mgr.setRotation(id, yaw); });

    // playerNpcSetNametag(id, text) -> bool（空串清除）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetNametag",
        [&mgr](int64_t id, std::string const& text) -> bool { return mgr.setNametag(id, text); });

    // playerNpcSetSkin(id, skinId) -> bool（换肤 = respawn, 协议限制有一次重入）
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetSkin", [&mgr](int64_t id, std::string const& skinId) -> bool {
            return mgr.setSkin(id, skinId);
        });

    // playerNpcSetViewDistance(id, dist) -> bool（<=0 无限制）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetViewDistance",
        [&mgr](int64_t id, double dist) -> bool { return mgr.setViewDistance(id, dist); });

    // playerNpcSetEnabled(id, enabled) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetEnabled", [&mgr](int64_t id, bool enabled) -> bool { return mgr.setEnabled(id, enabled); });

    // 可见玩家白名单
    // playerNpcSetVisiblePlayers(id, [names]) -> bool（空列表 = 清除限制）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetVisiblePlayers",
        [&mgr](int64_t id, std::vector<std::string> playerNames) -> bool {
            return mgr.setVisiblePlayers(id, playerNames);
        });
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcClearVisiblePlayers", [&mgr](int64_t id) -> bool { return mgr.clearVisiblePlayers(id); });
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetVisiblePlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool { return mgr.setVisiblePlayer(id, playerName); });

    // 诊断
    // playerNpcGetDebugInfo(id) -> string
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcGetDebugInfo", [&mgr](int64_t id) -> std::string { return mgr.getDebugInfo(id); });
}

} // namespace debugshape_export
