// ViewOverrideLogic.h - 客户端视图覆盖里"不依赖引擎"的那部分判断（可离线测试）
//
// 这里只放纯函数: 坐标打包 / 区块归属 / 幂等判断 / 元数据计划 / 类型名清洗。
// 管理器 (ViewOverrideManager) 与离线 fixture (tests/view_override_fixture.cpp) 共用同一份,
// 保证"离线验过的语义"就是线上跑的语义 —— 与交易结算域同一套路子(TradeSettlementLogic.h)。
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace debugshape_export::view::logic {

// 与 mc/network/MinecraftPacketIds.h 对齐。这边不引 BDS 头是为了能离线编译;
// 管理器里对每个值都做了 static_assert（对不上会编译失败, 不会静默错位）。
inline constexpr int kAddPlayer         = 12;
inline constexpr int kAddActor          = 13;
inline constexpr int kRemoveActor       = 14;
inline constexpr int kUpdateBlock       = 21;
inline constexpr int kSetActorData      = 39;
inline constexpr int kMovePlayer       = 19; // 玩家专属位移包（被替换成生物的玩家, 这条要吃掉）
inline constexpr int kLevelChunk        = 58;
inline constexpr int kUpdateBlockSynced = 110;

// ── 坐标打包 ──
// 与 BDS 的方块坐标哈希同构: x 26 位 / z 26 位 / y 12 位, 各自补码。
// 覆盖表用它当键（一个 int64 顶三个 int, 查表一次比对）。
inline constexpr std::int64_t packPos(int x, int y, int z) {
    return (static_cast<std::int64_t>(x & 0x3FFFFFF) << 38)
         | (static_cast<std::int64_t>(z & 0x3FFFFFF) << 12)
         | static_cast<std::int64_t>(y & 0xFFF);
}

inline constexpr int signExtend(std::int64_t value, int bits) {
    auto const half = static_cast<std::int64_t>(1) << (bits - 1);
    auto const mask = (static_cast<std::int64_t>(1) << bits) - 1;
    auto const v    = value & mask;
    return static_cast<int>(v >= half ? v - (mask + 1) : v);
}

inline constexpr int unpackX(std::int64_t key) { return signExtend(key >> 38, 26); }
inline constexpr int unpackZ(std::int64_t key) { return signExtend((key >> 12) & 0x3FFFFFF, 26); }
inline constexpr int unpackY(std::int64_t key) { return signExtend(key & 0xFFF, 12); }

inline constexpr int chunkOf(int coord) { return coord >> 4; }

// 这个键落在 (cx, cz) 这个区块里吗
inline constexpr bool inChunk(std::int64_t key, int chunkX, int chunkZ) {
    return chunkOf(unpackX(key)) == chunkX && chunkOf(unpackZ(key)) == chunkZ;
}

// ── 幂等 ──
// 同一条包可能经过多条发包路径（sendToClient 内部会再走 sendTo, 广播要逐个收件人套用）,
// 覆盖会被套用多次。这两条判断保证重复套用结果一致, 且不会把"覆盖值"错记成真实值。
inline constexpr bool needsRewrite(std::uint32_t current, std::uint32_t override_) {
    return current != override_;
}
inline constexpr bool shouldRecordReal(std::uint32_t current, std::uint32_t override_) {
    return current != override_;
}

// ── 元数据计划 ──
// 只动两项: Name(4) 与 NametagAlwaysShow(81)。hasNametag = false 表示"不改名字牌"
// （用它区分"清空名字"与"别动"）。26.40 的元数据表里没有 scale / glowing / invisible 旗标,
// 所以这一版 API 也不提供 —— 见 HologramLib.h 的域注释。
struct MetaPlan {
    bool        setName{false};
    std::string name;
    bool        setAlwaysShow{false};
    bool        alwaysShow{false};

    [[nodiscard]] bool empty() const { return !setName && !setAlwaysShow; }
};

inline MetaPlan metaPlanOf(bool hasNametag, std::string_view nametag, bool nametagAlwaysShow) {
    MetaPlan plan;
    if (!hasNametag) return plan;
    plan.setName        = true;
    plan.name           = std::string{nametag};
    plan.setAlwaysShow  = true;
    plan.alwaysShow     = nametagAlwaysShow;
    return plan;
}

// ── 类型名清洗 ──
// 方便调用方: "cow" 这种短名补成 "minecraft:cow"（原版命名空间）; 前后空白去掉;
// 空串 = "不改"（原样返回空）。带命名空间的（"my_pack:thing"）不动。
inline std::string normaliseType(std::string_view raw) {
    std::size_t begin = 0;
    std::size_t end   = raw.size();
    auto const  isSpace = [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };
    while (begin < end && isSpace(raw[begin])) ++begin;
    while (end > begin && isSpace(raw[end - 1])) --end;
    if (begin == end) return {};
    std::string_view trimmed = raw.substr(begin, end - begin);
    if (trimmed.find(':') != std::string_view::npos) return std::string{trimmed};
    return "minecraft:" + std::string{trimmed};
}

// 玩家在协议层上报的 y 在**眼睛**处（Bedrock 特有）: 脚 = y − 1.62（站立实测值）。
// 这是纯协议层的换算, 不去问服务端实体要碰撞箱。
inline constexpr float kPlayerEyeHeight = 1.62f;

// 覆盖记录里"不限维度"的哨兵（玩家不在线 / 全局覆盖时用）
inline constexpr int kAnyDimension = -1;

inline constexpr bool dimensionMatches(int recordDim, int packetDim) {
    return recordDim == kAnyDimension || recordDim == packetDim;
}

} // namespace debugshape_export::view::logic
