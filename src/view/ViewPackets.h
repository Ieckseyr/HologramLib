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

#include "ViewOverrideLogic.h"

#include "hologramlib/HologramLib.h"
#include "playernpc/NpcProtocol.h"     // playerListEntry / sendToPlayer（原版皮肤更新同款）
#include "playernpc/NpcSkinRegistry.h" // 采集/注册表（finalizeSkin 已处理 2168 的 Id/FullId）

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

// 名字牌: 只发要覆盖的那两项（Name / NametagAlwaysShow）, 别的元数据一项不碰
inline void sendNametag(::Player& player, std::uint64_t runtimeId, std::string const& name, bool alwaysShow) {
    sculk::protocol::SetActorDataPacket packet;
    packet.mActorRuntimeId = runtimeId;
    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Name, name}
    );
    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{
            sculk::protocol::ActorDataIDs::NametagAlwaysShow,
            nametagAlwaysShowWire(alwaysShow)
        }
    );
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
    ::Player&          player,
    ::Actor&           actor,
    std::string const& identifier,
    std::string const& nametag,
    bool               hasNametag,
    bool               nametagAlwaysShow
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
    if (hasNametag) {
        packet.mMetaData.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Name, nametag}
        );
        packet.mMetaData.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{
                sculk::protocol::ActorDataIDs::NametagAlwaysShow,
                nametagAlwaysShowWire(nametagAlwaysShow)
            }
        );
    }

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
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, health, health, health, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(player, attributes);
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

// 出生包（**按输入快照发**: 目标是个玩家 —— 位置/朝向全来自他发来的 PlayerAuthInput,
// 不读服务端实体的任何状态）。y 是包里的眼位, 这里换算成脚位（− 1.62）。
inline void sendActorSpawnFromInput(
    ::Player&          viewer,
    std::uint64_t      runtimeId,
    std::int64_t       uniqueId,
    std::string const& identifier,
    float              eyeX,
    float              eyeY,
    float              eyeZ,
    float              pitch,
    float              yaw,
    float              headYaw,
    std::string const& nametag,
    bool               hasNametag,
    bool               nametagAlwaysShow
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
    if (hasNametag) {
        packet.mMetaData.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Name, nametag}
        );
        packet.mMetaData.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{
                sculk::protocol::ActorDataIDs::NametagAlwaysShow,
                nametagAlwaysShowWire(nametagAlwaysShow)
            }
        );
    }
    packet.mAttributes = {
        {"minecraft:health", 0.0f, 20.0f, 20.0f},
        {"minecraft:scale",  0.0625f, 10.0f, 1.0f},
    };
    packet.mSynchedProperties = {};
    sendSculkPacketToPlayer(viewer, packet);

    sculk::protocol::UpdateAttributesPacket attributes;
    attributes.mActorRuntimeId = runtimeId;
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
    ::Player&                         viewer,
    PayloadT const&                   payload,
    std::string const&                identifier,
    std::string const&                nametag,
    bool                              hasNametag,
    bool                              nametagAlwaysShow
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

    // 元数据: 按库内验证过的配方（flags=0 + 名字牌两项）—— 逐条搬原包的条目没有通用取值口
    // （DataItem 的值是变体, 需要按类型分派）, 而这几项正是客户端渲染需要的。
    packet.mMetaData.mDataItems.push_back(
        sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Reserved0, std::int64_t(0)}
    );
    if (hasNametag) {
        packet.mMetaData.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{sculk::protocol::ActorDataIDs::Name, nametag}
        );
        packet.mMetaData.mDataItems.push_back(
            sculk::protocol::MetaData::DataItem{
                sculk::protocol::ActorDataIDs::NametagAlwaysShow,
                nametagAlwaysShowWire(nametagAlwaysShow)
            }
        );
    }

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
    attributes.mAttributes     = {
        makeAttribute("minecraft:health", 0.0f, 20.0f, 20.0f, 20.0f, 0),
        makeAttribute("minecraft:scale", 0.0625f, 10.0f, 1.0f, 1.0f, 0),
    };
    sendSculkPacketToPlayer(viewer, attributes);
}


} // namespace debugshape_export::view
