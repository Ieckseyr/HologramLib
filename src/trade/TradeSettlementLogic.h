#pragma once

#include "hologramlib/HologramLib.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace debugshape_export::trade::settlement {

inline constexpr int kCombinedInventory   = 12;
inline constexpr int kHotbar              = 28;
inline constexpr int kInventory           = 29;
inline constexpr int kOffhand             = 34;
inline constexpr int kCursor              = 59;
inline constexpr int kIngredient1         = 31;
inline constexpr int kIngredient2         = 32;
inline constexpr int kResultPreview       = 33;
inline constexpr int kTrade2Ingredient1   = 47;
inline constexpr int kTrade2Ingredient2   = 48;
inline constexpr int kTrade2ResultPreview = 49;
inline constexpr int kCreatedOutput       = 60;

[[nodiscard]] inline bool isPaymentContainer(int c) {
    return c == kIngredient1 || c == kIngredient2 || c == kTrade2Ingredient1 || c == kTrade2Ingredient2;
}

[[nodiscard]] inline bool isInventoryContainer(int c) {
    return c == kCombinedInventory || c == kHotbar || c == kInventory;
}

[[nodiscard]] inline int inventorySlotOf(int containerEnum, int slot) {
    if (slot < 0)
        return -1;
    if (containerEnum == kInventory)
        return slot + 9;
    return slot;
}

[[nodiscard]] inline bool isResultContainer(int c) {
    return c == kResultPreview || c == kTrade2ResultPreview || c == kCreatedOutput;
}

struct RequestAction {
    int  type{0};
    int  amount{1};
    bool hasSrc{false};
    bool hasDst{false};
    int  srcContainer{0};
    int  srcContainerId{-1};
    int  srcSlot{-1};
    int  dstContainer{0};
    int  dstContainerId{-1};
    int  dstSlot{-1};
    int  recipeNetId{-1};

    int srcNetId{0};
    int dstNetId{0};
};

enum class ActionKind {
    Other,
    PayIn,
    PayOut,
    ResultOut,
    Deal,
    PickUp,
    CursorPayIn,
    CursorBack,
};

struct Classified {
    ActionKind kind{ActionKind::Other};
    int        offerIndex{-1};
    int        amount{0};
    int        slot{-1};
    int        tradeContainer{0};
    int        tradeSlot{-1};
    int        srcContainer{0};
    bool       touchesCursor{false};
};

struct HeldPayment {
    int                        tradeContainer{0};
    int                        tradeSlot{-1};
    hologramlib::TradeMenuItem item;
};

[[nodiscard]] inline std::vector<Classified> classify(
    std::vector<RequestAction> const& actions,
    int                               netIdBase,
    int                               offerCount) {
    std::vector<Classified> out;
    out.reserve(actions.size());
    for (auto const& a : actions) {
        Classified c;

        if (a.recipeNetId >= netIdBase && a.recipeNetId < netIdBase + offerCount) {
            c.kind       = ActionKind::Deal;
            c.offerIndex = a.recipeNetId - netIdBase;
            out.push_back(c);
            continue;
        }
        bool const srcPay = a.hasSrc && isPaymentContainer(a.srcContainer);
        bool const dstPay = a.hasDst && isPaymentContainer(a.dstContainer);
        bool const srcRes = a.hasSrc && isResultContainer(a.srcContainer);
        bool const srcInv = a.hasSrc && isInventoryContainer(a.srcContainer);
        bool const dstInv = a.hasDst && isInventoryContainer(a.dstContainer);
        bool const srcCur = a.hasSrc && a.srcContainer == kCursor;
        bool const dstCur = a.hasDst && a.dstContainer == kCursor;
        if (srcInv && dstCur) {

            c.kind         = ActionKind::PickUp;
            c.amount       = a.amount;
            c.slot         = a.srcSlot;
            c.srcContainer = a.srcContainer;
        } else if (srcCur && dstPay) {

            c.kind           = ActionKind::CursorPayIn;
            c.amount         = a.amount;
            c.tradeContainer = a.dstContainer;
            c.tradeSlot      = a.dstSlot;
        } else if (srcCur && dstInv) {

            c.kind         = ActionKind::CursorBack;
            c.amount       = a.amount;
            c.slot         = a.dstSlot;
            c.srcContainer = a.dstContainer;
        } else if (srcInv && dstPay) {
            c.kind           = ActionKind::PayIn;
            c.amount         = a.amount;
            c.slot           = a.srcSlot;
            c.srcContainer   = a.srcContainer;
            c.tradeContainer = a.dstContainer;
            c.tradeSlot      = a.dstSlot;
        } else if (srcPay && dstInv) {
            c.kind           = ActionKind::PayOut;
            c.amount         = a.amount;
            c.slot           = a.dstSlot;
            c.srcContainer   = a.dstContainer;
            c.tradeContainer = a.srcContainer;
            c.tradeSlot      = a.srcSlot;
        } else if (srcRes && dstInv) {
            c.kind         = ActionKind::ResultOut;
            c.slot         = a.dstSlot;
            c.srcContainer = a.dstContainer;
        } else if (srcCur || dstCur) {

            c.touchesCursor = true;
        }
        out.push_back(c);
    }
    return out;
}

[[nodiscard]] inline bool planConsumption(
    std::vector<HeldPayment> const&    held,
    hologramlib::TradeMenuOffer const& offer,
    std::vector<HeldPayment>&          outTaken) {
    outTaken.clear();
    bool const sameType = !offer.buyA.type.empty() && offer.buyA.type == offer.buyB.type;
    for (auto const* cost : {&offer.buyA, &offer.buyB}) {
        if (cost->type.empty() || (sameType && cost == &offer.buyB))
            continue;

        std::int64_t remaining = std::max(1, cost->count);
        if (sameType)
            remaining += std::max(1, offer.buyB.count);
        for (auto const& h : held) {
            if (remaining <= 0)
                break;
            if (h.item.type != cost->type)
                continue;
            int const take = static_cast<int>(std::min<std::int64_t>(remaining, std::max(0, h.item.count)));
            if (take <= 0)
                continue;
            HeldPayment taken = h;
            taken.item.count  = take;
            outTaken.push_back(std::move(taken));
            remaining -= take;
        }
        if (remaining > 0) {
            outTaken.clear();
            return false;
        }
    }
    return true;
}

} // namespace debugshape_export::trade::settlement
