#pragma once

#include "core/DiagFlags.h"
#include "ItemDisplayManager.h"
#include "kernel/EventIds.h"

#include "DiagLog.h"

#include "kernel/PacketSink.h"

#include "core/TickDriver.h"

#include <random>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/deps/nbt/ShortTag.h>
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/enchanting/Enchant.h>
#include <mc/world/item/enchanting/EnchantmentInstance.h>
#include <mc/world/item/enchanting/ItemEnchants.h>
#include <mc/world/item/registry/ItemRegistryRef.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/level/MolangVersion.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/AnimateEntityPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <map>
#include <optional>

namespace debugshape_export {
namespace ide_ops {

struct AnimEntry {
    mce::UUID     playerUuid;
    std::uint64_t runtimeId;
    std::string   animation;
    std::string   controller;
    std::string   stopExpression;
    bool          noAliveCheck{false};

    bool                       equip{false};
    std::optional<::ItemStack> equipStack{};
};

inline std::multimap<std::uint64_t, AnimEntry>& animQueue() {
    static std::multimap<std::uint64_t, AnimEntry> queue;
    return queue;
}

inline std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

inline Player* findPlayerByUuid(mce::UUID const& uuid) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    return level->getPlayer(uuid);
}

inline bool is2DBlockItemName(std::string const& rawName) {
    auto name = std::string_view{rawName};
    if (name.starts_with("minecraft:"))
        name.remove_prefix(10);

    static constexpr std::string_view exactNames[] = {
        "ladder",
        "vine",
        "weeping_vines",
        "twisting_vines",
        "glow_lichen",
        "sculk_vein",
        "lily_pad",
        "tripwire",
        "tripwire_hook",
        "redstone_wire",
        "deadbush",
        "fern",
        "large_fern",
        "short_grass",
        "tall_grass",
        "seagrass",
        "tall_seagrass",
        "kelp",
        "cocoa",
    };
    for (auto exact : exactNames) {
        if (name == exact)
            return true;
    }

    static constexpr std::string_view patterns[] = {
        "flower",
        "sapling",
        "coral_fan",
        "torch",
        "rail",
        "button",
        "lever",
        "pressure_plate",
        "carpet",
        "roots",
        "sprouts",
        "fungus",
        "mushroom",
        "bush",
    };
    return std::ranges::any_of(patterns, [&name](auto pattern) {
        return name.contains(pattern);
    });
}

inline std::unordered_map<std::string, std::string> const& legacyItemNames() {
    static std::unordered_map<std::string, std::string> const map = {
        {"grass", "short_grass"},
        {"grass_path", "dirt_path"},
        {"sign", "oak_sign"},
        {"skull", "player_head"},
    };
    return map;
}

inline std::string normalizeItemName(std::string const& raw) {
    std::string_view shortName = raw;
    if (shortName.starts_with("minecraft:"))
        shortName.remove_prefix(10);
    if (auto it = legacyItemNames().find(std::string(shortName)); it != legacyItemNames().end()) {
        return "minecraft:" + it->second;
    }
    if (raw.find(':') == std::string::npos)
        return "minecraft:" + raw;
    return raw;
}

inline std::optional<::ItemStack> buildItemStack(std::string const& rawName, int aux, std::string const& nbt, bool glint) {
    auto level = ll::service::getLevel();
    if (!level)
        return std::nullopt;

    std::unique_ptr<::CompoundTag> tag;
    if (!nbt.empty()) {
        auto parsed = ::CompoundTag::fromSnbt(nbt);
        if (parsed) {
            tag = std::make_unique<::CompoundTag>(std::move(*parsed));
        } else {
            HLIB_LOG_WARN(
                "[ItemDisplay] itemNbt SNBT parse failed for '{}', ignored. 原文前 80 字节: [{}]",
                rawName,
                nbt.substr(0, 80));
        }
    }

    if (glint) {
        if (!tag)
            tag = std::make_unique<::CompoundTag>();

        ::CompoundTag* comps = nullptr;
        if (auto found = tag->mTags.find("components"); found != tag->mTags.end()) {
            comps = std::get_if<::CompoundTag>(&found->second.mTagStorage);
        }
        if (!comps) {
            (*tag)["components"] = ::CompoundTagVariant{::CompoundTag{}};
            comps                = std::get_if<::CompoundTag>(&(*tag)["components"].mTagStorage);
        }
        if (comps) {
            ::CompoundTag levels;
            levels["minecraft:sharpness"] = ::CompoundTagVariant{::ShortTag{1}};
            ::CompoundTag ench;
            ench["levels"]                     = std::move(levels);
            (*comps)["minecraft:enchantments"] = std::move(ench);
        }
    }

    auto tryGet = [&](std::string const& name) -> std::optional<::ItemStack> {
        auto weak = level->getItemRegistry().getItem(::HashedString(name));
        if (!weak)
            return std::nullopt;

        ::ItemStack stack{};
        stack.reinit(*weak, 1, aux);
        if (tag)
            stack.setUserData(std::make_unique<::CompoundTag>(*tag));
        return stack;
    };
    std::optional<::ItemStack> stack = tryGet(rawName);
    if (!stack) {
        stack = tryGet(normalizeItemName(rawName));
    }
    if (!stack)
        return std::nullopt;

    if (glint && !stack->isEnchanted()) {
        auto                  ench = stack->constructItemEnchantsFromUserData();
        ::EnchantmentInstance inst{};
        inst.mEnchantType = ::Enchant::Type::Sharpness;
        inst.mLevel       = 1;

        auto instances = ench.getEnchantInstances();
        instances[0].push_back(inst);
        ench.setEnchantInstances(std::move(instances));
        stack->saveEnchantsToUserData(ench);
        HLIB_LOG_WARN(
            "[ItemDisplay] 光效自写未生效(isEnchanted=false), 已走原生兜底。自写: [{}] 原生格式: [{}]",
            tag ? tag->toSnbt(::SnbtFormat::Minimize) : std::string("(null)"),
            stack->mUserData ? stack->mUserData->toSnbt(::SnbtFormat::Minimize) : std::string("(null)"));
    }

    if (!nbt.empty() || glint) {
        HLIB_LOG_INFO(
            "[ItemDisplay] 物品构建 '{}': NBT {} 字节, glint={}, isEnchanted={}",
            rawName,
            nbt.size(),
            glint,
            stack->isEnchanted());
    }
    return stack;
}

inline int effectiveMode(ItemDisplayConfig const& data, ::ItemStack const& stack) {
    if (data.mode != 0)
        return data.mode;
    bool const isBlock = stack.mBlock != nullptr && !is2DBlockItemName(stack.getRawNameId());
    return isBlock ? 2 : 1;
}

using Runtime = ItemDisplayManager::Runtime;

inline std::string animNum(double v) {
    return std::format("{:.6g}", v);
}
inline std::string animParen(std::string const& s) {
    return "(" + s + ")";
}
inline std::string animAdd(std::string const& a, std::string const& b) {
    return "(" + a + "+" + b + ")";
}
inline std::string animMul(std::string const& a, std::string const& b) {
    return "(" + a + "*" + b + ")";
}
inline std::string animSin(double w, std::string const& t) {
    return std::format("math.sin(({})*{})", animNum(w), t);
}
inline std::string animCos(double w, std::string const& t) {
    return std::format("math.cos(({})*{})", animNum(w), t);
}

inline void applyAnimToConfig(
    ItemDisplayConfig&       d,
    ItemDisplayAnim const&   a,
    Runtime::AnimBase const& base,
    bool                     itemMode) {

    d.offsetX = base.ox;
    d.offsetY = base.oy;
    d.offsetZ = base.oz;
    d.rotX    = base.rx;
    d.rotY    = base.ry;
    d.rotZ    = base.rz;
    d.scale   = base.scale;

    std::string const t  = std::format("(q.life_time+({}))", animNum(a.phase));
    std::string       rx = animParen(base.rx);
    std::string       ry = animParen(base.ry);
    std::string       rz = animParen(base.rz);

    if (a.spinSpeed != 0.0f) {
        auto& axis = (a.spinAxis == 0) ? rx : ((a.spinAxis == 2) ? rz : ry);
        axis       = animAdd(axis, animMul(animNum(a.spinSpeed), t));
    }

    if (a.tumbleSpeed != 0.0f) {
        rx = animAdd(rx, animMul(animNum(a.tumbleSpeed), t));
        rz = animAdd(rz, animMul(animNum(-0.7 * a.tumbleSpeed), t));
    }

    if (a.billboard && itemMode) {
        ry = "q.rotation_to_camera(1)";
        if (a.billboardPitch)
            rx = "q.rotation_to_camera(0)";
    }

    if (a.pendulumAmplitude != 0.0f && a.pendulumPeriod > 0.01f) {
        ry = animAdd(ry, animMul(animNum(a.pendulumAmplitude), animSin(360.0f / a.pendulumPeriod, t)));
    }

    if (a.swayAmplitude != 0.0f && a.swayPeriod > 0.01f) {
        double const w = 360.0 / a.swayPeriod;
        rx             = animAdd(rx, animMul(animNum(a.swayAmplitude), animSin(w, t)));
        rz             = animAdd(rz, animMul(animNum(a.swayAmplitude), animCos(w, t)));
    }

    if (a.wobbleAmplitude != 0.0f) {
        double const w = 120.0 * (a.wobbleSpeed > 0.01f ? a.wobbleSpeed : 1.0f);
        rx             = animAdd(rx, animMul(animNum(a.wobbleAmplitude), animSin(w, t)));
        rz             = animAdd(
            rz,
            animMul(animNum(a.wobbleAmplitude), std::format("math.sin(({})*{}+1.1)", animNum(w * 1.37), t)));
    }

    std::string oy = animParen(base.oy);
    if (a.bobAmplitude != 0.0f && a.bobPeriod > 0.01f) {
        oy = animAdd(oy, animMul(animNum(a.bobAmplitude), animSin(360.0f / a.bobPeriod, t)));
    }
    d.offsetY = oy;

    if (a.orbitRadius != 0.0f && a.orbitPeriod > 0.01f) {
        double const      w = 360.0 / a.orbitPeriod;
        std::string const c = animMul(animNum(a.orbitRadius), animCos(w, t));
        std::string const s = animMul(animNum(a.orbitRadius), animSin(w, t));
        if (a.orbitPlane == 0) {
            d.offsetX = animAdd(animParen(base.ox), c);
            d.offsetY = animAdd(d.offsetY, s);
        } else if (a.orbitPlane == 1) {
            d.offsetZ = animAdd(animParen(base.oz), c);
            d.offsetY = animAdd(d.offsetY, s);
        } else {
            d.offsetX = animAdd(animParen(base.ox), c);
            d.offsetZ = animAdd(animParen(base.oz), s);
        }
    }

    std::string scale = animParen(base.scale);
    if (a.pulseAmplitude != 0.0f && a.pulsePeriod > 0.01f) {
        scale = animMul(scale, animAdd("1", animMul(animNum(a.pulseAmplitude), animSin(360.0f / a.pulsePeriod, t))));
    }
    if (a.heartbeatAmplitude != 0.0f && a.heartbeatPeriod > 0.01f) {
        double const w    = 360.0 / a.heartbeatPeriod;
        std::string  beat = std::format("math.pow((0.5+0.5*math.sin(({})*{})),4)", animNum(w), t);
        scale             = animMul(scale, animAdd("1", animMul(animNum(a.heartbeatAmplitude), beat)));
    }
    if (a.lodNear > 0.0f && a.lodFar > 0.0f && a.lodMinScale > 0.0f) {
        scale = animMul(
            scale,
            std::format(
                "math.lerp(({}),1,q.camera_distance_range_lerp(({}),({})))",
                animNum(a.lodMinScale),
                animNum(a.lodFar),
                animNum(a.lodNear)));
    }
    if (a.hideInFirstPerson) {
        scale = animMul(scale, "(1-q.is_first_person)");
    }

    d.rotX  = rx;
    d.rotY  = ry;
    d.rotZ  = rz;
    d.scale = scale;
}

inline void sendFoxActor(Player& player, ItemDisplayConfig const& data, Runtime const& rt) {
    sculk::protocol::AddActorPacket pkt;
    pkt.mActorUniqueId  = static_cast<std::int64_t>(rt.uniqueId);
    pkt.mActorRuntimeId = rt.runtimeId;
    pkt.mIdentifier     = "minecraft:fox";
    pkt.mPosition       = {data.x, data.y, data.z};
    pkt.mVelocity       = {0, 0, 0};
    pkt.mRotation       = {0, 0};
    pkt.mYHeadRotation  = 0;
    pkt.mYBodyRotation  = 0;
    pkt.mAttributes     = {
        {"minecraft:health", 0, 20.f, 20.f},
        {"minecraft:movement", 0, 0.3f, 3.4028235e38f},
        {"minecraft:underwater_movement", 0, 0.02f, 3.4028235e38f},
        {"minecraft:lava_movement", 0, 0.02f, 3.4028235e38f},
        {"minecraft:absorption", 0, 0, 16.f},
        {"minecraft:luck", -1024, 0, 1024},

        {"minecraft:knockback_resistance", 1.f, 0.f, 1.f},
    };
    pkt.mSynchedProperties = {};
    pkt.mActorLinks        = {};

    std::int64_t flags       = (std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31);
    pkt.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0, flags},
        {sculk::protocol::ActorDataIDs::Reserved53, data.hitboxWidth},
        {sculk::protocol::ActorDataIDs::Reserved54, data.hitboxHeight},
    };
    wire::send(player, pkt);
}

