// ViewOverrideManager.h - 客户端视图覆盖: 覆盖表 + 包改写（协议层拦截, 1.25.0）
//
// 挂钩点在 OutboundViewHook.cpp（LoopbackPacketSender 的按玩家发包函数）。这里只做两件事:
//   1. 存"某玩家该看到什么"（覆盖表: 按玩家 + 一份全局的）
//   2. 给它一条出站包, 决定改哪些字段（能原包改的就原包改, 改不了的交 runPostActions 补发）
//
// 设计要点:
//   · **零开销快路径**: 全库没有任何覆盖时 (active() == false), 钩子里一条分支就走完, 不读包
//   · **命中才碰包**: 目标玩家没有覆盖时 applyToOutbound 立刻返回 false, 一个字段都不读
//   · **只改字段不拼字节**: 所有包仍由 BDS 自己序列化 —— 我们只改载荷里的值
//   · **幂等**: 同一条包可能经过多条发包路径, 覆盖要能被重复套用而不出错
//   · **可还原**: 广播时同一条包要发给多个玩家, 逐收件人改完必须放回原值（restoreOutbound）,
//     否则后一个收件人会看到前一个的覆盖
#pragma once

#include "ViewOverrideLogic.h"

#include "hologramlib/HologramLib.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mc/network/NetworkIdentifier.h>
#include <mc/world/actor/DataItem.h>

class NetworkIdentifier;
class Player;
class Packet;
class Actor;

namespace debugshape_export::view {

class ViewOverrideManager {
public:
    static ViewOverrideManager& getInstance();

    // ── IViewOverride 的落点（HologramLibImpl.cpp 转过来）──
    bool overrideEntity(std::string const& playerName, std::int64_t uniqueId, hologramlib::EntityView const& view);
    bool overrideBlock(std::string const& playerName, int x, int y, int z, hologramlib::BlockView const& view);
    bool clearEntity(std::string const& playerName, std::int64_t uniqueId);
    bool clearBlock(std::string const& playerName, int x, int y, int z);
    void clearAll(std::string const& playerName);
    [[nodiscard]] std::string describeFor(std::string const& playerName) const;

    // 全库有没有任何覆盖 —— 出站钩子的零开销快路径
    [[nodiscard]] bool active() const { return mActive.load(std::memory_order_relaxed); }
    // 这名玩家（含全局那一份）有没有覆盖
    [[nodiscard]] bool hasOverridesFor(std::string const& playerName) const;

    // ── 出站改写 ──
    // 一次改写里需要记下来的东西: 决定了什么 + 怎么还原 + 原包之后补什么
    struct Applied {
        bool        hit{false};      // 命中了覆盖（false = 调用方原样发包）
        bool        drop{false};     // 这一包不要发给这名玩家
        // 玩家"变生物": 出生包被吃掉后, 用同一 runtimeId/uniqueId 补发一只该类型的实体
        std::int64_t  respawnUniqueId{-1};
        std::uint64_t respawnRuntimeId{0};
        std::string   respawnIdentifier;
        std::string   respawnPlayerName;
        // 原包发出后要补发的方块（区块重发会把覆盖冲掉）: 打包坐标 + 覆盖后的网络 id
        std::vector<std::pair<std::int64_t, std::uint32_t>> repush;

        // 名字牌补发（原包之后由我们自己的 SetActorData 压过去）: uniqueId + runtimeId
        std::vector<std::pair<std::int64_t, std::uint64_t>> nametagPush;
    };

    // 命中才碰包; 返回 false = 这名玩家没有任何覆盖（一个字段都没读）
    bool applyToOutbound(::Player& target, ::Packet& packet, Applied& applied);
    // 原包已经发出去之后调用: 补发需要补的方块
    void runPostActions(::Player& target, Applied const& applied);

    // 玩家下线（覆盖表里的这一份没人管了, 清掉）/ 关服
    void onPlayerLeave(std::string const& playerName);
    void shutdown();

