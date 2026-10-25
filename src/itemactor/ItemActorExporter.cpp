// ItemActorExporter.cpp - 协议层掉落物 LSE 导出 + 公开接口适配
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

    // itemActorSpawn(itemId, aux, count, x, y, z, dim, vx, vy, vz) -> id
    //   凭空造一只掉落物（服务端没有它）。vx/vy/vz 是初速度, vy>0 = 向上抛。
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

    // itemActorSetMotion(id, vx, vy, vz) -> bool（飞行途中改速度与方向; 0,0,0 = 停住）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "itemActorSetMotion",
        [&mgr](int64_t id, float vx, float vy, float vz) -> bool {
            return mgr.setMotion(id, vx, vy, vz);
        });

    // itemActorMoveTo(id, x, y, z) -> bool（瞬移, 不发速度）
    hologramlib::lse::exportAs(
        NAMESPACE, "itemActorMoveTo", [&mgr](int64_t id, float x, float y, float z) -> bool {
            return mgr.moveTo(id, x, y, z);
        });

    // itemActorTake(id, playerName) -> bool（播"飞向该玩家"的动画再删掉; 物品不进背包）
    hologramlib::lse::exportAs(
        NAMESPACE, "itemActorTake", [&mgr](int64_t id, std::string const& playerName) -> bool {
            return mgr.take(id, playerName);
        });

    // itemActorDestroy(id) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "itemActorDestroy", [&mgr](int64_t id) -> bool {
            return mgr.destroy(id);
        });

    // itemActorDestroyAll() -> void
    hologramlib::lse::exportAs(NAMESPACE, "itemActorDestroyAll", [&mgr]() -> void {
        mgr.destroyAll();
    });

    // itemActorExists(id) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "itemActorExists", [&mgr](int64_t id) -> bool {
            return mgr.exists(id);
        });

    // itemActorGetIds() -> [i]
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