inline void sendEquipment(Player& player, std::uint64_t runtimeId, ::ItemStack const& stack) {
    ::MobEquipmentPacket equipPkt{
        ::ActorRuntimeID{runtimeId},
        stack,
        0,
        0,
        ContainerID::Inventory};
    equipPkt.sendTo(player);
}

inline void sendDataPacket(Player& player, std::uint64_t runtimeId, float hitboxWidth, float hitboxHeight) {
    sculk::protocol::SetActorDataPacket pkt;
    pkt.mActorRuntimeId    = runtimeId;
    pkt.mSynchedProperties = {};
    pkt.mTick              = 0;

    std::int64_t flags       = (std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31);
    pkt.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0, flags},
        {sculk::protocol::ActorDataIDs::Reserved53, hitboxWidth},
        {sculk::protocol::ActorDataIDs::Reserved54, hitboxHeight},
    };
    wire::send(player, pkt);
}

inline void sendHitboxPacket(Player& player, std::uint64_t runtimeId, float width, float height) {
    sculk::protocol::SetActorDataPacket pkt;
    pkt.mActorRuntimeId      = runtimeId;
    pkt.mSynchedProperties   = {};
    pkt.mTick                = 0;
    std::int64_t flags       = (std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31);
    pkt.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0, flags},
        {sculk::protocol::ActorDataIDs::Reserved53, width},
        {sculk::protocol::ActorDataIDs::Reserved54, height},
    };
    wire::send(player, pkt);
}

