// ItemDetailManager.h - 物品详情悬浮显示
//
// 在任意坐标显示"物品名 ×数量"详情（原 itemdetail / DropName 场景的通用化）。
// 实现完全构建在 FloatingTextManager 之上（单一多行文本形状）,
// 不引入新的渲染路径 —— 渲染语义与悬浮字完全一致。
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace debugshape_export {

class ItemDetailManager {
public:
    static ItemDetailManager& getInstance();

    ItemDetailManager(const ItemDetailManager&)            = delete;
    ItemDetailManager& operator=(const ItemDetailManager&) = delete;

    // 在 dimId 维度 (x,y,z) 显示物品详情
    // - itemId: 形如 "minecraft:diamond"; 自动翻译为本地化名称
    // - aux: 物品附加值; count: 数量(<=1 不显示数量)
    // - customText: 非空则完全替代自动文本
    // 返回悬浮字 ID（失败 <0）
    int64_t show(
        int                dimId,
        float              x,
        float              y,
        float              z,
        const std::string& itemId,
        int                aux,
        int                count,
        const std::string& customText = ""
    );

    bool hide(int64_t id);

private:
    ItemDetailManager() = default;
    ~ItemDetailManager() = default;

    // 物品名翻译（注册表 + I18n 兜底链）
    std::string resolveItemName(const std::string& itemId, int aux) const;

    std::mutex mMutex;
};

} // namespace debugshape_export
