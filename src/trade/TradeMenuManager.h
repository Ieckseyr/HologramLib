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
// **本域是纯展示: 不读客户端的点击/物品请求, 也没有任何回调。** 早期版本挂钩 147/AuthInput
// 解析"点了哪一条交易"（配方 netId 定位）与逐动作回调, 已按需求整体删除 —— 要能点的列表界面
// 用 ContainerMenuManager（虚拟容器）, 那里点击是一次物品拾取, 任何输入设备都发包。
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

#include "hologramlib/HologramLib.h" // TradeMenuSpec / TradeMenuOffer

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
        // 纯协议层路径下各条交易用的配方 id 基准（第 i 条 = netIdBase + i）。
        // 只影响我们自己构造的 Offers NBT 内容（客户端靠它区分条目）, 与回调无关。
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
    // 就地重发交易表（addOffer / setTier 用; 玩家可能已离线, 静默失败）
    bool resendTradeTable(int64_t menuId);
    int  allocateContainerId(); // 动态区间 1..100（抓包实测 BDS 也是如此分配）

    mutable std::mutex                       mMutex;
    std::unordered_map<int64_t, Menu>        mMenus;     // menuId → 菜单
    std::unordered_map<std::string, int64_t> mByPlayer;  // 玩家名 → menuId
    int64_t                                  mNextMenuId{1};
    int                                      mNextContainerId{3}; // 原生实测用过 2/3/5/7, 从 3 起步避开库存容器
};

// 公开接口适配（IHologramLib::tradeMenus()）
hologramlib::ITradeMenu& tradeMenuAdapter();

} // namespace debugshape_export
