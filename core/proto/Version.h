#pragma once

// 协议版本入口：换版本改这里 + 更新 bedrock-protocol schema（见 docs/internal/协议升级流程.md）。
// 业务代码只用不带版本后缀的包名。

#include <bedrock/protocol.hpp>

namespace debugshape_export::core::proto {

namespace bp = ::bedrock::protocol;

inline constexpr int kProtocolVersion = 2168; // protocol-docs r26_u4 = 1.26.40.8

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
using ActorDataIDs  = bp::ActorDataIDs_<kProtocolVersion>;
using DataItemEntry = bp::DataItemEntry_<kProtocolVersion>;
// CopyableDataList 是带 .data 的包装结构；业务里真正要 push 的是它内部的 vector
using ActorDataList = std::vector<DataItemEntry>;

// ── 属性 ─────────────────────────────────────────────────────────────────
using AttributeData          = bp::AttributeData;
using UpdateAttributesPacket = bp::UpdateAttributesPacket;

// ── 玩家外观 / 玩家列表 ──────────────────────────────────────────────────
using GameType             = bp::GameType;
using PlayerListPacketType = bp::PlayerListPacketType;
using BuildPlatform        = bp::BuildPlatform;
using PlayerListAddEntry   = bp::PlayerListPacket_<kProtocolVersion>::AddEntry;
using PlayerListRemoveEntry = bp::PlayerListPacket_<kProtocolVersion>::RemoveEntry;
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
using ActorEvent             = bp::ActorEvent_<kProtocolVersion>;
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
using ItemStackResponseInfo     =
    std::remove_cvref_t<decltype(ItemStackResponsePacket::responses)>::value_type;
using ItemStackNetResult        = bp::ItemStackNetResult;
using SetHudPacket              = bp::SetHudPacket_<kProtocolVersion>;
using HudElement               = bp::HudElement;
using HudVisibility            = bp::HudVisibility;
using SetDisplayObjectivePacket = bp::SetDisplayObjectivePacket;
using ObjectiveSortOrder        = bp::ObjectiveSortOrder;
using RemoveObjectivePacket     = bp::RemoveObjectivePacket;
using SetScorePacket            = bp::SetScorePacket_<kProtocolVersion>;
using SpawnParticleEffectPacket = bp::SpawnParticleEffectPacket;
using MolangVersion             = bp::MolangVersion_<kProtocolVersion>;

// ── 物品栈 ───────────────────────────────────────────────────────────────
using AddPlayerPacket        = bp::AddPlayerPacket_<kProtocolVersion>;
using AddActorPacket         = bp::AddActorPacket_<kProtocolVersion>;
using SetActorDataPacket     = bp::SetActorDataPacket_<kProtocolVersion>;
using PrimitiveShapesPacket = bp::PrimitiveShapesPacket_<kProtocolVersion>;
// 形状载荷：2168 选 v1001 那一版；2193+ 的 TextDataPayload 多一个 line_gap_height
// （schema: `line_gap_height: float = field(since=2193)`），烧死版本会多写 4 个字节。
using PrimitiveShapeDataPayload = bp::PrimitiveShapeDataPayload_<kProtocolVersion>;
using TextDataPayload           = bp::TextDataPayload_<kProtocolVersion>;
using ScriptPrimitiveShapeType =
    std::remove_cvref_t<decltype(PrimitiveShapeDataPayload::shape_type)>::value_type;
using PlayerListPacket      = bp::PlayerListPacket_<kProtocolVersion>;
using ArmSizeType           = std::remove_cvref_t<decltype(PlayerListPacket::AddEntry{}.skin.arm_size)>;
using TrustedSkinFlag       = std::remove_cvref_t<decltype(PlayerListPacket::AddEntry{}.skin.trusted_skin_flag)>;
using SkinColorType         = std::remove_cvref_t<decltype(PlayerListPacket::AddEntry{}.skin.skin_color)>;
using MobEquipmentPacket    = bp::MobEquipmentPacket_<kProtocolVersion>;
using NpcDialoguePacket     = bp::NpcDialoguePacket;
using EmotePacket           = bp::EmotePacket;
using MoveActorAbsolutePacket = bp::MoveActorAbsolutePacket;
using SetActorLinkPacket      = bp::SetActorLinkPacket;
using MobEffectPacket       = bp::MobEffectPacket_<kProtocolVersion>;
using ActorEventPacket      = bp::ActorEventPacket_<kProtocolVersion>;
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

// ── 容器类型常量 ─────────────────────────────────────────────────────────
// 包字段的枚举类型随协议版本换命名空间：2168 是 base::ContainerType，
// 2225 是 v2225::ContainerType；直接把 bp::ContainerType 赋进字段会因类型不同编译失败。
// 这里按「字段自身的类型」做一次显式转换，赋值处只写 proto::ctype::Xxx。
namespace ctype {
inline constexpr ContainerTypeType Container =
    static_cast<ContainerTypeType>(bp::ContainerType::Container);
inline constexpr ContainerTypeType MinecartChest =
    static_cast<ContainerTypeType>(bp::ContainerType::MinecartChest);
inline constexpr TradeContainerTypeType Trade =
    static_cast<TradeContainerTypeType>(bp::ContainerType::Trade);
} // namespace ctype

// ── 版本间字段差异的兼容垫片 ──────────────────────────────────────────────
// play_fab_id 是 26.51 线（2193+）才加进 PlayerListPacket::AddEntry 的字段；
// 2168 的 AddEntry 没有这个成员，直接写会编译失败。探测后赋值，两个版本同一份代码。
template <class Entry>
concept HasPlayFabId = requires (Entry& e) { e.play_fab_id; };

template <class Entry>
inline void clearPlayFabId(Entry& entry) {
    if constexpr (HasPlayFabId<Entry>) {
        entry.play_fab_id = {};
    }
}

// line_gap_height 是 TextDataPayload 在 2193+ 才有的字段（schema: field(since=2193)）；
// 2168 的同一个结构体没有它，写上去会多出 4 个字节、后面所有字段错位。
template <class Payload>
concept HasLineGapHeight = requires (Payload& p) { p.line_gap_height; };

} // namespace debugshape_export::core::proto
