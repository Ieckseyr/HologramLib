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

#include "core/VirtualActorIds.h" // runtimeId 号段（唯一出处）

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
using ItemDisplayAnim   = hologramlib::ItemDisplayAnim;

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
    // 无感创建（1.12.0）: mode/viewDistance/可见玩家白名单在首次 spawn 之前写入,
    // 全程只发一次 Add 序列（ItemPhys 无感创建等价）。旧路径 create+setMode+
    // setViewDistance+setVisiblePlayer 会在创建后触发白名单收窄(他人 Add→Remove)
    // 与脏 respawn(Remove+Add 换新 ID) —— 生成瞬间多波 Add/Remove 交错 = 闪烁。
    // mode: -1=不改(用 config 默认) 1/2=指定; viewDistance: -1=不改; visiblePlayer: 空串=不限
    int64_t createSeamless(
        ItemDisplayConfig const& config,
        int                     mode,
        double                  viewDistance,
        std::string const&      visiblePlayer
    );
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

    // 可见玩家白名单（1.10.0 追加; 仅指定玩家可见; 空列表 = 清除 = 全员可见）
    // 名单按玩家名（Player::getRealName, 即 LSE player.realName）匹配
    bool setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames);
    bool clearVisiblePlayers(int64_t id);
    // 标量版（1.10.1）: 单玩家白名单 —— 规避 LSE 数组参数编组差异, JS 侧推荐
    bool setVisiblePlayer(int64_t id, std::string const& playerName);
    // 诊断探针（1.10.1）: 返回展示运行态摘要（dim/pos/mode/filter/shown 等, 字符串返回便于 JS 读取）
    std::string getDebugInfo(int64_t id) const;

    // 单次跳变缩放（1.12.0）: 纯 setter 基元, 无动画调度、无 respawn
    // 以新 scale 常量重发完整方块动画序列（controller 名带 .<id> 后缀原地覆盖）。
    // 每次调用后实体处于单一稳定配置（sleeping 矩阵与 swelling 同步基于新常量）,
    // 不存在逐帧双写入者竞争——出现/消失等动画序列由 LSE 消费者自行步进驱动
    //（前置库保持纯洁: 只提供基元, 不内置动画逻辑; animateScale 已于 1.13.0 移除）
    // 仅方块路径(mode=0 auto/2)
    bool scaleTo(int64_t id, double targetScale);

    // ── 1.17.0 追加 ──
    // 展示跟随玩家: 每 tick 同步目标玩家实时坐标（含跨维度）, 对已见玩家发
    // MoveActorAbsolute（非 teleport, 客户端插值）平滑位移, 全程无 respawn 无闪烁。
    // 玩家下线自动解除（方块原地保留）; setPosition 手动设位解除跟随。
    bool follow(int64_t id, std::string const& playerName, float offX, float offY, float offZ);
    bool unfollow(int64_t id); // 解除跟随; 无跟随关系返回 true
    // AABB 判定体积: R53/R54 元数据广播（即时, 无 respawn）, 数值持久入配置;
    // 0/0 = 恢复不可命中（与历史行为一致）
    bool setHitbox(int64_t id, float width, float height);

    // ── 26.40.12 追加 ──
    // 非等比拉伸: XZ（swelling_scale1）/ Y（swelling_scale2）两条通道; X 与 Z 共轴。
    // 值写常量或 Molang 表达式（"1" = 不拉伸）。setStretch 走 respawn; setStretchLive 只重发动画包。
    bool setStretch(int64_t id, std::string const& xz, std::string const& y);
    bool setStretchLive(int64_t id, std::string const& xz, std::string const& y);
    // live 变换: 改配置后只重发 FMBE 动画包（item 2 / block 5）, 不换实体 ID、不闪。
    bool setRotationLive(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz);
    bool setOffsetLive(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz);
    bool setScaleLive(int64_t id, std::string const& scale);
    bool setTransformLive(
        int64_t id,
        std::string const& ox, std::string const& oy, std::string const& oz,
        std::string const& rx, std::string const& ry, std::string const& rz,
        std::string const& scale
    );
    // 动画预设（库内逻辑; 组合成 Molang 并 live 重发）
    bool setAnim(int64_t id, ItemDisplayAnim const& anim);
    // 单通道预设: spin/bob/pulse/sway/orbit/tumble/wobble/heartbeat/pendulum/billboard/lod/firstperson/clear
    bool setAnimPreset(int64_t id, std::string const& preset, float a, float b, int c);
    bool clearAnim(int64_t id);
    // 单发原语: 让 playerName 看到"uid 这只实体"被画成 FMBE 物品模型。
    //   一次性发 RemoveActor + AddActor(fox) + MobEquipment + SetActorData + FMBE 动画, 不保存任何状态。
    //   cfg 提供 offset/rot/scale/nbt/glint; 朝向默认取该实体当前速度方向。
    //   返回实际渲染模式 1=item 2=block; 失败返回 -1。
    int emitForEntity(std::string const& playerName, std::int64_t uniqueId, ItemDisplayConfig const& cfg);

    std::vector<int64_t> getAllIds() const;

    // 查找距 (x,y,z) 最近的显示（dim 匹配; maxDist<=0 无限制; 无匹配返回 -1）
    int64_t findNearest(float x, float y, float z, int dim, double maxDist) const;

    // runtimeId -> 库内 id 反查（ghost 交互路由用; 无匹配返回 false）
    bool findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const;

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

        // 26.40.12: 非等比拉伸（XZ / Y 两条缩放通道; "1" = 不拉伸）。
        // 不放进 ItemDisplayConfig 是为了不改公开结构体大小（旧编译的 C++ 消费方不会读到界外）。
        std::string                   stretchXZ{"1"};
        std::string                   stretchY{"1"};

        // 26.40.12: 动画预设状态（基准 = 首次 setAnim 时的 config; clearAnim 恢复）
        bool                          animActive{false};
        ItemDisplayAnim               anim{};
        struct AnimBase {
            std::string ox{"0"}, oy{"-4"}, oz{"0"};
            std::string rx{"180"}, ry{"0"}, rz{"180"};
            std::string scale{"0.375"};
        }                             animBase;
    };

