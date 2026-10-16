// ViewPackets.h - 视图覆盖要"自己补发"的几条包（走库已有的 sculk 发送原语）
//
// 出生包（sendActorSpawn）的配方**逐字段对齐 customentity 域** —— 那套是线上验证过能正常渲染的:
//   · 必带 flags 元数据项（Reserved0）: 少它会出问题
//   · 必带 minecraft:health / minecraft:scale 属性, 且**后面紧跟一条 UpdateAttributesPacket**
//     （客户端对出生包里的属性只初始化不生效 —— 这是 customentity 域踩过的 bugfix）
//   · 补发完出生包还要补一条装备/属性的二次确认由调用方按需做（这里做属性那一份）
//   · **NametagAlwaysShow 的协议值与直觉相反**: 0 = 名字牌常显, 1 = 仅准星对准才显示
//     （2026-08-27 用户实测复验, 见 CustomEntityManager.cpp 同处注释）
#pragma once

#include "SculkPacketSend.h"

#include "DiagLog.h"

#include "ViewOverrideLogic.h"

#include "hologramlib/HologramLib.h"
#include "playernpc/NpcProtocol.h"     // playerListEntry / sendToPlayer（原版皮肤更新同款）
#include "playernpc/NpcSkinRegistry.h" // 采集/注册表（finalizeSkin 已处理 2168 的 Id/FullId）

#include <atomic>
#include <cstdint>
#include <string>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/ActorDataIDs.h>
#include <mc/world/actor/ActorFlags.h>
#include <mc/world/actor/DataItem.h>
#include <mc/world/actor/player/Player.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/actor/attribute/Attribute.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateAttributesPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/UpdateBlockPacket.hpp>


namespace debugshape_export::view {
// TypedStorage 取值（小尺寸/对齐一致时会退化成透明别名, 两种形态统一处理）
template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return (value.get());
    } else {
        return (value);
    }
}
template <typename T>
constexpr decltype(auto) unwrap(T& value) {
    if constexpr (requires { value.get(); }) {
        return (value.get());
    } else {
        return (value);
    }
}



// 名字牌常显: 用户语义 → 协议值（反的）
inline constexpr std::uint8_t nametagAlwaysShowWire(bool alwaysShow) {
    return static_cast<std::uint8_t>(alwaysShow ? 0 : 1);
}

// 隐藏: 移除该玩家客户端上的这只实体（按 uniqueId, 客户端认这个）
inline void sendRemoveActor(::Player& player, std::int64_t uniqueId) {
    sculk::protocol::RemoveActorPacket packet;
    packet.mActorUniqueId = uniqueId;
    sendSculkPacketToPlayer(player, packet);
}

// ── 当前服务器 tick ───────────────────────────────────────────────────────────
// `SetActorDataPacket` 与 `UpdateAttributesPacket` **都带 mTick 字段**（26.40 线格式的最后一项）。
// 实测判据（2026-10-16）: 视图覆盖这条路上这两个包以前都没填 tick（= 0），客户端把"元数据更新/属性更新"
// 当成过期包丢掉 —— 表现就是: 出生包里明明带着 `id2/id43/id3/id82`（抓包可见、值也对），画面却
// 还是默认外观; 河豚也一直是不鼓的模样（属性二次确认同样被丢）。customentity 域一直带着 tick 发
// （见 CustomEntityManager 的 `sadp.mTick = tick`），所以那边的 setVariant / setColorIndex 一直是好的。
// tick 由 ViewOverrideManager::tickPulse 每拍同步一次。
inline std::uint64_t& viewTickRef() {
    static std::uint64_t tick = 0;
    return tick;
}

[[nodiscard]] inline std::uint64_t currentViewTick() { return viewTickRef(); }