    // A 的最近一条输入包快照 —— **协议层的输入真相**（客户端自己上报的位置/朝向/输入位/界面状态）。
    // 代理复现 A 的动作以它为准, 而不是等服务端把状态同步出来（那会慢一拍, 也可能被服务端修正掉）。
    struct InputSnapshot {
        std::int64_t  uniqueId{0};  // 他本人的 uniqueId（入站钩子里就能拿到）→ 替换路径按它找快照
        std::uint64_t runtimeId{0}; // 他当前的 runtimeId → 替换出生包要用同一个 id
        float         x{0}, y{0}, z{0};
        float         pitch{0}, yaw{0}, headYaw{0};
        std::uint64_t bits{0};     // PlayerAuthInputPacketPayload::InputData 位（我们只用低 64 位）
        int           playMode{0}; // ClientPlayMode（2 = Screen: A 在某个界面里）
        std::uint64_t tick{0};     // 收到时的服务器 tick（判断新鲜度）
    };
    // 收到 A 的 PlayerAuthInput 时更新（输入操作 → 代理）
    void noteAuthInput(std::string const& playerName, InputSnapshot const& snapshot);
    // 按 uniqueId 取快照（替换成生物的玩家用; 不依赖"见过他的出生包"）
    [[nodiscard]] bool inputSnapshotOf(std::int64_t uniqueId, InputSnapshot& out) const;

    // 心跳（服务器每 tick 调一次）: 实体"重新进入视野" / 玩家换区块时把覆盖重新推一遍。
    // 为什么需要它: 实体出生包不走按玩家发包函数（实测), 所以"走出视野再回来"客户端会重新
    // 拿到一只真实的实体 —— 拦截不到就靠这里补。方块同理（区块重发会把覆盖冲掉）。
    // 全库没有覆盖时, 这里只有一次原子读 + 一次 map 查找。
    void tickPulse();


    // 诊断
    [[nodiscard]] std::size_t globalEntityCount() const;
    [[nodiscard]] std::size_t globalBlockCount() const;

    // 诊断计数: 出站钩子到底有没有被调用、哪几类包到过改写入口。
    // 现象是"设了没反应"时先看它 —— calls 一直是 0 就说明挂钩点根本不在发包路径上。
    struct HookCounters {
        std::uint64_t calls{0};   // 进过出站钩子（四条路径合计）
        std::uint64_t spawn{0};   // 走到改写入口的实体出生包（AddActor/AddPlayer）
        std::uint64_t meta{0};    // 走到改写入口的元数据包（SetActorData）
        std::uint64_t block{0};   // 走到改写入口的方块包（UpdateBlock*）
        std::uint64_t chunk{0};   // 走到改写入口的区块包（FullChunkData）
    };
    void          noteHookCall();
    [[nodiscard]] HookCounters hookCounters() const;

private:
    struct EntityRecord {
        std::int64_t            uniqueId{0};
        hologramlib::EntityView view;
        bool                    runtimeKnown{false}; // 知道它的 runtimeId（出生包见过 / 从世界里问到过）
        std::uint64_t           runtimeId{0};
        bool                    substituted{false}; // 玩家"变生物"= 出生包被替换成同 id 的生物（不是代理）
        std::string             playerName;   // 该实体的玩家名（从 AddPlayer 包里的 mName 拿, 纯协议层）
        float                   proxyX{0}, proxyY{0}, proxyZ{0}, proxyYaw{0}, proxyPitch{0}; // 上次推过的位置/朝向（变了才发）
        std::string             proxySig;    // 状态位+手持签名的上次值（变了才发）
    };

    struct BlockRecord {
        hologramlib::BlockView view;
        std::uint32_t          viewId{0};      // 覆盖方块的网络 id（覆盖时解析一次）
        std::uint32_t          realId{0};      // 拦到的真实网络 id（撤销时改回去）
        bool                   hasReal{false};
        int                    dim{logic::kAnyDimension};
    };

    struct Table {
        std::unordered_map<std::int64_t, EntityRecord> entities;         // uniqueId → 记录
        std::unordered_map<std::int64_t, BlockRecord>  blocks;           // 打包坐标 → 记录
        std::unordered_map<std::uint64_t, std::int64_t> byRuntime;       // runtimeId → uniqueId

