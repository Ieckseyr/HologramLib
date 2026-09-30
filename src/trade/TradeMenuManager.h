// TradeMenuManager.h - 村民交易菜单（协议层; 1.21.0 新能力域）
//
// 机制全部来自 26.40 抓包实测（logs/fullpkts/pkt80_*.bin）:
//   · UpdateTradePacket 字段: ContainerType=15(TRADE), 动态 ContainerId(1..100),
//     TraderTier 0 基, LastTradingPlayer=-1, DisplayName=交易类型翻译键字符串,
//     两个 flag 置 1, Data=Offers（构造见 trade/TradeOfferNbt.h, 有逐字节对拍）
//   · 交易界面只靠 UpdateTrade 打开（原生流程里没有 TRADE 类型的 ContainerOpen）
//   · 经验条不在包内 → 载体实体的 TradeTier/MaxTradeTier/TradeExperience 元数据
//
// 两条路径（spec.usePacketOffers 选）:
//   ① 纯协议层（默认）: 只发我们自己构造的 UpdateTrade, 服务端不放交易表。天然只读 ——
//      玩家往付费槽放东西的请求会被 BDS 拒掉、物品弹回（与参考实现 GMLIB ChestUI 同路）。
//   ② 真实交易表（usePacketOffers=false）: 给载体装真实交易表 + 走 BDS 的 openTrading ——
//      客户端与服务端持有同一份交易表, 玩家是真的在交易（物品真的消耗）。
//
// **纯展示是默认; 开了 settleLocally 才走真结算**（1.24.0）: 库接住客户端的付费放置/取回/
// 成交请求, 自己从真实背包扣付费、把产物写进背包（不依赖服务端交易表）。接住的动作只有两类:
// 落在交易槽上的物品动作, 以及带自建配方 id（netIdBase + 条目下标）的 CraftRecipe 动作。
//
// 载体实体: 交易界面需要 EntityUniqueId, 故打开菜单时在玩家身后 5 格生成一个
// 隐身、仅该玩家可见的假村民（villager_v2）, 关闭即删除。载体是异步到客户端的, 所以
// UpdateTrade 会在几 tick 后发送（背靠背发时客户端还不认识该实体, 界面绑不上去）。
// 玩家会移动, 所以载体在**打开的那一刻**按当时位置与朝向生成。
#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "hologramlib/HologramLib.h" // TradeMenuSpec / TradeMenuOffer / TradeSettlementEvent
#include "trade/TradeSettlementLogic.h" // 真结算的纯判定逻辑（离线可测）

#include <ll/api/event/Listener.h>

#include <mc/world/item/ItemStack.h> // 账本/光标托管要原样保存玩家那份物品（退还时不丢 NBT）

class Player;

namespace debugshape_export {

class TradeMenuManager {
public:
    static TradeMenuManager& getInstance();

    int64_t open(std::string const& playerName, hologramlib::TradeMenuSpec const& spec);
    bool    update(int64_t menuId, hologramlib::TradeMenuSpec const& spec);
    bool    close(int64_t menuId);
    void    closeAll();
    bool    isOpen(int64_t menuId) const;
    std::vector<int64_t> getAllIds() const;

    // 追加一条交易 / 改档位与经验, 就地重发（不重开界面）; LSE 导出用的就是这两个
    bool addOffer(int64_t menuId, hologramlib::TradeMenuOffer const& offer);
    bool setTier(int64_t menuId, int tier, int experience);

    // 该玩家是否有打开中的菜单（其他域/钩子判断用）
    [[nodiscard]] bool hasMenuFor(std::string const& playerName) const;

    // 客户端关闭了该容器（由 ContainerClosePacket 钩子调用）: 结束菜单 + 删载体。
    // 返回 true = 这个容器属于本域（调用方无需再处理）。
    // 纯协议层路径按我们发的容器 id 认; 真实交易表路径的 id 是 BDS 分配的, 按"该玩家有没有菜单"认。
    bool handleContainerClose(std::string const& playerName, int containerId);

