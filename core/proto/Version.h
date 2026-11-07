#pragma once

// 协议版本入口：换版本改这里 + 更新 bedrock-protocol schema（见 docs/internal/协议升级流程.md）。
// 业务代码只用不带版本后缀的包名。

#include <bedrock/protocol.hpp>

namespace debugshape_export::core::proto {

namespace bp = ::bedrock::protocol;

inline constexpr int kProtocolVersion = 2225; // protocol-docs r26_u6 = 1.26.61.1

// ── 基础类型 ─────────────────────────────────────────────────────────────
using BlockPos       = bp::BlockPos;
using BlockRuntimeId = bp::BlockRuntimeId;
using ActorUniqueID  = bp::ActorUniqueID;

// ── 方块 ─────────────────────────────────────────────────────────────────
using UpdateBlockPacket              = bp::UpdateBlockPacket_<kProtocolVersion>;
using UpdateSubChunkBlocksPacket      = bp::UpdateSubChunkBlocksPacket_<kProtocolVersion>;
using UpdateSubChunkNetworkBlockInfo = bp::UpdateSubChunkNetworkBlockInfo_<kProtocolVersion>;
using UpdateSubChunkBlocksChangedInfo = bp::UpdateSubChunkBlocksChangedInfo_<kProtocolVersion>;
using ActorBlockSyncMessage          = bp::ActorBlockSyncMessage;

// ── 容器 / 掉落物 ────────────────────────────────────────────────────────
using TakeItemActorPacket = bp::TakeItemActorPacket;
using ContainerOpenPacket = bp::ContainerOpenPacket_<kProtocolVersion>;
using ContainerClosePacket = bp::ContainerClosePacket_<kProtocolVersion>;

// 强类型随版本变化：从包字段反推，换版本不用改这里
using ContainerIDType        = std::remove_cvref_t<decltype(ContainerOpenPacket::container_id)>;
using ContainerTypeType      = std::remove_cvref_t<decltype(ContainerOpenPacket::type)>;
using NetworkBlockPosition   = std::remove_cvref_t<decltype(ContainerOpenPacket::pos)>;

// ── 几何 / 标识 ──────────────────────────────────────────────────────────
using Vec2 = bp::Vec2;
using Vec3 = bp::Vec3;
using UUID = bp::UUID;
using Color = bp::Color;

// ── 元数据（AddActor / SetActorData 的 DataItem 列表）────────────────────
using ActorDataIDs  = bp::ActorDataIDs;
using DataItemEntry = bp::DataItemEntry;
// CopyableDataList 是带 .data 的包装结构；业务里真正要 push 的是它内部的 vector
using ActorDataList = std::vector<bp::DataItemEntry>;

// ── 属性 ─────────────────────────────────────────────────────────────────
using AttributeData          = bp::AttributeData;
using UpdateAttributesPacket = bp::UpdateAttributesPacket;

// ── 玩家外观 / 玩家列表 ──────────────────────────────────────────────────
using GameType             = bp::GameType;
using PlayerListPacketType = bp::PlayerListPacketType;
using BuildPlatform        = bp::BuildPlatform;
using PlayerListAddEntry   = bp::PlayerListPacket::AddEntry;
using PlayerListRemoveEntry = bp::PlayerListPacket::RemoveEntry;
using SerializedSkinRef      = bp::SerializedSkinRef_<kProtocolVersion>;
using PlayerSkinPacket       = bp::PlayerSkinPacket_<kProtocolVersion>;
using PieceType               = bp::PieceType_<kProtocolVersion>;
using SerializedPersonaPiece  = bp::SerializedPersonaPieceHandle_<kProtocolVersion>;
using SkinImage               = bp::SkinImage;
using AnimatedImageData       = bp::AnimatedImageData;
using TintMapColor            = bp::TintMapColor;
using AnimatedTextureType     = bp::AnimatedTextureType;
using AnimationExpression     = bp::AnimationExpression;
using TrustedSkinFlag         = bp::TrustedSkinFlag;

// ── actor 杂项 ───────────────────────────────────────────────────────────
using ActorLink              = bp::ActorLink;
using ActorLinkType          = bp::ActorLinkType;
using ActorEvent             = bp::ActorEvent;
using BossEventPacket        = bp::BossEventPacket_<kProtocolVersion>;
using ChangeMobPropertyPacket = bp::ChangeMobPropertyPacket;
using MoveActorAbsoluteData  = bp::MoveActorAbsoluteData;

// MoveActorAbsoluteData.header 的位定义（线上就是 bit 标志，生成头里只给了 uint8）
namespace move_flags {
inline constexpr std::uint8_t kNone            = 0;
inline constexpr std::uint8_t kOnGround        = 1u << 0;
inline constexpr std::uint8_t kTeleport        = 1u << 1;
inline constexpr std::uint8_t kForceMove       = 1u << 2;
inline constexpr std::uint8_t kForceCompletion = 1u << 3;
} // namespace move_flags

// ── 其它包 ───────────────────────────────────────────────────────────────
using BlockActorDataPacket      = bp::BlockActorDataPacket_<kProtocolVersion>;
using ItemStackResponsePacket   = bp::ItemStackResponsePacket_<kProtocolVersion>;
using ItemStackResponseInfo     = bp::ItemStackResponseInfo;
using ItemStackNetResult        = bp::ItemStackNetResult;
using SetHudPacket              = bp::SetHudPacket_<kProtocolVersion>;
using HudElement               = bp::HudElement;
using HudVisibility            = bp::HudVisibility;
using SetDisplayObjectivePacket = bp::SetDisplayObjectivePacket;
using ObjectiveSortOrder        = bp::ObjectiveSortOrder;
using RemoveObjectivePacket     = bp::RemoveObjectivePacket;
using SetScorePacket            = bp::SetScorePacket_<kProtocolVersion>;
using SpawnParticleEffectPacket = bp::SpawnParticleEffectPacket;
using MolangVersion             = bp::MolangVersion;

// ── 物品栈 ───────────────────────────────────────────────────────────────
using AddPlayerPacket        = bp::AddPlayerPacket;
using AddActorPacket         = bp::AddActorPacket;
using SetActorDataPacket     = bp::SetActorDataPacket_<kProtocolVersion>;
using PrimitiveShapesPacket = bp::PrimitiveShapesPacket;
using PlayerListPacket      = bp::PlayerListPacket;
using ArmSizeType           = std::remove_cvref_t<decltype(bp::PlayerListPacket::AddEntry{}.skin.arm_size)>;
using TrustedSkinFlag       = std::remove_cvref_t<decltype(bp::PlayerListPacket::AddEntry{}.skin.trusted_skin_flag)>;
using SkinColorType         = std::remove_cvref_t<decltype(bp::PlayerListPacket::AddEntry{}.skin.skin_color)>;
using MobEquipmentPacket    = bp::MobEquipmentPacket;
using NpcDialoguePacket     = bp::NpcDialoguePacket;
using EmotePacket           = bp::EmotePacket;
using MoveActorAbsolutePacket = bp::MoveActorAbsolutePacket;
using SetActorLinkPacket      = bp::SetActorLinkPacket;
using MobEffectPacket       = bp::MobEffectPacket;
using ActorEventPacket      = bp::ActorEventPacket;
using AnimateEntityPacket   = bp::AnimateEntityPacket;
using RespawnPacket         = bp::RespawnPacket;
using RemoveActorPacket     = bp::RemoveActorPacket;
using SetActorMotionPacket  = bp::SetActorMotionPacket;
using UpdateTradePacket      = bp::UpdateTradePacket_<kProtocolVersion>;
using TradeContainerTypeType = std::remove_cvref_t<decltype(UpdateTradePacket::type)>;
using AddItemActorPacket = bp::AddItemActorPacket_<kProtocolVersion>;
using InventoryContentPacket = bp::InventoryContentPacket_<kProtocolVersion>;
using InventoryItemDescriptor =
    std::remove_cvref_t<decltype(InventoryContentPacket::slots)>::value_type;

} // namespace debugshape_export::core::proto