inline void sendFollowMove(Player& player, std::uint64_t runtimeId, float x, float y, float z) {
    sculk::protocol::MoveActorAbsolutePacket pkt;
    pkt.mActorRuntimeId = runtimeId;
    pkt.mFlags          = {};
    pkt.mPosition       = {x, y, z};
    pkt.mRotationX      = 0;
    pkt.mRotationY      = 0;
    pkt.mRotationYHead  = 0;
    wire::send(player, pkt);
}

inline void sendRemove(Player& player, Runtime const& rt) {
    sculk::protocol::RemoveActorPacket pkt;
    pkt.mActorUniqueId = static_cast<std::int64_t>(rt.uniqueId);
    wire::send(player, pkt);
}

inline std::string buildItemScaleExpr(
    ItemDisplayConfig const& d,
    std::string const&       stretchXZ,
    std::string const&       stretchY) {

    return std::format(
        "v.scale=({0});v.adscale=math.sqrt(math.abs(v.scale));v.adscaled=2.1385*v.adscale;"
        "v.xbasepos=({1});v.ybasepos=({2});v.zbasepos=({3});"
        "v.xpos=({4});v.ypos=({5});v.zpos=({6});"
        "v.xrot=({7});v.yrot=(({8})+205);v.zrot=({9});"
        "v.swelling_scale1=v.adscaled*({10});v.swelling_scale2=v.adscaled*({11});",
        d.scale,
        d.baseOffsetX,
        d.baseOffsetY,
        d.baseOffsetZ,
        d.offsetX,
        d.offsetY,
        d.offsetZ,
        d.rotX,
        d.rotY,
        d.rotZ,
        stretchXZ,
        stretchY);
}

