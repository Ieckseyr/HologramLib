// ItemDetailManager.cpp - 物品详情悬浮显示实现
#include "ItemDetailManager.h"
#include "FloatingTextManager.h"

#include <ll/api/service/Bedrock.h>

#include "mc/deps/core/string/HashedString.h"
#include "mc/locale/I18n.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/registry/ItemRegistryRef.h"
#include "mc/world/level/Level.h"

#include <format>
#include <optional>

namespace debugshape_export {

ItemDetailManager& ItemDetailManager::getInstance() {
    static ItemDetailManager instance;
    return instance;
}

std::string ItemDetailManager::resolveItemName(const std::string& itemId, int aux) const {
    // 注册表解析 → ItemStack → I18n 翻译链（与 ItemPhys DropName 同源方案）
    auto level = ll::service::getLevel();
    if (level) {
        auto weak = level->getItemRegistry().getItem(::HashedString(itemId));
        if (weak) {
            ::ItemStack stack(*weak, 1, aux, nullptr);
            auto        key = stack.getDescriptionId();
            if (!key.empty()) {
                auto& i18n       = getI18n();
                auto  locale     = i18n.getLocaleFor("en_US");
                auto  translated = i18n.get(key, {}, locale);
                if (!translated.empty() && translated != key) {
                    return translated;
                }
            }
            auto name = stack.getName();
            if (!name.empty()) return name;
            name = stack.getDescriptionName();
            if (!name.empty()) return name;
        }
    }
    return itemId; // 兜底: 原样显示 ID（不阻断显示）
}

int64_t ItemDetailManager::show(
    int                dimId,
    float              x,
    float              y,
    float              z,
    const std::string& itemId,
    int                aux,
    int                count,
    const std::string& customText
) {
    std::lock_guard<std::mutex> lock(mMutex);

    std::string text = customText;
    if (text.empty()) {
        std::string name = resolveItemName(itemId, aux);
        text = count > 1 ? std::format("{} x{}", name, count) : name;
    }

    auto& holo = FloatingTextManager::getInstance();
    int64_t id = holo.create(x, y, z);
    if (id < 0) return -1;

    holo.addLine(id, text);
    if (!holo.drawToDimension(id, dimId)) {
        // 绘制失败（如维度无效）直接回收, 不留僵尸数据
        holo.destroy(id);
        return -1;
    }
    return id;
}

bool ItemDetailManager::hide(int64_t id) {
    std::lock_guard<std::mutex> lock(mMutex);
    return FloatingTextManager::getInstance().destroy(id);
}

} // namespace debugshape_export
