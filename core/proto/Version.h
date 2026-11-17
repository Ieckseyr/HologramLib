#pragma once

// 协议版本入口：换版本改这里 + 更新 bedrock-protocol schema（见 docs/internal/协议升级流程.md）。
// 业务代码只用不带版本后缀的包名。

#include <bedrock/protocol.hpp>

namespace debugshape_export::core::proto {

namespace bp = ::bedrock::protocol;

inline constexpr int kProtocolVersion = 898; // protocol-docs r21_u13 = 1.21.132.3

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
using DataItemBytePayload   = bp::DataItemBytePayload_<kProtocolVersion>;
using DataItemShortPayload  = bp::DataItemShortPayload_<kProtocolVersion>;
using DataItemIntPayload    = bp::DataItemIntPayload_<kProtocolVersion>;
using DataItemInt64Payload  = bp::DataItemInt64Payload_<kProtocolVersion>;
using DataItemFloatPayload  = bp::DataItemFloatPayload_<kProtocolVersion>;
using DataItemStringPayload = bp::DataItemStringPayload_<kProtocolVersion>;
using DataItemVec3Payload   = bp::DataItemVec3Payload_<kProtocolVersion>;
using DataItemCompoundTagPayload = bp::DataItemCompoundTagPayload_<kProtocolVersion>;
using DataItemPosPayload    = bp::DataItemPosPayload_<kProtocolVersion>;
// CopyableDataList 是带 .data 的包装结构；DataItemEntry / ActorDataList 在
// SetActorDataPacket 之后定义（类型要从包字段反推，早写会用到未声明的包类型）。

// ── 属性 ─────────────────────────────────────────────────────────────────
using AttributeData          = bp::AttributeData;
using UpdateAttributesPacket = bp::UpdateAttributesPacket;

// ── 玩家外观 / 玩家列表 ──────────────────────────────────────────────────
using GameType             = bp::GameType;
using PlayerListPacketType = bp::PlayerListPacketType;
using BuildPlatform        = bp::BuildPlatform;
// 898 的 PlayerList / PlayerSkin 皮肤结构不同，别名分开。
using PlayerListPacket       = bp::PlayerListPacket_<kProtocolVersion>;
using PlayerListEntry        = std::remove_cvref_t<decltype(PlayerListPacket{}.entries)>::value_type;
using PlayerListAddEntry     = PlayerListEntry;
using PlayerListRemoveEntry  = PlayerListPacket::RemoveEntry;
using PlayerSkinPacket       = bp::PlayerSkinPacket_<kProtocolVersion>;
using SerializedSkinRef      = std::remove_cvref_t<decltype(PlayerListEntry{}.skin)>;
using PlayerSkinRef          = std::remove_cvref_t<decltype(PlayerSkinPacket{}.skin)>;
using PieceType               = bp::PieceType_<kProtocolVersion>;
using SerializedPersonaPiece  = bp::SerializedPersonaPieceHandle_<kProtocolVersion>;
using SkinImage               = bp::SkinImage;
using AnimatedImageData       = std::remove_cvref_t<decltype(SerializedSkinRef{}.animated_image_data)>::value_type;
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
using ItemStackResponseInfo =
    std::remove_cvref_t<decltype(ItemStackResponsePacket{}.responses)>::value_type;
using ItemStackNetResult        = bp::ItemStackNetResult;
using SetHudPacket              = bp::SetHudPacket_<kProtocolVersion>;
using HudElement               = bp::HudElement;
using HudVisibility            = bp::HudVisibility;
using SetDisplayObjectivePacket = bp::SetDisplayObjectivePacket;
using ObjectiveSortOrder        = bp::ObjectiveSortOrder;
using RemoveObjectivePacket     = bp::RemoveObjectivePacket;
using SetScorePacket            = bp::SetScorePacket_<kProtocolVersion>;
using ScorePacketTypeEnum       = std::remove_cvref_t<decltype(SetScorePacket{}.type)>;
using ScorePacketInfo           = std::remove_cvref_t<decltype(SetScorePacket{}.score_info)>::value_type;
using ScoreIdentityType         = std::remove_cvref_t<decltype(ScorePacketInfo{}.identity_type)>;
using SpawnParticleEffectPacket = bp::SpawnParticleEffectPacket;
using MolangVersion             = bp::MolangVersion;

// ── 物品栈 ───────────────────────────────────────────────────────────────
using AddPlayerPacket        = bp::AddPlayerPacket_<kProtocolVersion>;
using AddActorPacket         = bp::AddActorPacket_<kProtocolVersion>;
using SetActorDataPacket     = bp::SetActorDataPacket_<kProtocolVersion>;
// packed_items 是 CopyableDataList 包装（内含 .data 向量），业务要 push 的是里面的 vector
using ActorDataList          = std::remove_cvref_t<decltype(SetActorDataPacket{}.packed_items)>;
using DataItemEntry          = std::remove_cvref_t<decltype(ActorDataList{}.data)>::value_type;
using PrimitiveShapesPacket = bp::PrimitiveShapesPacket;
using ArmSizeType           = std::remove_cvref_t<decltype(PlayerListAddEntry{}.skin.arm_size)>;
using SkinColorType         = std::remove_cvref_t<decltype(PlayerListAddEntry{}.skin.skin_color)>;
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

// ── 跨版本垫片：字段存在性差异（898 ↔ 2225）────────────────────────────
// 统一用 requires 判别，字段不存在就是空操作。

// 皮肤信任标记：2225 有 trusted_skin_flag / profile_hash，898 没有。
template <class Skin>
inline constexpr bool skinHasTrustedFlag = requires(Skin& s) { s.trusted_skin_flag; };
template <class Skin>
inline constexpr bool skinHasProfileHash = requires(Skin& s) { s.profile_hash; };

template <class Skin>
inline void setSkinTrusted(Skin& s, TrustedSkinFlag v) {
    if constexpr (skinHasTrustedFlag<Skin>) {
        s.trusted_skin_flag = static_cast<std::remove_cvref_t<decltype(s.trusted_skin_flag)>>(v);
    } else {
        (void)s;
        (void)v;
    }
}

template <class Skin>
inline void setSkinProfileHash(Skin& s, std::string v) {
    if constexpr (skinHasProfileHash<Skin>) {
        s.profile_hash = std::move(v);
    } else {
        (void)s;
        (void)v;
    }
}

// 位图：2225 是 image_data{width,height,image_bytes}，898 是三个平铺字段，cape 同理。
template <class Skin>
inline constexpr bool skinHasImageData = requires(Skin& s) { s.image_data; };
template <class Skin>
inline constexpr bool skinHasCapeImageData = requires(Skin& s) { s.cape_image_data; };

template <class Skin>
inline void setSkinImage(Skin& s, std::uint32_t w, std::uint32_t h, std::string bytes) {
    if constexpr (skinHasImageData<Skin>) {
        s.image_data.width       = w;
        s.image_data.height      = h;
        s.image_data.image_bytes = std::move(bytes);
    } else {
        s.image_width  = w;
        s.image_height = h;
        s.image_bytes  = std::move(bytes);
    }
}

template <class Skin>
inline void getSkinImage(Skin const& s, std::uint32_t& w, std::uint32_t& h, std::string& bytes) {
    if constexpr (skinHasImageData<Skin>) {
        w     = s.image_data.width;
        h     = s.image_data.height;
        bytes = s.image_data.image_bytes;
    } else {
        w     = s.image_width;
        h     = s.image_height;
        bytes = s.image_bytes;
    }
}

template <class Skin>
inline void setSkinCapeImage(Skin& s, std::uint32_t w, std::uint32_t h, std::string bytes) {
    if constexpr (skinHasCapeImageData<Skin>) {
        s.cape_image_data.width       = w;
        s.cape_image_data.height      = h;
        s.cape_image_data.image_bytes = std::move(bytes);
    } else {
        s.cape_image_width  = w;
        s.cape_image_height = h;
        s.cape_image_bytes  = std::move(bytes);
    }
}

template <class Skin>
inline void getSkinCapeImage(Skin const& s, std::uint32_t& w, std::uint32_t& h, std::string& bytes) {
    if constexpr (skinHasCapeImageData<Skin>) {
        w     = s.cape_image_data.width;
        h     = s.cape_image_data.height;
        bytes = s.cape_image_data.image_bytes;
    } else {
        w     = s.cape_image_width;
        h     = s.cape_image_height;
        bytes = s.cape_image_bytes;
    }
}

// 皮肤颜色：2225 是 int32 Color，898 是 hex 字符串（留空=默认色）。
template <class Skin>
inline void setSkinColor(Skin& s, std::uint32_t v) {
    using FieldT = std::remove_cvref_t<decltype(s.skin_color)>;
    if constexpr (std::is_integral_v<FieldT>) {
        s.skin_color = static_cast<FieldT>(v);
    } else {
        (void)s;
        (void)v;
    }
}

template <class Skin>
inline std::uint32_t getSkinColor(Skin const& s) {
    using FieldT = std::remove_cvref_t<decltype(s.skin_color)>;
    if constexpr (std::is_integral_v<FieldT>) {
        return static_cast<std::uint32_t>(s.skin_color);
    } else {
        return 0;
    }
}

// 方块坐标：898 用 NetworkBlockPosition（与 BlockPos 同构、不同类型）。
inline NetworkBlockPosition toNetworkPos(BlockPos const& p) {
    return NetworkBlockPosition{p.x, p.y, p.z};
}


// 皮肤信任标记读取
template <class Skin>
inline TrustedSkinFlag getSkinTrusted(Skin const& s) {
    if constexpr (skinHasTrustedFlag<Skin>) {
        return static_cast<TrustedSkinFlag>(static_cast<std::int8_t>(s.trusted_skin_flag));
    } else {
        return TrustedSkinFlag::True;
    }
}

// persona / tint 只在两边容器类型一致时搬，避免为单条版本线写死结构转换。
template <class Dst, class Src>
inline void copySkin(Dst& dst, Src const& src) {
    dst.id                                = src.id;
    dst.resource_patch                    = src.resource_patch;
    dst.animated_image_data               = src.animated_image_data;
    dst.geometry_data                     = src.geometry_data;
    dst.geometry_data_min_engine_version  = src.geometry_data_min_engine_version;
    dst.animation_data                    = src.animation_data;
    dst.cape_id                           = src.cape_id;
    dst.full_id                           = src.full_id;
    dst.arm_size                          = src.arm_size;
    dst.is_premium                        = src.is_premium;
    dst.is_persona                        = src.is_persona;
    dst.is_persona_cape_on_classic_skin   = src.is_persona_cape_on_classic_skin;
    dst.is_primary_user                   = src.is_primary_user;
    dst.overrides_player_appearance       = src.overrides_player_appearance;

    std::uint32_t w = 0;
    std::uint32_t h = 0;
    std::string   bytes;

    getSkinImage(src, w, h, bytes);
    setSkinImage(dst, w, h, bytes);
    getSkinCapeImage(src, w, h, bytes);
    setSkinCapeImage(dst, w, h, bytes);
    setSkinColor(dst, getSkinColor(src));

    if constexpr (skinHasTrustedFlag<Dst>) {
        setSkinTrusted(dst, getSkinTrusted(src));
    }
    if constexpr (skinHasProfileHash<Dst>) {
        if constexpr (skinHasProfileHash<Src>) {
            setSkinProfileHash(dst, src.profile_hash);
        }
    }
    if constexpr (requires { dst.play_fab_id; }) {
        if constexpr (requires { src.play_fab_id; }) {
            dst.play_fab_id = src.play_fab_id;
        }
    }
    using DstPersona = std::remove_cvref_t<decltype(dst.persona_pieces)>;
    using SrcPersona = std::remove_cvref_t<decltype(src.persona_pieces)>;
    if constexpr (std::is_same_v<DstPersona, SrcPersona>) {
        dst.persona_pieces = src.persona_pieces;
    }
    using DstTint = std::remove_cvref_t<decltype(dst.piece_tint_colors)>;
    using SrcTint = std::remove_cvref_t<decltype(src.piece_tint_colors)>;
    if constexpr (std::is_same_v<DstTint, SrcTint>) {
        dst.piece_tint_colors = src.piece_tint_colors;
    }
}


// PlayerList 条目上「有没有 action / play_fab_id」的差别，必须放在模板里判断
//（非模板上下文里 requires 遇到不存在的成员是硬错误，不是 false）。
template <class Entry>
inline void setPlayerListAction(Entry& e, PlayerListPacketType action) {
    if constexpr (requires(Entry& x) { x.action; }) {
        e.action = action;
    }
}

template <class Entry>
inline void clearPlayFabId(Entry& e) {
    if constexpr (requires(Entry& x) { x.play_fab_id; }) {
        e.play_fab_id = {};
    }
}

template <class Skin>
inline std::size_t skinImageByteCount(Skin const& s) {
    std::uint32_t w = 0;
    std::uint32_t h = 0;
    std::string   bytes;
    getSkinImage(s, w, h, bytes);
    return bytes.size();
}

// 删除条目：898 走包级 action + removed_entries，2225 走 entries 里的 RemoveEntry。
// 必须写成模板：非模板上下文里 if constexpr 的废弃分支照样会被语义检查。
template <class PacketT>
inline void pushPlayerListRemove(PacketT& packet, UUID const& uuid) {
    if constexpr (requires(PacketT& p) { p.removed_entries; }) {
        packet.action = PlayerListPacketType::Remove;
        packet.removed_entries.push_back(uuid);
    } else {
        typename PacketT::RemoveEntry entry;
        entry.action = PlayerListPacketType::Remove;
        entry.uuid   = uuid;
        packet.entries.push_back(std::move(entry));
    }
}

// 898 的 LL 头里 PlayerListEntry 只有前向声明，读不到条目字段。
#if kProtocolVersion >= 2168
#define HOLO_HAS_PLAYERLIST_ENTRY_FIELDS 1
#else
#define HOLO_HAS_PLAYERLIST_ENTRY_FIELDS 0
#endif

// persona / tint 等丰富皮肤字段：2168 之后才有；非模板上下文只能用预处理器切。
#if kProtocolVersion >= 2168
#define HOLO_PROTOCOL_HAS_RICH_SKIN 1
#else
#define HOLO_PROTOCOL_HAS_RICH_SKIN 0
#endif

// 动画皮肤帧：2225 是 image{SkinImage}，898 是平铺字段。
template <class Anim>
inline void getAnimImage(Anim const& a, std::uint32_t& w, std::uint32_t& h, std::string& bytes) {
    if constexpr (requires(Anim const& x) { x.image; }) {
        w     = a.image.width;
        h     = a.image.height;
        bytes = a.image.image_bytes;
    } else {
        w     = a.image_width;
        h     = a.image_height;
        bytes = a.image_bytes;
    }
}

template <class Anim>
inline void setAnimImage(Anim& a, std::uint32_t w, std::uint32_t h, std::string bytes) {
    if constexpr (requires(Anim& x) { x.image; }) {
        a.image.width       = w;
        a.image.height      = h;
        a.image.image_bytes = std::move(bytes);
    } else {
        a.image_width  = w;
        a.image_height = h;
        a.image_bytes  = std::move(bytes);
    }
}

template <class Anim>
inline std::size_t animImageByteCount(Anim const& a) {
    std::uint32_t w = 0;
    std::uint32_t h = 0;
    std::string   bytes;
    getAnimImage(a, w, h, bytes);
    return bytes.size();
}

} // namespace debugshape_export::core::proto