constexpr std::string_view kItemPosRotExpr =

    "v.adjust_xz=8*v.adscaled+v.zbasepos/v.adscaled;"
    "v.adjust_y=(-5-v.ybasepos/v.adscaled/v.adscaled)*v.adscaled;"
    "v.x=v.xbasepos/v.adscaled;v.y=v.adjust_y;v.z=v.adjust_xz;"
    "v.ty=v.y*math.cos(v.xrot)-v.z*math.sin(v.xrot);"
    "v.tz=v.y*math.sin(v.xrot)+v.z*math.cos(v.xrot);"
    "v.y=v.ty;v.z=v.tz;"
    "v.tx=-v.x*math.cos(v.zrot)+v.y*math.sin(v.zrot);"
    "v.ty=v.x*math.sin(v.zrot)+v.y*math.cos(v.zrot);"
    "v.x=v.tx;v.y=v.ty;"
    "v.tx=v.x*math.cos(v.yrot)+v.z*math.sin(v.yrot);"
    "v.tz=-v.x*math.sin(v.yrot)+v.z*math.cos(v.yrot);"
    "v.x=v.tx;v.z=v.tz;"
    "v.head_position_x=v.x+v.xpos/v.adscaled;"
    "v.head_position_y=7.48/v.adscale+v.z+v.zpos/v.adscaled;"
    "v.head_position_z=v.y-v.ypos/v.adscaled;"
    "v.head_rotation_x=90+v.xrot;"
    "v.head_rotation_y=v.zrot;"
    "v.head_rotation_z=v.yrot;";