// 元数据项: 把 MetaPlan 里"设了的那几项"逐条压进一个 MetaData。
// 只推设了的项 —— 一个字段一项, 别的元数据一项不碰（名字牌两项 + 姿态 / 缩放 / 变种）。
// 缩放那条会**连带**推客户端碰撞箱（53/54）: 只放大模型不给碰撞箱的话, 客户端上点不到脚、
// 站立位置也不对（元数据 53/54 就是客户端碰撞箱的宽/高）。
template <typename MetaDataT>
inline void pushMetaItems(MetaDataT& meta, logic::MetaPlan const& plan) {
    using ids = sculk::protocol::ActorDataIDs;   // 枚举类, 只能起类型别名（namespace 别名在这儿是错的）
    if (plan.setName) {
        meta.mDataItems.push_back(sculk::protocol::MetaData::DataItem{ids::Name, plan.name});
    }
    if (plan.setAlwaysShow) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::NametagAlwaysShow, nametagAlwaysShowWire(plan.alwaysShow)}
        );
    }
    if (plan.setPose) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::PoseIndex, static_cast<std::int64_t>(logic::clampPose(plan.pose))}
        );
    }
    if (plan.setScale) {
        // 越界值一律整项不发（发出去客户端行为未定义）—— 与 ICustomEntity::setScale 的硬限同口径
        if (logic::scaleAcceptable(plan.scale)) {
            meta.mDataItems.push_back(sculk::protocol::MetaData::DataItem{ids::Reserved38, plan.scale});
            meta.mDataItems.push_back(
                sculk::protocol::MetaData::DataItem{ids::Reserved53, logic::hitboxWidthOf(plan.scale)}
            );
            meta.mDataItems.push_back(
                sculk::protocol::MetaData::DataItem{ids::Reserved54, logic::hitboxHeightOf(plan.scale)}
            );
        }
    }
    if (plan.setVariant) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::Variant, static_cast<std::int32_t>(plan.variant)}
        );
    }
    if (plan.setMarkVariant) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::MarkVariant, static_cast<std::int32_t>(plan.markVariant)}
        );
    }
    // 颜色两项: 协议类型是 **Byte**（引擎自己的真实包: 羊 `id3(type0)=12`、`id82(type0)=0`）
    if (plan.setColorIndex && logic::colorAcceptable(plan.colorIndex)) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::ColorIndex, static_cast<std::uint8_t>(plan.colorIndex)}
        );
    }
    if (plan.setColor2 && logic::colorAcceptable(plan.color2)) {
        meta.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{ids::Color2Index, static_cast<std::uint8_t>(plan.color2)}
        );
    }
}

// 把一条 EntityView 折成"要往出生包 / SetActorData 里塞哪几项"的计划
// （姿态 / 缩放 / 变种 / 颜色; 1.29.0）。管理器与 LSE 的 metaprobe 都走这一份。
inline logic::MetaPlan planOf(hologramlib::EntityView const& view) {
    auto plan = logic::metaPlanOf(
        view.hasNametag,
        view.nametag,
        view.nametagAlwaysShow,
        view.hasPose,
        view.pose,
        view.hasMetaScale,
        view.metaScale,
        view.hasVariant,
        view.variant,
        view.hasMarkVariant,
        view.markVariant
    );
    return logic::withColors(plan, view.hasColorIndex, view.colorIndex, view.hasColor2, view.color2);
}

// 元数据探针（无客户端也能验）: 把一条 EntityView 折成的元数据**真序列化一遍**再十六进制回读。
// 用途: 验"变种/颜色到底有没有按引擎那套 id+type 发出去" —— 不依赖任何在线玩家。
inline std::string describeMetaItems(hologramlib::EntityView const& view) {
    auto plan = planOf(view);
    if (plan.empty()) return "items=0 plan=empty";

    sculk::protocol::MetaData      meta;
    pushMetaItems(meta, plan);
    std::vector<std::byte>         buffer;
    sculk::protocol::BinaryStream  stream{buffer};
    meta.write(stream);

    std::string hex;
    for (std::size_t i = 0; i < stream.size(); ++i) {
        hex += std::format("{:02x}", static_cast<unsigned>(stream.data()[i]));
    }
    return std::format("items={} bytes={} hex={}", meta.mDataItems.size(), stream.size(), hex);
}

// 名字牌 + 姿态等: 只发要覆盖的那几项, 别的元数据一项不碰。
// （1.25.0 起这个方法叫 sendNametag、只发 Name/NametagAlwaysShow; 1.29.0 起带上
//  姿态 / 缩放 / 变种 / 颜色 —— 名字牌仍是其中一项, 语义一字未改。）
inline void sendViewMeta(::Player& player, std::uint64_t runtimeId, logic::MetaPlan const& plan) {
    if (plan.empty()) return;
    sculk::protocol::SetActorDataPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mTick           = currentViewTick();   // ⚠ 不填这一项客户端当过期包丢掉（见上面的说明）
    pushMetaItems(packet.mMetaData, plan);
    if (packet.mMetaData.mDataItems.empty()) return; // 全是越界值被丢掉时不要白发一条空包
    sendSculkPacketToPlayer(player, packet);
}

