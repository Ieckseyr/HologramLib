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
// 掉落物的出生包（**不是** AddActor）: 引擎对 item 实体走 AddItemActor。
// 视图覆盖漏了它时, "把掉落物换成别的实体"永远只能做一半 —— 设覆盖那一刻能把已经在看的玩家换掉,
// 但引擎自己随后发的这条出生包会把物品图标又塞回客户端（2026-10-16 实机: RemoveActor+AddActor(鱼)
// 之后 13ms 就来一条同 id 的 AddItemActor, 客户端最终看到的还是物品图标）。
inline constexpr int kAddItemActor      = 15;
inline constexpr int kRemoveActor       = 14;
inline constexpr int kUpdateBlock       = 21;
inline constexpr int kSetActorData      = 39;
inline constexpr int kMovePlayer       = 19; // 玩家专属位移包（被替换成生物的玩家, 这条要吃掉）
inline constexpr int kMoveAbsoluteActor = 18; // 生物位移/朝向（换成我们的模型后这些包对该观看者吃掉, 改由库推）
inline constexpr int kMoveDeltaActor    = 111; // 生物小位移增量（同上）
inline constexpr int kSetActorMotion    = 40;  // 击退/速度（换成玩家模型后不能让它带走 —— 客户端会按物理把它摔下去）
inline constexpr int kLevelChunk        = 58;
inline constexpr int kUpdateBlockSynced = 110;

// ── 掉落物出生包（AddItemActor, id 15）的处理计划 ──
// 掉落物是 item 实体, 它的出生包走 AddItemActor 而不是 AddActor（1.28.0 之前视图覆盖没接这条,
// 于是"把掉落物换类型/隐藏"只做一半）。这里把"这一包怎么办"抽成纯函数, 管理器与离线 fixture 共用:
//   Pass    原包照发（这条记录只想改名字牌之类）
//   Drop    丢包（对该观看者隐藏这只掉落物）
//   Rewrite 丢原包 + 用同一个 runtimeId/uniqueId 手写一只 AddActor(目标类型)
enum class ItemSpawnPlan { Pass, Drop, Rewrite };

inline constexpr ItemSpawnPlan planItemSpawn(bool hidden, bool hasIdentifier) noexcept {
    if (hidden) return ItemSpawnPlan::Drop;
    if (hasIdentifier) return ItemSpawnPlan::Rewrite;
    return ItemSpawnPlan::Pass;
}

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
// 打算写进 SetActorData / 出生包的那几项。名字牌两项是 1.25.0 起就有的;
// **1.29.0 追加姿态 / 缩放 / 变种**（`EntityView::pose|scale|variant`）——
// 这三项此前只有 `ICustomEntity`（库自己造的实体）能用, 视图覆盖域一个都没有,
// 所以"变成某只生物之后调它的姿势/大小/变种"过去无从落地（Disguise 的实际需求）。
//
// 每项带一个 set 开关: "不改"与"改成 0 / 1 倍"必须分得开（同 hasNametag 的用法）。
// 注意 26.40 的元数据表里**没有** glowing / invisible 旗标, 隐身是效果（MobEffectPacket）——
// 那两项仍然不在本 API 内。
//
// 协议项（与 sculk::protocol::ActorDataIDs 对齐, 管理器里有 static_assert 兜底）:
//   Name 4 / Variant 2 / MarkVariant 43 / Reserved38(scale) 53·54(客户端碰撞箱) / PoseIndex 79
struct MetaPlan {
    bool        setName{false};
    std::string name;
    bool        setAlwaysShow{false};
    bool        alwaysShow{false};
    bool        setPose{false};
    int         pose{0};
    bool        setScale{false};
    float       scale{1.0f};
    bool        setVariant{false};
    int         variant{0};
    bool        setMarkVariant{false};
    int         markVariant{0};
    // 26.40.10 追加: 颜色两项（ColorIndex 3 / Color2Index 82, 协议类型都是 Byte）。
    // 热带鱼这种"主色 + 花纹色"决定外观的实体靠它们; 越界（<0 或 >15）整项忽略。
    bool        setColorIndex{false};
    int         colorIndex{0};
    bool        setColor2{false};
    int         color2{0};

    [[nodiscard]] bool empty() const {
        return !setName && !setAlwaysShow && !setPose && !setScale && !setVariant && !setMarkVariant
            && !setColorIndex && !setColor2;
    }
};

// 名字牌两项（1.25.0 起的原语义: hasNametag = false = 别动名字牌）
inline MetaPlan metaPlanOf(bool hasNametag, std::string_view nametag, bool nametagAlwaysShow) {
    MetaPlan plan;
    if (!hasNametag) return plan;
    plan.setName        = true;
    plan.name           = std::string{nametag};
    plan.setAlwaysShow  = true;
    plan.alwaysShow     = nametagAlwaysShow;
    return plan;
}

