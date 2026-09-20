// ContainerMenuManager.h - 虚拟容器（列表）界面
//
// 机制（与参考实现 GMLIB ChestUI 同路, 逐条复刻; 全部手写包）:
//   1. UpdateBlockPacket 在玩家头上摆一个客户端侧箱子方块（只发给该玩家, 服务端世界里没有）
//   2. BlockActorDataPacket 摆该方块的方块实体 NBT: id=Chest / CustomName=标题 / x,y,z /
//      pairx+pairz+pairlead（大箱子）/ Items=[条目物品]  ← 条目物品走这份 NBT, 不是逐格 InventorySlot
//   3. 等 10 tick 发 ContainerOpen（ContainerType=Container(0), 位置=方块, 目标实体=-1）
//   4. 玩家点击 → 客户端发 ItemStackRequest(147) → 槽位命中本容器 → 回调（槽位号 = 条目下标）
//   5. 关闭 → 客户端/服务端 ContainerClose → 把真方块改回去 → 回调 closed
//
// 关键性质: **服务端根本没有这个容器** —— 物品只是"摆在那里", 玩家拿走/移动都不会真的改变任何
// 东西（天然只读）。这正好适合当任务列表、菜单这类"只展示 + 点击回调"的界面。
//
// 大小容器: rows=3 → 单箱子 27 格; rows=6 → 大箱子 54 格（两个配对箱子方块）。
//
// 容器 id 用 100 以上的显示区间, 避开 BDS 给真实容器分配的动态 id（1..100）。
#pragma once

#include "hologramlib/HologramLib.h"

#include <ll/api/io/Logger.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace debugshape_export {

class ContainerMenuManager {
public:
    static ContainerMenuManager& getInstance();

    int64_t open(std::string const& playerName, hologramlib::ContainerMenuSpec const& spec);
    // 就地换内容（翻页/子菜单/返回）: 复用载体方块, 不拆界面、不重摆方块、不等延迟
    bool    update(int64_t menuId, hologramlib::ContainerMenuSpec const& spec);
    // 按槽刷新: 一条 InventorySlot 换掉一格（无延迟、无闪烁）
    bool    setItem(int64_t menuId, int slot, hologramlib::ContainerMenuItem const& item);
    bool    close(int64_t menuId);
    void    closeAll();
    [[nodiscard]] bool                 isOpen(int64_t menuId) const;
    [[nodiscard]] std::vector<int64_t> getAllIds() const;

    uint64_t addClickListener(std::function<void(hologramlib::ContainerClickEvent const&)> listener);
    bool     removeClickListener(uint64_t token);

    // 147 动作派发（由唯一的 ItemStackRequest 钩子调用, 避免同址多挂）。
    // 返回 true = 这个动作属于本域的某个容器（已回调）。
    bool handleSlotAction(std::string const& playerName, int containerEnum, int containerId, int slot);

    // 客户端关闭容器（由 ContainerClosePacket 钩子调用）
    bool handleContainerClose(std::string const& playerName, int containerId);

    // 该玩家是否有打开中的容器菜单（147 钩子的入口条件: 两个域共用一个钩子, 谁开着都要进）
    [[nodiscard]] bool hasMenuFor(std::string const& playerName) const;
    // 该玩家打开中的容器 menuId（无则 -1）。原始动作诊断用: 交易域要拿它当"菜单已开"的依据。
    [[nodiscard]] int64_t menuIdFor(std::string const& playerName) const;

    void onPlayerLeave(std::string const& playerName);
    void shutdown();

private:
    ContainerMenuManager() = default;

    struct Menu {
        int64_t                                menuId{0};
        std::string                            playerName;
        int                                    containerId{0};
        // 客户端侧箱子方块的位置（打开时按玩家当时位置选定, 关闭时按它恢复真方块）
        int                                    posX{0};
        int                                    posY{0};
        int                                    posZ{0};
        // 旧路径（useMinecart=true）用的合成矿车实体; GMLIB 方案下恒为 -1/0
        int64_t                                carrierLibId{-1};
        std::uint64_t                          carrierUniqueId{0};
        std::uint64_t                          carrierRuntimeId{0};
        hologramlib::ContainerMenuSpec         spec;

        [[nodiscard]] bool isBig() const { return spec.rows >= 6; }
        [[nodiscard]] int  slotCount() const { return isBig() ? 54 : 27; }
    };

    int allocateContainerId();

    void dispatch(hologramlib::ContainerClickEvent const& event);
    // GMLIB 方案的三个发送步骤（全部手写包）
    void sendChestBlocks(::Player& player, Menu const& menu);
    void sendChestBlockActor(::Player& player, Menu const& menu);
    // 只重发方块实体 NBT（就地换内容用）
    void resendBlockActor(::Player& player, Menu const& menu);
    void sendContainerOpen(::Player& player, Menu const& menu);
    // 关闭时把真方块改回去（含大箱子的第二格）
    void restoreRealBlocks(::Player& player, Menu const& menu);
    // 旧路径（useMinecart）: 合成箱子矿车 + ContainerOpen(MinecartChest)
    void sendMinecartCarrier(::Player& player, Menu const& menu);
    // 延时分步发送（在服务器线程上重取快照再发; 玩家可能已离线、菜单可能已关）
    void laterSendBlocks(std::string playerName, int64_t menuId);
    void laterSendOpen(std::string playerName, int64_t menuId);
    void laterMinecartOpen(std::string playerName, int64_t menuId);
    [[nodiscard]] bool snapshotMenu(int64_t menuId, Menu& out) const;

    mutable std::mutex                        mMutex;
    std::unordered_map<int64_t, Menu>         mMenus;
    std::unordered_map<std::string, int64_t>  mByPlayer;
    std::unordered_map<uint64_t, std::function<void(hologramlib::ContainerClickEvent const&)>> mListeners;
    int64_t  mNextMenuId{1};
    int      mNextContainerId{101}; // 显示区间: 避开真实容器的 1..100
    uint64_t mNextListenerToken{1};
};

// 公开接口适配（IHologramLib::containerMenus()）
hologramlib::IContainerMenu& containerMenuAdapter();

} // namespace debugshape_export
