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
#include <deque>
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
    // 可交互模式开关（见 ContainerMenuSpec::interactive）
    bool    setInteractive(int64_t menuId, bool on);
    // 就地换标题（复用载体方块, 重发方块实体 NBT + ContainerOpen; 不重摆方块、不等延迟）
    bool    setTitle(int64_t menuId, std::string const& title);
    bool    close(int64_t menuId);
    void    closeAll();
    [[nodiscard]] bool                 isOpen(int64_t menuId) const;
    [[nodiscard]] std::vector<int64_t> getAllIds() const;

    uint64_t addClickListener(std::function<void(hologramlib::ContainerClickEvent const&)> listener);
    bool     removeClickListener(uint64_t token);

    // ── LSE 轮询（脚本侧注册不了 C++ 监听器, 走队列）──
    // 取走并清空待处理的点击/关闭事件（与 C++ 监听器拿到的是同一份事件）
    std::vector<hologramlib::ContainerClickEvent> pollClicks();
    void                                          clearClicks();
    // 事件格式化为可解析字符串: "player=X menuId=N slot=S closed=0|1"
    [[nodiscard]] static std::string formatClick(hologramlib::ContainerClickEvent const& event);

    // 物品请求动作派发（由 interaction/PlayerInteractionHooks.cpp 的 147/AuthInput 钩子调用）。
    // 返回 true = 这个动作属于本域的某个容器（已回调）。
    bool handleSlotAction(std::string const& playerName, int containerEnum, int containerId, int slot);

    // 一条请求里的"一侧槽位"（src 或 dst）
    struct RequestSlot {
        int container{0};   // ContainerEnumName
        int containerId{-1};
        int slot{-1};
    };

    // **可交互模式**: 把一条请求涉及的全部槽位交进来。只有当
    //   ① 该玩家开着 interactive 的菜单, 且
    //   ② 所有槽位都落在本容器内（没有玩家背包/光标等外部槽位）
    // 时, 才把改动应用到条目表、回传点击, 并返回 true（调用方据此回成功应答、且不再交给 BDS）。
    // 其它情况返回 false（调用方走默认路径: 只回传 + 放行）。
    bool handleInteractiveRequest(std::string const& playerName, std::vector<RequestSlot> const& slots, int amount);

    // 客户端关闭容器（由 ContainerClosePacket 钩子调用）
    bool handleContainerClose(std::string const& playerName, int containerId);

    // 该玩家是否有打开中的容器菜单（物品请求钩子的入口条件）
    [[nodiscard]] bool hasMenuFor(std::string const& playerName) const;
    // 该玩家打开中的容器 menuId（无则 -1）
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
        // 可交互模式下的"光标物品"（客户端手持的那一份; 只在容器内部移动时维护）
        hologramlib::ContainerMenuItem         cursor;

        [[nodiscard]] bool isBig() const { return spec.rows >= 6; }
        [[nodiscard]] int  slotCount() const { return isBig() ? 54 : 27; }
    };

    int allocateContainerId();

    void dispatch(hologramlib::ContainerClickEvent const& event);
    // 可交互: 把一条请求的槽位序列应用到条目表（transfer 语义按动作顺序推演光标与槽位）
    void applyInteractiveSlots(Menu& menu, std::vector<RequestSlot> const& slots, int amount);
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
    std::deque<hologramlib::ContainerClickEvent> mClickQueue; // LSE 轮询队列
    int64_t  mNextMenuId{1};
    int      mNextContainerId{101}; // 显示区间: 避开真实容器的 1..100
    uint64_t mNextListenerToken{1};
};

// 公开接口适配（IHologramLib::containerMenus()）
hologramlib::IContainerMenu& containerMenuAdapter();

} // namespace debugshape_export
