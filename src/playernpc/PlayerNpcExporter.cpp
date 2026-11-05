#include "PlayerNpcExporter.h"
#include "PlayerNpcManager.h"

#include "lse/LseBridge.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

std::string b64Encode(std::string const& raw) {
    static constexpr char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string           out;
    out.reserve(((raw.size() + 2) / 3) * 4);
    std::size_t i = 0;
    for (; i + 2 < raw.size(); i += 3) {
        std::uint32_t const v = (static_cast<std::uint8_t>(raw[i]) << 16) | (static_cast<std::uint8_t>(raw[i + 1]) << 8) | static_cast<std::uint8_t>(raw[i + 2]);
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
        std::uint32_t const v = (static_cast<std::uint8_t>(raw[i]) << 16) | (static_cast<std::uint8_t>(raw[i + 1]) << 8);
        out += kTable[(v >> 18) & 63];
        out += kTable[(v >> 12) & 63];
        out += kTable[(v >> 6) & 63];
        out += '=';
    }
    return out;
}

bool b64Decode(std::string const& text, std::string& out) {
    auto const val = [](char c) -> int {
        if (c >= 'A' && c <= 'Z')
            return c - 'A';
        if (c >= 'a' && c <= 'z')
            return c - 'a' + 26;
        if (c >= '0' && c <= '9')
            return c - '0' + 52;
        if (c == '+')
            return 62;
        if (c == '/')
            return 63;
        return -1;
    };
    out.clear();
    out.reserve(text.size() / 4 * 3);
    std::uint32_t buf  = 0;
    int           bits = 0;
    for (char const c : text) {
        if (c == '=' || c == '\n' || c == '\r' || c == ' ' || c == '\t')
            continue;
        int const v = val(c);
        if (v < 0)
            return false;
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
    bool                 isIdUsed(int64_t id) const override { return PlayerNpcManager::getInstance().isIdUsed(id); }
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

    bool setPositionLight(int64_t id, float x, float y, float z, int dim) override {
        return PlayerNpcManager::getInstance().setPositionLight(id, x, y, z, dim);
    }
    bool injectSkin(std::string const& viewerName, std::string const& targetName, std::string const& skinId) override {
        return PlayerNpcManager::getInstance().injectSkin(viewerName, targetName, skinId);
    }
    bool injectSkinAll(std::string const& targetName, std::string const& skinId) override {
        return PlayerNpcManager::getInstance().injectSkinAll(targetName, skinId);
    }

    bool playAnimation(int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks)
        override {
        return PlayerNpcManager::getInstance().playAnimation(id, animation, stopExpression, durationTicks);
    }
    bool playAnimationTo(
        int64_t            id,
        std::string const& playerName,
        std::string const& animation,
        std::string const& stopExpression,
        int                durationTicks) override {
        return PlayerNpcManager::getInstance().playAnimationTo(id, playerName, animation, stopExpression, durationTicks);
    }
    void setEntitySpawnCallback(std::function<void(int64_t, std::string const&)> callback) override {
        PlayerNpcManager::getInstance().setEntitySpawnCallback(std::move(callback));
    }

    bool stopAnimation(int64_t id) override { return PlayerNpcManager::getInstance().stopAnimation(id); }
    bool stopAnimationTo(int64_t id, std::string const& playerName) override {
        return PlayerNpcManager::getInstance().stopAnimationTo(id, playerName);
    }

    bool setPose(int64_t id, int pose) override { return PlayerNpcManager::getInstance().setPose(id, pose); }
    bool setEquipment(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) override {
        return PlayerNpcManager::getInstance().setEquipment(id, slot, name, aux, nbt);
    }
    bool setRideUniqueId(int64_t id, std::int64_t vehicleUniqueId) override {
        return PlayerNpcManager::getInstance().setRideUniqueId(id, vehicleUniqueId);
    }
    bool setRidePlayer(int64_t id, std::string const& playerName) override {
        return PlayerNpcManager::getInstance().setRidePlayer(id, playerName);
    }
    bool setRideEntity(int64_t id, int64_t vehicleEntityId) override {
        return PlayerNpcManager::getInstance().setRideEntity(id, vehicleEntityId);
    }
    bool clearRide(int64_t id) override { return PlayerNpcManager::getInstance().clearRide(id); }
    bool playEmote(int64_t id, std::string const& pieceId, int lengthTicks) override {
        return PlayerNpcManager::getInstance().playEmote(id, pieceId, lengthTicks);
    }
};

hologramlib::IPlayerNpc& playerNpcAdapter() {
    static PlayerNpcAdapter adapter;
    return adapter;
}

void PlayerNpcExporter::exportAll() {
    auto& mgr = PlayerNpcManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcRegisterSkin",
        [&mgr](std::string const& pngPath, std::string const& skinId, std::string const& geometry, std::string const& armSize) -> bool {
            hologramlib::PlayerNpcSkin skin;
            skin.pngPath = pngPath;
            skin.skinId  = skinId;
            if (!geometry.empty())
                skin.geometry = geometry;
            if (!armSize.empty())
                skin.armSize = armSize;
            return mgr.registerSkin(skin);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcRegisterSkinData",
        [&mgr](std::string const& pngPath,
               std::string const& skinId,
               std::string const& armSize,
               std::string const& geometryData) -> bool {
            hologramlib::PlayerNpcSkin skin;
            skin.pngPath      = pngPath;
            skin.skinId       = skinId;
            skin.armSize      = armSize.empty() ? "wide" : armSize;
            skin.geometryData = geometryData;
            return mgr.registerSkin(skin);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcRegisterSkinDataOpaque",
        [&mgr](std::string const& pngPath,
               std::string const& skinId,
               std::string const& armSize,
               std::string const& geometryData) -> bool {
            hologramlib::PlayerNpcSkin skin;
            skin.pngPath      = pngPath;
            skin.skinId       = skinId;
            skin.armSize      = armSize.empty() ? "wide" : armSize;
            skin.geometryData = geometryData;
            return mgr.registerSkinOpaque(skin);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcRegisterImageSkin",
        [&mgr](std::string const& pngPath, std::string const& skinId, float sizeBlocks, bool transparent) -> bool {
            return mgr.registerImageSkin(pngPath, skinId, sizeBlocks, transparent);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCaptureSkin",
        [&mgr](std::string const& skinId, std::string const& playerName) -> bool {
            return mgr.captureSkin(skinId, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcImportSkins",
        [&mgr](std::string const& dirPath) -> int {
            return mgr.importSkins(dirPath);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcHasSkin", [&mgr](std::string const& skinId) -> bool {
            return mgr.hasSkin(skinId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcUnregisterSkin",
        [&mgr](std::string const& skinId) -> bool {
            return mgr.unregisterSkin(skinId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcGetSkinIds", [&mgr]() -> std::vector<std::string> {
            return mgr.getSkinIds();
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCreate",
        [&mgr](float x, float y, float z, int dim, std::string const& name, std::string const& skinId) -> int64_t {
            PlayerNpcConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            if (!name.empty())
                cfg.name = name;
            if (!skinId.empty())
                cfg.skinId = skinId;
            return mgr.create(cfg);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCreateEx",
        [&mgr](float              x,
               float              y,
               float              z,
               int                dim,
               std::string const& name,
               std::string const& skinId,
               float              yaw,
               float              scale) -> int64_t {
            PlayerNpcConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            if (!name.empty())
                cfg.name = name;
            if (!skinId.empty())
                cfg.skinId = skinId;
            cfg.yaw = yaw;
            if (scale > 0)
                cfg.scale = std::clamp(scale, 0.0625f, 10.0f);
            return mgr.create(cfg);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCreateRandom",
        [&mgr](float x, float y, float z, int dim, std::string const& name, std::string const& skinId) -> int64_t {
            PlayerNpcConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            if (!name.empty())
                cfg.name = name;
            if (!skinId.empty())
                cfg.skinId = skinId;
            return mgr.createRandom(cfg);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcCreateWithId",
        [&mgr](float              x,
               float              y,
               float              z,
               int                dim,
               std::string const& name,
               std::string const& skinId,
               int64_t            desiredId) -> int64_t {
            PlayerNpcConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            if (!name.empty())
                cfg.name = name;
            if (!skinId.empty())
                cfg.skinId = skinId;
            return mgr.createWithId(cfg, desiredId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcDestroy", [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });
    hologramlib::lse::exportAs(NAMESPACE, "playerNpcDestroyAll", [&mgr]() -> void {
        mgr.destroyAll();
    });
    hologramlib::lse::exportAs(NAMESPACE, "playerNpcExists", [&mgr](int64_t id) -> bool {
        return mgr.exists(id);
    });
    hologramlib::lse::exportAs(NAMESPACE, "playerNpcIsIdUsed", [&mgr](int64_t id) -> bool {
        return mgr.isIdUsed(id);
    });
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcGetAllIds", [&mgr]() -> std::vector<int64_t> {
            return mgr.getAllIds();
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetPos",
        [&mgr](int64_t id, float x, float y, float z, int dim) -> bool {
            return mgr.setPosition(id, x, y, z, dim);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetRotation", [&mgr](int64_t id, float yaw) -> bool {
            return mgr.setRotation(id, yaw);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetNametag",
        [&mgr](int64_t id, std::string const& text) -> bool {
            return mgr.setNametag(id, text);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetSkin", [&mgr](int64_t id, std::string const& skinId) -> bool {
            return mgr.setSkin(id, skinId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetViewDistance",
        [&mgr](int64_t id, double dist) -> bool {
            return mgr.setViewDistance(id, dist);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetScale", [&mgr](int64_t id, float scale) -> bool {
            return mgr.setScale(id, scale);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetEnabled", [&mgr](int64_t id, bool enabled) -> bool {
            return mgr.setEnabled(id, enabled);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetVisiblePlayers",
        [&mgr](int64_t id, std::vector<std::string> playerNames) -> bool {
            return mgr.setVisiblePlayers(id, playerNames);
        });
    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcClearVisiblePlayers", [&mgr](int64_t id) -> bool {
            return mgr.clearVisiblePlayers(id);
        });
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetVisiblePlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.setVisiblePlayer(id, playerName);
        });

    hologramlib::lse::exportAs(NAMESPACE, "playerNpcRuntimeId", [&mgr](int64_t id) -> int64_t {
        auto rid = mgr.runtimeIdOf(id);
        return rid == 0 ? -1 : static_cast<int64_t>(rid);
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcGetDebugInfo", [&mgr](int64_t id) -> std::string {
            return mgr.getDebugInfo(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcInjectSkin",
        [&mgr](std::string const& viewerName, std::string const& targetName, std::string const& skinId) -> bool {
            return mgr.injectSkin(viewerName, targetName, skinId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcInjectSkinAll", [&mgr](std::string const& targetName, std::string const& skinId) -> bool {
            return mgr.injectSkinAll(targetName, skinId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcGetSkinBlobB64", [&mgr](std::string const& skinId) -> std::string {
            std::string blob;
            if (!mgr.getSkinBlob(skinId, blob))
                return {};
            return b64Encode(blob);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcRegisterSkinFromBlobB64", [&mgr](std::string const& b64) -> bool {
            std::string blob;
            if (!b64Decode(b64, blob) || blob.empty())
                return false;
            return mgr.registerSkinFromBlob(blob);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcRegisterSkinFromBlobFile", [&mgr](std::string const& path) -> bool {
            std::ifstream f(path, std::ios::binary);
            if (!f)
                return false;
            std::string blob((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            if (blob.empty())
                return false;
            return mgr.registerSkinFromBlob(blob);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcPlayAnimation",
        [&mgr](int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks)
            -> bool {
            return mgr.playAnimation(id, animation, stopExpression, durationTicks);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcStopAnimation", [&mgr](int64_t id) -> bool {
            return mgr.stopAnimation(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcStopAnimationTo",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.stopAnimationTo(id, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetPose", [&mgr](int64_t id, int pose) -> bool {
            return mgr.setPose(id, pose);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetEquipment",
        [&mgr](int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) -> bool {
            return mgr.setEquipment(id, slot, name, aux, nbt);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetRideUniqueId",
        [&mgr](int64_t id, int64_t vehicleUniqueId) -> bool {
            return mgr.setRideUniqueId(id, vehicleUniqueId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetRidePlayer",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.setRidePlayer(id, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetRideEntity",
        [&mgr](int64_t id, int64_t vehicleEntityId) -> bool {
            return mgr.setRideEntity(id, vehicleEntityId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcClearRide", [&mgr](int64_t id) -> bool {
            return mgr.clearRide(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetPosLight",
        [&mgr](int64_t id, float x, float y, float z, int dim) -> bool {
            return mgr.setPositionLight(id, x, y, z, dim);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcSetRotLight", [&mgr](int64_t id, float yaw) -> bool {
            return mgr.setRotationLight(id, yaw);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcSetPlayerRotation",
        [&mgr](int64_t id, std::string const& playerName, float yaw) -> bool {
            return mgr.setPlayerRotation(id, playerName, yaw);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcClearPlayerRotation",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.clearPlayerRotation(id, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerNpcClearPlayerRotations", [&mgr](int64_t id) -> bool {
            return mgr.clearPlayerRotations(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcPlayAnimationTo",
        [&mgr](
            int64_t            id,
            std::string const& playerName,
            std::string const& animation,
            std::string const& stopExpression,
            int                durationTicks) -> bool {
            return mgr.playAnimationTo(id, playerName, animation, stopExpression, durationTicks);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerNpcPlayEmote",
        [&mgr](int64_t id, std::string const& pieceId, int lengthTicks) -> bool {
            return mgr.playEmote(id, pieceId, lengthTicks);
        });

    hologramlib::lse::exportAs(NAMESPACE, "playerNpcPollSpawns", [&mgr]() -> std::vector<std::string> {
        return mgr.pollSpawns();
    });

    hologramlib::lse::exportAs(NAMESPACE, "playerNpcClearSpawns", [&mgr]() -> void {
        mgr.clearSpawns();
    });
}

} // namespace debugshape_export
