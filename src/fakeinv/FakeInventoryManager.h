// FakeInventoryManager.h - 背包虚容器（1.23.0 未发布线）
//
// 把**客户端看到的玩家背包**换成调用方给的那一份（协议层手写包; 服务端背包不动）。
// 机制、边界与"与交易菜单/虚拟容器共存"的说明见 HologramLib.h 的域注释。
//
// 点击检测: 玩家对自己背包的操作走 ItemStackRequest(147 / AuthInput 内嵌), 容器枚举是
// CombinedHotbarAndInventoryContainer(12) / Hotbar(28) / Inventory(29) —— 与虚拟容器的
// LevelEntityContainer(7) 不同, 所以两个域同开也不会互相抢。命中"伪造且非空"的槽位才回调,
// 回调后把那一格重发一次（BDS 的失败回滚会把客户端那格刷成真实物品, 重发把伪造内容盖回去）。
//
// 周期重发: ServerLevelTickEvent 自驱（与悬浮字动画同一套办法）, 间隔 = spec.refreshIntervalTicks。
#pragma once

#include "hologramlib/HologramLib.h" // FakeInventorySpec / FakeInventoryClickEvent

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace debugshape_export {

class FakeInventoryManager {
public:
    static FakeInventoryManager& getInstance();

    bool apply(std::string const& playerName, hologramlib::FakeInventorySpec const& spec);
    bool setSlot(std::string const& playerName, int slot, hologramlib::ContainerMenuItem const& item);
    bool refresh(std::string const& playerName);
    bool clear(std::string const& playerName); // 撤销伪造: 停周期重发 + 重发真实背包
    void clearAll();

    [[nodiscard]] bool                     isActive(std::string const& playerName) const;
    [[nodiscard]] std::vector<std::string> getActivePlayers() const;

    uint64_t addClickListener(std::function<void(hologramlib::FakeInventoryClickEvent const&)> listener);
    bool     removeClickListener(uint64_t token);

    // ── LSE 轮询（脚本侧注册不了 C++ 监听器, 走队列）──
    std::vector<hologramlib::FakeInventoryClickEvent> pollClicks();
    void                                              clearClicks();
    // 事件格式化: "player=X slot=S"
    [[nodiscard]] static std::string formatClick(hologramlib::FakeInventoryClickEvent const& event);

    // 物品请求动作派发（由 interaction/PlayerInteractionHooks.cpp 的 147/AuthInput 钩子调用）。
    // 返回 true = 这次动作命中了本域的某个伪造槽位（已回调）。
    bool handleInventoryAction(std::string const& playerName, int containerEnum, int slot);

    // 玩家下线: 清记录（不需要发撤销包 —— 客户端重连会自己拿真实背包）
    void onPlayerLeave(std::string const& playerName);
    void shutdown();

    // 每个服务器 tick 调一次（由本文件底部注册的 ServerLevelTickEvent 监听转发;
    // 与悬浮字动画同一套静态 bootstrap, 不需要 ModEntry 显式调用）
    void tick();

private:
    FakeInventoryManager() = default;

    struct Session {
        std::string                   playerName;
        hologramlib::FakeInventorySpec spec;
        int                            ticksSinceRefresh{0};
    };

    [[nodiscard]] static bool slotIsFaked(Session const& session, int slot);
    // 持锁: 找到会话（不存在返回 nullptr）
    Session* findLocked(std::string const& playerName);

    mutable std::mutex                          mMutex;
    std::unordered_map<std::string, Session>    mSessions;
    std::unordered_map<uint64_t, std::function<void(hologramlib::FakeInventoryClickEvent const&)>> mListeners;
    std::deque<hologramlib::FakeInventoryClickEvent> mClickQueue; // LSE 轮询队列
    uint64_t                                    mNextListenerToken{1};
};

// 公开接口适配（IHologramLib::fakeInventories()）
hologramlib::IFakeInventory& fakeInventoryAdapter();

} // namespace debugshape_export
