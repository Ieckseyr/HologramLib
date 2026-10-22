// trade_settlement_fixture.cpp - 纯协议真结算的判定逻辑离线自检
//
// 测的是**库内同一份** src/trade/TradeSettlementLogic.h（不是重写一遍的副本）:
//   ① classify()       —— 一条请求里的动作怎么归类（付费进/付费回/产物出/成交/无关）
//   ② planConsumption()—— 暂存的付费够不够一条交易, 要扣哪些条目
// 用例里的槽位与配方 id 都取自真机实测记录（logs/tradetest.log 的 [ACT] / [CLICK] 行）:
//   付费放进交易槽 = src(container=12, slot=12) → dst(container=47, slot=4) amount=64
//   成交           = CraftRecipe recipeNetId=3676（= 库分配的 netIdBase + 条目下标 0）
//   产物出槽       = src(container=60 CreatedOutput, slot=50) → dst(container=59 光标, slot=0)
//
// 这个头不依赖 BDS 类型, 所以本 fixture 只吃 include 目录, 不需要 Protocol.lib。
// 用法: trade_settlement_fixture（失败时返回非 0）
#include "trade/TradeSettlementLogic.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace debugshape_export::trade::settlement;

namespace {

int gChecks = 0;
int gFailed = 0;

void check(bool ok, std::string const& what) {
    ++gChecks;
    if (!ok) {
        ++gFailed;
        std::printf("FAIL  %s\n", what.c_str());
    } else {
        std::printf("ok    %s\n", what.c_str());
    }
}

// 造一个转移动作（Take/Place 同结构: amount + src + dst）
RequestAction transfer(int amount, int srcC, int srcSlot, int dstC, int dstSlot, int srcDyn = -1, int dstDyn = -1) {
    RequestAction a;
    a.type           = 1; // Place（分类不看类型, 只看容器方向）
    a.amount         = amount;
    a.hasSrc         = true;
    a.srcContainer   = srcC;
    a.srcSlot        = srcSlot;
    a.srcContainerId = srcDyn;
    a.hasDst         = true;
    a.dstContainer   = dstC;
    a.dstSlot        = dstSlot;
    a.dstContainerId = dstDyn;
    return a;
}

RequestAction craft(int recipeNetId) {
    RequestAction a;
    a.type        = 12; // CraftRecipe
    a.recipeNetId = recipeNetId;
    return a;
}

hologramlib::TradeMenuItem item(std::string type, int count) {
    hologramlib::TradeMenuItem i;
    i.type  = std::move(type);
    i.count = count;
    return i;
}

HeldPayment held(int container, int slot, std::string type, int count) {
    HeldPayment h;
    h.tradeContainer = container;
    h.tradeSlot      = slot;
    h.item           = item(std::move(type), count);
    return h;
}

int totalTaken(std::vector<HeldPayment> const& taken, std::string const& type) {
    int sum = 0;
    for (auto const& t : taken) {
        if (t.item.type == type) sum += t.item.count;
    }
    return sum;
}

} // namespace