// 属性项（与 CustomEntityManager 的 makeAttribute 同形）
inline sculk::protocol::Attribute makeAttribute(
    std::string   name,
    float         minV,
    float         maxV,
    float         curV,
    float         defV,
    std::uint64_t tick
) {
    sculk::protocol::Attribute a;
    a.mName            = std::move(name);
    a.mMinValue        = minV;
    a.mMaxValue        = maxV;
    a.mCurrentValue    = curV;
    a.mDefaultMinValue = minV;
    a.mDefaultMaxValue = maxV;
    a.mDefaultValue    = defV;
    a.mTick            = tick;
    return a;
}

// flags 位掩码: 取视觉上会看出来的那几位（隐形 / 着火 / 潜行 / 疾跑 / 可显示名字 / 名字常显）
inline std::int64_t visualFlagsOf(::Actor& actor) {
    std::int64_t flags = 0;
    auto const   set   = [&](::ActorFlags flag, int bit) {
        if (actor.getStatusFlag(flag)) flags |= (std::int64_t(1) << bit);
    };
    set(::ActorFlags::Onfire, 0);
    set(::ActorFlags::Sneaking, 1);
    set(::ActorFlags::Sprinting, 3);
    set(::ActorFlags::Invisible, 5);
    set(::ActorFlags::CanShowName, 14);
    set(::ActorFlags::AlwaysShowName, 15);
    return flags;
}

