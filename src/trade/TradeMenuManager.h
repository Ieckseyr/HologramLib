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
//   ① 纯协议层（默认）: 只发我们自己构造的 UpdateTrade, 服务端不放交易表。玩家的放料/成交
//      请求走 ItemStackRequest(147), 由本域回调上报; BDS 那边没有对应容器, 所以物品不会真的
//      消耗（天然只读, 与参考实现 GMLIB ChestUI 同路）。
//   ② 真实交易表（usePacketOffers=false）: 给载体装真实交易表 + 走 BDS 的 openTrading ——
//      客户端与服务端持有同一份交易表, 成交由 BDS 完成（物品真的消耗）。
//
// 点击回调（"点了哪一条交易"）: 新交易界面里条目就是配方, 客户端点它时会在 147 请求里带一个
// CraftRecipe 动作, 其 mRecipeNetId 就是该条的配方 id:
//   · 路径①: netId = kOfferNetIdBase + 条目下标（我们分配的）→ 减出下标即可
//   · 路径②: netId 由 BDS 分配 → 从载体身上的 MerchantRecipeList 里反查下标
// 付费/产物槽上的动作（容器 31/32/33/47/48/49）另外回传一次逐动作事件。
//
// 载体实体: 交易界面需要 EntityUniqueId, 故打开菜单时在玩家身后 5 格生成一个
// 隐身、仅该玩家可见的假村民（villager_v2）, 关闭即删除。载体是异步到客户端的, 所以
// UpdateTrade 会在几 tick 后发送（背靠背发时客户端还不认识该实体, 界面绑不上去）。
// 玩家会移动, 所以载体在**打开的那一刻**按当时位置与朝向生成。
#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "hologramlib/HologramLib.h" // TradeMenuSpec / TradeClickEvent

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

    uint64_t addClickListener(std::function<void(hologramlib::TradeClickEvent const&)> listener);
    bool     removeClickListener(uint64_t token);

    // 该玩家是否有打开中的菜单（钩子诊断用）
    [[nodiscard]] bool hasMenuFor(std::string const& playerName) const;
    // 库存事务钩子入口: 命中该玩家打开中的菜单容器 → 回调监听并返回 true
    // outReject=true 表示调用方应拒绝该事务（displayOnly = 物品不消耗）
    bool handleTransaction(std::string const& playerName, int containerId, int slot, bool& outReject);



    // 该玩家当前菜单是否为 displayOnly（无菜单返回 false）
    [[nodiscard]] bool isDisplayOnlyFor(std::string const& playerName) const;

    // 该玩家当前菜单是否要由库自己接住"把付费放进交易槽"的动作（纯协议层路径 + 该开关打开）
    [[nodiscard]] bool acceptsPlacementLocally(std::string const& playerName) const;

    // 逐动作回传（含关闭哨兵）—— 语义与参考实现 GMLIB ChestUI 的 ChestUICallback 一致:
    //   src/dst 为槽位引用, amount 为数量; 关闭时以 {slot=-1} + amount=-1 调用一次。
    // recipeNetId: 同一请求里 CraftRecipe 类动作携带的配方 id（"点了哪一条"的信号; 无则 -1）。
    // 返回 true = 需要拦下这次动作。只在**服务端真的会成交**时才会返回 true（displayOnly 的真实
    // 交易表路径 + 动的是产物槽）; 纯协议层路径一律返回 false —— 那里放行后 BDS 自己会失败,
    // 天然只读, 而自己代答失败应答会让客户端弹错误提示（实测）。
    bool handleAction(
        std::string const&          playerName,
        hologramlib::TradeSlotRef   src,
        hologramlib::TradeSlotRef   dst,
        int                         amount,
        bool                        srcIsResult,
        int                         recipeNetId = -1
    );

    // 客户端点了配方列表里的某一条（CraftRecipe 动作带回了 recipeNetId）→ 按它定位条目并回调。
    // 返回 true = 已回调。
    bool handleOfferClick(std::string const& playerName, int recipeNetId);
    // 客户端关闭了该容器（由 ContainerClosePacket 钩子调用）: 发关闭哨兵 + 结束菜单。
    // 返回 true = 这个容器属于本域（调用方无需再处理）。
    // 纯协议层路径按我们发的容器 id 认; 真实交易表路径的 id 是 BDS 分配的, 按"该玩家有没有菜单"认。
    bool handleContainerClose(std::string const& playerName, int containerId);

    // 诊断用: 每一个动作都回传（不筛容器）, 供探针排查"客户端到底发了什么"
    void dispatchRawAction(std::string const& playerName, hologramlib::TradeRawAction action);
    uint64_t addRawActionListener(hologramlib::TradeRawActionCallback listener);
    bool     removeRawActionListener(uint64_t token);

    uint64_t addActionListener(std::function<void(
                                   std::string const&,
                                   int64_t,
                                   hologramlib::TradeSlotRef const&,
                                   hologramlib::TradeSlotRef const&,
                                   int
                               )> listener);
    bool     removeActionListener(uint64_t token);


    // 玩家下线: 关掉其菜单并删除载体实体
    void onPlayerLeave(std::string const& playerName);
    // 库卸载: 全部清理
    void shutdown();

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
        // 纯协议层路径下分配给各条交易的配方 id 基准（第 i 条 = netIdBase + i）;
        // 客户端点条目时回传的 recipeNetId 减掉它即得下标。
        int                          netIdBase{3676};
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
    // 真实交易表路径下按 BDS 分配的 mRecipeNetId 反查条目下标（纯协议层路径不需要）
    [[nodiscard]] int offerIndexFromServerTrades(Menu const& menu, int recipeNetId);
    void dispatch(hologramlib::TradeClickEvent const& event);
    int  allocateContainerId(); // 动态区间 1..100（抓包实测 BDS 也是如此分配）

    mutable std::mutex                                  mMutex;
    std::unordered_map<int64_t, Menu>                   mMenus;     // menuId → 菜单
    std::unordered_map<std::string, int64_t>            mByPlayer;  // 玩家名 → menuId
    std::unordered_map<uint64_t, std::function<void(hologramlib::TradeClickEvent const&)>> mListeners;
    std::unordered_map<
        uint64_t,
        std::function<void(std::string const&, int64_t, hologramlib::TradeSlotRef const&, hologramlib::TradeSlotRef const&, int)>>
        mActionListeners;
    std::unordered_map<uint64_t, hologramlib::TradeRawActionCallback> mRawActionListeners;
    uint64_t mNextListenerToken{1};
    int64_t  mNextMenuId{1};
    int      mNextContainerId{3}; // 原生实测用过 2/3/5/7, 从 3 起步避开库存容器
};

// 公开接口适配（IHologramLib::tradeMenus()）
hologramlib::ITradeMenu& tradeMenuAdapter();

} // namespace debugshape_export
