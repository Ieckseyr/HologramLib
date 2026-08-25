// CustomEntityManager.h - 自定义实体协议层生成管理器
//
// 通用协议层实体生成（1.10.0 新能力域）:
//   AddActorPacket(任意 identifier)   在客户端生成纯视觉实体（不占服务端实体系统）
//   RemoveActorPacket                 客户端实体删除
//   metadata: flags/名字/变种/标记变种/颜色索引
//   attributes: health + minecraft:scale 缩放（0.0625~10 客户端钳制）
//
// 与 ItemDisplay 域的区别: 不做物品/动画编排, 面向"任意类型实体"的轻量生成
// （NPC 壳、装饰生物、盔甲架布景等）; Actor ID 用 0x6E 段与 0x6D 物品域隔离。
//
// 防串台策略与 ItemDisplay 同源: setter 只标脏, tick 内合并单次 respawn;
// respawn 必换新 uniqueId/runtimeId（防客户端同帧 Remove+Add 同 ID 重映射串台）。
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ll/api/event/Listener.h>

#include "hologramlib/HologramLib.h" // hologramlib::CustomEntityConfig（公开头单一定义）

#include "mc/platform/UUID.h"

class Player; // ::Player

namespace debugshape_export {

using CustomEntityConfig = hologramlib::CustomEntityConfig;

class CustomEntityManager {
public:
    static CustomEntityManager& getInstance();

    // 生命周期（事件监听; 由 ModEntry 调用）
    void init();
    void shutdown();

    // 管理接口（id 驱动; 创建失败返回 < 0）
    int64_t create(CustomEntityConfig const& config);
    // 随机 ID 段 [0x10000000,0x7FFFFFFF) 自动生成不重复 ID; 失败返回 < 0
    int64_t createRandom(CustomEntityConfig const& config);
    // 指定 ID 创建（持久化恢复用）; desiredId<=0 或占用返回 -2
    int64_t createWithId(CustomEntityConfig const& config, int64_t desiredId);
    // 查询 ID 是否在用
    bool isIdUsed(int64_t id) const;
    bool    destroy(int64_t id);
    void    destroyAll();
    bool    exists(int64_t id) const;
    bool    get(int64_t id, CustomEntityConfig& out) const; // 拷贝输出当前配置

    bool setIdentifier(int64_t id, std::string const& identifier);
    bool setPosition(int64_t id, float x, float y, float z, int dim);
    bool setRotation(int64_t id, float yaw, float pitch);
    bool setNametag(int64_t id, std::string const& text); // 空串清除
    bool setScale(int64_t id, float scale);               // 客户端有效域 0.0625~10
    bool setVariant(int64_t id, int variant);
    bool setMarkVariant(int64_t id, int markVariant);
    bool setColorIndex(int64_t id, int colorIndex);
    bool setFlags(int64_t id, int64_t flags);   // 原始 flags 位掩码（0x20=隐身 等）
    bool setInvisible(int64_t id, bool on);     // 便捷开关（与 flags 独立, 发包时合成）
    bool setEnabled(int64_t id, bool enabled);
    bool setViewDistance(int64_t id, double dist);

    std::vector<int64_t> getAllIds() const;

    // 查找距 (x,y,z) 最近的实体（dim 匹配; maxDist<=0 无限制; 无匹配返回 -1）
    int64_t findNearest(float x, float y, float z, int dim, double maxDist) const;

private:
    CustomEntityManager()  = default;
    ~CustomEntityManager() = default;

public:
    // 运行时状态（发包原语需要访问）
    struct Runtime {
        std::uint64_t                 uniqueId{};   // ActorUniqueID
        std::uint64_t                 runtimeId{};  // ActorRuntimeID
        std::unordered_set<mce::UUID> shownPlayers; // 已向其发送实体的玩家
    };

private:
    // 内部: 持锁状态下刷新可见性 / 变更后刷新
    void refreshLocked(int64_t id);
    void syncVisibilityLocked();
    // tick 内合并处理脏实体（同一 tick 的多次 setter 调用合并为单次 respawn）
    void processDirtyLocked();
    // 持锁状态下的创建主体（id 已查重）; 失败返回 < 0
    int64_t createLocked(CustomEntityConfig const& config, int64_t id);
    // 实体 ID 分配（respawn 换新 ID, 防客户端"同帧 Remove+Add 同 ID"重映射串台）
    [[nodiscard]] std::uint64_t allocUniqueIdLocked();
    [[nodiscard]] std::uint64_t allocRuntimeIdLocked();

    friend struct CustomEntityTickHookAccess;

    mutable std::recursive_mutex                     mMutex;
    int64_t                                          mNextId{1};
    std::unordered_map<int64_t, CustomEntityConfig>  mConfigs;
    std::unordered_map<int64_t, Runtime>             mRuntimes;
    std::unordered_set<int64_t>                      mDirtyIds;
    // Actor ID 段: 0x6E 前缀（与 ItemDisplay 的 0x6D 段隔离, 客户端 ID 空间全局不撞）
    std::uint64_t                                    mNextActorUniqueId{0x6E00000000000001ULL};
    std::uint64_t                                    mNextRuntimeId{0x6E000000ULL};
    std::unordered_set<mce::UUID>                    mInitializedPlayers;
    ll::event::ListenerPtr                           mJoinListener;
    ll::event::ListenerPtr                           mDisconnectListener;
};

} // namespace debugshape_export