        [[nodiscard]] bool empty() const { return entities.empty() && blocks.empty(); }
    };

    // 心跳用的每玩家状态（"上次看到它在视野里吗" / "上次在哪个区块"）
    struct PulseState {
        std::uint32_t                                    ticks{0};   // 进服后的心跳拍数（宽限期用）
        int                                              chunkX{0};
        int                                              chunkZ{0};
        bool                                             hasChunk{false};
        std::unordered_map<std::int64_t, bool>           visible;   // uniqueId → 上一拍在不在视野
    };

    // 下列 Locked 结尾的都在持锁状态下调用
    Table&       ensureTableLocked(std::string const& playerName);
    Table const* tableOfLocked(std::string const& playerName) const;
    // 玩家表优先, 其次全局表（全局 = playerName 空串的那一份）
    EntityRecord* findEntityLocked(std::string const& playerName, std::int64_t uniqueId);
    EntityRecord* findEntityByRuntimeLocked(std::string const& playerName, std::uint64_t runtimeId);
    BlockRecord*  findBlockLocked(std::string const& playerName, std::int64_t posKey);
    void          refreshActiveLocked();


    // 表操作（不持锁）: 设/撤覆盖之后要补发的包在这里发
    void onEntityOverrideChanged(std::string const& playerName, std::int64_t uniqueId, EntityRecord const& record);
    void onBlockOverrideCleared(std::string const& playerName, std::int64_t posKey, BlockRecord const& record);
    // 换外观: RemoveActor + 重发一只带该类型的实体（同一个 runtimeId/uniqueId）
    void respawnForPlayer(
        ::Player&          player,
        ::Actor&           actor,
        std::string const& identifier,
        std::string const& nametag,
        bool               hasNametag,
        bool               alwaysShow
    );
    // 把一条实体覆盖"恢复原样"（撤销时用）: 换过类型/隐藏过 → 重发真实类型; 只改过名字牌 → 推回真实名字牌
    void restoreEntityForPlayer(::Player& player, std::int64_t uniqueId, EntityRecord const& record);
    // 把一条实体覆盖"重新套用"（心跳发现它重新进入视野时用）
    // 单名玩家的心跳
    void pulseForPlayer(::Player& player);
    // 清表之后按收集到的记录把客户端恢复原样
    void restoreCleared(
        std::string const&                                 playerName,
        std::vector<std::pair<std::int64_t, EntityRecord>> const& entities,
        std::vector<std::int64_t> const&                   blockKeys
    );

    mutable std::mutex                     mMutex;
    Table                                  mGlobal;    // 对所有玩家生效的那一份
    std::unordered_map<std::string, Table> mPlayers;
    std::unordered_map<std::string, PulseState>    mPulse;  // 心跳状态（按玩家名）
    std::unordered_map<std::string, InputSnapshot> mInputs; // 最近一条输入包（按玩家名）

    // 操控: 操控者 → { 目标玩家名, 是否跟随, 上一拍上报的位置（用来算这一拍的移动量）}
    std::uint64_t                                  mServerTick{0}; // 心跳自增, 用于"输入是否新鲜"
    std::atomic<bool>                      mActive{false};

    // 诊断计数（只增不减; 用来看钩子有没有被走到）
    std::atomic<std::uint64_t> mHookCalls{0};
    std::atomic<std::uint64_t> mSpawnSeen{0};
    std::atomic<std::uint64_t> mMetaSeen{0};
    std::atomic<std::uint64_t> mBlockSeen{0};
    std::atomic<std::uint64_t> mChunkSeen{0};
    std::atomic<std::uint64_t> mControlSteps{0}; // 操控驱动的实际推动次数（诊断）
};

// 注册"玩家下线清运行时 id 索引"的监听（事件总线一次; 重复调用无副作用）
void initViewOverrideEvents();

} // namespace debugshape_export::view

namespace debugshape_export {

// IViewOverride 适配器（HologramLibImpl 用; 实现在 ViewOverrideManager.cpp）
hologramlib::IViewOverride& viewOverrideAdapter();

} // namespace debugshape_export