inline std::string buildBlockMatrixExpr(ItemDisplayConfig const& d) {

    char const* ypos  = (d.offsetY == "-4") ? "0.125" : d.offsetY.c_str();
    char const* scale = (d.scale == "0.375") ? "0.5" : d.scale.c_str();
    return std::format(
        "v.xpos=({0});v.ypos=({1});v.zpos=({2});"
        "v.xrot=({3});v.yrot=({4});v.zrot=({5});"
        "v.scale=({6});v.extend_scale=({7});v.extend_xrot=({8});v.extend_yrot=({9});"
        "v.xbasepos=({10});v.ybasepos=({11});v.zbasepos=({12});"
        "v.F.r5=-math.sin(v.xrot);v.F.r2=-math.sin(v.yrot);v.F.r3=-math.sin(v.zrot);"
        "v.F.r4=math.cos(v.zrot);v.F.r8=math.cos(v.yrot);"
        "v.F.r0=-v.F.r5*v.F.r2*v.F.r3+v.F.r8*v.F.r4;"
        "v.F.r1=-v.F.r5*v.F.r2*v.F.r4-v.F.r8*v.F.r3;"
        "v.F.r6=-v.F.r5*v.F.r8*v.F.r3-v.F.r2*v.F.r4;"
        "v.F.r7=-v.F.r5*v.F.r8*v.F.r4+v.F.r2*v.F.r3;"
        "v.F.r2=v.F.r2*math.cos(v.xrot);v.F.r3=v.F.r3*math.cos(v.xrot);"
        "v.F.r4=v.F.r4*math.cos(v.xrot);v.F.r8=v.F.r8*math.cos(v.xrot);"
        "v.F.e0=math.cos(v.extend_yrot);v.F.e4=math.cos(v.extend_xrot);"
        "v.F.e5=-math.sin(v.extend_xrot);v.F.e6=math.sin(v.extend_yrot);"
        "v.F.e1=v.F.e5*v.F.e6;v.F.e2=-v.F.e4*v.F.e6;"
        "v.F.e7=-v.F.e5*v.F.e0;v.F.e8=v.F.e4*v.F.e0;"
        "v.F.p0=v.F.r0*v.F.e0+v.F.r2*v.F.e6;"
        "v.F.p1=v.F.r0*v.F.e1+v.F.r1*v.F.e4+v.F.r2*v.F.e7;"
        "v.F.p2=v.F.r0*v.F.e2+v.F.r1*v.F.e5+v.F.r2*v.F.e8;"
        "v.F.p3=v.F.r3*v.F.e0+v.F.r5*v.F.e6;"
        "v.F.p4=v.F.r3*v.F.e1+v.F.r4*v.F.e4+v.F.r5*v.F.e7;"
        "v.F.p5=v.F.r3*v.F.e2+v.F.r4*v.F.e5+v.F.r5*v.F.e8;"
        "v.F.p6=v.F.r6*v.F.e0+v.F.r8*v.F.e6;"
        "v.F.p7=v.F.r6*v.F.e1+v.F.r7*v.F.e4+v.F.r8*v.F.e7;"
        "v.F.p8=v.F.r6*v.F.e2+v.F.r7*v.F.e5+v.F.r8*v.F.e8;",
        d.offsetX,
        ypos,
        d.offsetZ,
        d.rotX,
        d.rotY,
        d.rotZ,
        scale,
        d.extendScale,
        d.extendRotX,
        d.extendRotY,
        d.baseOffsetX,
        d.baseOffsetY,
        d.baseOffsetZ);
}

