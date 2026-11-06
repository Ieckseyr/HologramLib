#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "hologramlib/HologramLib.h"
#include "trade/TradeSettlementLogic.h"

#include <ll/api/event/Listener.h>

#include <mc/world/item/ItemStack.h>

class Player;

namespace debugshape_export {

class TradeMenuManager {
public:
    static TradeMenuManager& getInstance();

    int64_t open(std::string const& playerName, hologramlib::TradeMenuSpec const& spec);
    bool    update(int64_t menuId, hologramlib::TradeMenuSpec const& spec);

    // 取当前生效的 spec（含已追加的 offers），供 LSE 的「就地改类型/等级/经验」用
    [[nodiscard]] bool   getSpec(int64_t menuId, hologramlib::TradeMenuSpec& out) const;
    bool                 close(int64_t menuId);
    void                 closeAll();
    bool                 isOpen(int64_t menuId) const;
    std::vector<int64_t> getAllIds() const;

    bool addOffer(int64_t menuId, hologramlib::TradeMenuOffer const& offer);
    bool setTier(int64_t menuId, int tier, int experience);

    [[nodiscard]] bool hasMenuFor(std::string const& playerName) const;

    bool handleContainerClose(std::string const& playerName, int containerId);

    void onPlayerLeave(std::string const& playerName);

    void onPlayerDisconnect(::Player& player);

    void init();

    void shutdown();

    using TradeRequestAction = trade::settlement::RequestAction;
    using ActionKind         = trade::settlement::ActionKind;
    using ClassifiedAction   = trade::settlement::Classified;
    using HeldPayment        = trade::settlement::HeldPayment;

    struct LedgerEntry {
        HeldPayment logical;
        ::ItemStack stack;
    };

    struct CursorShadow {
        bool        active{false};
        ::ItemStack stack;
        int         fromSlot{-1};
    };

    bool handleItemRequest(
        ::Player&                              player,
        std::vector<TradeRequestAction> const& actions,
        int                                    requestId);
    bool               setSettleLocally(int64_t menuId, bool on);
    [[nodiscard]] bool isSettleLocally(int64_t menuId) const;

    [[nodiscard]] bool hasSettlingMenuFor(std::string const& playerName) const;

    uint64_t addSettlementListener(std::function<void(hologramlib::TradeSettlementEvent const&)> listener);
    bool     removeSettlementListener(uint64_t token);

    std::vector<std::string>         pollSettlements();
    [[nodiscard]] static std::string formatSettlement(hologramlib::TradeSettlementEvent const& event);

    // 队列满时丢掉的结算条数：不暴露计数，脚本侧看不出自己在漏
    [[nodiscard]] std::uint64_t droppedSettlements() const {
        return mDroppedSettlements.load(std::memory_order_relaxed);
    }

private:
    TradeMenuManager() = default;

    struct Menu {
        int64_t                    menuId{0};
        std::string                playerName;
        int                        containerId{0};
        int64_t                    carrierLibId{-1};
        std::uint64_t              carrierUniqueId{0};
        std::uint64_t              carrierRuntimeId{0};
        hologramlib::TradeMenuSpec spec;

        bool serverTrades{false};

        int netIdBase{3676};

        bool                     settleLocally{false};
        std::vector<LedgerEntry> ledger;
        CursorShadow             cursor;
    };

    int64_t spawnCarrier(::Player& player, std::string const& playerName, hologramlib::TradeMenuSpec const& spec);

    void destroyCarrier(Menu& menu);

    bool installTrades(::Player& player, Menu const& menu);
    void sendUpdateTrade(::Player& player, Menu const& menu);
    void sendContainerClose(::Player& player, Menu const& menu);
    void sendTradeExp(::Player& player, Menu const& menu);

    void laterSendTrade(std::string playerName, int64_t menuId);

    bool resendTradeTable(int64_t menuId);
    int  allocateContainerId();

    bool applyPayIn(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);

    bool applyPickUp(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);

    bool applyCursorPayIn(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);

    bool applyCursorBack(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);

    bool applyPayOut(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);

    bool settleDeal(
        ::Player&               player,
        Menu&                   menu,
        ClassifiedAction const& action,
        int                     productSlot,
        std::string&            reason);

    int  refundHeld(::Player& player, std::vector<LedgerEntry>& ledger);
    void stashPendingRefund(std::string const& playerName, std::vector<LedgerEntry>& ledger);
    void drainPendingRefunds(::Player& player);

    bool sendTradeSlotUpdate(::Player& player, Menu const& menu, int tradeSlot, ::ItemStack const& item);
    void emit(hologramlib::TradeSettlementEvent const& event);

    // 结算队列上限：只有 tradePollSettlements 会排空，脚本只挂监听器时就是净增长
    static constexpr std::size_t kSettlementQueueCap = 1024;

    mutable std::mutex                                        mMutex;
    std::unordered_map<int64_t, Menu>                         mMenus;
    std::unordered_map<std::string, int64_t>                  mByPlayer;
    std::unordered_map<std::string, std::vector<LedgerEntry>> mPendingRefunds;
    int64_t                                                   mNextMenuId{1};
    int                                                       mNextContainerId{3};

    std::unordered_map<uint64_t, std::function<void(hologramlib::TradeSettlementEvent const&)>>
                                                  mSettlementListeners;
    std::deque<hologramlib::TradeSettlementEvent> mSettlementQueue;
    std::atomic<std::uint64_t>                    mDroppedSettlements{0};
    uint64_t                                      mNextSettlementToken{1};
    ll::event::ListenerPtr                        mDisconnectListener;
    ll::event::ListenerPtr                        mJoinListener;
};

hologramlib::ITradeMenu& tradeMenuAdapter();

} // namespace debugshape_export
