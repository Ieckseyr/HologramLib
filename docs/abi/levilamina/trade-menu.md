# 1.10 ITradeMenu · 交易菜单

完整签名（`#include "HologramLib/HologramLib.h"`）：

`IHologramLib::tradeMenus()`。**手写包**打开交易界面：界面完全由本库构造的 `UpdateTradePacket` 驱动，载荷与 BDS 26.40 原生村民交易逐字节一致（`tests/check-trade-packet.bat` 整包对拍、`check-trade-offers.bat` 只对 Offers 段）。交易界面只靠这个包打开——原生流程里没有 `ContainerType=Trade` 的 `ContainerOpen`。

> **默认不读任何点击**；开了 `settleLocally` 才读**交易槽上的物品动作**与带自建配方 id 的 `CraftRecipe` 动作。
>
> **真结算（1.24.0）**：`TradeMenuSpec::settleLocally = true`（且 `usePacketOffers = true`）时，界面仍是自建 `UpdateTrade`（服务端不放交易表），但库接住客户端的付费放置 / 取回 / 成交请求并**自己落地**：从真实背包扣下付费暂存、成交时校验账本并按条目把产物写进背包、关界面把没花掉的付费退回；"点一下拿起"那份也由库托管（光标）。结果通过 `TradeSettlementEvent` 回传（C++ 监听器 / LSE `tradePollSettlements`）。判定逻辑独立在 `src/trade/TradeSettlementLogic.h`，离线自检 `tests/check-trade-settlement.bat`（43 项）。**已知限制**：客户端要求服务端在应答里回带交易槽的槽位更正，而该段在本协议版本与协议库线格式不一致（照协议库手写会打崩客户端）—— 放料被服务端受理，但客户端界面可能撤回它。**要稳定成交用 `usePacketOffers=false`（BDS 结算）或容器 UI。**
>
> 实测边界：触屏在纯协议层到不了成交（付费拖不进交易槽）——触屏用真实交易表路径或容器 UI；客户端**买不起就不发包**；账本按"类型 + 数量"扣（不细分交易槽位）；`numCrafts` 不展开（一次成交发一份产物）。

两条路径（`TradeMenuSpec::usePacketOffers`）：

- **`true` + `false`（默认，纯展示）**：只发自建 `UpdateTrade`，服务端不放交易表。玩家往付费槽放东西的请求会被 BDS 拒掉、物品弹回，界面停在"不可成交"（与参考实现 GMLIB `ChestUI` 同路）。
- **`true` + `true`（1.24.0，纯协议真结算）**：见上方说明。
- **`false`（真实交易表）**：给载体实体装真实交易表并调 BDS 的 `openTrading`，**玩家是真的在交易**（物品真的消耗；触屏可走完整流程）。`carrierUniqueIdOverride != 0` 时直接以该真实实体为交易对象；这条路上 `settleLocally` 无效。

载体实体：交易界面需要 `EntityUniqueId`，因此打开时会在玩家身后 5 格生成一个**隐身、仅该玩家可见**的假村民（`minecraft:villager_v2`），关闭即删除。载体是异步送达客户端的，所以 `UpdateTrade` 与经验条元数据都在几 tick 后发送（背靠背发时客户端还不认识该实体，界面绑不上去）。经验条不在包内，由载体实体的 `TradeTier` / `MaxTradeTier`（恒 4）/ `TradeExperience` 元数据驱动。

| 分类 | 方法 | 签名 | 说明 |
|------|------|------|------|
| 生命周期 | open | `(std::string const& playerName, TradeMenuSpec const&) -> int64_t` | 同一玩家重复调用先关旧菜单；失败 -1 |
| | update / close / closeAll / isOpen / getAllIds | — | `update` 重发交易表（真实表路径会重开界面） |
| 内容（1.22.0 追加） | addOffer | `(int64_t menuId, TradeMenuOffer const&) -> bool` | 追加一条并**就地重发**交易表（不重开界面、不等延迟） |
| | setTier | `(int64_t menuId, int tier, int experience) -> bool` | 改显示栏值 / 经验条，同样就地重发 |
| 真结算（1.24.0） | setSettleLocally / isSettleLocally | `(int64_t, bool) -> bool` / `(int64_t) const -> bool` | 打开后随时切换真结算（初值在 `TradeMenuSpec::settleLocally`） |
| | addSettlementListener / removeSettlementListener | `(std::function<void(TradeSettlementEvent const&)>) -> uint64_t` / `(uint64_t) -> bool` | 成交 / 失败回传（C++ 侧） |
| | pollSettlements | `() -> std::vector<std::string>` | LSE 轮询（取走并清空）; 条目 `"player=Steve menuId=3 offer=1 ok=1 reason="` |

```cpp
struct TradeMenuOffer {
    TradeMenuItem buyA, buyB, sell;   // buyB.type 空 = 无第二付费项
    int  tier{kTradeTierNovice};      // 1 基: 1=新手 … 5=大师；高于 spec.tier 即显示为未解锁
    int  maxUses{16};
    int  traderExp{0};
    bool locked{false};               // true = 无论 spec.tier 多高都显示为未解锁
};

struct TradeMenuSpec {
    std::string tradeType{"entity.villager.butcher"}; // 自由翻译键
    int         tier{kTradeTierNovice};               // 1 基, 超出夹紧
    int         experience{0};                        // 经验条当前经验
    bool        usePacketOffers{true};                // 见上: 纯协议层展示 / 真实交易表
    std::int64_t carrierUniqueIdOverride{0};          // 非 0 = 用该真实实体（不放自建 offers）
    std::vector<TradeMenuOffer> offers;
    std::string carrierIdentifier{"minecraft:villager_v2"}; // 或 "minecraft:wandering_trader"
};
```

```cpp
// 载体实体类型（TradeMenuSpec 尾部追加）: 默认村民; 流浪商人 = "minecraft:wandering_trader"
// （界面外观/头图随类型变化; 两条路径都适用, 真实交易表路径装表方式相同）
spec.carrierIdentifier = "minecraft:wandering_trader";
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
