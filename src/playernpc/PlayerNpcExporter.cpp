// PlayerNpcExporter.cpp - 假玩家 NPC LSE 导出实现 + IPlayerNpc 适配器
//
// 适配器将 IPlayerNpc 接口（冻结契约）映射到 PlayerNpcManager 单例;
// LSE 导出命名对齐现有风格（itemDisplay* / customEntity* → playerNpc*）。
#include "PlayerNpcExporter.h"
#include "PlayerNpcManager.h"

#include "lse/LseBridge.h"

#include <string>
#include <vector>
#include <cstdint>

// ── 皮肤 blob 的 base64（LSE 字符串编组对二进制不安全: blob 内含 NUL / 任意字节）──
// 配对: playerNpcGetSkinBlobB64 / playerNpcRegisterSkinFromBlobB64。
// 用途: 消费方把"借来的皮肤/原皮肤"落盘（Disguise/MSkinventory 的玩家自视换肤）, 重启后源不在线也能恢复。
namespace {

std::string b64Encode(std::string const& raw) {
    static constexpr char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string           out;
    out.reserve(((raw.size() + 2) / 3) * 4);
    std::size_t i = 0;
    for (; i + 2 < raw.size(); i += 3) {
        std::uint32_t const v = (static_cast<std::uint8_t>(raw[i]) << 16)
                              | (static_cast<std::uint8_t>(raw[i + 1]) << 8)
                              | static_cast<std::uint8_t>(raw[i + 2]);
        out += kTable[(v >> 18) & 63];
        out += kTable[(v >> 12) & 63];
        out += kTable[(v >> 6) & 63];
        out += kTable[v & 63];
    }
    if (i + 1 == raw.size()) {
        std::uint32_t const v = static_cast<std::uint8_t>(raw[i]) << 16;
        out += kTable[(v >> 18) & 63];
        out += kTable[(v >> 12) & 63];
        out += "==";
    } else if (i + 2 == raw.size()) {
        std::uint32_t const v = (static_cast<std::uint8_t>(raw[i]) << 16)
                              | (static_cast<std::uint8_t>(raw[i + 1]) << 8);
        out += kTable[(v >> 18) & 63];
        out += kTable[(v >> 12) & 63];
        out += kTable[(v >> 6) & 63];
        out += '=';
    }
    return out;
}

bool b64Decode(std::string const& text, std::string& out) {
    auto const val = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    out.clear();
    out.reserve(text.size() / 4 * 3);
    std::uint32_t buf  = 0;
    int           bits = 0;
    for (char const c : text) {
        if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
        int const v = val(c);
        if (v < 0) return false;
        buf = (buf << 6) | static_cast<std::uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out += static_cast<char>((buf >> bits) & 0xFF);
        }
    }
    return true;
}

} // namespace

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
    int importSkins(std::string const& dirPath) override {
        return PlayerNpcManager::getInstance().importSkins(dirPath);
    }
    bool getSkinBlob(std::string const& skinId, std::string& out) const override {
        return PlayerNpcManager::getInstance().getSkinBlob(skinId, out);
    }
    bool registerSkinFromBlob(std::string const& blob) override {
        return PlayerNpcManager::getInstance().registerSkinFromBlob(blob);
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
    bool setScale(int64_t id, float scale) override {
        return PlayerNpcManager::getInstance().setScale(id, scale);
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

    // ── 1.20.0: 轻量朝向 / 逐客户端朝向 ──
    bool setRotationLight(int64_t id, float yaw) override {
        return PlayerNpcManager::getInstance().setRotationLight(id, yaw);
    }
    bool setPlayerRotation(int64_t id, std::string const& playerName, float yaw) override {
        return PlayerNpcManager::getInstance().setPlayerRotation(id, playerName, yaw);
    }
    bool clearPlayerRotation(int64_t id, std::string const& playerName) override {
        return PlayerNpcManager::getInstance().clearPlayerRotation(id, playerName);
    }
    bool clearPlayerRotations(int64_t id) override {
        return PlayerNpcManager::getInstance().clearPlayerRotations(id);
    }

    // ── 1.26.0: 轻量位置 / 玩家皮肤注入 ──
    bool setPositionLight(int64_t id, float x, float y, float z, int dim) override {
        return PlayerNpcManager::getInstance().setPositionLight(id, x, y, z, dim);
    }
    bool injectSkin(std::string const& viewerName, std::string const& targetName, std::string const& skinId) override {
        return PlayerNpcManager::getInstance().injectSkin(viewerName, targetName, skinId);
    }
    bool injectSkinAll(std::string const& targetName, std::string const& skinId) override {
        return PlayerNpcManager::getInstance().injectSkinAll(targetName, skinId);
    }

    // ── 1.26.0: 动画 ──
    bool playAnimation(int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks)
        override {
        return PlayerNpcManager::getInstance().playAnimation(id, animation, stopExpression, durationTicks);
    }
    bool playAnimationTo(
        int64_t id,
        std::string const& playerName,
        std::string const& animation,
        std::string const& stopExpression,
        int durationTicks
    ) override {
        return PlayerNpcManager::getInstance().playAnimationTo(id, playerName, animation, stopExpression, durationTicks);
    }
    void setEntitySpawnCallback(std::function<void(int64_t, std::string const&)> callback) override {
        PlayerNpcManager::getInstance().setEntitySpawnCallback(std::move(callback));
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

    // playerNpcImportSkins(dirPath) -> int（目录批量导入; skinId = 子文件夹名）
    // playerNpcCaptureSkin(skinId, playerName) -> bool（运行时快照注册, 换肤不影响; 库不落盘）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCaptureSkin",
        [&mgr](std::string const& skinId, std::string const& playerName) -> bool {
            return mgr.captureSkin(skinId, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcImportSkins",
        [&mgr](std::string const& dirPath) -> int { return mgr.importSkins(dirPath); });

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

    // playerNpcSetScale(id, scale) -> bool（0.0625~10 客户端硬限）
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetScale", [&mgr](int64_t id, float scale) -> bool { return mgr.setScale(id, scale); });

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

    // ── 皮肤注入 + 皮肤 blob 持久化（26.40.8 / 1.26.0; Disguise/MSkinventory 的"玩家自视换肤"用）──
    // playerNpcInjectSkin(viewerName, targetName, skinId) -> bool
    //   viewerName 空串 = 所有在线玩家（**含 target 本人** —— "自己看自己被换肤"的正路, 同 C++ 头注释）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcInjectSkin",
        [&mgr](std::string const& viewerName, std::string const& targetName, std::string const& skinId) -> bool {
            return mgr.injectSkin(viewerName, targetName, skinId);
        });
    // playerNpcInjectSkinAll(targetName, skinId) -> bool（便捷: 等价 viewerName 空串）
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcInjectSkinAll", [&mgr](std::string const& targetName, std::string const& skinId) -> bool {
            return mgr.injectSkinAll(targetName, skinId);
        });
    // playerNpcGetSkinBlobB64(skinId) -> string（全字段二进制快照的 base64; 空串 = 未注册）
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcGetSkinBlobB64", [&mgr](std::string const& skinId) -> std::string {
            std::string blob;
            if (!mgr.getSkinBlob(skinId, blob)) return {};
            return b64Encode(blob);
        });
    // playerNpcRegisterSkinFromBlobB64(b64) -> bool（与上一条配对; 重启后源玩家不在线也能恢复皮肤）
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcRegisterSkinFromBlobB64", [&mgr](std::string const& b64) -> bool {
            std::string blob;
            if (!b64Decode(b64, blob) || blob.empty()) return false;
            return mgr.registerSkinFromBlob(blob);
        });
}

} // namespace debugshape_export
