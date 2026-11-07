#include "trade/TradeMenuManager.h"
#include "DiagLog.h"
#include "container/ContainerResponse.h"
#include "kernel/PacketSink.h"
#include "trade/TradeMenuRuntimeOps.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {
using namespace trade_ops;

void TradeMenuManager::emit(hologramlib::TradeSettlementEvent const& event) {
    std::vector<std::function<void(hologramlib::TradeSettlementEvent const&)>> snapshot;
    {
        std::lock_guard lock(mMutex);

        // 只有 tradePollSettlements 会排空这条队列；脚本只用监听器不轮询时它就是净增长
        if (mSettlementQueue.size() >= kSettlementQueueCap) {
            mSettlementQueue.pop_front();
            mDroppedSettlements.fetch_add(1, std::memory_order_relaxed);
        }
        mSettlementQueue.push_back(event);
        snapshot.reserve(mSettlementListeners.size());
        for (auto const& [token, fn] : mSettlementListeners)
            snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn)
            fn(event);
    }
}

std::string TradeMenuManager::formatSettlement(hologramlib::TradeSettlementEvent const& event) {
    return std::format(
        "player={} menuId={} offer={} ok={} reason={}",
        event.playerName,
        event.menuId,
        event.offerIndex,
        event.ok ? 1 : 0,
        event.reason);
}

bool TradeMenuManager::applyPayIn(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason) {
    auto&     inv     = player.getInventory();
    int const size    = inv.getContainerSize();
    int const srcSlot = trade::settlement::inventorySlotOf(action.srcContainer, action.slot);
    if (srcSlot < 0 || srcSlot >= size) {
        reason = "slot-out-of-range";
        return false;
    }
    ::ItemStack cur = inv.getItem(srcSlot);
    if (cur.isNull()) {
        reason = "slot-empty";
        return false;
    }
    int const have = static_cast<int>(cur.mCount);
    int const take = std::min(action.amount < 1 ? have : action.amount, have);
    if (take <= 0) {
        reason = "amount-zero";
        return false;
    }
    std::string const type = cur.getTypeName();
    if (takeFromSlot(player, srcSlot, take) != take) {
        reason = "take-failed";
        return false;
    }

    int const maxStack = std::max<int>(1, static_cast<int>(cur.getMaxStackSize()));

    // 只合并「同物品 + 同 damage + 同 NBT」且不超堆叠上限的；否则另起一条。
    // 之前只比 typeName，附魔/耐久不同的物品会按第一件的 NBT 退还，数量还会按字节回绕。
    bool merged = false;
    for (auto& e : menu.ledger) {
        if (e.logical.tradeContainer != action.tradeContainer || e.logical.tradeSlot != action.tradeSlot)
            continue;
        if (e.logical.item.type != type || e.stack.getDamageValue() != cur.getDamageValue())
            continue;
        if (!e.stack.hasSameUserData(cur))
            continue;
        if (e.logical.item.count + take > maxStack)
            continue;
        e.logical.item.count += take;
        e.stack.mCount = static_cast<unsigned char>(e.logical.item.count);
        merged         = true;
        break;
    }
    if (!merged) {
        int remaining = take;
        while (remaining > 0) {
            int const   chunk = std::min(remaining, maxStack);
            LedgerEntry e;
            e.logical.tradeContainer = action.tradeContainer;
            e.logical.tradeSlot      = action.tradeSlot;
            e.logical.item.type      = type;
            e.logical.item.count     = chunk;
            e.logical.item.damage    = static_cast<int>(cur.getDamageValue());
            e.stack                  = cur;
            e.stack.mCount           = static_cast<unsigned char>(chunk);
            menu.ledger.push_back(std::move(e));
            remaining -= chunk;
        }
    }
    HLIB_LOG_INFO(
        "[TradeMenu] 付费已入账: player={} 槽={} 类型={} 数量={} 暂存槽=(c{},{})",
        menu.playerName,
        action.slot,
        type,
        take,
        action.tradeContainer,
        action.tradeSlot);
    return true;
}

