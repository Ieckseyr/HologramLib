#include "ItemActorExporter.h"
#include "ItemActorManager.h"

#include "hologramlib/HologramLib.h"
#include "lse/LseBridge.h"

#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void ItemActorExporter::exportAll() {
    auto& mgr = ItemActorManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemActorSpawn",
        [&mgr](
            std::string const& itemId,
            int                aux,
            int                count,
            float              x,
            float              y,
            float              z,
            int                dim,
            float              vx,
            float              vy,
            float              vz) -> int64_t {
            hologramlib::ItemActorSpec spec;
            spec.itemId    = itemId;
            spec.aux       = aux;
            spec.count     = count;
            spec.x         = x;
            spec.y         = y;
            spec.z         = z;
            spec.dimension = dim;
            spec.vx        = vx;
            spec.vy        = vy;
            spec.vz        = vz;
            return mgr.spawn(spec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemActorSetMotion",
        [&mgr](int64_t id, float vx, float vy, float vz) -> bool {
            return mgr.setMotion(id, vx, vy, vz);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "itemActorMoveTo", [&mgr](int64_t id, float x, float y, float z) -> bool {
            return mgr.moveTo(id, x, y, z);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "itemActorTake", [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.take(id, playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "itemActorDestroy", [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });

    hologramlib::lse::exportAs(NAMESPACE, "itemActorDestroyAll", [&mgr]() -> void {
        mgr.destroyAll();
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "itemActorExists", [&mgr](int64_t id) -> bool {
            return mgr.exists(id);
        });

    hologramlib::lse::exportAs(NAMESPACE, "itemActorRuntimeId", [&mgr](int64_t id) -> int64_t {
        auto rid = mgr.runtimeIdOf(id);
        return rid == 0 ? -1 : static_cast<int64_t>(rid);
    });

    hologramlib::lse::exportAs(NAMESPACE, "itemActorGetIds", [&mgr]() -> std::vector<int64_t> {
        return mgr.getAllIds();
    });
}

namespace {

class ItemActorAdapter final : public hologramlib::IItemActor {
public:
    int64_t spawn(hologramlib::ItemActorSpec const& spec) override {
        return ItemActorManager::getInstance().spawn(spec);
    }
    bool setMotion(int64_t id, float vx, float vy, float vz) override {
        return ItemActorManager::getInstance().setMotion(id, vx, vy, vz);
    }
    bool moveTo(int64_t id, float x, float y, float z) override {
        return ItemActorManager::getInstance().moveTo(id, x, y, z);
    }
    bool take(int64_t id, std::string const& playerName) override {
        return ItemActorManager::getInstance().take(id, playerName);
    }
    bool                               destroy(int64_t id) override { return ItemActorManager::getInstance().destroy(id); }
    void                               destroyAll() override { ItemActorManager::getInstance().destroyAll(); }
    [[nodiscard]] bool                 exists(int64_t id) const override { return ItemActorManager::getInstance().exists(id); }
    [[nodiscard]] std::vector<int64_t> getAllIds() const override {
        return ItemActorManager::getInstance().getAllIds();
    }
};

} // namespace

hologramlib::IItemActor& itemActorAdapter() {
    static ItemActorAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