inline std::string buildBlockSwellingExpr(std::string const& stretchXZ, std::string const& stretchY) {

    return std::format(
        "v.F.s=math.sqrt(32/7*v.scale);"
        "v.swelling_scale1=v.F.s*({0});"
        "v.swelling_scale2=v.extend_scale*({1})*v.F.s;",
        stretchXZ,
        stretchY);
}

constexpr std::string_view kBlockHeadPosExpr =

    "v.head_position_x=-16/v.F.s*((v.xpos-1)*v.F.p1+(v.ypos-1/"
    "128)*v.F.p4+v.zpos*v.F.p7+(v.xbasepos*v.F.e1+(v.ybasepos+10/"
    "7)*v.extend_scale*v.F.e4+(v.zbasepos-16/7)*v.F.e7)*v.scale);"
    "v.head_position_y=16/v.F.s*(((v.xpos-1)*v.F.p2+(v.ypos-1/128)*v.F.p5+v.zpos*v.F.p8)/"
    "v.extend_scale+(v.xbasepos*v.F.e2+(v.ybasepos+10/7)*v.extend_scale*v.F.e5+(v.zbasepos-16/"
    "7)*v.F.e8)*v.scale);"
    "v.head_position_z=16/v.F.s*((v.xpos-1)*v.F.p0+(v.ypos-1/"
    "128)*v.F.p3+v.zpos*v.F.p6+(v.xbasepos*v.F.e0+(v.zbasepos-16/7)*v.F.e6)*v.scale);"
    "v.head_rotation_x=v.F.e6?math.atan2(0,-v.F.e6):math.atan2(-v.F.e8,v.F.e5);"
    "v.head_rotation_y=math.asin(-v.F.e0);"
    "v.head_rotation_z=v.F.e6?math.atan2(-v.F.e2,-v.F.e1):0;";

constexpr std::string_view kBlockBodyRotExpr =

    "v.body_x_rot=v.F.p5||v.F.p3?math.atan2(v.F.p5,-v.F.p3):math.atan2(-v.F.p0,-v.F.p2);"
    "v.body_z_rot=v.F.p5||v.F.p3?math.atan2(-v.F.p1,v.F.p7):0;";

constexpr std::string_view kBlockAttackRotExpr =
    "v.attack_body_rot_y=math.asin(-v.F.p4);";

