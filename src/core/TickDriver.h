// TickDriver.h - 全库唯一的 Level::tick 驱动
//
// 收敛前有 4 个 detour 各自挂在 Level::$tick 上（customentity / itemdisplay / particles /
// playernpc），执行顺序由**静态初始化顺序**决定 —— 跨 TU 未指定，且每个 detour 都要付一次
// trampoline。收敛后只有 1 个 detour，顺序变成显式的 order 值。
//
// 设计约束（有意为之）:
//   · 回调是**无捕获函数指针**，不用 std::function —— 注册发生在静态初始化期，不能有堆分配,
//     也不该把某个单例的生命周期拖进静态初始化顺序问题。
//   · 注册表定长（kMax），add 失败返回 false 并在诊断里可见, 绝不静默丢弃。
//   · run() 只读已发布计数，热路径上没有锁。
//
// 与 ServerLevelTickEvent 的关系: 那是 LL 事件, 由 EventBus 驱动（fakeinv / 悬浮字动态行 /
// 形状兜底重发三类仍走它）。本文件只负责"引擎钩子"这一路。把它们合并成同一张表是下一步,
// 但两种来源混跑会一 tick 跑两遍，所以**不要**让同一个回调两边都注册。
#pragma once

#include <atomic>
#include <cstddef>
#include <string>

namespace debugshape_export {
namespace tick {

using Callback = void (*)();

// 执行顺序: order 小者先。**必须显式给**, 因为注册发生在各 TU 的静态初始化期。
enum class Order : int {
    PlayerNpc     = 10, // 玩家 NPC: 脏刷新 / 轻脏朝向 / 动画队列 / Tab 列表
    CustomEntity  = 20, // 自定义实体: 脏刷新 + 20 tick 可见性同步
    ItemDisplay   = 30, // 物品展示: 脏刷新 / 跟随 / 动画队列 / 20 tick 同步
    ParticleShape = 40, // 粒子形状: 引擎同路径 flush
    ViewOverride  = 50, // 视图覆盖心跳: **最后**跑, 让覆盖套在已经落地的实体状态上
};

class TickDriver {
public:
    static TickDriver& getInstance() noexcept;

    // 返回 false = 注册表已满（kMax）或被重复注册
    bool add(char const* name, Callback cb, Order order);

    // 由唯一 hook 调用; 每次引擎 tick 一次
    void run();

    std::size_t size() const noexcept;
    // 按执行顺序拼出 "name(10) -> name(20)"，诊断/测试用
    std::string describe() const;

    // 定长表容量: 每个域的 per-tick 工作是**逐 tick 成本**, 表不该无限长
    static constexpr std::size_t kMax = 16;

private:
    TickDriver() = default;

    struct Entry {
        char const* name{};
        Callback    cb{};
        int         order{};
    };

    Entry              mEntries[kMax]{};
    std::size_t        mCount{};
    std::atomic<std::size_t> mPublished{0};
};

// 静态初始化期自动注册（RAII 只是为了让 add 的调用点一行写完）
struct AutoRegister {
    AutoRegister(char const* name, Callback cb, Order order) {
        (void)TickDriver::getInstance().add(name, cb, order);
    }
};

} // namespace tick
} // namespace debugshape_export