bool TradeMenuManager::applyPickUp(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason) {
    auto&     inv     = player.getInventory();
    int const size    = inv.getContainerSize();
    int const srcSlot = trade::settlement::inventorySlotOf(action.srcContainer, action.slot);
    if (srcSlot < 0 || srcSlot >= size) {
        reason = "slot-out-of-range";
        return false;
    }
    ::ItemStack cur = inv.getItem(srcSlot);
    if (cur.isNull()) {
        reason = "slot-empty";
        return false;
    }
    menu.cursor.active   = true;
    menu.cursor.stack    = cur;
    menu.cursor.fromSlot = srcSlot;
    HLIB_LOG_INFO(
        "[TradeMenu] 光标托管: player={} 拿起 {}x{}（背包槽 {}, 服务端暂不动）",
        menu.playerName,
        cur.getTypeName(),
        static_cast<int>(cur.mCount),
        action.slot);
    return true;
}

bool TradeMenuManager::applyCursorPayIn(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason) {
    if (!menu.cursor.active) {
        reason = "no-cursor-item";
        return false;
    }
    int const onCursor = static_cast<int>(menu.cursor.stack.mCount);
    int const take     = action.amount < 1 ? onCursor : action.amount;
    if (take <= 0 || take > onCursor) {
        reason = "cursor-amount-mismatch";
        return false;
    }
    if (takeFromSlot(player, menu.cursor.fromSlot, take) != take) {
        reason = "slot-empty";
        return false;
    }
    std::string const type   = menu.cursor.stack.getTypeName();
    bool              merged = false;
    for (auto& e : menu.ledger) {
        if (e.logical.tradeContainer == action.tradeContainer && e.logical.tradeSlot == action.tradeSlot && e.logical.item.type == type) {
            e.logical.item.count += take;
            e.stack.mCount = static_cast<unsigned char>(e.logical.item.count);
            merged         = true;
            break;
        }
    }
    if (!merged) {
        LedgerEntry e;
        e.logical.tradeContainer = action.tradeContainer;
        e.logical.tradeSlot      = action.tradeSlot;
        e.logical.item.type      = type;
        e.logical.item.count     = take;
        e.logical.item.damage    = static_cast<int>(menu.cursor.stack.getDamageValue());
        e.stack                  = menu.cursor.stack;
        e.stack.mCount           = static_cast<unsigned char>(take);
        menu.ledger.push_back(std::move(e));
    }
    int const left = onCursor - take;
    if (left <= 0) {
        menu.cursor = CursorShadow{};
    } else {
        menu.cursor.stack.mCount = static_cast<unsigned char>(left);
    }
    HLIB_LOG_INFO(
        "[TradeMenu] 付费已入账（光标）: player={} {}x{} → 交易槽(c{},{})",
        menu.playerName,
        type,
        take,
        action.tradeContainer,
        action.tradeSlot);
    return true;
}

bool TradeMenuManager::applyCursorBack(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason) {
    if (!menu.cursor.active) {
        reason = "no-cursor-item";
        return false;
    }
    auto&     inv     = player.getInventory();
    int const size    = inv.getContainerSize();
    int const dstSlot = trade::settlement::inventorySlotOf(action.srcContainer, action.slot);
    if (dstSlot < 0 || dstSlot >= size) {
        reason = "slot-out-of-range";
        return false;
    }
    int const count = static_cast<int>(menu.cursor.stack.mCount);
    if (dstSlot == menu.cursor.fromSlot) {
        menu.cursor = CursorShadow{};
        return true;
    }
    ::ItemStack dst = inv.getItem(dstSlot);
    if (dst.isNull()) {
        if (takeFromSlot(player, menu.cursor.fromSlot, count) != count) {
            reason = "slot-empty";
            return false;
        }
        inv.setItem(dstSlot, menu.cursor.stack);
    } else if (dst.getTypeName() == menu.cursor.stack.getTypeName()) {
        int const total = static_cast<int>(dst.mCount) + count;
        if (total > 127) {
            reason = "slot-full";
            return false;
        }
        if (takeFromSlot(player, menu.cursor.fromSlot, count) != count) {
            reason = "slot-empty";
            return false;
        }
        dst.mCount = static_cast<unsigned char>(total);
        inv.setItem(dstSlot, dst);
    } else {
        reason = "slot-occupied";
        return false;
    }
    menu.cursor = CursorShadow{};
    return true;
}