// 外观四项一起: 名字牌 + 姿态 + 缩放 + 变种（颜色两项由调用方随后设, 见 withColors）
inline MetaPlan metaPlanOf(
    bool             hasNametag,
    std::string_view nametag,
    bool             nametagAlwaysShow,
    bool             hasPose,
    int              pose,
    bool             hasScale,
    float            scale,
    bool             hasVariant,
    int              variant,
    bool             hasMarkVariant,
    int              markVariant
) {
    auto plan          = metaPlanOf(hasNametag, nametag, nametagAlwaysShow);
    plan.setPose       = hasPose;
    plan.pose          = pose;
    plan.setScale      = hasScale;
    plan.scale         = scale;
    plan.setVariant    = hasVariant;
    plan.variant       = variant;
    plan.setMarkVariant = hasMarkVariant;
    plan.markVariant   = markVariant;
    return plan;
}

// 颜色两项（ColorIndex 3 / Color2Index 82）: 客户端有效域 0..15, 越界整项忽略
inline constexpr int kColorMin = 0;
inline constexpr int kColorMax = 15;

inline constexpr bool colorAcceptable(int color) noexcept {
    return color >= kColorMin && color <= kColorMax;
}

// 往已有计划上补颜色两项（hasXxx = false 就不动）
inline MetaPlan withColors(MetaPlan plan, bool hasColorIndex, int colorIndex, bool hasColor2, int color2) {
    plan.setColorIndex = hasColorIndex;
    plan.colorIndex    = colorIndex;
    plan.setColor2     = hasColor2;
    plan.color2        = color2;
    return plan;
}

// 姿态编号: 与 ICustomEntity::setPose 同口径（PoseIndex 0..13; 0=Standing, 3..13 坐/睡/跳舞等）。
// 越界一律钳进范围 —— 客户端拿到越界值的行为未定义, 钳一下比发出去好。
inline constexpr int kPoseMin = 0;
inline constexpr int kPoseMax = 13;

inline constexpr int clampPose(int pose) noexcept {
    return pose < kPoseMin ? kPoseMin : (pose > kPoseMax ? kPoseMax : pose);
}

// 缩放: 客户端有效域 0.0625~10（与 ICustomEntity::setScale 的硬限一致）; <=0 = 非法。
inline constexpr float kScaleMin = 0.0625f;
inline constexpr float kScaleMax = 10.0f;

// 返回 false = 这个值不能要（调用方按"忽略这一项"处理, 而不是发一个越界值出去）
inline constexpr bool scaleAcceptable(float scale) noexcept {
    return scale >= kScaleMin && scale <= kScaleMax;
}

// 客户端碰撞箱（元数据 53/54）: 26.40 里逐实体换类型后, 碰撞箱要跟着一起给,
// 否则"放大 2 倍"只是模型变大、点不到脚/穿模。基准取 0.6 × 1.8（玩家体型; 与
// ViewPackets.h 里玩家替身的取值一致）, 再乘缩放。
inline constexpr float kBaseHitboxWidth  = 0.6f;
inline constexpr float kBaseHitboxHeight = 1.8f;

inline constexpr float hitboxWidthOf(float scale) noexcept {
    return kBaseHitboxWidth * scale;
}
inline constexpr float hitboxHeightOf(float scale) noexcept {
    return kBaseHitboxHeight * scale;
}

// ── "这一条视图改了什么" ──
// 全空 = 撤销。**每一项都必须算进来**: 早期版本漏了 asPlayer, 结果"伪装成某个玩家"被当成撤销
// （清掉旧覆盖还报失败; 2026-10-09 实测）。往后加字段时先改这里 —— 漏一项的后果是
// "设置成功但被当成撤销, 连旧覆盖一起清掉", 只在实机上才看得出来。
inline constexpr bool viewHasEffect(
    bool hasIdentifier,
    bool hasAsPlayer,
    bool hasSkin,
    bool hasHidden,
    bool hasNametag,
    bool hasPose      = false,
    bool hasScale     = false,
    bool hasVariant   = false,
    bool hasMarkVariant = false
) {
    return hasIdentifier || hasAsPlayer || hasSkin || hasHidden || hasNametag
        || hasPose || hasScale || hasVariant || hasMarkVariant;
}

// 前后空白去掉（皮肤 id / 类型名这类从脚本传来的字符串先洗一遍）
inline std::string trimText(std::string_view raw) {
    // 用十六进制转义: 空白字符别写字面量（复制粘贴/编码转换很容易把字面量弄坏）
    auto const isSpace = [](char c) {
        return c == ' ' || c == '\x09' || c == '\x0D' || c == '\x0A';
    };
    std::size_t begin = 0;
    std::size_t end   = raw.size();
    while (begin < end && isSpace(raw[begin])) ++begin;
    while (end > begin && isSpace(raw[end - 1])) --end;
    return std::string{raw.substr(begin, end - begin)};
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