// 重发一只实体的出生包（换类型 / 撤销隐藏 / 撤销换类型用）: 同一个 runtimeId + uniqueId。
// 客户端把它当成"这只实体重新出现"; 服务端后续的移动/元数据更新用的还是同一个 runtimeId,
// 所以会照常作用到这只实体上 —— 是"换皮", 不是分身。
//
// 为什么要走这一步: 换类型只能体现在出生包里, 而实体往往**早就出生过了**（拦截那次出生
// 已经来不及）—— 那就自己重做一次出生。放在这里用库自己的发送原语, 与"出生包走哪条路"无关。
inline void sendActorSpawn(
    ::Player&               player,
    ::Actor&                actor,
    std::string const&      identifier,
    logic::MetaPlan const&  plan
) {
    auto const runtimeId = static_cast<std::uint64_t>(actor.getRuntimeID());

    sculk::protocol::AddActorPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mActorUniqueId  = actor.getOrCreateUniqueID().rawID;
    packet.mIdentifier     = identifier;

    // 非玩家实体的位置就是脚位（只有玩家在协议层上报眼位）—— 这里直接用 getPosition()。
    auto const& pos = actor.getPosition();
    packet.mPosition.mX = pos.x;
    packet.mPosition.mY = pos.y;
    packet.mPosition.mZ = pos.z;

    auto const velocity = actor.getVelocity();
    packet.mVelocity.mX = velocity.x;
    packet.mVelocity.mY = velocity.y;
    packet.mVelocity.mZ = velocity.z;

    auto const& rot     = actor.getRotation(); // Vec2{pitch, yaw}, 与 BDS 的 ActorRotation 同序
    packet.mRotation.mX = rot.x;
    packet.mRotation.mY = rot.y;
    packet.mYHeadRotation = actor.getYHeadRot();
    packet.mYBodyRotation = actor.getYHeadRot(); // 躯干朝向没有单独取值口, 用头部朝向近似
    packet.mActorLinks    = {};

    // flags: 少这一项客户端会出问题（customentity 域同款配方）
    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, visualFlagsOf(actor)}
    );
    pushMetaItems(packet.mMetaData, plan); // 名字牌 / 姿态 / 缩放 / 变种（设了哪项发哪项）

    // 属性: health 与 scale 必须都在（客户端的属性表按名找）
    float const health = static_cast<float>(actor.getMaxHealth());
    packet.mAttributes = {
        {"minecraft:health", 0.0f, health, health},
        {"minecraft:scale",  0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(player, packet);

    // 属性二次确认: 客户端对出生包里的属性只初始化不生效（customentity 域的 bugfix 同款）
    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = runtimeId;
    attributes.mTick           = currentViewTick();   // ⚠ 同上: 不填 = 客户端当过期包丢掉
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, health, health, health, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(player, attributes);
}

// ── 掉落物（item 实体）换类型 / 隐藏 ─────────────────────────────────────────
// 单开一条的理由:
//   ① 掉落物的出生包是 **AddItemActor(id 15)**, 不是 AddActor —— 换类型必须在它那条路上也拦；
//   ② **ItemActor 的 maxHealth = 0**: 照抄它的属性会发一只 0 血的目标生物出去
//      （customentity 域那套 health 20 + scale 1 才是线上验证过的配方）。
// 语义仍是"同一个 runtimeId / uniqueId 的换皮": 服务端那只 item 实体一点不动。
inline void sendItemActorSpawn(
    ::Player&          player,
    std::uint64_t      runtimeId,
    std::int64_t       uniqueId,
    std::string const& identifier,
    float              x,
    float              y,
    float              z,
    float              vx,
    float              vy,
    float              vz,
    float                   yaw,
    float                   pitch,
    logic::MetaPlan const&  plan
) {
    // 客户端手上可能已经有这只（物品本体 / 上一版替身）—— 先按 uniqueId 清掉再补
    sendRemoveActor(player, uniqueId);

    sculk::protocol::AddActorPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mActorUniqueId  = uniqueId;
    packet.mIdentifier     = identifier;
    packet.mPosition.mX    = x;
    packet.mPosition.mY    = y;
    packet.mPosition.mZ    = z;
    packet.mVelocity.mX    = vx;
    packet.mVelocity.mY    = vy;
    packet.mVelocity.mZ    = vz;
    packet.mRotation.mX    = pitch;
    packet.mRotation.mY    = yaw;
    packet.mYHeadRotation  = yaw;
    packet.mYBodyRotation  = yaw;
    packet.mActorLinks     = {};

    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, std::int64_t(0)}
    );
    pushMetaItems(packet.mMetaData, plan);

    packet.mAttributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale",  0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(player, packet);

    // 属性二次确认（与 sendActorSpawn 同款 bugfix: 出生包里的属性客户端只初始化不生效）
    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = runtimeId;
    attributes.mTick           = currentViewTick();   // ⚠ 同上: 不填 = 客户端当过期包丢掉
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(player, attributes);

    // 元数据二次确认（**同一个坑**）: 出生包里的元数据客户端同样只初始化不生效 —— 变种（热带鱼体型/
    // 花纹/颜色、河豚鼓胀档位）必须出生之后再发一条 SetActorData 才真正落到渲染上。
    // 实机判据（2026-10-16）: 抓包确认出生包里 `id2/id43/id3/id82` 都在、值也对, 但客户端画面
    // 仍是默认外观; customentity 域的 setVariant/setColorIndex 一直是好的, 差别就在它走**后置**
    // SetActorData。这里补一条即时的, 管理器再排一条 ~2 tick 后的（scheduleMetaPush）。
    sendViewMeta(player, runtimeId, plan);
}

// 从 **AddItemActor 的载荷** 直接换类型发（拦到原版出生包时走这条: 位置/速度取自载荷）
template <typename PayloadT>
inline void sendItemSpawnRewritten(
    ::Player&               player,
    PayloadT const&         payload,
    std::string const&      identifier,
    logic::MetaPlan const&  plan
) {
    auto const pos = unwrap(payload.mPos);
    auto const vel = unwrap(payload.mVelocity);
    sendItemActorSpawn(
        player,
        static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID),
        unwrap(payload.mId).rawID,
        identifier,
        pos.x,
        pos.y,
        pos.z,
        vel.x,
        vel.y,
        vel.z,
        0.0f,   // AddItemActor 载荷里没有朝向（掉落物本来就是无朝向的）
        0.0f,
        plan
    );
}

// 从**引擎里的 ItemActor** 换类型发（"设覆盖那一刻它早就出生过"的场合走这条）
inline void sendItemActorAsOther(
    ::Player&               player,
    ::Actor&                actor,
    std::string const&      identifier,
    logic::MetaPlan const&  plan
) {
    auto const pos = actor.getPosition();
    auto const vel = actor.getVelocity();
    auto const rot = actor.getRotation(); // Vec2{pitch, yaw}
    sendItemActorSpawn(
        player,
        static_cast<std::uint64_t>(actor.getRuntimeID()),
        actor.getOrCreateUniqueID().rawID,
        identifier,
        pos.x,
        pos.y,
        pos.z,
        vel.x,
        vel.y,
        vel.z,
        rot.y,
        rot.x,
        plan
    );
}