bool TradeMenuManager::applyPayOut(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason) {
    auto it = std::find_if(menu.ledger.begin(), menu.ledger.end(), [&](LedgerEntry const& e) {
        return e.logical.tradeContainer == action.tradeContainer && e.logical.tradeSlot == action.tradeSlot;
    });
    if (it == menu.ledger.end()) {
        reason = "nothing-held";
        return false;
    }
    int const have = it->logical.item.count;
    int const take = std::min(action.amount < 1 ? have : action.amount, have);
    if (take <= 0) {
        reason = "amount-zero";
        return false;
    }
    ::ItemStack back  = it->stack;
    back.mCount       = static_cast<unsigned char>(take);
    int const dstSlot = trade::settlement::inventorySlotOf(action.srcContainer, action.slot);
    if (giveToInventory(player, back, dstSlot) < 0) {
        reason = "inventory-full";
        return false;
    }
    it->logical.item.count -= take;
    it->stack.mCount = static_cast<unsigned char>(it->logical.item.count);
    if (it->logical.item.count <= 0)
        menu.ledger.erase(it);
    return true;
}

bool TradeMenuManager::settleDeal(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    int                     productSlot,
    std::string&            reason) {
    int const index = action.offerIndex;
    if (index < 0 || index >= static_cast<int>(menu.spec.offers.size())) {
        reason = "offer-out-of-range";
        return false;
    }
    auto const& offer = menu.spec.offers[static_cast<std::size_t>(index)];

    std::vector<HeldPayment> heldView;
    heldView.reserve(menu.ledger.size());
    for (auto const& e : menu.ledger)
        heldView.push_back(e.logical);
    std::vector<HeldPayment> taken;
    if (!trade::settlement::planConsumption(heldView, offer, taken)) {
        reason = "payment-missing";
        return false;
    }
    if (offer.sell.type.empty()) {
        reason = "no-product";
        return false;
    }
    ::ItemStack product = makeItemStackOf(offer.sell);
    if (product.isNull()) {
        reason = "bad-product";
        return false;
    }

    bool const canPlace =
        [&] {
            auto&     inv  = player.getInventory();
            int const size = inv.getContainerSize();
            if (productSlot >= 0 && productSlot < size) {
                ::ItemStack const& cur = inv.getItem(productSlot);
                if (cur.isNull() || cur.getTypeName() == product.getTypeName())
                    return true;
            }
            for (int slot = 0; slot < size; ++slot) {
                ::ItemStack const& cur = inv.getItem(slot);
                if (cur.isNull())
                    return true;
                if (cur.getTypeName() == product.getTypeName() && static_cast<int>(cur.mCount) + static_cast<int>(product.mCount) <= 127) {
                    return true;
                }
            }
            return false;
        }();
    if (!canPlace) {
        reason = "inventory-full";
        return false;
    }

    int const placed = giveToInventory(player, product, productSlot);
    if (placed < 0) {
        reason = "inventory-full";
        return false;
    }
    for (auto const& t : taken) {
        for (auto& e : menu.ledger) {
            if (e.logical.tradeContainer != t.tradeContainer || e.logical.tradeSlot != t.tradeSlot)
                continue;
            if (e.logical.item.type != t.item.type || e.logical.item.count <= 0)
                continue;
            int const cut = std::min(t.item.count, e.logical.item.count);
            e.logical.item.count -= cut;
            e.stack.mCount = static_cast<unsigned char>(e.logical.item.count);
            break;
        }
    }
    menu.ledger.erase(
        std::remove_if(
            menu.ledger.begin(),
            menu.ledger.end(),
            [](LedgerEntry const& e) {
                return e.logical.item.count <= 0;
            }),
        menu.ledger.end());
    HLIB_LOG_INFO(
        "[TradeMenu] 真结算完成: player={} 条目={} 产物={}x{} → 背包槽={}",
        menu.playerName,
        index,
        offer.sell.type,
        offer.sell.count,
        placed);
    return true;
}

