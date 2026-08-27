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
#include <map>
#include <mutex>
#include <optional>
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
    bool setPose(int64_t id, int pose);         // PoseIndex 0..13（盔甲架坐姿/睡姿/跳舞等）
    // 装备槽位: 0=mainhand 1=offhand 2=head 3=chest 4=legs 5=feet；name 空清空
    bool setEquipmentSlot(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt);

    // ── 1.12.0 追加 ──
    bool scaleBy(int64_t id, double factor);    // 相对缩放（结果钳制 0.0625~10）
    // 可见玩家白名单（按 Player::getRealName 匹配; 空列表 = 清除 = 全员可见）
    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames);
    bool clearVisiblePlayers(int64_t id);
    bool setVisiblePlayer(int64_t id, std::string const& playerName);
    std::string getDebugInfo(int64_t id) const; // 诊断探针（找不到返回 "not_found"）
    // 骑乘链接（SetActorLinkPacket; 变更经 respawn 重放; 两者互斥, 后设者生效）
    bool setRidePlayer(int64_t id, std::string const& playerName); // 骑到指定玩家头上（须在线）
    bool setRideEntity(int64_t id, int64_t vehicleEntityId);      // 骑到另一自定义实体上
    bool clearRide(int64_t id);
    // 播放原版动画（AnimateEntityPacket; controller 名按实体 id 自动唯一化）
    // stopExpression 空串 = 常驻; durationTicks>0 时到期自动停止
    bool playAnimation(
        int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks
    );

    std::vector<int64_t> getAllIds() const;

    // 查找距 (x,y,z) 最近的实体（dim 匹配; maxDist<=0 无限制; 无匹配返回 -1）
    int64_t findNearest(float x, float y, float z, int dim, double maxDist) const;

    // runtimeId -> 库内 id 反查（ghost 交互路由用; 无匹配返回 false）
    bool findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const;

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
    void refreshLightLocked(int64_t id); // 轻脏增量刷新（不换ID, 零闪烁, 动画驱动）
    void syncVisibilityLocked();
    // tick 内合并处理脏实体（同一 tick 的多次 setter 调用合并为单次 respawn）
    void processDirtyLocked();
    // 持锁状态下的创建主体（id 已查重）; 失败返回 < 0
    int64_t createLocked(CustomEntityConfig const& config, int64_t id);
    // 实体 ID 分配（respawn 换新 ID, 防客户端"同帧 Remove+Add 同 ID"重映射串台）
    [[nodiscard]] std::uint64_t allocUniqueIdLocked();
    [[nodiscard]] std::uint64_t allocRuntimeIdLocked();
    // 骑乘链接载具 uniqueId 解析（持锁; 玩家须在线 / 载具实体须存在; 无链接返回 nullopt）
    [[nodiscard]] std::optional<std::uint64_t> resolveVehicleUniqueIdLocked(CustomEntityConfig const& data);
    // 持锁状态下的动画队列 flush（到期条目发送 + 失效条目丢弃）
    void flushAnimsLocked();

    friend struct CustomEntityTickHookAccess;

    // 动画延迟发送队列（AnimateEntityPacket; playAnimation 入队, tick hook 经 processDirtyLocked flush）
    struct EntityAnimEntry {
        mce::UUID     playerUuid;
        std::uint64_t runtimeId;
        int64_t       entityId;
        std::string   animation;
        std::string   controller;
        std::string   stopExpression;
    };

    mutable std::recursive_mutex                     mMutex;
    int64_t                                          mNextId{1};
    std::unordered_map<int64_t, CustomEntityConfig>  mConfigs;
    std::unordered_map<int64_t, Runtime>             mRuntimes;
    // 重脏: 需 respawn 重建(identifier/pose/equipment/flags/variant/color/markVariant/viewDistance/enabled)
    std::unordered_set<int64_t>                      mDirtyIds;
    // 轻脏: 增量包刷新即可(setPosition/setRotation/setScale/setNametag/setInvisible)
    // —— 不换 uniqueId/runtimeId, 客户端无闪烁; 动画每 tick 驱动只走此路径
    std::unordered_set<int64_t>                      mLightDirtyIds;
    // 可见玩家白名单（id -> 玩家名集合; 无条目 = 全员可见, 兼容默认行为）
    std::unordered_map<int64_t, std::unordered_set<std::string>> mVisibleFilter;
    std::multimap<std::uint64_t, EntityAnimEntry>    mAnimQueue;
    // Actor ID 段: 0x6E 前缀（与 ItemDisplay 的 0x6D 段隔离, 客户端 ID 空间全局不撞）
    std::uint64_t                                    mNextActorUniqueId{0x6E00000000000001ULL};
    std::uint64_t                                    mNextRuntimeId{0x6E000000ULL};
    std::unordered_set<mce::UUID>                    mInitializedPlayers;
    ll::event::ListenerPtr                           mJoinListener;
    ll::event::ListenerPtr                           mDisconnectListener;
};

} // namespace debugshape_export