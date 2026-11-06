#include "AnchorPool.h"

#include "hologramlib/HologramLib.h"
#include "itemactor/ItemActorManager.h"
#include "lse/LseBridge.h"

namespace debugshape_export {

namespace {

constexpr float kAnchorY = -120.0f;

constexpr char const* kAnchorItem = "minecraft:structure_void";

} // namespace

AnchorPool& AnchorPool::getInstance() {
    static AnchorPool instance;
    return instance;
}

std::int64_t AnchorPool::acquire(int dimension) {
    hologramlib::ItemActorSpec spec;
    spec.itemId = kAnchorItem;
    spec.count  = 1;

    spec.x         = 0.0f;
    spec.y         = kAnchorY;
    spec.z         = 0.0f;
    spec.dimension = dimension;

    auto&        items  = ItemActorManager::getInstance();
    std::int64_t itemId = items.spawn(spec);
    if (itemId < 0)
        return -1;

    items.setMotion(itemId, 0.0f, 0.0f, 0.0f);

    auto const uniqueId = static_cast<std::int64_t>(items.uniqueIdOf(itemId));
    if (uniqueId == 0) {
        items.destroy(itemId);
        return -1;
    }

    std::lock_guard lock(mMutex);
    mAnchors[uniqueId] = itemId;
    return uniqueId;
}

bool AnchorPool::release(std::int64_t uniqueId) {
    std::int64_t itemId = 0;
    {
        std::lock_guard lock(mMutex);
        auto            it = mAnchors.find(uniqueId);
        if (it == mAnchors.end())
            return false;
        itemId = it->second;
        mAnchors.erase(it);
    }
    ItemActorManager::getInstance().destroy(itemId);
    return true;
}

void AnchorPool::releaseAll() {
    std::unordered_map<std::int64_t, std::int64_t> victims;
    {
        std::lock_guard lock(mMutex);
        victims.swap(mAnchors);
    }
    auto& items = ItemActorManager::getInstance();
    for (auto const& [uniqueId, itemId] : victims) {
        items.destroy(itemId);
    }
}

int AnchorPool::count() const {
    std::lock_guard lock(mMutex);
    return static_cast<int>(mAnchors.size());
}

bool AnchorPool::exists(std::int64_t uniqueId) const {
    std::lock_guard lock(mMutex);
    return mAnchors.contains(uniqueId);
}

void AnchorPool::shutdown() {
    releaseAll();
}

} // namespace debugshape_export
