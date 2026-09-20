// TradeOfferNbt.h - 村民交易 Offers NBT 构造（26.40 实测规格）
//
// 规格不是推测的: 逐字段来自 BDS 26.40 原生村民交易抓包
//   logs/fullpkts/pkt80_*.bin  (解码器 tests/decode_update_trade.py)
// 并由 tests/check-trade-offers.py 做逐字节对拍（构造结果必须与 BDS 原始 Data 段完全一致）。
//
// 根 compound 两个键:
//   Recipes: List<Compound>, 每条 13 个字段（NBT map 按字母序落盘, 与 BDS 输出一致）:
//     buyA / buyB / sell   物品 item NBT: {Name:string, Count:byte, Damage:short, WasPickedUp:byte}
//                          自定义名/Lore 走 tag.display.Name / tag.display.Lore
//                          （Bedrock 标准 item NBT; 抓包里的原版交易物品不带 tag, 故此项未经真值验证）
//     buyCountA / buyCountB  对应物品数量（实测与物品 Count 相同）
//     tier                 该条解锁等级: 0=新手 … 4=大师。高于村民等级时客户端显示为未解锁
//     uses / maxUses       已用 / 上限
//     demand               需求（实测 0）
//     priceMultiplierA/B   价格浮动（实测 0.05 / 0）
//     rewardExp            是否给经验（byte, 实测 1）
//     traderExp            本条给村民的经验
//     netId                每条唯一 id（BDS 实测 3676…, 跨会话稳定不重发新值）
//   TierExpRequirements: List<Compound> = [{0:0},{1:10},{2:70},{3:150},{4:250}]
//                         等级经验门槛; 经验条由它 + 实体元数据 TradeExperience(103) 共同驱动
//
// 编码方言（已由抓包证实, 与 sculk ValueTag 一致）: Int/Long 是 zigzag varint,
// Short/Float/Double 定长, 字符串与列表计数 varint。所以直接用 sculk 的 NBT 类型构造即可,
// 不要按经典定宽 NBT 手写字节。
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <sculk/protocol/codec/nbt/CompoundTag.hpp>
#include <sculk/protocol/codec/nbt/ListTag.hpp>
#include <sculk/protocol/codec/nbt/TagVariant.hpp>
#include <sculk/protocol/codec/nbt/TagType.hpp>

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

// TagVariant 是聚合体, 内含一层 TaggedVariant(std::variant 的包装);
// 直接用值聚合初始化即可 (TaggedVariant 的转发构造器会选中对应分支)
template <typename T>
inline TagVariant makeTag(T&& value) {
    return TagVariant{std::forward<T>(value)};
}

// 一条交易的付费/产出物品
struct TradeItem {
    std::string              type;      // minecraft:xxx（空 = 无此物品, 例如无第二付费项）
    int                      count{1};
    int                      damage{0};
    std::string              name;      // 自定义名（空 = 无; 走 tag.display.Name）
    std::vector<std::string> lore;      // 描述行（走 tag.display.Lore）
    // 方块类物品: 实测 item NBT 会多一个 Block 复合
    //   Block:{name:"minecraft:xxx", states:{}, version:18168865}
    // 原版村民交易里 dried_kelp_block 这类物品带它, 普通物品不带。version 是 26.40 实测值。
    bool isBlock{false};
    int  blockVersion{18168865};
};

// 一条交易（对应 Offers.Recipes 的一个元素）
struct TradeOffer {
    TradeItem buyA;                     // 付费物品 A（必填）
    TradeItem buyB;                     // 付费物品 B（type 空 = 省略该键）
    TradeItem sell;                     // 产出物品
    int       tier{0};                  // 显示栏值, 0 基（0=新手 … 4=大师）
    int       maxUses{16};
    int       uses{0};
    int       netId{0};                 // 每条唯一; <=0 时由 buildOffers 自动分配
    int       traderExp{0};
    int       demand{0};
    float     priceMultiplierA{0.05f};
    float     priceMultiplierB{0.0f};
    bool      rewardExp{true};
};