int main() {
    constexpr int kNetIdBase = 3676; // 库的默认基准（TradeMenuManager::Menu::netIdBase）

    std::printf("== 容器判定 ==\n");
    check(isPaymentContainer(kTrade2Ingredient1) && isPaymentContainer(kTrade2Ingredient2),
          "47/48（新交易界面付费 A/B）算付费槽");
    check(isPaymentContainer(kIngredient1) && isPaymentContainer(kIngredient2), "31/32（老交易界面）算付费槽");
    check(!isPaymentContainer(kResultPreview) && !isPaymentContainer(kTrade2ResultPreview),
          "产物槽不算付费槽");
    check(isInventoryContainer(kCombinedInventory) && isInventoryContainer(kHotbar)
              && isInventoryContainer(kInventory),
          "12/28/29 算玩家背包（36 格）");
    check(!isInventoryContainer(kCursor) && !isInventoryContainer(kOffhand),
          "光标/副手不算背包容器（槽号会串, 光标走 PickUp/CursorPayIn/CursorBack）");
    check(inventorySlotOf(kHotbar, 5) == 5 && inventorySlotOf(kCombinedInventory, 20) == 20
              && inventorySlotOf(kInventory, 0) == 9 && inventorySlotOf(kInventory, 26) == 35,
          "容器 29 的槽号 +9 映射到服务端背包（12/28 同号）");
    check(isResultContainer(kCreatedOutput) && isResultContainer(kTrade2ResultPreview),
          "60(CreatedOutput) 与 49 算产物侧");

    std::printf("\n== classify: 实机记录过的三种动作 ==\n");
    {
        // [ACT] src(slot=12,container=12) dst(slot=4,container=47) amount=64
        auto const cls = classify({transfer(64, kCombinedInventory, 12, kTrade2Ingredient1, 4)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::PayIn, "付费放进交易槽 → PayIn");
        check(cls[0].amount == 64 && cls[0].slot == 12, "PayIn 带出数量 64 与背包侧槽位 12");
        check(cls[0].tradeContainer == kTrade2Ingredient1 && cls[0].tradeSlot == 4, "PayIn 带出交易侧槽位 (47,4)");
    }
    {
        // 反向: 从交易槽取回
        auto const cls = classify({transfer(64, kTrade2Ingredient1, 4, kCombinedInventory, 12)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::PayOut, "从交易槽取回 → PayOut");
        check(cls[0].slot == 12 && cls[0].tradeSlot == 4, "PayOut 的背包侧槽位 = 客户端预测的落点");
    }
    {
        // [RAW] src[slot=50 c=60] dst[slot=0 c=12] amount=10 —— 产物落点是背包
        auto const cls = classify({transfer(10, kCreatedOutput, 50, kCombinedInventory, 19)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::ResultOut, "产物出槽 → ResultOut");
        check(cls[0].slot == 19 && cls[0].srcContainer == kCombinedInventory,
              "ResultOut 带出产物落点与所在容器（换算服务端槽号用）");
    }
    {
        // 产物落到光标: 本域不认（BDS 那边没有这个产物, 认了就给不出东西）—— 作废光标托管即可
        auto const cls = classify({transfer(10, kCreatedOutput, 50, kCursor, 0)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::Other && cls[0].touchesCursor,
              "产物落到光标 → 不认（touchesCursor, 库会另找空位）");
    }
    {
        // "点一下拿起": 背包 → 光标
        auto const cls = classify({transfer(64, kHotbar, 8, kCursor, 0)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::PickUp, "背包 → 光标 → PickUp（点一下拿起）");
        check(cls[0].slot == 8 && cls[0].srcContainer == kHotbar && cls[0].amount == 64,
              "PickUp 带出源槽与数量");
    }
    {
        // "再点一下放进交易槽": 光标 → 付费槽
        auto const cls = classify({transfer(3, kCursor, 0, kTrade2Ingredient1, 4)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::CursorPayIn, "光标 → 交易付费槽 → CursorPayIn");
        check(cls[0].amount == 3 && cls[0].tradeContainer == kTrade2Ingredient1 && cls[0].tradeSlot == 4,
              "CursorPayIn 带出数量与交易槽位");
    }
    {
        // 放回背包: 光标 → 背包
        auto const cls = classify({transfer(61, kCursor, 0, kCombinedInventory, 8)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::CursorBack, "光标 → 背包 → CursorBack（放回/取消）");
        check(cls[0].slot == 8 && cls[0].srcContainer == kCombinedInventory, "CursorBack 带出落点");
    }
    {
        // 光标 → 快捷栏: 按"放回"处理（那一格若是别的东西, 执行期以 slot-occupied 拒绝,
        // 请求交回 BDS, 客户端安静撤回 —— 不会丢东西）
        auto const cls = classify({transfer(1, kCursor, 0, kHotbar, 2)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::CursorBack && cls[0].slot == 2,
              "光标 → 背包任意格 → CursorBack（占用冲突在执行期拒绝）");
    }
    {
        // 牵着光标但我们不认的动作（例如把手里那份丢出去: src=光标, 没有目标槽）
        RequestAction a;
        a.type         = 3; // Drop
        a.amount       = 1;
        a.hasSrc       = true;
        a.srcContainer = kCursor;
        a.srcSlot      = 0;
        auto const cls = classify({a}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::Other && cls[0].touchesCursor,
              "牵涉光标但没认出的动作 → Other + touchesCursor（作废托管、放行给 BDS）");
    }
    {
        // [CLICK] offerIndex=0 recipeNetId=3676
        auto const cls = classify({craft(kNetIdBase)}, kNetIdBase, 3);
        check(cls.size() == 1 && cls[0].kind == ActionKind::Deal && cls[0].offerIndex == 0,
              "CraftRecipe(3676) → 成交第 0 条");
    }
    {
        auto const cls = classify({craft(kNetIdBase + 2)}, kNetIdBase, 3);
        check(cls[0].kind == ActionKind::Deal && cls[0].offerIndex == 2, "CraftRecipe(3678) → 成交第 2 条");
    }
    {
        auto const cls = classify({craft(kNetIdBase + 3)}, kNetIdBase, 3);
        check(cls[0].kind == ActionKind::Other, "超出条目数的配方 id 不认（别人的配方）");
    }
    {
        auto const cls = classify({craft(999)}, kNetIdBase, 3);
        check(cls[0].kind == ActionKind::Other, "完全无关的配方 id 不认");
    }
    {
        // 背包内部搬东西 / 与交易无关的容器动作: 一律 Other（库不该碰）
        auto const cls = classify(
            {transfer(1, kCombinedInventory, 1, kCombinedInventory, 2),
             transfer(1, kTrade2Ingredient1, 4, kTrade2Ingredient1, 5)},
            kNetIdBase,
            3
        );
        check(cls[0].kind == ActionKind::Other, "背包内部移动 → Other（放行给 BDS）");
        check(cls[1].kind == ActionKind::Other, "交易槽内部移动 → Other（不参与结算）");
    }
    {
        // 整条"点条目"请求: 放料 + 成交 + 产物出槽（顺序不保证, 判定必须都认）
        auto const cls = classify(
            {craft(kNetIdBase), transfer(3, kCombinedInventory, 12, kTrade2Ingredient1, 4),
             transfer(3, kCreatedOutput, 50, kCombinedInventory, 19)},
            kNetIdBase,
            3
        );
        int payIn = 0, deal = 0, resultOut = 0;
        for (auto const& c : cls) {
            if (c.kind == ActionKind::PayIn) ++payIn;
            if (c.kind == ActionKind::Deal) ++deal;
            if (c.kind == ActionKind::ResultOut) ++resultOut;
        }
        check(payIn == 1 && deal == 1 && resultOut == 1, "一条请求里放料/成交/产物三件事都能认出来");
    }

    std::printf("\n== planConsumption: 账本够不够 ==\n");
    {
        hologramlib::TradeMenuOffer offer;
        offer.buyA = item("minecraft:diamond", 3);
        offer.sell = item("minecraft:paper", 1);

        std::vector<HeldPayment> taken;
        check(planConsumption({held(kTrade2Ingredient1, 4, "minecraft:diamond", 3)}, offer, taken),
              "账本正好够 → 可成交");
        check(totalTaken(taken, "minecraft:diamond") == 3, "要扣 3 个钻石");

        taken.clear();
        check(planConsumption(
                  {held(kTrade2Ingredient1, 4, "minecraft:diamond", 1), held(kTrade2Ingredient1, 5, "minecraft:diamond", 2)},
                  offer,
                  taken
              ),
              "分两次放料（1+2）也能成交");
        check(taken.size() == 2 && totalTaken(taken, "minecraft:diamond") == 3, "两条账本各扣对应数量");

        taken.clear();
        check(!planConsumption({held(kTrade2Ingredient1, 4, "minecraft:diamond", 2)}, offer, taken),
              "账本不够 → 不能成交");
        check(taken.empty(), "不够时不留半截扣款计划");

        taken.clear();
        check(!planConsumption({held(kTrade2Ingredient1, 4, "minecraft:emerald", 3)}, offer, taken),
              "种类不对 → 不能成交");

        taken.clear();
        check(planConsumption({held(kTrade2Ingredient1, 4, "minecraft:diamond", 5)}, offer, taken)
                  && totalTaken(taken, "minecraft:diamond") == 3,
              "放多了只扣需要的 3 个（多余的留在账本里, 关界面退还）");
    }
    {
        // 双付费项（buyA + buyB）
        hologramlib::TradeMenuOffer offer;
        offer.buyA = item("minecraft:diamond", 3);
        offer.buyB = item("minecraft:emerald", 1);
        offer.sell = item("minecraft:paper", 2);

        std::vector<HeldPayment> taken;
        check(!planConsumption({held(kTrade2Ingredient1, 4, "minecraft:diamond", 3)}, offer, taken),
              "双付费项只放了一种 → 不能成交");
        check(planConsumption(
                  {held(kTrade2Ingredient1, 4, "minecraft:diamond", 3), held(kTrade2Ingredient2, 0, "minecraft:emerald", 1)},
                  offer,
                  taken
              ),
              "两种都放齐 → 可成交");
        check(totalTaken(taken, "minecraft:diamond") == 3 && totalTaken(taken, "minecraft:emerald") == 1,
              "两项各自扣对数量");
    }
    {
        // 只有 buyA 的交易（buyB 为空）
        hologramlib::TradeMenuOffer offer;
        offer.buyA = item("minecraft:emerald", 1);
        offer.sell = item("minecraft:paper", 1);
        std::vector<HeldPayment> taken;
        check(planConsumption({held(kTrade2Ingredient1, 4, "minecraft:emerald", 1)}, offer, taken),
              "无第二付费项时只看 buyA");
    }
    {
        // 免费交易（buyA 也为空, 客户端"空气换东西"那条）: 不需要账本
        hologramlib::TradeMenuOffer offer;
        offer.sell                = item("minecraft:paper", 1);
        std::vector<HeldPayment> taken;
        check(planConsumption({}, offer, taken) && taken.empty(), "零成本交易不需要账本");
    }

    std::printf("\n%d/%d 项通过\n", gChecks - gFailed, gChecks);
    return gFailed == 0 ? 0 : 1;
}