// 方块: flag = 3（通知邻居 + 立刻重绘; 与虚容器域的客户端侧箱子方块同值）
inline void sendBlockUpdate(::Player& player, int x, int y, int z, std::uint32_t runtimeId) {
    sculk::protocol::UpdateBlockPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mRuntimeId     = runtimeId;
    packet.mFlag          = 3;
    packet.mLayer         = 0;
    sendSculkPacketToPlayer(player, packet);
}

// 逐观看者换皮肤: 把 target 在 viewer 客户端上的皮肤换成 sourcePlayer 的
// （sourcePlayer == target 的名字 = 恢复他自己的皮肤）。
//
// 机制与原版"皮肤更新"完全一致: 用 target **自己的 UUID** 再发一条 PlayerList(Add) 就地更新 ——
// 皮肤数据走 playerNpc 域的在线采集 + 注册表（Id/FullId 等 2168 细节由注册表的 finalizeSkin 补全,
// 这是当年"皮肤不渲染"修出来的那一段）。整条包只发给这名观看者, 别人看到的照旧。
inline bool sendPlayerSkin(::Player& viewer, ::Player const& target, std::string const& sourcePlayer) {
    auto&             npcs    = hologramlib::IHologramLib::getInstance().playerNpcs();
    std::string const skinId  = "view:" + sourcePlayer;
    if (!npcs.captureSkin(skinId, sourcePlayer)) return false; // 源玩家不在线 / 采集失败

    sculk::protocol::SerializedSkin skin;
    if (!NpcSkinRegistry::getInstance().getSkin(skinId, skin)) return false;

    auto const& uuid = target.getUuid();
    sculk::protocol::PlayerListPacket packet;
    packet.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
    packet.mPlayerEntryList = {npc_protocol::playerListEntry(
        sculk::protocol::UUID{uuid.a, uuid.b},
        static_cast<std::int64_t>(target.getOrCreateUniqueID().rawID),
        target.getRealName(),
        skin
    )};
    return npc_protocol::sendToPlayer(viewer, packet, NetworkPeer::Reliability::Reliable);
}

// ── 真身恢复（1.25.0 修复）: 把**真实玩家**重新发回观看者客户端 ──
// 为什么需要: "变生物 / 隐藏"这条路会把真身从客户端删掉（RemoveActor）, 而 BDS 不会为单个观看者
// 重发玩家出生包 —— 撤销后客户端上就空着。旧代码在这里补的是 AddActor(标识符=minecraft:player),
// 客户端不把那种包当玩家模型（玩家实体在客户端上必须由 AddPlayer 成立）, 实测现象就是
// "卸下后人不在了 / 还是那只假的"。
// 身份纪律与换皮肤同源: UUID / uniqueId / 名字全用他本人的 —— 客户端靠 UUID 去 PlayerList 条目
// 取皮肤, 身份对不上会丢皮肤, 严重时出幽灵玩家, 所以这里连列表条目一起补。
// 返回 false = 皮肤没准备好（调用方**不要**动客户端: 先删后补不回来比先不恢复更糟）。
inline bool captureOwnSkin(::Player& target, sculk::protocol::SerializedSkin& out) {
    std::string const skinId = "view:" + target.getRealName();
    if (!hologramlib::IHologramLib::getInstance().playerNpcs().captureSkin(skinId, target.getRealName())) {
        return false;
    }
    return NpcSkinRegistry::getInstance().getSkin(skinId, out);
}

// 输入包里的 PlayMode -> 协议的 GameType（只放行原版的几档, 其它一律按生存, 免得写出越界枚举）
inline sculk::protocol::GameType gameTypeOfPlayMode(int playMode) {
    switch (playMode) {
    case 1:
        return sculk::protocol::GameType::Creative;
    case 2:
        return sculk::protocol::GameType::Adventure;
    case 5:
        return sculk::protocol::GameType::Default;
    case 6:
        return sculk::protocol::GameType::Spectator;
    default:
        return sculk::protocol::GameType::Survival;
    }
}

