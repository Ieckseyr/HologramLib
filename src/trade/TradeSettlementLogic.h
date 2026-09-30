// TradeSettlementLogic.h - 纯协议真结算的**纯逻辑**（不碰 BDS 类型, 离线可测）
//
// 抽出来的原因: "哪个动作属于本次交易"与"暂存的付费够不够成交"是最容易写错的部分, 而它们
// 完全不需要 Player / 背包 —— 放在这里就能用离线 fixture 直接测（tests/check-trade-settlement.bat）。
// 真正动背包的那层在 TradeMenuManager.cpp（薄薄一层, 调 BDS 的 Inventory）。
//
// 动作来源（实测, 见 logs/tradetest.log 的 [ACT]/[CLICK]/[RAW] 记录）:
//   ① 付费放进交易槽: Take/Place, src=背包(12/28/29), dst=交易付费槽(31/32/47/48)
//   ② 成交: CraftRecipe 动作带自建配方 id（= netIdBase + 条目下标）
//   ③ 产物出槽: src=产物槽(33/49) 或 CreatedOutput(60), dst=背包/光标
#pragma once

#include "hologramlib/HologramLib.h" // TradeMenuItem / TradeMenuOffer

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace debugshape_export::trade::settlement {

// 容器枚举（= ContainerEnumName 的线上值; 真值见 BedrockProtocol-main 的 ContainerEnumName.hpp）
inline constexpr int kCombinedInventory   = 12; // CombinedHotbarAndInventoryContainer
inline constexpr int kHotbar              = 28;
inline constexpr int kInventory           = 29;
inline constexpr int kOffhand             = 34;
inline constexpr int kCursor              = 59;
inline constexpr int kIngredient1         = 31; // 老交易界面的付费 A/B 与产物
inline constexpr int kIngredient2         = 32;
inline constexpr int kResultPreview       = 33;
inline constexpr int kTrade2Ingredient1   = 47; // 新交易界面（实测客户端用的就是这两个）
inline constexpr int kTrade2Ingredient2   = 48;
inline constexpr int kTrade2ResultPreview = 49;
inline constexpr int kCreatedOutput       = 60; // 合成产物: 新交易界面的产物走这条（实测）

// 付费槽（客户端把东西放进去的容器）
[[nodiscard]] inline bool isPaymentContainer(int c) {
    return c == kIngredient1 || c == kIngredient2 || c == kTrade2Ingredient1 || c == kTrade2Ingredient2;
}
// 玩家"背包 36 格"对应的容器（付费的来处与产物的去处）:
//   CombinedHotbarAndInventory(12) / Hotbar(28) / Inventory(29)
// **不含光标(59)**: 光标是客户端手里那一份, 服务端背包里没有它; 拿它的槽号去读背包会读错槽
// （实测 bug: 源是光标时按 slot 0 去背包拿, 会拿走背包第 0 格的东西）。光标单独走
// PickUp / CursorPayIn / CursorBack 三种动作。
// **不含副手(34)**: 副手不在 Player::getInventory() 的 36 格里, 槽号会串到背包上。
[[nodiscard]] inline bool isInventoryContainer(int c) {
    return c == kCombinedInventory || c == kHotbar || c == kInventory;
}

// 客户端容器槽号 → 服务端 Inventory（36 格: 0..8 快捷栏 + 9..35 背包）槽号。
//   CombinedHotbarAndInventory(12) / Hotbar(28): 编号与背包一致
//   Inventory(29): 它的 0 对应背包第 9 格
[[nodiscard]] inline int inventorySlotOf(int containerEnum, int slot) {
    if (slot < 0) return -1;
    if (containerEnum == kInventory) return slot + 9;
    return slot;
}
// 产物侧（产物预览槽 / CreatedOutput）
[[nodiscard]] inline bool isResultContainer(int c) {
    return c == kResultPreview || c == kTrade2ResultPreview || c == kCreatedOutput;
}

// 一条请求里的一个物品动作（由 interaction 钩子从 147/AuthInput 的 cereal 动作填好）
struct RequestAction {
    int  type{0};           // ItemStackRequestAction::Type（0=Take 1=Place 12=CraftRecipe …）
    int  amount{1};
    bool hasSrc{false};
    bool hasDst{false};
    int  srcContainer{0};
    int  srcContainerId{-1};
    int  srcSlot{-1};
    int  dstContainer{0};
    int  dstContainerId{-1};
    int  dstSlot{-1};
    int  recipeNetId{-1};   // CraftRecipe 类动作带的配方 id（成交定位主信号; 无 = -1）
    // 槽位的 netId（客户端给这次交互分配的物品网络 id）。回带槽位更正时必须原样带上,
    // 否则客户端对不上号、把这次更正丢掉（实测: 不带就更正无效, 付款反复弹回）。
    int  srcNetId{0};
    int  dstNetId{0};
};

enum class ActionKind {
    Other,        // 与交易无关
    PayIn,        // 背包 → 交易付费槽（直接拖拽, 服务端背包里有这份物品）
    PayOut,       // 交易付费槽 → 背包
    ResultOut,    // 产物槽/CreatedOutput → 背包（客户端预测的产物落点; 结算靠它选槽）
    Deal,         // 成交（CraftRecipe 带自建配方 id）
    PickUp,       // 背包 → 光标（"点一下拿起"）—— 库托管这份"手里拿着的"
    CursorPayIn,  // 光标 → 交易付费槽（"再点一下放进槽", 实测常见走法）
    CursorBack,   // 光标 → 背包（放回背包/取消拿起）
};

struct Classified {
    ActionKind kind{ActionKind::Other};
    int        offerIndex{-1};    // Deal: 第几条
    int        amount{0};
    int        slot{-1};          // 背包侧槽位（PayIn/PickUp 的 src; PayOut/ResultOut/CursorBack 的 dst）
    int        tradeContainer{0}; // 交易侧容器枚举
    int        tradeSlot{-1};     // 交易侧槽号
    int        srcContainer{0};   // 背包侧容器的枚举（换算服务端槽号要用: 12/28 同号, 29 要 +9）
    bool       touchesCursor{false}; // 牵涉光标但本域不认（调用方据此作废"手里那份"的托管）
};

// 账本条目: 已经从真实背包扣下、暂存在"交易槽"里的付费
struct HeldPayment {
    int                        tradeContainer{0};
    int                        tradeSlot{-1};
    hologramlib::TradeMenuItem item; // 种类/数量/自定义名都在里面
};

// 一条请求里各动作的分类。recipeNetId ∈ [netIdBase, netIdBase + offerCount) 判为"我们的成交"。
[[nodiscard]] inline std::vector<Classified> classify(
    std::vector<RequestAction> const& actions,
    int                               netIdBase,
    int                               offerCount
) {
    std::vector<Classified> out;
    out.reserve(actions.size());
    for (auto const& a : actions) {
        Classified c;
        // 成交: CraftRecipe* 带我们分配的配方 id（第 i 条 = netIdBase + i）
        if (a.recipeNetId >= netIdBase && a.recipeNetId < netIdBase + offerCount) {
            c.kind       = ActionKind::Deal;
            c.offerIndex = a.recipeNetId - netIdBase;
            out.push_back(c);
            continue;
        }
        bool const srcPay   = a.hasSrc && isPaymentContainer(a.srcContainer);
        bool const dstPay   = a.hasDst && isPaymentContainer(a.dstContainer);
        bool const srcRes   = a.hasSrc && isResultContainer(a.srcContainer);
        bool const srcInv   = a.hasSrc && isInventoryContainer(a.srcContainer);
        bool const dstInv   = a.hasDst && isInventoryContainer(a.dstContainer);
        bool const srcCur   = a.hasSrc && a.srcContainer == kCursor;
        bool const dstCur   = a.hasDst && a.dstContainer == kCursor;
        if (srcInv && dstCur) {
            // "点一下拿起": 背包 → 光标。服务端背包里这份还在原处（BDS 的光标一直是空的）
            c.kind         = ActionKind::PickUp;
            c.amount       = a.amount;
            c.slot         = a.srcSlot;
            c.srcContainer = a.srcContainer;
        } else if (srcCur && dstPay) {
            // "再点一下放进交易槽": 从托管的那份里取, 落进交易槽
            c.kind           = ActionKind::CursorPayIn;
            c.amount         = a.amount;
            c.tradeContainer = a.dstContainer;
            c.tradeSlot      = a.dstSlot;
        } else if (srcCur && dstInv) {
            // 放回背包（取消拿起）
            c.kind         = ActionKind::CursorBack;
            c.amount       = a.amount;
            c.slot         = a.dstSlot;
            c.srcContainer = a.dstContainer; // 落点所在容器（换算服务端槽号用）
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
            c.srcContainer   = a.dstContainer; // 落点所在容器
            c.tradeContainer = a.srcContainer;
            c.tradeSlot      = a.srcSlot;
        } else if (srcRes && dstInv) {
            c.kind         = ActionKind::ResultOut;
            c.slot         = a.dstSlot;
            c.srcContainer = a.dstContainer; // 产物落点所在容器
        } else if (srcCur || dstCur) {
            // 其它牵涉光标的动作（换手/丢弃/与别的容器交换…）: 本域不认 —— 由调用方作废托管,
            // 请求放行给 BDS（它的光标是空的, 会失败 → 客户端安静地撤回这次预测, 不丢东西）
            c.touchesCursor = true;
        }
        out.push_back(c);
    }
    return out;
}

// 账本能否覆盖一条交易的付费要求; 能则把"要从哪些账本条目里扣多少"填进 outTaken。
// 同类型可跨条目累计（玩家可能分几次放料）; 不够返回 false（outTaken 内容作废）。
[[nodiscard]] inline bool planConsumption(
    std::vector<HeldPayment> const&    held,
    hologramlib::TradeMenuOffer const& offer,
    std::vector<HeldPayment>&          outTaken
) {
    outTaken.clear();
    std::vector<std::pair<std::string, int>> need;
    if (!offer.buyA.type.empty()) need.emplace_back(offer.buyA.type, std::max(1, offer.buyA.count));
    if (!offer.buyB.type.empty()) need.emplace_back(offer.buyB.type, std::max(1, offer.buyB.count));

    for (auto const& [type, count] : need) {
        int remaining = count;
        for (auto const& h : held) {
            if (remaining <= 0) break;
            if (h.item.type != type) continue;
            int const take = std::min(remaining, std::max(0, h.item.count));
            if (take <= 0) continue;
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
