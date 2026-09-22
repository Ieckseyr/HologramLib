// SulfurDisplayManager.cpp - 硫磺立方体展示域实现（委托 CustomEntityManager）
#include "SulfurDisplayManager.h"

#include "customentity/CustomEntityManager.h"
#include "sulfur/SulfurDisplayPackets.h" // kArchetypeProperty

#include <format>

namespace debugshape_export {

namespace {

// 立方体的 identifier（行为包 entities/sulfur_cube.json）
constexpr char const* kSulfurIdentifier = "minecraft:sulfur_cube";

// 主手槽位（MobEquipmentPacket 编码: 0=mainhand）
constexpr int kMainhandSlot = 0;

// 装备物品的自定义名/描述 → 物品 user data 的 SNBT（CustomEntityEquipment::nbt 的格式）
std::string buildDisplaySnbt(hologramlib::ContainerMenuItem const& item) {
    if (item.name.empty() && item.lore.empty()) return {};
    std::string out = "{display:{";
    bool        first = true;
    if (!item.name.empty()) {
        out += std::format(R"x(Name:"{}")x", item.name);
        first = false;
    }
    if (!item.lore.empty()) {
        if (!first) out += ',';
        out += "Lore:[";
        for (std::size_t i = 0; i < item.lore.size(); ++i) {
            if (i) out += ',';
            out += std::format(R"x("{}")x", item.lore[i]);
        }
        out += ']';
    }
    out += "}}";
    return out;
}

hologramlib::CustomEntityEquipment toEquipment(hologramlib::ContainerMenuItem const& item) {
    hologramlib::CustomEntityEquipment eq;
    if (item.type.empty()) return eq; // 空手
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

SulfurDisplayManager::Entry const* SulfurDisplayManager::findLocked(int64_t id) const {
    auto it = mEntries.find(id);
    return it == mEntries.end() ? nullptr : &it->second;
}

int64_t SulfurDisplayManager::create(hologramlib::SulfurDisplaySpec const& spec) {
    hologramlib::CustomEntityConfig cfg;
    cfg.identifier   = kSulfurIdentifier;
    cfg.x            = spec.x;
    cfg.y            = spec.y;
    cfg.z            = spec.z;
    cfg.dimension    = spec.dim;
    cfg.scale        = spec.scale;
    cfg.viewDistance = spec.viewDistance;
    cfg.variant      = spec.variant;
    cfg.invisible    = spec.invisible;
    cfg.equipment[kMainhandSlot] = toEquipment(spec.block);
    if (!spec.archetype.empty()) {
        cfg.mobProperties.push_back(hologramlib::EntityMobProperty{sulfur::kArchetypeProperty, spec.archetype});
    }

    int64_t const entityId = CustomEntityManager::getInstance().create(cfg);
    if (entityId < 0) return -1;

    // 可见玩家白名单（空 = 全员可见）
    if (!spec.visiblePlayers.empty()) {
        CustomEntityManager::getInstance().setVisiblePlayers(entityId, spec.visiblePlayers);
    }

    std::lock_guard lock(mMutex);
    int64_t const   id = mNextId++;
    mEntries[id]       = Entry{entityId};
    return id;
}

bool SulfurDisplayManager::setBlock(int64_t id, hologramlib::ContainerMenuItem const& item) {
    int64_t entityId = -1;
    {
        std::lock_guard lock(mMutex);
        auto const*     entry = findLocked(id);
        if (entry == nullptr) return false;
        entityId = entry->entityId;
    }
    auto const eq = toEquipment(item);
    return CustomEntityManager::getInstance().setEquipmentSlot(entityId, kMainhandSlot, eq.name, eq.aux, eq.nbt);
}

bool SulfurDisplayManager::setArchetype(int64_t id, std::string const& archetype) {
    int64_t entityId = -1;
    {
        std::lock_guard lock(mMutex);
        auto const*     entry = findLocked(id);
        if (entry == nullptr) return false;
        entityId = entry->entityId;
    }
    if (archetype.empty()) return CustomEntityManager::getInstance().clearMobProperties(entityId);
    return CustomEntityManager::getInstance().setMobProperty(entityId, sulfur::kArchetypeProperty, archetype);
}

bool SulfurDisplayManager::setInvisible(int64_t id, bool on) {
    std::lock_guard lock(mMutex);
    auto const*     entry = findLocked(id);
    if (entry == nullptr) return false;
    return CustomEntityManager::getInstance().setInvisible(entry->entityId, on);
}

bool SulfurDisplayManager::setScale(int64_t id, float scale) {
    std::lock_guard lock(mMutex);
    auto const*     entry = findLocked(id);
    if (entry == nullptr) return false;
    return CustomEntityManager::getInstance().setScale(entry->entityId, scale);
}

bool SulfurDisplayManager::destroy(int64_t id) {
    Entry entry{};
    {
        std::lock_guard lock(mMutex);
        auto            it = mEntries.find(id);
        if (it == mEntries.end()) return false;
        entry = it->second;
        mEntries.erase(it);
    }
    if (entry.entityId >= 0) CustomEntityManager::getInstance().destroy(entry.entityId);
    return true;
}

void SulfurDisplayManager::destroyAll() {
    std::vector<int64_t> ids;
    {
        std::lock_guard lock(mMutex);
        for (auto const& [id, entry] : mEntries) ids.push_back(id);
    }
    for (auto const id : ids) destroy(id);
}

bool SulfurDisplayManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mEntries.contains(id);
}

std::vector<int64_t> SulfurDisplayManager::getAllIds() const {
    std::lock_guard      lock(mMutex);
    std::vector<int64_t> out;
    out.reserve(mEntries.size());
    for (auto const& [id, entry] : mEntries) out.push_back(id);
    return out;
}

int64_t SulfurDisplayManager::entityIdOf(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto const*     entry = findLocked(id);
    return entry == nullptr ? -1 : entry->entityId;
}

// ── 公开接口适配 ──
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
    bool destroy(int64_t id) override { return SulfurDisplayManager::getInstance().destroy(id); }
    void destroyAll() override { SulfurDisplayManager::getInstance().destroyAll(); }
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