private:
    // 内部: 持锁状态下刷新可见性 / 变更后刷新
    void refreshLocked(int64_t id);
    void syncVisibilityLocked();
    // tick 内合并处理脏展示（同一 tick 的多次 setter 调用合并为单次 respawn）
    void processDirtyLocked();
    // 1.17.0: 跟随同步（tick hook 每 tick 调用; 更新配置坐标 + 发移动包 + 跨维度标脏）
    void followTickLocked();
    // 持锁状态下的创建主体（id 已查重）; 失败返回 < 0
    int64_t createLocked(ItemDisplayConfig const& config, int64_t id);
    // 26.40.12: 只重发 FMBE 动画包（调用方持锁; 不 despawn/不换 ID）
    bool resendFmbeLocked(int64_t id, ItemDisplayConfig const& data, Runtime& rt);
    // 把一套 FMBE 动画按 +2..+6 排进队列（调用方持锁; 单玩家）
    void enqueueFmbeAnimsLocked(
        int64_t id, ItemDisplayConfig const& data, Runtime const& rt,
        mce::UUID const& uuid, std::uint64_t base, bool block
    );
    // 实体 ID 分配（respawn 换新 ID, 防客户端"同帧 Remove+Add 同 ID"重映射串台）
    [[nodiscard]] std::uint64_t allocUniqueIdLocked();
    [[nodiscard]] std::uint64_t allocRuntimeIdLocked();

    friend struct ItemDisplayTickHookAccess;

    // 1.17.0: 跟随状态（id -> 目标玩家 + 偏移 + 上次同步坐标; 空表零开销）
    struct FollowState {
        std::string playerName; // Player::getRealName（LSE realName）
        float       offX{0}, offY{0}, offZ{0};
        float       lastX{0}, lastY{0}, lastZ{0};
        bool        synced{false};
    };

    mutable std::recursive_mutex                        mMutex;
    int64_t                                             mNextId{1};
    std::unordered_map<int64_t, ItemDisplayConfig>      mConfigs;
    std::unordered_map<int64_t, Runtime>                mRuntimes;
    std::unordered_set<int64_t>                         mDirtyIds;
    // 可见玩家白名单（id -> 玩家名集合; 无条目 = 全员可见, 兼容默认行为）
    std::unordered_map<int64_t, std::unordered_set<std::string>> mVisibleFilter;
    // 1.17.0: 跟随登记表（id -> 跟随状态; destroy/下线/setPosition 时清除）
    std::unordered_map<int64_t, FollowState> mFollowers;

    std::uint64_t                                       mNextActorUniqueId{0x6D00000000000001ULL};
    std::uint64_t                                       mNextRuntimeId{core::actorIds::itemDisplay.first};
    std::unordered_set<mce::UUID>                       mInitializedPlayers;
    ll::event::ListenerPtr                              mJoinListener;
    ll::event::ListenerPtr                              mDisconnectListener;
};

} // namespace debugshape_export
