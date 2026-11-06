#include "SulfurDisplayManager.h"

#include "customentity/CustomEntityManager.h"
#include "sulfur/SulfurDisplayPackets.h"

#include <format>
#include <memory>

namespace debugshape_export {

namespace {

constexpr char const* kSulfurIdentifier = "minecraft:sulfur_cube";

constexpr int kMainhandSlot = 0;

std::string buildDisplaySnbt(hologramlib::ContainerMenuItem const& item) {
    if (item.name.empty() && item.lore.empty())
        return {};
    std::string out   = "{display:{";
    bool        first = true;
    if (!item.name.empty()) {
        out += std::format(R"x(Name:"{}")x", item.name);
        first = false;
    }
    if (!item.lore.empty()) {
        if (!first)
            out += ',';
        out += "Lore:[";
        for (std::size_t i = 0; i < item.lore.size(); ++i) {
            if (i)
                out += ',';
            out += std::format(R"x("{}")x", item.lore[i]);
        }
        out += ']';
    }
    out += "}}";
    return out;
}

hologramlib::CustomEntityEquipment toEquipment(hologramlib::ContainerMenuItem const& item) {
    hologramlib::CustomEntityEquipment eq;
    if (item.type.empty())
        return eq;
    eq.name = item.type;
    eq.aux  = item.damage;
    eq.nbt  = buildDisplaySnbt(item);
    return eq;
}

} // namespace

SulfurDisplayManager& SulfurDisplayManager::getInstance() {
    static SulfurDisplayManager instance;
    return instance;
}

int64_t SulfurDisplayManager::create(hologramlib::SulfurDisplaySpec const& spec) {
    hologramlib::CustomEntityConfig cfg;
    cfg.identifier               = kSulfurIdentifier;
    cfg.x                        = spec.x;
    cfg.y                        = spec.y;
    cfg.z                        = spec.z;
    cfg.dimension                = spec.dim;
    cfg.scale                    = spec.scale;
    cfg.viewDistance             = spec.viewDistance;
    cfg.variant                  = spec.variant;
    cfg.invisible                = spec.invisible;
    cfg.equipment[kMainhandSlot] = toEquipment(spec.block);
    if (!spec.archetype.empty()) {
        cfg.mobProperties.push_back(hologramlib::EntityMobProperty{sulfur::kArchetypeProperty, spec.archetype});
    }

    int64_t const entityId = CustomEntityManager::getInstance().create(cfg);
    if (entityId < 0)
        return -1;

    if (!spec.visiblePlayers.empty()) {
        CustomEntityManager::getInstance().setVisiblePlayers(entityId, spec.visiblePlayers);
        // 可见性过滤会重建实体，重建后装备不一定会跟着重发 —— 这里补一次主手同步，
        // 否则「只对某个玩家可见」的硫磺立方体会显示成空手
        if (!spec.block.type.empty()) {
            auto const eq = toEquipment(spec.block);
            CustomEntityManager::getInstance().setEquipmentSlot(entityId, kMainhandSlot, eq.name, eq.aux, eq.nbt);
        }
    }

    return mEntries.insert(std::make_unique<Entry>(Entry{entityId}));
}

bool SulfurDisplayManager::setBlock(int64_t id, hologramlib::ContainerMenuItem const& item) {
    std::optional<int64_t> entityId;
    if (!mEntries.withObject(id, [&](Entry const& entry) {
            entityId = entry.entityId;
        }))
        return false;

    auto const eq = toEquipment(item);
    return CustomEntityManager::getInstance().setEquipmentSlot(*entityId, kMainhandSlot, eq.name, eq.aux, eq.nbt);
}

bool SulfurDisplayManager::setArchetype(int64_t id, std::string const& archetype) {
    std::optional<int64_t> entityId;
    if (!mEntries.withObject(id, [&](Entry const& entry) {
            entityId = entry.entityId;
        }))
        return false;

    if (archetype.empty())
        return CustomEntityManager::getInstance().clearMobProperties(*entityId);
    return CustomEntityManager::getInstance().setMobProperty(*entityId, sulfur::kArchetypeProperty, archetype);
}

bool SulfurDisplayManager::setInvisible(int64_t id, bool on) {
    std::optional<int64_t> entityId;
    if (!mEntries.withObject(id, [&](Entry const& entry) {
            entityId = entry.entityId;
        }))
        return false;

    return CustomEntityManager::getInstance().setInvisible(*entityId, on);
}

bool SulfurDisplayManager::setScale(int64_t id, float scale) {
    std::optional<int64_t> entityId;
    if (!mEntries.withObject(id, [&](Entry const& entry) {
            entityId = entry.entityId;
        }))
        return false;

    return CustomEntityManager::getInstance().setScale(*entityId, scale);
}

bool SulfurDisplayManager::destroy(int64_t id) {
    std::optional<int64_t> entityId;
    if (!mEntries.withObject(id, [&](Entry const& entry) {
            entityId = entry.entityId;
        }))
        return false;
    mEntries.erase(id);

    if (entityId && *entityId >= 0)
        CustomEntityManager::getInstance().destroy(*entityId);
    return true;
}

void SulfurDisplayManager::destroyAll() {

    for (auto const id : mEntries.snapshotAll()) {
        destroy(id);
    }
}

bool SulfurDisplayManager::exists(int64_t id) const {
    return mEntries.exists(id);
}

std::vector<int64_t> SulfurDisplayManager::getAllIds() const {
    return mEntries.snapshotAll();
}

int64_t SulfurDisplayManager::entityIdOf(int64_t id) const {
    std::optional<int64_t> entityId;
    if (!mEntries.withObject(id, [&](Entry const& entry) {
            entityId = entry.entityId;
        }))
        return -1;
    return *entityId;
}

namespace {
class SulfurDisplayAdapter final : public hologramlib::ISulfurDisplay {
public:
    int64_t create(hologramlib::SulfurDisplaySpec const& spec) override {
        return SulfurDisplayManager::getInstance().create(spec);
    }
    bool setBlock(int64_t id, hologramlib::ContainerMenuItem const& item) override {
        return SulfurDisplayManager::getInstance().setBlock(id, item);
    }
    bool setArchetype(int64_t id, std::string const& archetype) override {
        return SulfurDisplayManager::getInstance().setArchetype(id, archetype);
    }
    bool setInvisible(int64_t id, bool on) override {
        return SulfurDisplayManager::getInstance().setInvisible(id, on);
    }
    bool setScale(int64_t id, float scale) override {
        return SulfurDisplayManager::getInstance().setScale(id, scale);
    }
    bool               destroy(int64_t id) override { return SulfurDisplayManager::getInstance().destroy(id); }
    void               destroyAll() override { SulfurDisplayManager::getInstance().destroyAll(); }
    [[nodiscard]] bool exists(int64_t id) const override {
        return SulfurDisplayManager::getInstance().exists(id);
    }
    [[nodiscard]] std::vector<int64_t> getAllIds() const override {
        return SulfurDisplayManager::getInstance().getAllIds();
    }
    [[nodiscard]] int64_t entityIdOf(int64_t id) const override {
        return SulfurDisplayManager::getInstance().entityIdOf(id);
    }
};
} // namespace

hologramlib::ISulfurDisplay& sulfurDisplayAdapter() {
    static SulfurDisplayAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