// 等级经验门槛（BDS 默认值, 阶数与下标一致）
inline std::vector<int> defaultTierExpRequirements() { return {0, 10, 70, 150, 250}; }

namespace detail {

// 物品 item NBT（Bedrock 网络 NBT）: {Name, Count, Damage, WasPickedUp} (+ tag.display)
inline CompoundTag itemToNbt(TradeItem const& item) {
    CompoundTag out;
    out.mValue["Name"]        = makeTag(StringTag{item.type});
    out.mValue["Count"]       = makeTag(ByteTag{static_cast<std::int8_t>(item.count)});
    out.mValue["Damage"]      = makeTag(ShortTag{static_cast<std::int16_t>(item.damage)});
    out.mValue["WasPickedUp"] = makeTag(ByteTag{0}); // 实测原版均为 0
    if (item.isBlock) {
        CompoundTag block;
        block.mValue["name"]    = makeTag(StringTag{item.type});
        block.mValue["states"]  = makeTag(CompoundTag{}); // 实测空 compound
        block.mValue["version"] = makeTag(IntTag{item.blockVersion});
        out.mValue["Block"]     = makeTag(std::move(block));
    }
    if (!item.name.empty() || !item.lore.empty()) {
        CompoundTag display;
        if (!item.name.empty()) display.mValue["Name"] = makeTag(StringTag{item.name});
        if (!item.lore.empty()) {
            ListTag lore;
            lore.mType = TagType::String;
            lore.mValue.reserve(item.lore.size());
            for (auto const& line : item.lore) lore.mValue.push_back(makeTag(StringTag{line}));
            display.mValue["Lore"] = makeTag(std::move(lore));
        }
        CompoundTag tag;
        tag.mValue["display"] = makeTag(std::move(display));
        out.mValue["tag"]     = makeTag(std::move(tag));
    }
    return out;
}

inline CompoundTag offerToNbt(TradeOffer const& offer) {
    CompoundTag out;
    out.mValue["buyA"] = makeTag(itemToNbt(offer.buyA));
    if (!offer.buyB.type.empty()) out.mValue["buyB"] = makeTag(itemToNbt(offer.buyB));
    out.mValue["sell"] = makeTag(itemToNbt(offer.sell));

    out.mValue["buyCountA"]       = makeTag(IntTag{offer.buyA.count});
    out.mValue["buyCountB"]       = makeTag(IntTag{offer.buyB.type.empty() ? 0 : offer.buyB.count});
    out.mValue["tier"]            = makeTag(IntTag{offer.tier});
    out.mValue["uses"]            = makeTag(IntTag{offer.uses});
    out.mValue["maxUses"]         = makeTag(IntTag{offer.maxUses});
    out.mValue["demand"]          = makeTag(IntTag{offer.demand});
    out.mValue["priceMultiplierA"] = makeTag(FloatTag{offer.priceMultiplierA});
    out.mValue["priceMultiplierB"] = makeTag(FloatTag{offer.priceMultiplierB});
    out.mValue["rewardExp"]       = makeTag(ByteTag{static_cast<std::int8_t>(offer.rewardExp ? 1 : 0)});
    out.mValue["traderExp"]       = makeTag(IntTag{offer.traderExp});
    out.mValue["netId"]           = makeTag(IntTag{offer.netId});
    return out;
}

} // namespace detail

// 构造 UpdateTradePacket 的 Data 字段（根 compound）
// netId <= 0 的条目按 baseNetId + 序号自动分配（BDS 用一组稳定的小整数, 只要唯一即可）
inline CompoundTag buildOffers(
    std::vector<TradeOffer> const& offers,
    std::vector<int> const&        tierExpRequirements = defaultTierExpRequirements(),
    int                            baseNetId           = 3676
) {
    CompoundTag root;

    ListTag recipes;
    recipes.mType = TagType::Compound;
    recipes.mValue.reserve(offers.size());
    for (std::size_t i = 0; i < offers.size(); ++i) {
        TradeOffer o = offers[i];
        if (o.netId <= 0) o.netId = baseNetId + static_cast<int>(i);
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
