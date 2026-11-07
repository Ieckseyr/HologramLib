#pragma once

// 物品 NBT 构造（平台无关）：Name / Count / Damage / WasPickedUp / Block / tag{display{Name,Lore}, ench}
// 由 LeviLamina 版 trade 域抽出，现由 LL 与 endstone 共用（endstone 通过 holo_core_build_item_nbt 调用）。

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "core/proto/Version.h"

namespace debugshape_export::trade {

namespace bp = ::bedrock::protocol;

using ByteTag     = bp::ByteTag;
using CompoundTag = bp::CompoundTag;
using FloatTag    = bp::FloatTag;
using IntTag      = bp::IntTag;
using ListTag     = bp::ListTag;
using ShortTag    = bp::ShortTag;
using StringTag   = bp::StringTag;
using TagType     = bp::nbt::Type;
using Tag         = bp::Tag;

template <typename T>
inline Tag makeTag(T&& value) {
    return Tag{std::forward<T>(value)};
}

struct TradeItem {
    std::string              type;
    int                      count{1};
    int                      damage{0};
    std::string              name;
    std::vector<std::string> lore;

    bool isBlock{false};
    int  blockVersion{18168865};

    std::vector<std::pair<int, int>> enchants;
};

struct TradeOffer {
    TradeItem buyA;
    TradeItem buyB;
    TradeItem sell;
    int       tier{0};
    int       maxUses{16};
    int       uses{0};
    int       netId{0};
    int       traderExp{0};
    int       demand{0};
    float     priceMultiplierA{0.05f};
    float     priceMultiplierB{0.0f};
    bool      rewardExp{true};
};

inline std::vector<int> defaultTierExpRequirements() {
    return {0, 10, 70, 150, 250};
}

namespace detail {

inline CompoundTag itemToNbt(TradeItem const& item) {
    CompoundTag out;
    out["Name"]        = makeTag(StringTag{item.type});
    out["Count"]       = makeTag(ByteTag{static_cast<std::int8_t>(item.count)});
    out["Damage"]      = makeTag(ShortTag{static_cast<std::int16_t>(item.damage)});
    out["WasPickedUp"] = makeTag(ByteTag{0});
    if (item.isBlock) {
        CompoundTag block;
        block["name"]    = makeTag(StringTag{item.type});
        block["states"]  = makeTag(CompoundTag{});
        block["version"] = makeTag(IntTag{item.blockVersion});
        out["Block"]     = makeTag(std::move(block));
    }
    bool const hasDisplay = !item.name.empty() || !item.lore.empty();
    bool const hasEnch    = !item.enchants.empty();
    if (hasDisplay || hasEnch) {
        CompoundTag tag;
        if (hasDisplay) {
            CompoundTag display;
            if (!item.name.empty())
                display["Name"] = makeTag(StringTag{item.name});
            if (!item.lore.empty()) {
                ListTag lore(TagType::String);
                for (auto const& line : item.lore)
                    lore.push_back(Tag{StringTag{line}});
                display["Lore"] = makeTag(std::move(lore));
            }
            tag["display"] = makeTag(std::move(display));
        }
        if (hasEnch) {
            ListTag ench(TagType::Compound);
            for (auto const& [id, lvl] : item.enchants) {
                if (id < 0 || id > 42 || lvl < 1 || lvl > 255)
                    continue;
                CompoundTag one;
                one["id"]  = makeTag(ShortTag{static_cast<std::int16_t>(id)});
                one["lvl"] = makeTag(ShortTag{static_cast<std::int16_t>(lvl)});
                ench.push_back(Tag{std::move(one)});
            }
            tag["ench"] = makeTag(std::move(ench));
        }
        out["tag"] = makeTag(std::move(tag));
    }
    return out;
}

inline CompoundTag offerToNbt(TradeOffer const& offer) {
    CompoundTag out;
    out["buyA"] = makeTag(itemToNbt(offer.buyA));
    if (!offer.buyB.type.empty())
        out["buyB"] = makeTag(itemToNbt(offer.buyB));
    out["sell"] = makeTag(itemToNbt(offer.sell));

    out["buyCountA"]        = makeTag(IntTag{offer.buyA.count});
    out["buyCountB"]        = makeTag(IntTag{offer.buyB.type.empty() ? 0 : offer.buyB.count});
    out["tier"]             = makeTag(IntTag{offer.tier});
    out["uses"]             = makeTag(IntTag{offer.uses});
    out["maxUses"]          = makeTag(IntTag{offer.maxUses});
    out["demand"]           = makeTag(IntTag{offer.demand});
    out["priceMultiplierA"] = makeTag(FloatTag{offer.priceMultiplierA});
    out["priceMultiplierB"] = makeTag(FloatTag{offer.priceMultiplierB});
    out["rewardExp"]        = makeTag(ByteTag{static_cast<std::int8_t>(offer.rewardExp ? 1 : 0)});
    out["traderExp"]        = makeTag(IntTag{offer.traderExp});
    out["netId"]            = makeTag(IntTag{offer.netId});
    return out;
}

} // namespace detail

inline CompoundTag buildOffers(
    std::vector<TradeOffer> const& offers,
    std::vector<int> const&        tierExpRequirements = defaultTierExpRequirements(),
    int                            baseNetId           = 3676) {
    CompoundTag root;

    ListTag recipes(TagType::Compound);
    for (std::size_t i = 0; i < offers.size(); ++i) {
        TradeOffer o = offers[i];
        if (o.netId <= 0)
            o.netId = baseNetId + static_cast<int>(i);
        recipes.push_back(Tag{detail::offerToNbt(o)});
    }
    root["Recipes"] = makeTag(std::move(recipes));

    ListTag tierExp(TagType::Compound);
    for (std::size_t i = 0; i < tierExpRequirements.size(); ++i) {
        CompoundTag entry;
        entry[std::to_string(i)] = makeTag(IntTag{tierExpRequirements[i]});
        tierExp.push_back(Tag{std::move(entry)});
    }
    root["TierExpRequirements"] = makeTag(std::move(tierExp));

    return root;
}

} // namespace debugshape_export::trade
