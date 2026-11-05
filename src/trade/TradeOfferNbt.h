#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <sculk/protocol/codec/nbt/CompoundTag.hpp>
#include <sculk/protocol/codec/nbt/ListTag.hpp>
#include <sculk/protocol/codec/nbt/TagType.hpp>
#include <sculk/protocol/codec/nbt/TagVariant.hpp>

namespace debugshape_export::trade {

using sculk::protocol::ByteTag;
using sculk::protocol::CompoundTag;
using sculk::protocol::FloatTag;
using sculk::protocol::IntTag;
using sculk::protocol::ListTag;
using sculk::protocol::ShortTag;
using sculk::protocol::StringTag;
using sculk::protocol::TagType;
using sculk::protocol::TagVariant;

template <typename T>
inline TagVariant makeTag(T&& value) {
    return TagVariant{std::forward<T>(value)};
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
    out.mValue["Name"]        = makeTag(StringTag{item.type});
    out.mValue["Count"]       = makeTag(ByteTag{static_cast<std::int8_t>(item.count)});
    out.mValue["Damage"]      = makeTag(ShortTag{static_cast<std::int16_t>(item.damage)});
    out.mValue["WasPickedUp"] = makeTag(ByteTag{0});
    if (item.isBlock) {
        CompoundTag block;
        block.mValue["name"]    = makeTag(StringTag{item.type});
        block.mValue["states"]  = makeTag(CompoundTag{});
        block.mValue["version"] = makeTag(IntTag{item.blockVersion});
        out.mValue["Block"]     = makeTag(std::move(block));
    }
    bool const hasDisplay = !item.name.empty() || !item.lore.empty();
    bool const hasEnch    = !item.enchants.empty();
    if (hasDisplay || hasEnch) {
        CompoundTag tag;
        if (hasDisplay) {
            CompoundTag display;
            if (!item.name.empty())
                display.mValue["Name"] = makeTag(StringTag{item.name});
            if (!item.lore.empty()) {
                ListTag lore;
                lore.mType = TagType::String;
                lore.mValue.reserve(item.lore.size());
                for (auto const& line : item.lore)
                    lore.mValue.push_back(makeTag(StringTag{line}));
                display.mValue["Lore"] = makeTag(std::move(lore));
            }
            tag.mValue["display"] = makeTag(std::move(display));
        }
        if (hasEnch) {
            ListTag ench;
            ench.mType = TagType::Compound;
            for (auto const& [id, lvl] : item.enchants) {
                if (id < 0 || id > 42 || lvl < 1 || lvl > 255)
                    continue;
                CompoundTag one;
                one.mValue["id"]  = makeTag(ShortTag{static_cast<std::int16_t>(id)});
                one.mValue["lvl"] = makeTag(ShortTag{static_cast<std::int16_t>(lvl)});
                ench.mValue.push_back(makeTag(std::move(one)));
            }
            tag.mValue["ench"] = makeTag(std::move(ench));
        }
        out.mValue["tag"] = makeTag(std::move(tag));
    }
    return out;
}

inline CompoundTag offerToNbt(TradeOffer const& offer) {
    CompoundTag out;
    out.mValue["buyA"] = makeTag(itemToNbt(offer.buyA));
    if (!offer.buyB.type.empty())
        out.mValue["buyB"] = makeTag(itemToNbt(offer.buyB));
    out.mValue["sell"] = makeTag(itemToNbt(offer.sell));

    out.mValue["buyCountA"]        = makeTag(IntTag{offer.buyA.count});
    out.mValue["buyCountB"]        = makeTag(IntTag{offer.buyB.type.empty() ? 0 : offer.buyB.count});
    out.mValue["tier"]             = makeTag(IntTag{offer.tier});
    out.mValue["uses"]             = makeTag(IntTag{offer.uses});
    out.mValue["maxUses"]          = makeTag(IntTag{offer.maxUses});
    out.mValue["demand"]           = makeTag(IntTag{offer.demand});
    out.mValue["priceMultiplierA"] = makeTag(FloatTag{offer.priceMultiplierA});
    out.mValue["priceMultiplierB"] = makeTag(FloatTag{offer.priceMultiplierB});
    out.mValue["rewardExp"]        = makeTag(ByteTag{static_cast<std::int8_t>(offer.rewardExp ? 1 : 0)});
    out.mValue["traderExp"]        = makeTag(IntTag{offer.traderExp});
    out.mValue["netId"]            = makeTag(IntTag{offer.netId});
    return out;
}

} // namespace detail

inline CompoundTag buildOffers(
    std::vector<TradeOffer> const& offers,
    std::vector<int> const&        tierExpRequirements = defaultTierExpRequirements(),
    int                            baseNetId           = 3676) {
    CompoundTag root;

    ListTag recipes;
    recipes.mType = TagType::Compound;
    recipes.mValue.reserve(offers.size());
    for (std::size_t i = 0; i < offers.size(); ++i) {
        TradeOffer o = offers[i];
        if (o.netId <= 0)
            o.netId = baseNetId + static_cast<int>(i);
        recipes.mValue.push_back(makeTag(detail::offerToNbt(o)));
    }
    root.mValue["Recipes"] = makeTag(std::move(recipes));

    ListTag tierExp;
    tierExp.mType = TagType::Compound;
    tierExp.mValue.reserve(tierExpRequirements.size());
    for (std::size_t i = 0; i < tierExpRequirements.size(); ++i) {
        CompoundTag entry;
        entry.mValue[std::to_string(i)] = makeTag(IntTag{tierExpRequirements[i]});
        tierExp.mValue.push_back(makeTag(std::move(entry)));
    }
    root.mValue["TierExpRequirements"] = makeTag(std::move(tierExp));

    return root;
}

} // namespace debugshape_export::trade
