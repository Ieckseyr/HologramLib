#include "SulfurDisplayExporter.h"
#include "SulfurDisplayManager.h"

#include "lse/LseBridge.h"

#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void SulfurDisplayExporter::exportAll() {
    auto& mgr = SulfurDisplayManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "sulfurCreate",
        [&mgr](
            float              x,
            float              y,
            float              z,
            int                dim,
            std::string const& blockType,
            int                blockCount,
            int                blockDamage,
            std::string const& blockName,
            std::string const& archetype,
            bool               invisible) -> int64_t {
            hologramlib::SulfurDisplaySpec spec;
            spec.x            = x;
            spec.y            = y;
            spec.z            = z;
            spec.dim          = dim;
            spec.block.type   = blockType;
            spec.block.count  = blockCount < 1 ? 1 : blockCount;
            spec.block.damage = blockDamage;
            spec.block.name   = blockName;
            spec.archetype    = archetype;
            spec.invisible    = invisible;
            return mgr.create(spec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "sulfurSetBlock",
        [&mgr](
            int64_t            id,
            std::string const& type,
            int                count,
            int                damage,
            std::string const& name) -> bool {
            hologramlib::ContainerMenuItem item;
            item.type   = type;
            item.count  = count < 1 ? 1 : count;
            item.damage = damage;
            item.name   = name;
            return mgr.setBlock(id, item);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurSetArchetype", [&mgr](int64_t id, std::string const& archetype) -> bool {
            return mgr.setArchetype(id, archetype);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurSetInvisible", [&mgr](int64_t id, bool on) -> bool {
            return mgr.setInvisible(id, on);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurSetScale", [&mgr](int64_t id, float scale) -> bool {
            return mgr.setScale(id, scale);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurDestroy", [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });
    hologramlib::lse::exportAs(NAMESPACE, "sulfurDestroyAll", [&mgr]() -> void {
        mgr.destroyAll();
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurExists", [&mgr](int64_t id) -> bool {
            return mgr.exists(id);
        });
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurGetIds", [&mgr]() -> std::vector<int64_t> {
            return mgr.getAllIds();
        });
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurGetEntityId", [&mgr](int64_t id) -> int64_t {
            return mgr.entityIdOf(id);
        });
}

} // namespace debugshape_export
