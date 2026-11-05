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

    auto level = ll::service::getLevel();
    if (level) {
        auto weak = level->getItemRegistry().getItem(::HashedString(itemId));
        if (weak) {

            ::ItemStack stack{};
            stack.reinit(*weak, 1, aux);
            auto key = stack.getDescriptionId();
            if (!key.empty()) {
                auto& i18n       = getI18n();
                auto  locale     = i18n.getLocaleFor("en_US");
                auto  translated = i18n.get(key, {}, locale);
                if (!translated.empty() && translated != key) {
                    return translated;
                }
            }
            auto name = stack.getName();
            if (!name.empty())
                return name;
            name = stack.getDescriptionName();
            if (!name.empty())
                return name;
        }
    }
    return itemId;
}

int64_t ItemDetailManager::show(
    int                dimId,
    float              x,
    float              y,
    float              z,
    const std::string& itemId,
    int                aux,
    int                count,
    const std::string& customText) {
    std::lock_guard<std::mutex> lock(mMutex);

    std::string text = customText;
    if (text.empty()) {
        std::string name = resolveItemName(itemId, aux);
        text             = count > 1 ? std::format("{} x{}", name, count) : name;
    }

    auto&   holo = FloatingTextManager::getInstance();
    int64_t id   = holo.create(x, y, z);
    if (id < 0)
        return -1;

    holo.addLine(id, text);
    if (!holo.drawToDimension(id, dimId)) {

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
