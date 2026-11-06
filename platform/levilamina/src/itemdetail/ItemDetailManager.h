#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace debugshape_export {

// 物品详情悬浮字
class ItemDetailManager {
public:
    static ItemDetailManager& getInstance();

    ItemDetailManager(const ItemDetailManager&)            = delete;
    ItemDetailManager& operator=(const ItemDetailManager&) = delete;

    int64_t show(
        int                dimId,
        float              x,
        float              y,
        float              z,
        const std::string& itemId,
        int                aux,
        int                count,
        const std::string& customText = "");

    bool hide(int64_t id);

private:
    ItemDetailManager()  = default;
    ~ItemDetailManager() = default;

    std::string resolveItemName(const std::string& itemId, int aux) const;

    std::mutex mMutex;
};

} // namespace debugshape_export
