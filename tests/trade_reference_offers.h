// trade_reference_offers.h - 参考样本（logs/fullpkts/pkt80_194951_007.bin, 26.40 原生屠夫村民）
// 的 offers 逐字段解码结果。供两个离线对拍 fixture 共用, 避免两份数据各自漂移。
#pragma once

#include "trade/TradeOfferNbt.h"

#include <vector>

namespace debugshape_export::tests {

// 字段顺序见 src/trade/TradeOfferNbt.h 的 TradeOffer / TradeItem。
// 含一个方块物品（minecraft:dried_kelp_block, isBlock = true）—— 方块物品必须带 Block 复合。
inline std::vector<trade::TradeOffer> referenceOffers() {
    return {
        {"minecraft:porkchop", 7, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 0, 16, 0, 3676, 2, 0, 0.05f, 0.0f, true},
        {"minecraft:emerald", 1, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:rabbit_stew", 1, 0, "", {}, false, 0, 0, 12, 0, 3677, 1, 0, 0.05f, 0.0f, true},
        {"minecraft:coal", 15, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 1, 16, 0, 3678, 10, 0, 0.05f, 0.0f, true},
        {"minecraft:emerald", 1, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:cooked_chicken", 8, 0, "", {}, false, 0, 1, 16, 0, 3679, 5, 0, 0.05f, 0.0f, true},
        {"minecraft:beef", 10, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 2, 16, 0, 3680, 20, 0, 0.05f, 0.0f, true},
        {"minecraft:dried_kelp_block", 10, 0, "", {}, true, 18168865, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 3, 12, 0, 3681, 30, 0, 0.05f, 0.0f, true},
        {"minecraft:sweet_berries", 10, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 4, 12, 0, 3682, 30, 0, 0.05f, 0.0f, true},
    };
}

// 参考样本的包级字段（非 Data 段）。用于整包逐字节对拍。
// 取值与 referenceOffers() 同源 —— 都来自 pkt80_194951_007.bin, 不可混用其他样本。
inline constexpr int          kRefContainerId     = 2;
inline constexpr int          kRefContainerType   = 15; // ContainerType::Trade
inline constexpr int          kRefSize            = 0;
inline constexpr int          kRefTier            = 0;
inline constexpr std::int64_t kRefEntityUniqueId  = -768799145975LL;
inline constexpr std::int64_t kRefLastTrader      = -1;
inline constexpr char const*  kRefDisplayName     = "entity.villager.butcher";

} // namespace debugshape_export::tests
