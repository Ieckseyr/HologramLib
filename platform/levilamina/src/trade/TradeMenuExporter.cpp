#include "TradeMenuExporter.h"
#include "TradeMenuManager.h"

#include "lse/LseBridge.h"

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void TradeMenuExporter::exportAll() {
    auto& mgr = TradeMenuManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "tradeOpen",
        [&mgr](
            std::string const& playerName,
            std::string const& tradeType,
            int                tier,
            int                experience,
            bool               realTrades,
            std::string const& carrierIdentifier) -> int64_t {
            hologramlib::TradeMenuSpec spec;
            spec.tradeType       = tradeType;
            spec.tier            = tier;
            spec.experience      = experience;
            spec.usePacketOffers = !realTrades;
            if (!carrierIdentifier.empty())
                spec.carrierIdentifier = carrierIdentifier;
            return mgr.open(playerName, spec);
        });

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
            bool               locked) -> bool {
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

    hologramlib::lse::exportAs(
        NAMESPACE,
        "tradeSetTier",
        [&mgr](int64_t id, int tier, int experience) -> bool {
            return mgr.setTier(id, tier, experience);
        });

    hologramlib::lse::exportAs(NAMESPACE, "tradeClose", [&mgr](int64_t id) -> bool {
        return mgr.close(id);
    });

    hologramlib::lse::exportAs(NAMESPACE, "tradeCloseAll", [&mgr]() -> void {
        mgr.closeAll();
    });

    hologramlib::lse::exportAs(NAMESPACE, "tradeIsOpen", [&mgr](int64_t id) -> bool {
        return mgr.isOpen(id);
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "tradeGetIds", [&mgr]() -> std::vector<int64_t> {
            return mgr.getAllIds();
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "tradeSetSettle", [&mgr](int64_t id, bool on) -> bool {
            return mgr.setSettleLocally(id, on);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "tradeIsSettle", [&mgr](int64_t id) -> bool {
            return mgr.isSettleLocally(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "tradeUpdate",
        [&mgr](int64_t id, std::string const& tradeType, int tier, int experience) -> bool {
            hologramlib::TradeMenuSpec spec;
            if (!mgr.getSpec(id, spec))
                return false;
            if (!tradeType.empty())
                spec.tradeType = tradeType;
            if (tier > 0)
                spec.tier = tier;
            if (experience >= 0)
                spec.experience = experience;
            return mgr.update(id, spec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "tradePollSettlements", []() -> std::vector<std::string> {
            return TradeMenuManager::getInstance().pollSettlements();
        });

    hologramlib::lse::exportAs(NAMESPACE, "tradeDroppedSettlements", []() -> int64_t {
        return static_cast<int64_t>(TradeMenuManager::getInstance().droppedSettlements());
    });
}

} // namespace debugshape_export
