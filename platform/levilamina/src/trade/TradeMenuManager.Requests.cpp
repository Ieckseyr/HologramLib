#include "trade/TradeMenuManager.h"
#include "DiagLog.h"
#include "container/ContainerResponse.h"
#include "kernel/PacketSink.h"
#include "trade/TradeMenuRuntimeOps.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {

bool TradeMenuManager::handleItemRequest(
    ::Player&                              player,
    std::vector<TradeRequestAction> const& actions,
    int                                    requestId) {
    if (actions.empty())
        return false;

    Menu menu;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(player.getRealName());
        if (byPlayer == mByPlayer.end())
            return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end())
            return false;

        if (!menuIt->second.settleLocally || menuIt->second.serverTrades)
            return false;
        menu = menuIt->second;
    }

    auto const classified = trade::settlement::classify(
        actions,
        menu.netIdBase,
        static_cast<int>(menu.spec.offers.size()));

    bool anyTrade    = false;
    bool touchCursor = false;
    for (auto const& c : classified) {
        if (c.touchesCursor)
            touchCursor = true;
        if (c.kind == ActionKind::PayIn || c.kind == ActionKind::PayOut || c.kind == ActionKind::Deal || c.kind == ActionKind::PickUp || c.kind == ActionKind::CursorPayIn || c.kind == ActionKind::CursorBack) {
            anyTrade = true;
        }
    }
    if (!anyTrade) {

        if (touchCursor) {
            std::lock_guard lock(mMutex);
            if (auto it = mMenus.find(menu.menuId); it != mMenus.end())
                it->second.cursor = CursorShadow{};
        }
        return false;
    }

    int productSlot = -1;
    for (auto const& c : classified) {
        if (c.kind == ActionKind::ResultOut && c.slot >= 0) {

            productSlot = trade::settlement::inventorySlotOf(c.srcContainer, c.slot);
            break;
        }
    }

    bool changed = false;

    bool        needResync = false;
    bool        ok         = true;
    std::string reason;

    for (auto const& c : classified) {
        if (c.kind != ActionKind::PickUp)
            continue;
        if (!applyPickUp(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed = true;
    }
    for (auto const& c : classified) {
        if (!ok)
            break;
        if (c.kind != ActionKind::PayIn)
            continue;
        if (!applyPayIn(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed = true;
    }
    for (auto const& c : classified) {
        if (!ok)
            break;
        if (c.kind != ActionKind::CursorPayIn)
            continue;
        if (!applyCursorPayIn(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed = true;
    }
    for (auto const& c : classified) {
        if (!ok)
            break;
        if (c.kind != ActionKind::CursorBack)
            continue;
        if (!applyCursorBack(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed    = true;
        needResync = true;
    }
    for (auto const& c : classified) {
        if (!ok)
            break;
        if (c.kind != ActionKind::PayOut)
            continue;
        if (!applyPayOut(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed    = true;
        needResync = true;
    }
    int dealtOffer = -1;
    // 账本先落库、再通知：监听器在 emit 里关界面会按「已存的那份」退款，
    // 先 emit 后落库就等于退了成交前的账本（货已给出）→ 双倍退还
    auto commitLedger = [&] {
        std::lock_guard lock(mMutex);
        if (auto it = mMenus.find(menu.menuId); it != mMenus.end()) {
            it->second.ledger = std::move(menu.ledger);
            it->second.cursor = menu.cursor;
            if (touchCursor)
                it->second.cursor = CursorShadow{};
        }
        menu.ledger.clear();
    };
    for (auto const& c : classified) {
        if (!ok || c.kind != ActionKind::Deal)
            continue;
        if (!settleDeal(player, menu, c, productSlot, reason)) {
            ok = false;
            break;
        }
        changed    = true;
        needResync = true;
        dealtOffer = c.offerIndex;
        commitLedger();
        emit(hologramlib::TradeSettlementEvent{
            player.getRealName(),
            menu.menuId,
            c.offerIndex,
            true,
            {}});
    }

    commitLedger();

    container::sendRequestSuccess(player, {requestId});
    if (ok && changed) {
        for (auto const& c : classified) {
            if (c.kind != ActionKind::PayIn && c.kind != ActionKind::CursorPayIn && c.kind != ActionKind::PayOut) {
                continue;
            }

            ::ItemStack slotItem{};
            {
                std::lock_guard lock(mMutex);
                if (auto it = mMenus.find(menu.menuId); it != mMenus.end()) {
                    for (auto const& e : it->second.ledger) {
                        if (e.logical.tradeContainer == c.tradeContainer && e.logical.tradeSlot == c.tradeSlot) {
                            slotItem = e.stack;
                            break;
                        }
                    }
                }
            }
            sendTradeSlotUpdate(player, menu, c.tradeSlot, slotItem);
        }
    }

    if (!ok) {
        std::string const why = reason.empty() ? "settle-failed" : reason;

        if (changed)
            refundHeld(player, menu.ledger);
        HLIB_LOG_WARN(
            "[TradeMenu] 真结算未完成: player={} 原因={}（已退还本次扣下的付费, 请求交回 BDS）",
            menu.playerName,
            why);
        commitLedger();
        emit(hologramlib::TradeSettlementEvent{player.getRealName(), menu.menuId, dealtOffer, false, why});
        return false;
    }
    if (changed && needResync) {

        player.sendInventory(false);
    }
    return true;
}

} // namespace debugshape_export
