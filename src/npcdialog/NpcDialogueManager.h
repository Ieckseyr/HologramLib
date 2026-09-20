// NpcDialogueManager.h - NPC 对话界面（协议层; 1.22.0 新能力域）
//
// 协议依据（sculk + BDS 26.40 头文件）:
//   NpcDialoguePacket { mNpcId, mActionType(Open=0/Close=1), mDialogue, mSceneName, mNpcName, mActionJSON }
//   NpcRequestPacket  { mActorRuntimeId, mRequestType(ExecuteAction=1/ExecuteClosingCommands=2/…),
//                      mActions, mActionIndex, mSceneName }
// 多层级对话: 点击回传带 sceneName + mActionIndex → 调用方据此发送下一层对话。
//
// 载体实体: 对话框需要 NPC 实体承载名字/头像/按钮。载体是**纯协议实体**（合成 AddActor,
// 不进 BDS 实体系统）, 每玩家一个: 位置在世界下方 y=-66（客户端看不到实体, 但界面里的头像
// 照常渲染; 用隐形标志位反而会让头像消失）, 只发给该玩家; 点按钮/关闭/玩家离线时发
// RemoveActor 删掉客户端实体。identifier 必须是 NPC 家族（默认 minecraft:npc）。
#pragma once

#include <cstdint>
#include <deque>
#include <format>

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "hologramlib/HologramLib.h" // NpcDialogSpec / NpcDialogClickEvent

class Player;

namespace debugshape_export {

class NpcDialogueManager {
public:
    static NpcDialogueManager& getInstance();

    int64_t open(std::string const& playerName, hologramlib::NpcDialogSpec const& spec);
    bool    close(int64_t dialogId);
    // 就地换内容: 复用同一载体, 只重发 NpcDialoguePacket(Open); 并标记"本次点击已接管"。
    // 返回 false = 对话已不在（调用方应改用 open）
    bool    update(int64_t dialogId, hologramlib::NpcDialogSpec const& spec);
    void    closeAll();
    bool    isOpen(int64_t dialogId) const;
    // 取当前规格的拷贝（就地改内容前先读出来, 避免把 npcName/sceneName/载体类型一起冲掉）
    [[nodiscard]] bool getSpec(int64_t dialogId, hologramlib::NpcDialogSpec& out) const;
    std::vector<int64_t> getAllIds() const;

    uint64_t addClickListener(std::function<void(hologramlib::NpcDialogClickEvent const&)> listener);
    bool     removeClickListener(uint64_t token);

    // ── LSE 轮询（脚本侧注册不了 C++ 监听器, 走队列）──
    // 取走并清空待处理的点击/关闭事件（与 C++ 监听器拿到的是同一份事件）
    std::vector<hologramlib::NpcDialogClickEvent> pollClicks();
    void                                          clearClicks();
    // 事件格式化为可解析字符串: "player=X dialogId=N scene=S button=I actionId=A closed=0|1 commands=..."
    [[nodiscard]] static std::string formatClick(hologramlib::NpcDialogClickEvent const& event);

    // NpcRequestPacket 钩子入口。npcId = 客户端回传的 ActorUniqueID。
    // 返回 true = 命中本域的对话（已回调）。
    bool handleNpcRequest(
        std::string const& playerName,
        std::uint64_t      npcId,
        int                requestType,
        int                actionIndex,
        std::string const& sceneName
    );

    void onPlayerLeave(std::string const& playerName);
    void shutdown();

private:
    NpcDialogueManager() = default;

    struct Dialog {
        int64_t                     dialogId{0};
        std::string                 playerName;
        std::uint64_t               carrierUniqueId{0};  // 发包用（ActorUniqueID）
        std::uint64_t               carrierRuntimeId{0}; // 回传匹配用（ActorRuntimeID）
        hologramlib::NpcDialogSpec  spec;
        // 一次点击回传期间调用方是否已接管（调过 update）; 接管了库就不再自动拆这个对话
        bool                        touched{false};
    };

    // 每玩家一个的合成载体（只在客户端存在）
    struct Carrier {
        std::uint64_t uniqueId{0};
        std::uint64_t runtimeId{0};
    };

    // 生成/重建载体（合成 NPC·y=-66·只发给该玩家）; 返回载体 uniqueId, 0 = 失败
    std::uint64_t ensureCarrier(
        ::Player&                          player,
        std::string const&                 playerName,
        hologramlib::NpcDialogSpec const&  spec,
        std::string const&                 actionJson
    );
    // 删除客户端上的载体实体（关闭对话 / 玩家离线时调用）
    void          removeCarrierEntity(::Player& player, std::uint64_t uniqueId);
    [[nodiscard]] static std::uint64_t nextCarrierUniqueId();
    [[nodiscard]] static std::uint64_t nextCarrierRuntimeId();
    void    sendDialog(::Player& player, Dialog const& dialog, bool open);  // 下发 Open/Close
    void    dispatch(hologramlib::NpcDialogClickEvent const& event);

    mutable std::mutex                        mMutex;
    std::unordered_map<int64_t, Dialog>       mDialogs;    // dialogId → 对话
    std::unordered_map<std::string, int64_t>  mByPlayer;   // 玩家名 → dialogId
    std::unordered_map<std::string, Carrier>  mCarriers;   // 玩家名 → 共用载体
    std::unordered_map<uint64_t, std::function<void(hologramlib::NpcDialogClickEvent const&)>> mListeners;
    std::deque<hologramlib::NpcDialogClickEvent> mClickQueue; // LSE 轮询队列
    int64_t  mNextDialogId{1};
    uint64_t mNextListenerToken{1};
};

// 公开接口适配（IHologramLib::npcDialogs()）
hologramlib::INpcDialogue& npcDialogueAdapter();

} // namespace debugshape_export
