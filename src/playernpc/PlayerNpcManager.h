// PlayerNpcManager.h - 假玩家 NPC 管理器（1.16.0）
//
// 纯协议假玩家生命周期:
//   PlayerListPacket(Add, 皮肤) → AddPlayerPacket → [20 tick] PlayerListPacket(Remove)
//   （假玩家短暂出现在 Tab 后被移除, 实体因皮肤已缓存而持续渲染）
// 逐玩家可见性 + 视距滞回（进入视距立即 spawn; 退出需超出 viewDistance+4 才 despawn,
//   防边界抖动——每次 respawn 都要重发完整皮肤, 滞回显著降低带宽）
// 属性变更经脏刷新合并为单次 respawn（对齐 ItemDisplay/CustomEntity 模式）
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ll/api/event/Listener.h>

#include "hologramlib/HologramLib.h" // hologramlib::PlayerNpcConfig / PlayerNpcSkin

#include "mc/platform/UUID.h"

class Player; // ::Player

namespace debugshape_export {

using PlayerNpcConfig = hologramlib::PlayerNpcConfig;

class PlayerNpcManager {
public:
    static PlayerNpcManager& getInstance();

    // 生命周期（事件监听 + tick hook; 由 ModEntry 调用）
    void init();
    void shutdown();

    // 皮肤（委托 NpcSkinRegistry; 返回值语义见 IPlayerNpc 注释）
    bool registerSkin(hologramlib::PlayerNpcSkin const& skin);
    bool captureSkin(std::string const& skinId, std::string const& playerName);
    bool hasSkin(std::string const& skinId) const;
    bool unregisterSkin(std::string const& skinId); // 有 NPC 引用时拒绝
    std::vector<std::string> getSkinIds() const;
    // 目录批量导入（1.18.0; 一个子文件夹 = 一套皮肤: PNG + 可选 .json 几何）
    // 返回导入数量; 目录无效返回 -1
    int importSkins(std::string const& dirPath);
    // 皮肤全字段序列化导出 / blob 注册（消费方持久化用）
    bool getSkinBlob(std::string const& skinId, std::string& out) const;
    bool registerSkinFromBlob(std::string const& blob);

    // NPC 生命周期（创建失败: -1 常规 / -2 id 占用 / -3 皮肤未注册）
    int64_t create(PlayerNpcConfig const& config);
    int64_t createRandom(PlayerNpcConfig const& config);
    int64_t createWithId(PlayerNpcConfig const& config, int64_t desiredId);
    bool    destroy(int64_t id);
    void    destroyAll();
    bool    exists(int64_t id) const;
    bool    get(int64_t id, PlayerNpcConfig& out) const;
    bool    isIdUsed(int64_t id) const;
    std::vector<int64_t> getAllIds() const;

    // 属性（setter 标脏, tick 内合并 respawn）
    bool setPosition(int64_t id, float x, float y, float z, int dim); // dim<0 仅改坐标
    bool setRotation(int64_t id, float yaw);
    bool setNametag(int64_t id, std::string const& text);
    bool setSkin(int64_t id, std::string const& skinId); // 未注册返回 false
    bool setViewDistance(int64_t id, double dist);
    bool setScale(int64_t id, float scale); // <0.0625 或 >10 返回 false
    bool setEnabled(int64_t id, bool enabled);

    // 可见玩家白名单（Player::getRealName 即 LSE realName 匹配; 空列表 = 全员可见）
    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames);
    bool clearVisiblePlayers(int64_t id);
    bool setVisiblePlayer(int64_t id, std::string const& playerName);

    std::string getDebugInfo(int64_t id) const;

    // runtimeId -> 库内 id 反查（ghost 交互路由用; 无匹配返回 false）
    bool findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const;

private:
    PlayerNpcManager()  = default;
    ~PlayerNpcManager() = default;

public:
    // 运行时状态（发包原语需要访问）
    struct Runtime {
        std::uint64_t                 uniqueId{};   // ActorUniqueID
        std::uint64_t                 runtimeId{};  // ActorRuntimeID
        std::unordered_set<mce::UUID> shownPlayers; // 已向其发送假玩家的玩家
    };

    // 待处理 Tab 移除（spawn 后 20 tick; NPC id -> 条目列表）
    struct TabRemoval {
        mce::UUID     playerUuid;
        std::uint64_t dueTick{};
    };

private:
    // 内部（持锁状态）
    void    refreshLocked(int64_t id);            // respawn（换新实体 ID, 防串台）
    void    syncVisibilityLocked();              // 可见性重算（含滞回）
    void    processDirtyLocked();                 // tick 内合并脏刷新
    int64_t createLocked(PlayerNpcConfig const& config, int64_t id);
    bool    skinReferencedLocked(std::string const& skinId) const; // 是否有 NPC 在用

    friend struct PlayerNpcTickHookAccess;

    mutable std::recursive_mutex                              mMutex;
    int64_t                                                   mNextId{1};
    std::unordered_map<int64_t, PlayerNpcConfig>               mConfigs;
    std::unordered_map<int64_t, Runtime>                       mRuntimes;
    std::unordered_set<int64_t>                                mDirtyIds;
    std::unordered_map<int64_t, std::vector<TabRemoval>>        mTabRemovals;
    std::unordered_map<int64_t, std::unordered_set<std::string>> mVisibleFilter;

    // Actor ID 段: 0x6F 前缀（与 ItemDisplay 0x6D / CustomEntity 0x6E 段隔离, ghost 路由按段分发）
    std::uint64_t                                             mNextActorUniqueId{0x6F00000000000001ULL};
    std::uint64_t                                             mNextRuntimeId{0x6F000000ULL};

    std::unordered_set<mce::UUID>                             mInitializedPlayers;
    ll::event::ListenerPtr                                    mJoinListener;
    ll::event::ListenerPtr                                    mDisconnectListener;
};

} // namespace debugshape_export
