#include "CustomEntityExporter.h"
#include "CustomEntityManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void CustomEntityExporter::exportAll() {
    auto& mgr = CustomEntityManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityCreate",
        [&mgr](std::string const& identifier, float x, float y, float z, int dim) -> int64_t {
            CustomEntityConfig cfg;
            cfg.identifier = identifier;
            cfg.x          = x;
            cfg.y          = y;
            cfg.z          = z;
            cfg.dimension  = dim;
            return mgr.create(cfg);
        });

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
            std::string const& nametag) -> int64_t {
            CustomEntityConfig cfg;
            cfg.identifier = identifier;
            cfg.x          = x;
            cfg.y          = y;
            cfg.z          = z;
            cfg.dimension  = dim;
            cfg.yaw        = yaw;
            cfg.pitch      = pitch;
            cfg.scale      = scale;
            cfg.nametag    = nametag;
            return mgr.create(cfg);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityCreateRandom",
        [&mgr](std::string const& identifier, float x, float y, float z, int dim) -> int64_t {
            CustomEntityConfig cfg;
            cfg.identifier = identifier;
            cfg.x          = x;
            cfg.y          = y;
            cfg.z          = z;
            cfg.dimension  = dim;
            return mgr.createRandom(cfg);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityCreateWithId",
        [&mgr](std::string const& identifier, float x, float y, float z, int dim, int64_t desiredId) -> int64_t {
            CustomEntityConfig cfg;
            cfg.identifier = identifier;
            cfg.x          = x;
            cfg.y          = y;
            cfg.z          = z;
            cfg.dimension  = dim;
            return mgr.createWithId(cfg, desiredId);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "entityDestroy", [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });
    hologramlib::lse::exportAs(
        NAMESPACE, "entityDestroyAll", [&mgr]() -> void {
            mgr.destroyAll();
        });
    hologramlib::lse::exportAs(
        NAMESPACE, "entityExists", [&mgr](int64_t id) -> bool {
            return mgr.exists(id);
        });
    hologramlib::lse::exportAs(
        NAMESPACE, "entityIsIdUsed", [&mgr](int64_t id) -> bool {
            return mgr.isIdUsed(id);
        });
    hologramlib::lse::exportAs(
        NAMESPACE, "entityGetAllIds", [&mgr]() -> std::vector<int64_t> {
            return mgr.getAllIds();
        });

    hologramlib::lse::exportAs(NAMESPACE, "entitySetIdentifier",
                               [&mgr](int64_t id, std::string const& identifier) -> bool {
                                   return mgr.setIdentifier(id, identifier);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetPosition",
                               [&mgr](int64_t id, float x, float y, float z, int dim) -> bool {
                                   return mgr.setPosition(id, x, y, z, dim);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetRotation",
                               [&mgr](int64_t id, float yaw, float pitch) -> bool {
                                   return mgr.setRotation(id, yaw, pitch);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetNametag",
                               [&mgr](int64_t id, std::string const& text) -> bool {
                                   return mgr.setNametag(id, text);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetScale",
                               [&mgr](int64_t id, float scale) -> bool {
                                   return mgr.setScale(id, scale);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetVariant",
                               [&mgr](int64_t id, int variant) -> bool {
                                   return mgr.setVariant(id, variant);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetMarkVariant",
                               [&mgr](int64_t id, int markVariant) -> bool {
                                   return mgr.setMarkVariant(id, markVariant);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetColorIndex",
                               [&mgr](int64_t id, int colorIndex) -> bool {
                                   return mgr.setColorIndex(id, colorIndex);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetFlags",
                               [&mgr](int64_t id, int64_t flags) -> bool {
                                   return mgr.setFlags(id, flags);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetInvisible",
                               [&mgr](int64_t id, bool on) -> bool {
                                   return mgr.setInvisible(id, on);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetEnabled",
                               [&mgr](int64_t id, bool enabled) -> bool {
                                   return mgr.setEnabled(id, enabled);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "entitySetViewDistance",
                               [&mgr](int64_t id, float dist) -> bool {
                                   return mgr.setViewDistance(id, dist);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "entityFindNearest",
                               [&mgr](float x, float y, float z, int dim, float maxDist) -> int64_t {
                                   return mgr.findNearest(x, y, z, dim, maxDist);
                               });

    hologramlib::lse::exportAs(
        NAMESPACE, "entitySetPose", [&mgr](int64_t id, int pose) -> bool {
            return mgr.setPose(id, pose);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entitySetEquipmentSlot",
        [&mgr](int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) -> bool {
            return mgr.setEquipmentSlot(id, slot, name, aux, nbt);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "entityScaleBy", [&mgr](int64_t id, float factor) -> bool {
            return mgr.scaleBy(id, factor);
        });

    hologramlib::lse::exportAs(NAMESPACE, "entitySetVisiblePlayers",
                               [&mgr](int64_t id, std::vector<std::string> playerNames) -> bool {
                                   return mgr.setVisiblePlayers(id, playerNames);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "entityClearVisiblePlayers",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearVisiblePlayers(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "entitySetVisiblePlayer",
                               [&mgr](int64_t id, std::string const& playerName) -> bool {
                                   return mgr.setVisiblePlayer(id, playerName);
                               });

    hologramlib::lse::exportAs(
        NAMESPACE, "entityGetInfo", [&mgr](int64_t id) -> std::string {
            return mgr.getDebugInfo(id);
        });

    hologramlib::lse::exportAs(NAMESPACE, "entitySetRidePlayer",
                               [&mgr](int64_t id, std::string const& playerName) -> bool {
                                   return mgr.setRidePlayer(id, playerName);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "entitySetRideEntity",
                               [&mgr](int64_t id, int64_t vehicleEntityId) -> bool {
                                   return mgr.setRideEntity(id, vehicleEntityId);
                               });

    hologramlib::lse::exportAs(
        NAMESPACE, "entityClearRide", [&mgr](int64_t id) -> bool {
            return mgr.clearRide(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityPlayAnimation",
        [&mgr](int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks)
            -> bool {
            return mgr.playAnimation(id, animation, stopExpression, durationTicks);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "entityStopAnimation", [&mgr](int64_t id) -> bool {
            return mgr.stopAnimation(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityStopAnimationTo",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.stopAnimationTo(id, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entitySetPlayerNametag",
        [&mgr](int64_t id, std::string const& playerName, std::string const& text) -> bool {
            return mgr.setPlayerNametag(id, playerName, text);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entitySetPlayerScale",
        [&mgr](int64_t id, std::string const& playerName, float scale) -> bool {
            return mgr.setPlayerScale(id, playerName, scale);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entitySetPlayerEquipmentSlot",
        [&mgr](
            int64_t            id,
            std::string const& playerName,
            int                slot,
            std::string const& name,
            int                aux,
            std::string const& nbt) -> bool {
            return mgr.setPlayerEquipmentSlot(id, playerName, slot, name, aux, nbt);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityClearPlayerAppearance",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.clearPlayerAppearance(id, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entitySetPlayerRotation",
        [&mgr](int64_t id, std::string const& playerName, float yaw, float pitch) -> bool {
            return mgr.setPlayerRotation(id, playerName, yaw, pitch);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityClearPlayerRotation",
        [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.clearPlayerRotation(id, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "entityClearPlayerRotations", [&mgr](int64_t id) -> bool {
            return mgr.clearPlayerRotations(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "entityPlayAnimationTo",
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
        "entitySetMobProperty",
        [&mgr](int64_t id, std::string const& name, std::string const& value) -> bool {
            return mgr.setMobProperty(id, name, value);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "entityClearMobProperties", [&mgr](int64_t id) -> bool {
            return mgr.clearMobProperties(id);
        });

    hologramlib::lse::exportAs(NAMESPACE, "entityPollSpawns", [&mgr]() -> std::vector<std::string> {
        return mgr.pollSpawns();
    });

    hologramlib::lse::exportAs(NAMESPACE, "entityClearSpawns", [&mgr]() -> void {
        mgr.clearSpawns();
    });
}

} // namespace debugshape_export