    // 玩家下线: 关掉其菜单并删除载体实体
    void onPlayerLeave(std::string const& playerName);
    // 掉线清理（PlayerDisconnectEvent）: 关菜单 + 把真结算账本里暂存的付费还给玩家。
    // 与 onPlayerLeave 分开是因为这里拿得到**活着的 Player 引用** —— 掉线瞬间按名字查人可能已经查不到了,
    // 而账本里的付费是从真实背包扣下来的, 必须还回去（不然玩家的东西就丢了）。
    void onPlayerDisconnect(::Player& player);
    // 挂事件（ModEntry::enable 调用; 与 ItemDisplay/CustomEntity 同款）
    void init();
    // 库卸载: 全部清理
    void shutdown();

    // ── 1.24.0: 纯协议层真结算 ──
    // 判定逻辑（动作分类 / 账本够不够）在 trade/TradeSettlementLogic.h —— 不碰 BDS 类型,
    // 所以能离线测（tests/check-trade-settlement.bat）; 这里只做薄薄一层落地（动背包）。

    // 一条请求里的一个物品动作（由 interaction 钩子从 147/AuthInput 的 cereal 动作填好）
    using TradeRequestAction = trade::settlement::RequestAction;
    using ActionKind         = trade::settlement::ActionKind;
    using ClassifiedAction   = trade::settlement::Classified;
    using HeldPayment        = trade::settlement::HeldPayment;

    // 管理器侧的账本条目: 纯逻辑记录（给离线可测的判定用）+ **原样那一份物品**
    // （附魔等 NBT 只在 ItemStack 里, 退还时必须原样还回去）
    struct LedgerEntry {
        HeldPayment logical;
        ::ItemStack stack;
    };

    // 光标托管（"点一下拿起"那份东西）: 客户端手里拿着, 服务端背包里它还在 fromSlot。
    // 为什么需要: 纯协议路径下 BDS 的光标永远是空的（拿起请求被库接管了, 它没见过）,
    // 所以"光标 → 交易槽"这类请求只能由库自己兑现 —— 兑现在 fromSlot 上真扣。
    struct CursorShadow {
        bool        active{false};
        ::ItemStack stack;        // 手里那份（数量 = 客户端认为在手上的数量）
        int         fromSlot{-1}; // 服务端背包里它还在的那一格
    };

    // 真结算请求处理（由 interaction 钩子调用, 服务器线程）:
    //   认识的动作 = 落在交易付费槽上的转移 + 带自建配方 id 的 CraftRecipe + 光标托管。
    //   返回 true = 本域接管这条请求（**应答由本域自己发** —— 必须带交易槽的槽位更正,
    //   否则交易界面会把这次放料撤回; 调用方别重复回应答、也别交给 BDS）。
    bool handleItemRequest(
        ::Player&                              player,
        std::vector<TradeRequestAction> const& actions,
        int                                    requestId
    );
    bool setSettleLocally(int64_t menuId, bool on);
    [[nodiscard]] bool isSettleLocally(int64_t menuId) const;
    // 该玩家是否有"开着真结算"的菜单（钩子入口条件; 没开就整包放行, 一个字节都不解析）
    [[nodiscard]] bool hasSettlingMenuFor(std::string const& playerName) const;

    uint64_t addSettlementListener(std::function<void(hologramlib::TradeSettlementEvent const&)> listener);
    bool     removeSettlementListener(uint64_t token);
    // LSE 轮询: 取走并清空（条目 = formatSettlement 的一行）
    std::vector<std::string> pollSettlements();
    [[nodiscard]] static std::string formatSettlement(hologramlib::TradeSettlementEvent const& event);

private:
    TradeMenuManager() = default;

    struct Menu {
        int64_t                      menuId{0};
        std::string                  playerName;
        int                          containerId{0};
        int64_t                      carrierLibId{-1}; // 载体假村民（隐身·仅该玩家可见）
        std::uint64_t                carrierUniqueId{0};
        std::uint64_t                carrierRuntimeId{0};
        hologramlib::TradeMenuSpec   spec;
        // true = 界面由 BDS 自己打开（openTrading）, 交易表在服务端 —— 此时库不再发
        // UpdateTrade/ContainerClose/经验条, 全部交给 BDS
        bool                         serverTrades{false};
        // 纯协议层路径下各条交易用的配方 id 基准（第 i 条 = netIdBase + i）。
        // 只影响我们自己构造的 Offers NBT 内容（客户端靠它区分条目）。
        int                          netIdBase{3676};
        // 真结算: 与 spec.settleLocally 同步的运行时开关 + 已扣下的付费账本 + 光标托管
        bool                         settleLocally{false};
        std::vector<LedgerEntry>     ledger;
        CursorShadow                 cursor;
    };