int TradeMenuManager::refundHeld(::Player& player, std::vector<LedgerEntry>& ledger) {
    int refunded = 0;
    for (auto& e : ledger) {
        if (e.logical.item.type.empty() || e.logical.item.count <= 0)
            continue;
        ::ItemStack back = e.stack;
        back.mCount      = static_cast<unsigned char>(e.logical.item.count);
        if (!back.isNull() && giveToInventory(player, back, -1) >= 0) {
            HLIB_LOG_INFO(
                "[TradeMenu] 退还暂存的付费: player={} {} x{}",
                player.getRealName(),
                e.logical.item.type,
                e.logical.item.count);
            e.logical.item.count = 0;
            ++refunded;
        } else {
            HLIB_LOG_WARN(
                "[TradeMenu] 退还失败（背包满）: player={} {} x{} —— 保留在账本里, 下次关界面再试",
                player.getRealName(),
                e.logical.item.type,
                e.logical.item.count);
        }
    }
    ledger.erase(
        std::remove_if(ledger.begin(), ledger.end(), [](LedgerEntry const& e) {
            return e.logical.item.count <= 0;
        }),
        ledger.end());
    return refunded;
}

void TradeMenuManager::stashPendingRefund(std::string const& playerName, std::vector<LedgerEntry>& ledger) {
    if (ledger.empty())
        return;
    std::lock_guard lock(mMutex);
    auto&           dst = mPendingRefunds[playerName];
    for (auto& e : ledger) {
        dst.push_back(std::move(e));
    }
    HLIB_LOG_WARN(
        "[TradeMenu] 背包放不下，{} 条目转为待退还: player={}（进服或下次操作时重试）",
        dst.size(),
        playerName);
    ledger.clear();
}

void TradeMenuManager::drainPendingRefunds(::Player& player) {
    std::vector<LedgerEntry> pending;
    {
        std::lock_guard lock(mMutex);
        auto            it = mPendingRefunds.find(player.getRealName());
        if (it == mPendingRefunds.end() || it->second.empty())
            return;
        pending = std::move(it->second);
        mPendingRefunds.erase(it);
    }

    refundHeld(player, pending);
    if (pending.empty()) {
        return;
    }

    std::lock_guard lock(mMutex);
    auto&           dst = mPendingRefunds[player.getRealName()];
    for (auto& e : pending) {
        dst.push_back(std::move(e));
    }
}

bool TradeMenuManager::sendTradeSlotUpdate(
    ::Player&          player,
    Menu const&        menu,
    int                tradeSlot,
    ::ItemStack const& item) {
    ::InventorySlotPacket packet{::InventorySlotPacketPayload(
        static_cast<::ContainerID>(menu.containerId),
        static_cast<uint>(tradeSlot),
        item,
        ::FullContainerName{},
        ::ItemStack{})};
    (void)wire::send(player, packet);
    HLIB_LOG_INFO(
        "[TradeMenu] 交易槽内容已推送: containerId={} slot={} 物品={}x{}",
        menu.containerId,
        tradeSlot,
        item.isNull() ? "(空)" : item.getTypeName(),
        item.isNull() ? 0 : static_cast<int>(item.mCount));
    return true;
}

} // namespace debugshape_export