// ② 玩家身体（AddPlayer）: 位置/朝向取引擎当前值（脚位 —— 与引擎构造出生包同一取值口）;
//    手持物品留空、能力给默认档, 这两样客户端会随引擎后续的 MobEquipment / UpdateAbilities 自己纠正。
inline bool sendRealPlayerBody(
    ::Player&                              viewer,
    ::Player&                              target,
    sculk::protocol::SerializedSkin const& skin,
    int                                    playMode
) {
    auto const        uuid      = target.getUuid();
    auto const        runtimeId = static_cast<std::uint64_t>(target.getRuntimeID());
    auto const        uniqueId  = static_cast<std::int64_t>(target.getOrCreateUniqueID().rawID);
    auto const        pos       = target.getPosition();
    auto const        rot       = target.getRotation(); // Vec2{pitch, yaw}
    auto const        velocity  = target.getVelocity();
    std::string const name      = target.getRealName();
    sculk::protocol::UUID const uuidWire{uuid.a, uuid.b};

    // ① 玩家列表条目（他本人的身份 + 他本人的皮肤）—— 顺带把"借皮肤"那次留下的假条目盖掉
    {
        sculk::protocol::PlayerListPacket list;
        list.mAction          = sculk::protocol::PlayerListPacket::ActionType::Add;
        list.mPlayerEntryList = {npc_protocol::playerListEntry(uuidWire, uniqueId, name, skin)};
        sendSculkPacketToPlayer(viewer, list);
    }

    sculk::protocol::AddPlayerPacket body;
    body.mUuid           = uuidWire;
    body.mName           = name;
    body.mActorRuntimeId = runtimeId;
    body.mPos            = {pos.x, pos.y, pos.z};
    body.mVelocity       = {velocity.x, velocity.y, velocity.z};
    body.mRot            = {rot.x, rot.y};
    body.mYHeadRot       = target.getYHeadRot();
    body.mGameType       = gameTypeOfPlayMode(playMode);
    body.mAbilities.mPlayerRawId      = uniqueId;
    body.mAbilities.mPlayerPermission = 1; // Member
    body.mAbilities.mLayers           = {{0, 0, 0, 0.05f, 0.1f, 0.1f}};
    body.mBuildPlatform               = 1;
    body.mMetaData.mDataItems         = {
        {sculk::protocol::ActorDataIDs::Reserved0, visualFlagsOf(target)},
        {sculk::protocol::ActorDataIDs::Name, name},
        {sculk::protocol::ActorDataIDs::NametagAlwaysShow, nametagAlwaysShowWire(true)},
        {sculk::protocol::ActorDataIDs::Reserved38, 1.0f}, // scale
        {sculk::protocol::ActorDataIDs::Reserved53, 0.6f}, // 玩家默认碰撞箱
        {sculk::protocol::ActorDataIDs::Reserved54, 1.8f},
    };
    // 发送器是 void: 用失败计数自己比一次, 让调用方能判成败（越界枚举 / 序列化失败都会计进去）
    std::uint64_t const before = sculkSendFailureCount().load(std::memory_order_relaxed);
    sendSculkPacketToPlayer(viewer, body);
    return sculkSendFailureCount().load(std::memory_order_relaxed) == before;
}

// 位移: 玩家被"替换成生物"后, 服务端的 MovePlayer 对那名观看者被吃掉了 —— 位置改由这条推。
// 旋转是**打包成字节**的（rotationByte, 与 NpcProtocol 同源）。flag = Teleport（与自定义实体域同值）。
inline void sendMoveActorAbsolute(::Player& viewer, std::uint64_t runtimeId, float x, float y, float z, float pitch, float yaw, float headYaw) {
    sculk::protocol::MoveActorAbsolutePacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mFlags          = sculk::protocol::MoveActorAbsolutePacket::Flags::Teleport;
    packet.mPosition       = sculk::protocol::Vec3{x, y, z};
    packet.mRotationX      = npc_protocol::rotationByte(pitch);
    packet.mRotationY      = npc_protocol::rotationByte(yaw);
    packet.mRotationYHead  = npc_protocol::rotationByte(headYaw);
    sendSculkPacketToPlayer(viewer, packet);
}

// 诊断计数（describeFor 会读）
inline std::atomic<std::uint64_t> mSkinnedSpawnCounter{0};