inline void scheduleAnims(
    int64_t                  id,
    ItemDisplayConfig const& data,
    Runtime const&           rt,
    Player&                  player,
    int                      mode,
    bool                     noAliveCheck = false) {
    auto const uuid  = player.getUuid();
    auto const base  = currentTick();
    auto const block = (mode == 2);
    auto const tag   = "." + std::to_string(id);

    auto push = [&](std::uint64_t at, std::string anim, std::string ctrl, std::string stop) {
        AnimEntry e{uuid, rt.runtimeId, std::move(anim), std::move(ctrl), std::move(stop)};
        e.noAliveCheck = noAliveCheck;
        animQueue().emplace(at, std::move(e));
    };

    for (auto const pass : {std::uint64_t{0}, std::uint64_t{38}, std::uint64_t{98}}) {
        if (block) {
            push(base + pass + 2, "animation.player.sleeping", "controller.animation.fox.move" + tag, buildBlockMatrixExpr(data));
            push(base + pass + 3, "animation.creeper.swelling", "wiki.fmbe.3d_blocks.anim1" + tag, buildBlockSwellingExpr(rt.stretchXZ, rt.stretchY));
            push(base + pass + 4, "animation.ender_dragon.neck_head_movement", "wiki.fmbe.3d_blocks.anim2" + tag, std::string{kBlockHeadPosExpr});
            push(base + pass + 5, "animation.warden.move", "wiki.fmbe.3d_blocks.anim3" + tag, std::string{kBlockBodyRotExpr});
            push(base + pass + 6, "animation.player.attack.rotations", "wiki.fmbe.3d_blocks.anim4" + tag, std::string{kBlockAttackRotExpr});
        } else {
            push(base + pass + 2, "animation.player.sleeping", "controller.animation.fox.move" + tag, "");
            push(base + pass + 3, "animation.creeper.swelling", "wiki.scale" + tag, buildItemScaleExpr(data, rt.stretchXZ, rt.stretchY));
            push(base + pass + 4, "animation.ender_dragon.neck_head_movement", "wiki.posrot" + tag, std::string{kItemPosRotExpr});
        }

        if (rt.cachedStack) {
            AnimEntry e{uuid, rt.runtimeId, {}, {}, {}};
            e.noAliveCheck = noAliveCheck;
            e.equip        = true;
            e.equipStack   = *rt.cachedStack;
            animQueue().emplace(base + pass + (block ? 7 : 5), std::move(e));
        }
    }
}

inline bool spawnForPlayer(int64_t id, ItemDisplayConfig const& data, Runtime& rt, Player& player) {

    if (!ll::service::getLevel())
        return false;

    bool const changed    = rt.cachedItemName != data.item || rt.cachedItemAux != data.itemAux || rt.cachedItemNbt != data.itemNbt || rt.cachedGlint != data.itemGlint;
    bool const unresolved = !rt.cachedStack.has_value() && !rt.itemWarned;
    if (changed || unresolved) {
        if (changed)
            rt.itemWarned = false;
        rt.cachedStack    = buildItemStack(data.item, data.itemAux, data.itemNbt, data.itemGlint);
        rt.cachedItemName = data.item;
        rt.cachedItemAux  = data.itemAux;
        rt.cachedItemNbt  = data.itemNbt;
        rt.cachedGlint    = data.itemGlint;
        if (!rt.cachedStack) {
            rt.itemWarned = true;
            HLIB_LOG_WARN(
                "[ItemDisplay] unknown item '{}' for display #{} (tried raw/short-name/alias)",
                data.item,
                id);
        }
    }
    if (!rt.cachedStack)
        return false;
    auto const& stack = *rt.cachedStack;
    int const   mode  = effectiveMode(data, stack);

    sendFoxActor(player, data, rt);

    sendDataPacket(player, rt.runtimeId, data.hitboxWidth, data.hitboxHeight);
    scheduleAnims(id, data, rt, player, mode);
    HLIB_LOG_DEBUG(
        "[ItemDisplay] spawned #{} '{}' at ({:.1f},{:.1f},{:.1f}) dim={} mode={}",
        id,
        data.item,
        data.x,
        data.y,
        data.z,
        data.dimension,
        mode);
    return true;
}

inline void despawnForPlayer(Runtime const& rt, Player& player) {
    sendRemove(player, rt);

    auto& q = animQueue();
    for (auto it = q.begin(); it != q.end();) {
        if (it->second.runtimeId == rt.runtimeId) {
            it = q.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ide_ops
} // namespace debugshape_export
