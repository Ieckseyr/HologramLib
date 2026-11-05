#pragma once

#include "hologramlib/HologramLib.h"

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

#include "trade/TradeOfferNbt.h"

namespace debugshape_export::item {

inline std::string snbtEscape(std::string const& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char const c : s) {
        if (c == '"' || c == static_cast<char>(92))
            out.push_back(static_cast<char>(92));
        out.push_back(c);
    }
    return out;
}

inline ::ItemStack makeMenuItemStack(
    hologramlib::ContainerMenuItem const&   entry,
    std::vector<std::pair<int, int>> const& enchants) {
    if (entry.type.empty())
        return ::ItemStack{};

    int count = entry.count;
    if (count < 1)
        count = 1;
    if (count > 127)
        count = 127;

    std::string snbt = std::format(
        R"x({{Count:{}b,Damage:{}s,Name:"{}",WasPickedUp:0b)x",
        count,
        entry.damage,
        snbtEscape(entry.type));

    bool const hasDisplay = !entry.name.empty() || !entry.lore.empty();
    bool const hasEnch    = !enchants.empty();

    if (hasDisplay || hasEnch) {
        snbt += ",tag:{";
        if (hasDisplay) {
            snbt += "display:{";
            bool first = true;
            if (!entry.name.empty()) {
                snbt += std::format(R"x(Name:"{}")x", snbtEscape(entry.name));
                first = false;
            }
            if (!entry.lore.empty()) {
                if (!first)
                    snbt += ',';
                snbt += R"x(Lore:[)x";
                for (std::size_t i = 0; i < entry.lore.size(); ++i) {
                    if (i)
                        snbt += ',';
                    snbt += std::format(R"x("{}")x", snbtEscape(entry.lore[i]));
                }
                snbt += ']';
            }
            snbt += '}';
            if (hasEnch)
                snbt += ',';
        }
        if (hasEnch) {

            snbt += "ench:[";
            bool firstEnch = true;
            for (auto const& [id, lvl] : enchants) {
                if (id < 0 || id > 42 || lvl < 1 || lvl > 255)
                    continue;
                if (!firstEnch)
                    snbt += ',';
                firstEnch = false;
                snbt += std::format("{{id:{}s,lvl:{}s}}", id, lvl);
            }
            snbt += ']';
        }
        snbt += '}';
    }
    snbt += '}';
    auto tag = CompoundTag::fromSnbt(snbt);
    if (!tag.has_value())
        return ::ItemStack{};
    return ::ItemStack::fromTag(tag.value());
}

inline ::ItemStack makeMenuItemStack(hologramlib::ContainerMenuItem const& entry) {
    return makeMenuItemStack(entry, std::vector<std::pair<int, int>>{});
}

} // namespace debugshape_export::item
