// TradeMenuExporter.cpp - 村民交易菜单（纯展示）LSE 导出实现
//
// 域契约: 打开界面 + 摆出交易表, **不做任何点击事件监听**（见 HologramLib.h 的域注释）。
// 所以本域没有轮询/回调类导出 —— 要"点了有反应"的列表界面用 container* 域。
#include "TradeMenuExporter.h"
#include "TradeMenuManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void TradeMenuExporter::exportAll() {
    auto& mgr = TradeMenuManager::getInstance();

    // tradeOpen(playerName, tradeType, tier, experience, realTrades, carrierIdentifier) -> id
    //   tradeType: "entity.villager.butcher" / "entity.villager.priest" / 任意自定义串
    //   tier:      1 基显示栏值 1=新手 … 5=大师
    //   realTrades: false（推荐）= 纯协议层展示（服务端没有交易表, 天然只读）;
    //               true = 给载体装真实交易表并走 BDS openTrading（玩家真的能成交、物品真的消耗）
    //   carrierIdentifier: "" = 默认 minecraft:villager_v2; 也可 "minecraft:wandering_trader"
    hologramlib::lse::exportAs(
        NAMESPACE,
        "tradeOpen",
        [&mgr](
            std::string const& playerName,
            std::string const& tradeType,
            int                tier,
            int                experience,
            bool               realTrades,
            std::string const& carrierIdentifier
        ) -> int64_t {
            hologramlib::TradeMenuSpec spec;
            spec.tradeType       = tradeType;
            spec.tier            = tier;
            spec.experience      = experience;
            spec.usePacketOffers = !realTrades;
            if (!carrierIdentifier.empty()) spec.carrierIdentifier = carrierIdentifier;
            return mgr.open(playerName, spec);
        });

    // tradeAddOffer(id, buyAType, buyACount, buyBType, buyBCount, sellType, sellCount, sellName, tier, locked) -> bool
    //   追加一条交易并就地重发交易表（不重开界面）; buyBType 空 = 没有第二付费项
    hologramlib::lse::exportAs(
        NAMESPACE,
        "tradeAddOffer",
        [&mgr](
            int64_t            id,
            std::string const& buyAType,
            int                buyACount,
            std::string const& buyBType,
            int                buyBCount,
            std::string const& sellType,
            int                sellCount,
            std::string const& sellName,
            int                tier,
            bool               locked
        ) -> bool {
            hologramlib::TradeMenuOffer offer;
            offer.buyA.type  = buyAType;
            offer.buyA.count = buyACount < 1 ? 1 : buyACount;
            if (!buyBType.empty()) {
                offer.buyB.type  = buyBType;
                offer.buyB.count = buyBCount < 1 ? 1 : buyBCount;
            }
            offer.sell.type  = sellType;
            offer.sell.count = sellCount < 1 ? 1 : sellCount;
            offer.sell.name  = sellName;
            offer.tier       = tier;
            offer.locked     = locked;
            return mgr.addOffer(id, offer);
        });

    // tradeSetTier(id, tier, experience) -> bool（改显示栏值 / 经验条, 就地重发）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "tradeSetTier",
        [&mgr](int64_t id, int tier, int experience) -> bool { return mgr.setTier(id, tier, experience); });

    // tradeClose(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "tradeClose", [&mgr](int64_t id) -> bool { return mgr.close(id); });

    // tradeCloseAll() -> void（关掉全部交易菜单, 删除全部载体实体）
    hologramlib::lse::exportAs(NAMESPACE, "tradeCloseAll", [&mgr]() -> void { mgr.closeAll(); });

    // tradeIsOpen(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "tradeIsOpen", [&mgr](int64_t id) -> bool { return mgr.isOpen(id); });

    // tradeGetIds() -> [i]
    hologramlib::lse::exportAs(
        NAMESPACE, "tradeGetIds", [&mgr]() -> std::vector<int64_t> { return mgr.getAllIds(); });
}

} // namespace debugshape_export