    // 生成载体实体（玩家身后 5 格, 隐身, 仅该玩家可见）; 失败返回 -1
    int64_t spawnCarrier(::Player& player, std::string const& playerName, hologramlib::TradeMenuSpec const& spec);
    // 删除载体实体
    void destroyCarrier(Menu& menu);
    // 发包（不走 BDS 回读硬校验: 合成 offers 可能被读后校验拒绝, 但那不代表客户端收不到）
    // 把规格装成载体的服务端真实交易表（走 openTrading 路径时才需要）
    bool installTrades(::Player& player, Menu const& menu);
    void sendUpdateTrade(::Player& player, Menu const& menu);
    void sendContainerClose(::Player& player, Menu const& menu);
    void sendTradeExp(::Player& player, Menu const& menu);
    // 纯协议层路径的延时分步: 载体实体得先到客户端, UpdateTrade 才绑得上（背靠背发会打不开界面）
    void laterSendTrade(std::string playerName, int64_t menuId);
    // 就地重发交易表（addOffer / setTier 用; 玩家可能已离线, 静默失败）
    bool resendTradeTable(int64_t menuId);
    int  allocateContainerId(); // 动态区间 1..100（抓包实测 BDS 也是如此分配）

    // ── 真结算落地动作（服务器线程, 不持 mMutex）──
    // 把背包 slot 上的 amount 个物品扣下来记进账本; 失败 = 不接管这条请求
    bool applyPayIn(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);
    // "点一下拿起": 记下手里那份（服务端背包不动, 它还在 fromSlot）
    bool applyPickUp(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);
    // "再点一下放进交易槽": 从托管那份里取, 在 fromSlot 上真扣并记进账本
    bool applyCursorPayIn(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);
    // 放回背包（取消拿起）: 把背包里 fromSlot 那份"搬"到客户端预测的那一格
    bool applyCursorBack(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);
    // 把账本里暂存的付费还回背包
    bool applyPayOut(::Player& player, Menu& menu, ClassifiedAction const& action, std::string& reason);
    // 成交: 扣账本 + 发产物（productSlot >= 0 = 写进客户端预测的那一格, 否则找空位）
    bool settleDeal(
        ::Player&                      player,
        Menu&                          menu,
        ClassifiedAction const&        action,
        int                            productSlot,
        std::string&                   reason
    );
    // 把（账本里剩下的）付费还回背包; 返回还回去的条目数
    int  refundHeld(::Player& player, std::vector<LedgerEntry>& ledger);

    // 把交易槽内容推给客户端（一条 InventorySlot; item 空 = 清空该格）
    bool sendTradeSlotUpdate(::Player& player, Menu const& menu, int tradeSlot, ::ItemStack const& item);
    void emit(hologramlib::TradeSettlementEvent const& event);

    mutable std::mutex                       mMutex;
    std::unordered_map<int64_t, Menu>        mMenus;     // menuId → 菜单
    std::unordered_map<std::string, int64_t> mByPlayer;  // 玩家名 → menuId
    int64_t                                  mNextMenuId{1};
    int                                      mNextContainerId{3}; // 原生实测用过 2/3/5/7, 从 3 起步避开库存容器
    // 结算回传（mMutex 保护）: C++ 监听器 + LSE 轮询队列
    std::unordered_map<uint64_t, std::function<void(hologramlib::TradeSettlementEvent const&)>>
        mSettlementListeners;
    std::deque<hologramlib::TradeSettlementEvent> mSettlementQueue;
    uint64_t                                      mNextSettlementToken{1};
    ll::event::ListenerPtr                        mDisconnectListener;
};

// 公开接口适配（IHologramLib::tradeMenus()）
hologramlib::ITradeMenu& tradeMenuAdapter();

} // namespace debugshape_export