// ── 换成"我们的模型"（1.27.0）: 非玩家实体 → 玩家模型 + 已注册皮肤, **同一个 runtimeId/uniqueId** ──
// 复用的是 PlayerNpc 域那套**线上验证过能渲染**的包与原语（PlayerList(Add) 携带皮肤 → AddPlayer）,
// 只把 id 换成真身的 id —— 所以服务端后续的位置/状态包仍然作用在同一只实体上, 攻击也仍然结算在真身上。
// 返回 false = 皮肤没注册 / 发包失败（调用方要按 fail-safe 放行原包, 不要静默什么都不发）。
inline bool sendSkinnedSpawn(
    ::Player&          viewer,
    std::uint64_t      runtimeId,
    std::int64_t       uniqueId,
    float              x,
    float              y,
    float              z,
    float              yaw,
    std::string const& skinId,
    bool               hasNametag,
    std::string const& nametag,
    float              scale,
    float              yOffset
) {
    sculk::protocol::SerializedSkin skin;
    if (!NpcSkinRegistry::getInstance().getSkin(skinId, skin)) return false;

    // 皮肤身份**按库的 API 重建**（npc_protocol::finalizeSkinIds）:
    // 不能沿用皮肤来源的 Id/FullId —— 客户端按 Id 查皮肤缓存, 残留了"原玩家/旧 NPC 的缓存键"
    // 时会**不渲染所设皮肤、外观回退默认模型**（26.40.2 修过同款, 库 README 有记录）。
    // 这里按"替换目标实体"生成专属 Id, 每只实体一份, 缓存互不串。
    // 皮肤身份按**库自己的规则**给每只实体一份（npc_protocol::finalizeSkinIds —— PlayerNpc 域同款）:
    // 皮肤条目身份若沿用来源（原玩家 UUID / 旧 NPC 的 hl_npc_*）, 客户端会命中那份缓存 ——
    // 库 README 26.40.2 记过同款现象: "客户端不渲染所设皮肤、外观回退默认模型"（= 丢皮肤）。
    // 这里先清来源身份, 再生成 mskin_<实体uniqueId> 专属 id: 每只实体一份、重复套用幂等、缓存互不串。
    {
        std::string const perEntity = "mskin_" + std::to_string(static_cast<unsigned long long>(uniqueId));
        skin.mId                  = "HoloLibNpcSkin_" + perEntity;
        skin.mFullId              = skin.mId;
    }

    // 自证日志: 皮肤数据到底有多少字节进了包（抓包里模型 0kb 时先看这一行）
    if (skin.mSkinImageBytes.empty() || skin.mGeometryData.empty()) {
        HLIB_LOG_WARN(
            "[视图] 皮肤数据不完整: 皮肤={} 贴图={} 字节 几何={} 字节 资源补丁={} 字节 id='{}' —— 客户端会显示默认模型",
            skinId,
            skin.mSkinImageBytes.size(),
            skin.mGeometryData.size(),
            skin.mResourcePatch.size(),
            skin.mId
        );
    }

    // 名字: 名字牌常显/内容由 EntityView 决定; 没给名字就用一个空格 —— PlayerList 条目名空串
    // 在客户端上是不确定行为, 而这条目 20 tick 后就删掉了（真正的名字由引擎的 SetActorData 推）。
    std::string const name = hasNametag ? nametag : std::string{" "};

    // PlayerList 条目的 UUID 用 npcUuid(uniqueId) —— 与真身一一对应且稳定, 重复套用是幂等的
    // （Tab 清理也用同一个 id, 见 ViewOverrideManager::scheduleTabRemoval）。
    if (!npc_protocol::spawnPlayerList(viewer, uniqueId, static_cast<std::uint64_t>(uniqueId), name, skin)) {
        return false;
    }
    float const s = scale > 0.0f ? std::clamp(scale, 0.0625f, 10.0f) : 1.0f;
    bool const bodyOk = npc_protocol::spawnPlayerBody(
        viewer,
        uniqueId,
        runtimeId,
        static_cast<std::uint64_t>(uniqueId),
        Vec3{x, y + yOffset, z}, // 模型垂直微调（脚位锚点 + 皮肤自己的 yOffset）
        yaw,
        name,
        s
    );
    // 诊断（排查"看不到模型"必看）: 这条日志说明"让客户端把这只实体当玩家的包"已经发出去了。
    // 看不到这条 = 覆盖没命中这只实体的出生包（看 describeFor 的 spawn= 计数）;
    // 有这条但客户端没变 = 客户端侧不接受（皮肤/身份字段问题）。
    HLIB_LOG_INFO(
        "[视图] 换皮包已发: 观看者={} 实体 uniqueId={} runtimeId={} 皮肤={} 缩放={:.2f} 身体包={} "
        "（贴图={} B 几何={} B 皮肤id='{}'）",
        viewer.getRealName(),
        uniqueId,
        runtimeId,
        skinId,
        s,
        bodyOk,
        skin.mSkinImageBytes.size(),
        skin.mGeometryData.size(),
        skin.mId
    );
    ++mSkinnedSpawnCounter;
    return bodyOk;
}

