// ItemDisplayManager.h - FMBE 狐狸悬浮物显示管理器
//
// 基于 ItemPhys 的"狐狸显示 + 发包"技术（FMBE）：
//   AddActorPacket(minecraft:fox)      创建隐形假狐狸作为载体
//   MobEquipmentPacket                 让狐狸手持指定物品（显示内容）
//   SetActorDataPacket                 写入隐藏标记
//   AnimateEntityPacket × N            Molang 表达式驱动物品的位置/三轴旋转/缩放
//
// 所有变换字段（offset/rot/scale 等）为常量数字或 Molang 表达式字符串。
// 主键为 int64 id（库统一），配置持久化由消费者负责。
#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ll/api/event/Listener.h>

#include "hologramlib/HologramLib.h" // hologramlib::ItemDisplayConfig（公开头单一定义）

#include "mc/platform/UUID.h"
#include "mc/world/item/ItemStack.h"

class Player; // ::Player

namespace debugshape_export {

using ItemDisplayConfig = hologramlib::ItemDisplayConfig;

class ItemDisplayManager {
public:
    static ItemDisplayManager& getInstance();

    // 生命周期（tick hook / 事件监听; 由 ModEntry 调用）
    void init();
    void shutdown();

    // 管理接口（id 驱动; 创建失败返回 < 0）
    int64_t create(ItemDisplayConfig const& config);
    // 随机 ID 段 [0x10000000,0x7FFFFFFF) 自动生成不重复 ID; 失败返回 < 0
    int64_t createRandom(ItemDisplayConfig const& config);
    // 指定 ID 创建（持久化恢复用）; desiredId<=0 或占用返回 -2
    int64_t createWithId(ItemDisplayConfig const& config, int64_t desiredId);
    // 查询 ID 是否在用
    bool isIdUsed(int64_t id) const;
    bool    destroy(int64_t id);
    void    destroyAll();
    bool    exists(int64_t id) const;
    bool    get(int64_t id, ItemDisplayConfig& out) const; // 拷贝输出当前配置

    bool setItem(int64_t id, std::string const& item, int aux);
    bool setItemWithNbt(int64_t id, std::string const& item, int aux, std::string const& nbt);
    bool setPosition(int64_t id, float x, float y, float z, int dim);
    bool setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz);
    bool setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz);
    bool setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz);
    bool setScale(int64_t id, std::string const& scale);
    bool setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry);
    bool setMode(int64_t id, int mode);
    bool setEnabled(int64_t id, bool enabled);
    bool setViewDistance(int64_t id, double dist);
    bool rotateY(int64_t id, float delta); // 在现有 rotY（常量时）上叠加增量
    bool scaleBy(int64_t id, double factor); // 在现有 scale 上乘系数（放大/缩小）
    bool setGlint(int64_t id, bool on);      // 附魔光效开关（BDS 原生路径, 幂等）

    std::vector<int64_t> getAllIds() const;

    // 查找距 (x,y,z) 最近的显示（dim 匹配; maxDist<=0 无限制; 无匹配返回 -1）
    int64_t findNearest(float x, float y, float z, int dim, double maxDist) const;

private:
    ItemDisplayManager()  = default;
    ~ItemDisplayManager() = default;

public:
    // 运行时状态（发包原语需要访问）
    struct Runtime {
        std::uint64_t                 uniqueId{};   // ActorUniqueID
        std::uint64_t                 runtimeId{};  // ActorRuntimeID
        std::unordered_set<mce::UUID> shownPlayers; // 已向其发送实体的玩家

        // 物品解析缓存（避免每次可见性 tick 重建 ItemStack; setItem/setGlint 后自动失效）
        std::optional<::ItemStack>    cachedStack;
        std::string                   cachedItemName{};
        int                           cachedItemAux{0};
        std::string                   cachedItemNbt{};  // 缓存键含 NBT（换附魔时重解析）
        bool                          cachedGlint{false}; // 缓存键含光效开关
        bool                          itemWarned{false}; // 解析失败只告警一次（换物品后重置）
    };

private:
    // 内部: 持锁状态下刷新可见性 / 变更后刷新
    void refreshLocked(int64_t id);
    void syncVisibilityLocked();
    // 持锁状态下的创建主体（id 已查重）; 失败返回 < 0
    int64_t createLocked(ItemDisplayConfig const& config, int64_t id);

    friend struct ItemDisplayTickHookAccess;

    mutable std::recursive_mutex                        mMutex;
    int64_t                                             mNextId{1};
    std::unordered_map<int64_t, ItemDisplayConfig>      mConfigs;
    std::unordered_map<int64_t, Runtime>                mRuntimes;
    std::unordered_set<mce::UUID>                       mInitializedPlayers;
    ll::event::ListenerPtr                              mJoinListener;
    ll::event::ListenerPtr                              mDisconnectListener;
};

} // namespace debugshape_export
