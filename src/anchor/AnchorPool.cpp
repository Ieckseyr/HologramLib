// AnchorPool.cpp - 锚点实体池
#include "AnchorPool.h"

#include "hologramlib/HologramLib.h" // ItemActorSpec
#include "itemactor/ItemActorManager.h"
#include "lse/LseBridge.h"

namespace debugshape_export {

namespace {

// 虚空位置：主世界底部在 y=-64 左右，-120 一定在世界之外，玩家不可能看到。
constexpr float kAnchorY = -120.0f;
// 一块没有模型的物品：structure_void
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
    // x/z 用库内 id 区分，避免同一坐标叠在一起（坐标本身不重要）
    spec.x         = 0.0f;
    spec.y         = kAnchorY;
    spec.z         = 0.0f;
    spec.dimension = dimension;

    auto&        items  = ItemActorManager::getInstance();
    std::int64_t itemId = items.spawn(spec);
    if (itemId < 0)
        return -1;

    // 定住：不要让它在客户端往下掉
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
    std::lock_guard lock(mMutex);
    mAnchors.clear();
}

} // namespace debugshape_export
