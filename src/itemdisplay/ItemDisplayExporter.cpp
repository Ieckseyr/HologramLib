#include "ItemDisplayExporter.h"
#include "ItemDisplayManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void ItemDisplayExporter::exportAll() {
    auto& mgr = ItemDisplayManager::getInstance();

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayCreate",
                               [&mgr](float x, float y, float z, int dim, std::string const& item, int aux) -> int64_t {
                                   ItemDisplayConfig cfg;
                                   cfg.x         = x;
                                   cfg.y         = y;
                                   cfg.z         = z;
                                   cfg.dimension = dim;
                                   cfg.item      = item;
                                   cfg.itemAux   = aux;
                                   return mgr.create(cfg);
                               });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateAdvanced",
        [&mgr](float              x,
               float              y,
               float              z,
               int                dim,
               std::string const& item,
               int                aux,
               std::string const& offX,
               std::string const& offY,
               std::string const& offZ,
               std::string const& rotX,
               std::string const& rotY,
               std::string const& rotZ,
               std::string const& scale) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            cfg.offsetX   = offX;
            cfg.offsetY   = offY;
            cfg.offsetZ   = offZ;
            cfg.rotX      = rotX;
            cfg.rotY      = rotY;
            cfg.rotZ      = rotZ;
            cfg.scale     = scale;
            return mgr.create(cfg);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayEmitForEntity",
        [&mgr](std::string const& playerName,
               int64_t            uniqueId,
               std::string const& item,
               int                aux,
               std::string const& offsetY,
               std::string const& rotX,
               std::string const& rotY,
               std::string const& rotZ,
               std::string const& scale,
               std::string const& nbt,
               bool               glint) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.item      = item;
            cfg.itemAux   = aux;
            cfg.offsetY   = offsetY;
            cfg.rotX      = rotX;
            cfg.rotY      = rotY;
            cfg.rotZ      = rotZ;
            cfg.scale     = scale;
            cfg.itemNbt   = nbt;
            cfg.itemGlint = glint;
            return mgr.emitForEntity(playerName, uniqueId, cfg);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateFull",
        [&mgr](float x, float y, float z, int dim, std::string const& item, int aux,
               std::string const& offX, std::string const& offY, std::string const& offZ,
               std::string const& rotX, std::string const& rotY, std::string const& rotZ,
               std::string const& scale, std::string const& nbt, bool glint) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            cfg.offsetX   = offX;
            cfg.offsetY   = offY;
            cfg.offsetZ   = offZ;
            cfg.rotX      = rotX;
            cfg.rotY      = rotY;
            cfg.rotZ      = rotZ;
            cfg.scale     = scale;
            cfg.itemNbt   = nbt;
            cfg.itemGlint = glint;
            return mgr.create(cfg);
        });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayDestroy",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.destroy(id);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayDestroyAll",
                               [&mgr]() -> void {
                                   mgr.destroyAll();
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayRuntimeId", [&mgr](int64_t id) -> int64_t {
        auto rid = mgr.runtimeIdOf(id);
        return rid == 0 ? -1 : static_cast<int64_t>(rid);
    });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayExists",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.exists(id);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayGetAllIds",
                               [&mgr]() -> std::vector<int64_t> {
                                   return mgr.getAllIds();
                               });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateRandom",
        [&mgr](float x, float y, float z, int dim, std::string const& item, int aux) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            return mgr.createRandom(cfg);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateWithId",
        [&mgr](float x, float y, float z, int dim, std::string const& item, int aux, int64_t desiredId)
            -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            return mgr.createWithId(cfg, desiredId);
        });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayIsIdUsed",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.isIdUsed(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayScaleBy",
                               [&mgr](int64_t id, float factor) -> bool {
                                   return mgr.scaleBy(id, factor);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetItemWithNbt",
                               [&mgr](int64_t id, std::string const& item, int aux, std::string const& nbt) -> bool {
                                   return mgr.setItemWithNbt(id, item, aux, nbt);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetGlint",
                               [&mgr](int64_t id, bool on) -> bool {
                                   return mgr.setGlint(id, on);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetVisiblePlayers",
                               [&mgr](int64_t id, std::vector<std::string> playerNames) -> bool {
                                   return mgr.setVisiblePlayers(id, playerNames);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayClearVisiblePlayers",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.clearVisiblePlayers(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetVisiblePlayer",
                               [&mgr](int64_t id, std::string const& playerName) -> bool {
                                   return mgr.setVisiblePlayer(id, playerName);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayGetInfo",
                               [&mgr](int64_t id) -> std::string {
                                   return mgr.getDebugInfo(id);
                               });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayScaleTo",
        [&mgr](int64_t id, double targetScale) -> bool {
            return mgr.scaleTo(id, targetScale);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplayCreateSeamless",
        [&mgr](float              x,
               float              y,
               float              z,
               int                dim,
               std::string const& item,
               int                aux,
               std::string const& offX,
               std::string const& offY,
               std::string const& offZ,
               std::string const& rotX,
               std::string const& rotY,
               std::string const& rotZ,
               std::string const& scale,
               int                mode,
               double             viewDistance,
               std::string const& visiblePlayer) -> int64_t {
            ItemDisplayConfig cfg;
            cfg.x         = x;
            cfg.y         = y;
            cfg.z         = z;
            cfg.dimension = dim;
            cfg.item      = item;
            cfg.itemAux   = aux;
            cfg.offsetX   = offX;
            cfg.offsetY   = offY;
            cfg.offsetZ   = offZ;
            cfg.rotX      = rotX;
            cfg.rotY      = rotY;
            cfg.rotZ      = rotZ;
            cfg.scale     = scale;
            return mgr.createSeamless(cfg, mode, viewDistance, visiblePlayer);
        });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetItem",
                               [&mgr](int64_t id, std::string const& item, int aux) -> bool {
                                   return mgr.setItem(id, item, aux);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetPosition",
                               [&mgr](int64_t id, float x, float y, float z, int dim) -> bool {
                                   return mgr.setPosition(id, x, y, z, dim);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetOffset",
                               [&mgr](int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) -> bool {
                                   return mgr.setOffset(id, ox, oy, oz);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetBaseOffset",
                               [&mgr](int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) -> bool {
                                   return mgr.setBaseOffset(id, ox, oy, oz);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetRotation",
                               [&mgr](int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) -> bool {
                                   return mgr.setRotation(id, rx, ry, rz);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetScale",
                               [&mgr](int64_t id, std::string const& scale) -> bool {
                                   return mgr.setScale(id, scale);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetExtend",
                               [&mgr](int64_t id, std::string const& scale, std::string const& rx, std::string const& ry) -> bool {
                                   return mgr.setExtend(id, scale, rx, ry);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetMode",
                               [&mgr](int64_t id, int mode) -> bool {
                                   return mgr.setMode(id, mode);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetEnabled",
                               [&mgr](int64_t id, bool enabled) -> bool {
                                   return mgr.setEnabled(id, enabled);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetViewDistance",
                               [&mgr](int64_t id, float dist) -> bool {
                                   return mgr.setViewDistance(id, dist);
                               });
    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayRotateY",
                               [&mgr](int64_t id, float delta) -> bool {
                                   return mgr.rotateY(id, delta);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayFindNearest",
                               [&mgr](float x, float y, float z, int dim, float maxDist) -> int64_t {
                                   return mgr.findNearest(x, y, z, dim, maxDist);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayFollow",
                               [&mgr](int64_t id, std::string const& playerName, float offX, float offY, float offZ) -> bool {
                                   return mgr.follow(id, playerName, offX, offY, offZ);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplayUnfollow",
                               [&mgr](int64_t id) -> bool {
                                   return mgr.unfollow(id);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetHitbox",
                               [&mgr](int64_t id, float width, float height) -> bool {
                                   return mgr.setHitbox(id, width, height);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetStretch",
                               [&mgr](int64_t id, std::string const& xz, std::string const& y) -> bool {
                                   return mgr.setStretch(id, xz, y);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetStretchLive",
                               [&mgr](int64_t id, std::string const& xz, std::string const& y) -> bool {
                                   return mgr.setStretchLive(id, xz, y);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetRotationLive",
                               [&mgr](int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) -> bool {
                                   return mgr.setRotationLive(id, rx, ry, rz);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetOffsetLive",
                               [&mgr](int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) -> bool {
                                   return mgr.setOffsetLive(id, ox, oy, oz);
                               });

    hologramlib::lse::exportAs(NAMESPACE, "itemDisplaySetScaleLive",
                               [&mgr](int64_t id, std::string const& scale) -> bool {
                                   return mgr.setScaleLive(id, scale);
                               });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplaySetTransformLive",
        [&mgr](int64_t            id,
               std::string const& ox, std::string const& oy, std::string const& oz,
               std::string const& rx, std::string const& ry, std::string const& rz,
               std::string const& scale) -> bool {
            return mgr.setTransformLive(id, ox, oy, oz, rx, ry, rz, scale);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplaySetAnim",
        [&mgr](int64_t id,
               float spinSpeed, int spinAxis,
               float bobAmp, float bobPeriod,
               float pulseAmp, float pulsePeriod,
               float swayAmp, float swayPeriod,
               float phase, bool billboard) -> bool {
            ItemDisplayAnim anim;
            anim.spinSpeed      = spinSpeed;
            anim.spinAxis       = spinAxis;
            anim.bobAmplitude   = bobAmp;
            anim.bobPeriod      = bobPeriod;
            anim.pulseAmplitude = pulseAmp;
            anim.pulsePeriod    = pulsePeriod;
            anim.swayAmplitude  = swayAmp;
            anim.swayPeriod     = swayPeriod;
            anim.phase          = phase;
            anim.billboard      = billboard;
            return mgr.setAnim(id, anim);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "itemDisplayClearAnim", [&mgr](int64_t id) -> bool {
            return mgr.clearAnim(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemDisplaySetPreset",
        [&mgr](int64_t id, std::string const& preset, float a, float b, int c) -> bool {
            return mgr.setAnimPreset(id, preset, a, b, c);
        });
}

} // namespace debugshape_export
