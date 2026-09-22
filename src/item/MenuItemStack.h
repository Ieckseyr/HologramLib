// MenuItemStack.h - 界面条目（ContainerMenuItem）→ 物品的两条构造路径
//
// 抽出来共用: 虚拟容器（ContainerMenuItem 走方块实体 NBT / BDS InventorySlot）与
// 背包虚容器（条目走 sculk 的 NetworkItemStackDescriptor）都需要同一份"条目 → 物品"。
//   ① makeMenuItemStack  : 条目 → ::ItemInstance（BDS 侧; 数字 id / aux / NBT 都在里面）
#pragma once

#include "hologramlib/HologramLib.h" // hologramlib::ContainerMenuItem

#include <format>
#include <string>
#include <vector>

#include <mc/deps/nbt/CompoundTag.h>
#include <mc/world/item/ItemInstance.h>
#include <mc/world/item/ItemStack.h>

#include <sculk/protocol/codec/nbt/CompoundTag.hpp>
#include <sculk/protocol/codec/nbt/ListTag.hpp>
#include <sculk/protocol/codec/nbt/TagVariant.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include "trade/TradeOfferNbt.h" // trade::makeTag（库内统一的 NBT 构造）

namespace debugshape_export::item {

inline std::string snbtEscape(std::string const& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char const c : s) {
        if (c == '"' || c == static_cast<char>(92)) out.push_back(static_cast<char>(92));
        out.push_back(c);
    }
    return out;
}

// 条目 -> ItemInstance。拼装格式与库内"逐字节对拍过的物品 NBT 方言"（trade/TradeOfferNbt.h 的
// itemToNbt）保持一致:
//   {Count: Byte, Damage: Short, Name: String, WasPickedUp: Byte, tag:{display:{Name, Lore}}}
// 修复记录: 曾经把引号写成双份（""Count""）且 Count 用了 Short —— SNBT 非法, fromSnbt 返回
// nullopt, 于是返回空栈, 按槽刷新会把整页刷成空气（实测现象）。
inline ::ItemStack makeMenuItemStack(hologramlib::ContainerMenuItem const& entry) {
    if (entry.type.empty()) return ::ItemStack{};

    int count = entry.count; // Count 是 Byte 域（与 itemToNbt 一致）
    if (count < 1) count = 1;
    if (count > 127) count = 127;

    std::string snbt = std::format(
        R"x({{Count:{}b,Damage:{}s,Name:"{}",WasPickedUp:0b)x",
        count,
        entry.damage,
        snbtEscape(entry.type)
    );
    if (!entry.name.empty() || !entry.lore.empty()) {
        snbt += R"x(,tag:{display:{)x";
        bool first = true;
        if (!entry.name.empty()) {
            snbt += std::format(R"x(Name:"{}")x", snbtEscape(entry.name));
            first = false;
        }
        if (!entry.lore.empty()) {
            if (!first) snbt += ',';
            snbt += R"x(Lore:[)x";
            for (std::size_t i = 0; i < entry.lore.size(); ++i) {
                if (i) snbt += ',';
                snbt += std::format(R"x("{}")x", snbtEscape(entry.lore[i]));
            }
            snbt += ']';
        }
        snbt += '}'; // display
        snbt += '}'; // tag
    }
    snbt += '}';     // 根
    auto tag = CompoundTag::fromSnbt(snbt);
    if (!tag.has_value()) return ::ItemStack{};
    return ::ItemStack::fromTag(tag.value());
}

} // namespace debugshape_export::item
