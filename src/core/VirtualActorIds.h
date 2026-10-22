// VirtualActorIds.h - 虚拟实体的 runtimeId 号段（全库唯一出处）
//
// 协议层 ghost 实体（物品展示 / 自定义实体 / 假玩家 NPC）在客户端眼里是**真实存在的 Actor**，
// 所以必须占用一个真实的 runtimeId。三个域各自领一段，靠"落在哪一段"来决定一次交互事件
// 该派发给谁 —— 这个判别发生在 GhostInteractRouter 的收包热路径上，不查表。
//
// 收敛前这段契约散在四处:
//   · ItemDisplayManager.h 的 mNextRuntimeId{0x6D000000}
//   · CustomEntityManager.h 的 mNextRuntimeId{0x6E000000}
//   · PlayerNpcManager.h    的 mNextRuntimeId{0x6F000000}
//   · GhostInteractRouter.cpp 里又抄了一遍 6 个 constexpr（Min/Max 各三份）
// 改一处忘另一处 = 交互静默派发不到（客户端那侧什么都不会报错）。现在号段只在下面定义一次，
// 分配起点由各 Manager 直接取 `.first`，判别用 `.contains()`，越界关系由 static_assert 守着。
#pragma once

#include <cstdint>

namespace debugshape_export {
namespace core {
namespace actorIds {

// 半开区间 [first, last)
struct Range {
    std::uint64_t first;
    std::uint64_t last;

    constexpr bool contains(std::uint64_t value) const noexcept { return value >= first && value < last; }
    constexpr std::uint64_t size() const noexcept { return last - first; }
};

// 三个域各领 0x01000000（16,777,216）个 runtimeId —— 任何现实场景都用不完，
// 但**不要**把段缩小: 分配器只检查起点、不检查上界（见文件末尾的说明）。
inline constexpr Range itemDisplay{0x6D000000ULL, 0x6E000000ULL};
inline constexpr Range customEntity{0x6E000000ULL, 0x6F000000ULL};
inline constexpr Range playerNpc{0x6F000000ULL, 0x70000000ULL};

// 插件层随机 id 段（各域 createRandom 用）: 与自增段（从 1 起）长期隔离，不会撞车
inline constexpr std::int64_t randomIdBase = 0x10000000;
inline constexpr std::int64_t randomIdSpan = 0x70000000;

// ── 不变量 ───────────────────────────────────────────────────────────────────
// 1) 三段首尾相接：漏缝 = 落在缝里的 id 谁都不认；重叠 = 事件派发给错误的域
static_assert(itemDisplay.last == customEntity.first, "itemDisplay 与 customEntity 段必须首尾相接");
static_assert(customEntity.last == playerNpc.first, "customEntity 与 playerNpc 段必须首尾相接");
static_assert(playerNpc.last == 0x70000000ULL, "playerNpc 段的终点决定整块号段的上界");

// 2) 每段大小一致，且都是 0x01000000 的整数倍位置（现在正好就是）
static_assert(itemDisplay.size() == customEntity.size(), "三段大小必须一致，否则 0x6D/0x6E/0x6F 前缀不再对应");
static_assert(customEntity.size() == playerNpc.size(), "三段大小必须一致");

// 3) 整块号段落在 [0x60000000, 0x70000000)，避开真实实体的 runtimeId
static_assert(itemDisplay.first >= 0x60000000ULL, "虚拟号段必须避开真实实体的 runtimeId 区");
static_assert(playerNpc.last <= 0x70000000ULL, "虚拟号段上界不得越过 0x70000000");

constexpr bool anyContains(std::uint64_t value) noexcept {
    return itemDisplay.contains(value) || customEntity.contains(value) || playerNpc.contains(value);
}

// 号段名（诊断/日志用；返回 nullptr 表示不在任何虚拟段内）
constexpr char const* domainOf(std::uint64_t value) noexcept {
    if (itemDisplay.contains(value)) return "itemDisplay";
    if (customEntity.contains(value)) return "customEntity";
    if (playerNpc.contains(value)) return "playerNpc";
    return nullptr;
}

} // namespace actorIds
} // namespace core
} // namespace debugshape_export