// 出生包（**按输入快照发**: 目标是个玩家 —— 位置/朝向全来自他发来的 PlayerAuthInput,
// 不读服务端实体的任何状态）。y 是包里的眼位, 这里换算成脚位（− 1.62）。
inline void sendActorSpawnFromInput(
    ::Player&               viewer,
    std::uint64_t           runtimeId,
    std::int64_t            uniqueId,
    std::string const&      identifier,
    float                   eyeX,
    float                   eyeY,
    float                   eyeZ,
    float                   pitch,
    float                   yaw,
    float                   headYaw,
    logic::MetaPlan const&  plan
) {
    sculk::protocol::AddActorPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mActorUniqueId  = uniqueId;
    packet.mIdentifier     = identifier;
    packet.mPosition       = sculk::protocol::Vec3{eyeX, eyeY - logic::kPlayerEyeHeight, eyeZ};
    packet.mVelocity       = sculk::protocol::Vec3{0.0f, 0.0f, 0.0f};
    packet.mRotation       = sculk::protocol::Vec2{pitch, yaw}; // {pitch, yaw}
    packet.mYHeadRotation  = headYaw;
    packet.mYBodyRotation  = yaw;
    packet.mActorLinks     = {};

    // flags 位掩码: 与 customentity 域的配方逐项对齐（那套线上验证过）—— 它们发的是 0,
    // 名字牌只靠 Name / NametagAlwaysShow 两项。这里原来只写 CanShowName 一位, 是不一致项。
    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, std::int64_t(0)}
    );
    pushMetaItems(packet.mMetaData, plan);
    packet.mAttributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale",  0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(viewer, packet);

    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = runtimeId;
    attributes.mTick           = currentViewTick();   // ⚠ 同上: 不填 = 客户端当过期包丢掉
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(viewer, attributes);
}

// 从**原出生包的载荷**造一只"换了类型"的实体（纯协议: 内容逐项照抄原包, 只换 identifier）。
// 这样"内容齐全"由原包保证 —— 位置/朝向/元数据/属性/链接全部照搬, 不会漏项。
template <typename PayloadT>
inline void sendActorSpawnRewritten(
    ::Player&               viewer,
    PayloadT const&         payload,
    std::string const&      identifier,
    logic::MetaPlan const&  plan
) {
    sculk::protocol::AddActorPacket packet;
    packet.mActorRuntimeId = static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID);
    packet.mActorUniqueId  = unwrap(payload.mEntityId).rawID;
    packet.mIdentifier     = identifier;
    packet.mPosition       = sculk::protocol::Vec3{unwrap(payload.mPos).x, unwrap(payload.mPos).y, unwrap(payload.mPos).z};
    packet.mVelocity       = sculk::protocol::Vec3{unwrap(payload.mVelocity).x, unwrap(payload.mVelocity).y, unwrap(payload.mVelocity).z};
    packet.mRotation       = sculk::protocol::Vec2{unwrap(payload.mRot).x, unwrap(payload.mRot).y};
    packet.mYHeadRotation  = unwrap(payload.mYHeadRotation);
    packet.mYBodyRotation  = unwrap(payload.mYBodyRotation);

    // 元数据: 按库内验证过的配方（flags=0 + 计划里的那几项）—— 逐条搬原包的条目没有通用取值口
    // （DataItem 的值是变体, 需要按类型分派）, 而这几项正是客户端渲染需要的。
    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, std::int64_t(0)}
    );
    pushMetaItems(packet.mMetaData, plan);

    // 属性: 不在出生包里逐条搬（那是引擎侧的类型），改用标准 health/scale + 紧跟的
    // UpdateAttributesPacket 二次确认 —— 与库内其它域的配方一致。
    packet.mAttributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale",  0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(viewer, packet);

    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = packet.mActorRuntimeId;
    attributes.mTick           = currentViewTick();   // ⚠ 同上: 不填 = 客户端当过期包丢掉
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(viewer, attributes);
}


} // namespace debugshape_export::view
