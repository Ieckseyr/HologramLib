// ItemDisplayManager.cpp - FMBE 狐狸悬浮物显示实现
//
// 迁移自 ItemPhys FloatingDisplay（狐狸显示+发包技术）,
// 主键改为 int64 id，去除持久化与命令层（由消费者负责）。
#include "ItemDisplayManager.h"

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
#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/registry/ItemRegistryRef.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/level/MolangVersion.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/AnimateEntityPacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/utility/deps/BinaryStream.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <map>
#include <optional>

namespace debugshape_export {

namespace {

auto& logger() {
    static auto log = ll::io::LoggerRegistry::getInstance().getOrCreate("HologramLib");
    return *log;
}

// ── sculk 协议包通用发送（验证 + 头部封装 + peer 发送）──

template <typename PacketT>
void sendSculkToPlayer(Player& player, PacketT const& packet) {
    std::vector<std::byte>        bodyBuffer;
    sculk::protocol::BinaryStream bodyStream(bodyBuffer);
    packet.write(bodyStream);

    std::string          checkBuffer(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());
    ReadOnlyBinaryStream checkStream(checkBuffer, true);
    auto                 checkPacket = MinecraftPackets::createPacket(static_cast<MinecraftPacketIds>(packet.getId()));
    if (!checkPacket || !checkPacket->read(checkStream)) {
        logger().warn("Sculk packet validation failed for {}", packet.getName());
        return;
    }

    BinaryStream sendStream;
    sendStream.writeUnsignedVarInt(
        (static_cast<int>(packet.getId()) & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12),
        nullptr,
        nullptr
    );
    sendStream.mBuffer.append(reinterpret_cast<char const*>(bodyBuffer.data()), bodyBuffer.size());

    auto networkSystem = ll::service::getNetworkSystem();
    if (networkSystem) {
        auto const& networkId = player.getNetworkIdentifier();
        auto*       peer      = networkSystem->getPeerForUser(networkId);
        if (peer != nullptr) {
            peer->sendPacket(sendStream.mBuffer, NetworkPeer::Reliability::Reliable, Compressibility::Compressible);
        }
    }
}

// ── 动画调度队列（tick 延迟发送 AnimateEntityPacket）──

struct AnimEntry {
    mce::UUID        playerUuid;
    std::uint64_t    runtimeId;
    std::string      animation;
    std::string      controller;
    std::string      stopExpression;
};

std::multimap<std::uint64_t, AnimEntry>& animQueue() {
    static std::multimap<std::uint64_t, AnimEntry> queue;
    return queue;
}

std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

Player* findPlayerByUuid(mce::UUID const& uuid) {
    auto level = ll::service::getLevel();
    if (!level) return nullptr;
    return level->getPlayer(uuid);
}

// ── 2D 方块物品判定 ──

bool is2DBlockItemName(std::string const& rawName) {
    auto name = std::string_view{rawName};
    if (name.starts_with("minecraft:")) name.remove_prefix(10);

    static constexpr std::string_view exactNames[] = {
        "ladder",     "vine",          "weeping_vines", "twisting_vines", "glow_lichen", "sculk_vein", "lily_pad",
        "tripwire",   "tripwire_hook", "redstone_wire", "deadbush",       "fern",        "large_fern", "short_grass",
        "tall_grass", "seagrass",      "tall_seagrass", "kelp",           "cocoa",
    };
    for (auto exact : exactNames) {
        if (name == exact) return true;
    }

    static constexpr std::string_view patterns[] = {
        "flower", "sapling", "coral_fan", "torch", "rail", "button", "lever", "pressure_plate",
        "carpet", "roots",   "sprouts",   "fungus", "mushroom", "bush",
    };
    return std::ranges::any_of(patterns, [&name](auto pattern) { return name.contains(pattern); });
}

// ── ItemStack 构造（按名称从注册表解析）──

// 旧名 → 新名（1.16~1.20.3x 官方改名, 用户习惯兼容; 与 DropEnhancer 校验逻辑同步）
std::unordered_map<std::string, std::string> const& legacyItemNames() {
    static std::unordered_map<std::string, std::string> const map = {
        {"grass",      "short_grass"}, // 1.20.30 起草丛改名
        {"grass_path", "dirt_path"},   // 1.19.x
        {"sign",       "oak_sign"},    // 1.16 拆分
        {"skull",      "player_head"}, // 1.16 拆分
    };
    return map;
}

// 名称规范化: 短名补 minecraft: 前缀; 旧名映射到现行名
std::string normalizeItemName(std::string const& raw) {
    std::string_view shortName = raw;
    if (shortName.starts_with("minecraft:")) shortName.remove_prefix(10);
    if (auto it = legacyItemNames().find(std::string(shortName)); it != legacyItemNames().end()) {
        return "minecraft:" + it->second;
    }
    if (raw.find(':') == std::string::npos) return "minecraft:" + raw;
    return raw;
}

std::optional<::ItemStack> buildItemStack(std::string const& rawName, int aux) {
    auto level = ll::service::getLevel();
    if (!level) return std::nullopt;

    auto tryGet = [&](std::string const& name) -> std::optional<::ItemStack> {
        auto weak = level->getItemRegistry().getItem(::HashedString(name));
        if (!weak) return std::nullopt;
        return ::ItemStack(*weak, 1, aux, nullptr);
    };
    if (auto stack = tryGet(rawName)) return stack; // 原名（含自定义命名空间）
    return tryGet(normalizeItemName(rawName));     // 短名补全 / 旧名映射
}

// 解析实际渲染模式：auto 时按物品是否为 3D 方块决定
int effectiveMode(ItemDisplayConfig const& data, ::ItemStack const& stack) {
    if (data.mode != 0) return data.mode; // 1=item 2=block 手动指定
    bool const isBlock = stack.mBlock != nullptr && !is2DBlockItemName(stack.getRawNameId());
    return isBlock ? 2 : 1;
}

// ── 发包原语 ──

using Runtime = ItemDisplayManager::Runtime;

void sendFoxActor(Player& player, ItemDisplayConfig const& data, Runtime const& rt) {
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
        {"minecraft:health",              0,     20.f,  20.f         },
        {"minecraft:movement",            0,     0.3f,  3.4028235e38f},
        {"minecraft:underwater_movement", 0,     0.02f, 3.4028235e38f},
        {"minecraft:lava_movement",       0,     0.02f, 3.4028235e38f},
        {"minecraft:absorption",          0,     0,     16.f         },
        {"minecraft:luck",                -1024, 0,     1024         },
    };
    pkt.mSynchedProperties = {};
    pkt.mActorLinks        = {};
    // ItemPhys 原版配方: flags=(1<<5)|(1<<30)|(1<<31) + R53/R54=0.0f
    // (FMBE 动画生效后狐狸本体被 swelling 变形隐藏, 只显示叼的物品)
    std::int64_t flags       = (std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31);
    pkt.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0,  flags},
        {sculk::protocol::ActorDataIDs::Reserved53, 0.0f},
        {sculk::protocol::ActorDataIDs::Reserved54, 0.0f},
    };
    sendSculkToPlayer(player, pkt);
}

void sendEquipment(Player& player, std::uint64_t runtimeId, ::ItemStack const& stack) {
    ::MobEquipmentPacket equipPkt{
        ::ActorRuntimeID{runtimeId},
        stack,
        0, // slot
        0, // selectedSlot
        ContainerID::Inventory
    };
    equipPkt.sendTo(player);
}

void sendDataPacket(Player& player, std::uint64_t runtimeId) {
    sculk::protocol::SetActorDataPacket pkt;
    pkt.mActorRuntimeId    = runtimeId;
    pkt.mSynchedProperties = {};
    pkt.mTick              = 0;
    // 与 AddActor 完全一致的 flags（ItemPhys 原版: 两包同配方, 避免 flags 抖动）
    std::int64_t flags       = (std::int64_t(1) << 5) | (std::int64_t(1) << 30) | (std::int64_t(1) << 31);
    pkt.mMetaData.mDataItems = {
        {sculk::protocol::ActorDataIDs::Reserved0,  flags},
        {sculk::protocol::ActorDataIDs::Reserved53, 0.0f},
        {sculk::protocol::ActorDataIDs::Reserved54, 0.0f},
    };
    sendSculkToPlayer(player, pkt);
}

void sendRemove(Player& player, Runtime const& rt) {
    sculk::protocol::RemoveActorPacket pkt;
    pkt.mActorUniqueId = static_cast<std::int64_t>(rt.uniqueId);
    sendSculkToPlayer(player, pkt);
}

// ── FMBE Molang 模板（参数化三轴旋转/函数平移/缩放）──

std::string buildItemScaleExpr(ItemDisplayConfig const& d) {
    // 物品渲染：wiki.scale 路径（v.yrot 自动 +205 补偿狐狸头部朝向）
    return std::format(
        "v.scale={0};v.adscale=math.sqrt(v.scale);v.adscaled=2.1385*v.adscale;"
        "v.xbasepos={1};v.ybasepos={2};v.zbasepos={3};"
        "v.xpos={4};v.ypos={5};v.zpos={6};"
        "v.xrot={7};v.yrot=({8})+205;v.zrot={9};"
        "v.swelling_scale1=v.adscaled;v.swelling_scale2=v.adscaled;",
        d.scale,
        d.baseOffsetX,
        d.baseOffsetY,
        d.baseOffsetZ,
        d.offsetX,
        d.offsetY,
        d.offsetZ,
        d.rotX,
        d.rotY,
        d.rotZ
    );
}

constexpr std::string_view kItemPosRotExpr =
    // 物品渲染：wiki.posrot 路径（按 xrot→zrot→yrot 顺序旋转并平移）—— ItemPhys 原版表达式, 逐字保持
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

std::string buildBlockMatrixExpr(ItemDisplayConfig const& d) {
    // 3D 方块渲染：完整旋转矩阵路径（主三轴 + 二段扩展旋转）
    // 方块模式默认值归一化: 配置出厂默认是物品模式值(offsetY=-4/scale=0.375),
    // 方块下换用 ItemPhys 原版方块默认(ypos=0.125/scale=0.5); 用户设置过其他值则尊重用户值
    char const* ypos  = (d.offsetY == "-4")    ? "0.125" : d.offsetY.c_str();
    char const* scale = (d.scale == "0.375") ? "0.5"    : d.scale.c_str();
    return std::format(
        "v.xpos={0};v.ypos={1};v.zpos={2};"
        "v.xrot={3};v.yrot={4};v.zrot={5};"
        "v.scale={6};v.extend_scale={7};v.extend_xrot={8};v.extend_yrot={9};"
        "v.xbasepos={10};v.ybasepos={11};v.zbasepos={12};"
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
        d.baseOffsetZ
    );
}

constexpr std::string_view kBlockSwellingExpr =
    // 3D 方块渲染：立方体缩放
    "v.swelling_scale2=v.extend_scale*(v.swelling_scale1=(v.F.s=math.sqrt(32/7*v.scale)));";

constexpr std::string_view kBlockHeadPosExpr =
    // 3D 方块渲染：头骨定位（把方块放到狐狸头部位置）
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
    // 3D 方块渲染：身体旋转
    "v.body_x_rot=v.F.p5||v.F.p3?math.atan2(v.F.p5,-v.F.p3):math.atan2(-v.F.p0,-v.F.p2);"
    "v.body_z_rot=v.F.p5||v.F.p3?math.atan2(-v.F.p1,v.F.p7):0;";

constexpr std::string_view kBlockAttackRotExpr =
    "v.attack_body_rot_y=math.asin(-v.F.p4);";

// 调度完整 FMBE 动画序列（物品 3 包 / 方块 5 包）
// ItemPhys 原版节奏: 依次 +2/+3/+4(物品) 或 +2..+6(方块), 一包一 tick —— 逐字保持, 勿改为同帧
// 方块模式: sleeping(+2)携带完整旋转矩阵, swelling(+3)携带缩放表达式(v.F.s 基准) —— 两者不可错位,
// 否则 v.F.s 未初始化会导致方块缩放异常/不可见
void scheduleAnims(ItemDisplayConfig const& data, Runtime const& rt, Player& player, int mode) {
    auto const uuid  = player.getUuid();
    auto const base  = currentTick();
    auto const block = (mode == 2);

    auto push = [&](std::uint64_t at, std::string anim, std::string ctrl, std::string stop) {
        animQueue().emplace(at, AnimEntry{uuid, rt.runtimeId, std::move(anim), std::move(ctrl), std::move(stop)});
    };

    if (block) {
        push(base + 2, "animation.player.sleeping", "controller.animation.fox.move", buildBlockMatrixExpr(data));
        push(base + 3, "animation.creeper.swelling", "wiki.fmbe.3d_blocks.anim1", std::string{kBlockSwellingExpr});
        push(base + 4, "animation.ender_dragon.neck_head_movement", "wiki.fmbe.3d_blocks.anim2", std::string{kBlockHeadPosExpr});
        push(base + 5, "animation.warden.move", "wiki.fmbe.3d_blocks.anim3", std::string{kBlockBodyRotExpr});
        push(base + 6, "animation.player.attack.rotations", "wiki.fmbe.3d_blocks.anim4", std::string{kBlockAttackRotExpr});
    } else {
        push(base + 2, "animation.player.sleeping", "controller.animation.fox.move", "");
        push(base + 3, "animation.creeper.swelling", "wiki.scale", buildItemScaleExpr(data));
        push(base + 4, "animation.ender_dragon.neck_head_movement", "wiki.posrot", std::string{kItemPosRotExpr});
    }
}

// ── 生成/销毁完整序列 ──

bool spawnForPlayer(int64_t id, ItemDisplayConfig const& data, Runtime& rt, Player& player) {
    // Level 未就绪: 不缓存不告警, 下个 tick 重试
    if (!ll::service::getLevel()) return false;

    // 物品解析缓存（名称/附加值变更时失效重解析; 失败只告警一次）
    bool const changed     = rt.cachedItemName != data.item || rt.cachedItemAux != data.itemAux;
    bool const unresolved  = !rt.cachedStack.has_value() && !rt.itemWarned;
    if (changed || unresolved) {
        if (changed) rt.itemWarned = false; // 换物品后重置告警
        rt.cachedStack    = buildItemStack(data.item, data.itemAux);
        rt.cachedItemName = data.item;
        rt.cachedItemAux  = data.itemAux;
        if (!rt.cachedStack) {
            rt.itemWarned = true;
            logger().warn(
                "[ItemDisplay] unknown item '{}' for display #{} (tried raw/short-name/alias)",
                data.item,
                id
            );
        }
    }
    if (!rt.cachedStack) return false;
    auto const& stack = *rt.cachedStack;
    int const    mode  = effectiveMode(data, stack);

    sendFoxActor(player, data, rt);
    sendEquipment(player, rt.runtimeId, stack);
    sendDataPacket(player, rt.runtimeId);
    scheduleAnims(data, rt, player, mode);
    logger().debug(
        "[ItemDisplay] spawned #{} '{}' at ({:.1f},{:.1f},{:.1f}) dim={} mode={}",
        id,
        data.item,
        data.x,
        data.y,
        data.z,
        data.dimension,
        mode
    );
    return true;
}

void despawnForPlayer(Runtime const& rt, Player& player) {
    sendRemove(player, rt);
}

} // namespace

// ── Manager 公开实现 ──

ItemDisplayManager& ItemDisplayManager::getInstance() {
    static ItemDisplayManager instance;
    return instance;
}

void ItemDisplayManager::init() {
    std::lock_guard lock(mMutex);
    if (mJoinListener) return; // 已初始化

    mJoinListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerJoinEvent>(
        [this](ll::event::PlayerJoinEvent& ev) {
            std::lock_guard lock(mMutex);
            mInitializedPlayers.insert(ev.self().getUuid());
        }
    );
    mDisconnectListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
        [this](ll::event::PlayerDisconnectEvent& ev) {
            std::lock_guard lock(mMutex);
            auto const& uuid = ev.self().getUuid();
            mInitializedPlayers.erase(uuid);
            for (auto& [id, rt] : mRuntimes) rt.shownPlayers.erase(uuid);
        }
    );
}

void ItemDisplayManager::shutdown() {
    std::lock_guard lock(mMutex);
    if (mJoinListener) {
        ll::event::EventBus::getInstance().removeListener(mJoinListener);
        mJoinListener.reset();
    }
    if (mDisconnectListener) {
        ll::event::EventBus::getInstance().removeListener(mDisconnectListener);
        mDisconnectListener.reset();
    }
    mConfigs.clear();
    mRuntimes.clear();
    mInitializedPlayers.clear();
    animQueue().clear();
}

int64_t ItemDisplayManager::create(ItemDisplayConfig const& config) {
    std::lock_guard lock(mMutex);
    return createLocked(config, mNextId++);
}

int64_t ItemDisplayManager::createRandom(ItemDisplayConfig const& config) {
    // 随机 ID 段 [0x10000000,0x7FFFFFFF): 与自增段(从 1 起)长期隔离, 不会撞车
    static std::mt19937_64 rng{std::random_device{}()};
    std::lock_guard       lock(mMutex);
    constexpr std::int64_t kBase = 0x10000000, kSpan = 0x70000000;
    for (int tries = 0; tries < 128; ++tries) {
        auto const id = kBase + static_cast<std::int64_t>(rng() % static_cast<std::uint64_t>(kSpan));
        if (mConfigs.contains(id)) continue;
        return createLocked(config, id);
    }
    return -1;
}

int64_t ItemDisplayManager::createWithId(ItemDisplayConfig const& config, int64_t desiredId) {
    if (desiredId <= 0) return -2;
    std::lock_guard lock(mMutex);
    if (mConfigs.contains(desiredId)) return -2;
    // 防自增游标未来撞上该 ID
    if (desiredId >= mNextId && desiredId < 0x10000000) mNextId = desiredId + 1;
    return createLocked(config, desiredId);
}

bool ItemDisplayManager::isIdUsed(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mConfigs.contains(id);
}

int64_t ItemDisplayManager::createLocked(ItemDisplayConfig const& config, int64_t id) {
    // 生成本次运行的唯一 Actor ID（避免与真实实体冲突: 高位标记 + 自增）
    static std::uint64_t actorSeed = 0x6D00000000000000ULL; // 'm' 标记位
    Runtime rt{};
    rt.uniqueId  = actorSeed | static_cast<std::uint64_t>(id);
    rt.runtimeId = 0x6D000000ULL + static_cast<std::uint64_t>(id);

    mConfigs.emplace(id, config);
    mRuntimes.emplace(id, std::move(rt));

    syncVisibilityLocked();
    return id;
}

bool ItemDisplayManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);

    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;

    auto uuids = std::vector<mce::UUID>{rit->second.shownPlayers.begin(), rit->second.shownPlayers.end()};
    for (auto const& uuid : uuids) {
        if (auto* player = findPlayerByUuid(uuid)) {
            despawnForPlayer(rit->second, *player);
        }
    }
    mRuntimes.erase(rit);
    mConfigs.erase(id);
    return true;
}

void ItemDisplayManager::destroyAll() {
    std::lock_guard lock(mMutex);
    for (auto& [id, rt] : mRuntimes) {
        auto uuids = std::vector<mce::UUID>{rt.shownPlayers.begin(), rt.shownPlayers.end()};
        for (auto const& uuid : uuids) {
            if (auto* player = findPlayerByUuid(uuid)) {
                despawnForPlayer(rt, *player);
            }
        }
    }
    mConfigs.clear();
    mRuntimes.clear();
}

bool ItemDisplayManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mConfigs.contains(id);
}

bool ItemDisplayManager::get(int64_t id, ItemDisplayConfig& out) const {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    out = it->second;
    return true;
}

bool ItemDisplayManager::setItem(int64_t id, std::string const& item, int aux) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.item    = item;
    it->second.itemAux = aux;
    refreshLocked(id);
    return true;
}

bool ItemDisplayManager::setPosition(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    if (dim >= 0) it->second.dimension = dim;
    refreshLocked(id);
    syncVisibilityLocked(); // 位置/维度变化影响可见性
    return true;
}

bool ItemDisplayManager::setOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.offsetX = ox;
    it->second.offsetY = oy;
    it->second.offsetZ = oz;
    refreshLocked(id);
    return true;
}

bool ItemDisplayManager::setBaseOffset(int64_t id, std::string const& ox, std::string const& oy, std::string const& oz) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.baseOffsetX = ox;
    it->second.baseOffsetY = oy;
    it->second.baseOffsetZ = oz;
    refreshLocked(id);
    return true;
}

bool ItemDisplayManager::setRotation(int64_t id, std::string const& rx, std::string const& ry, std::string const& rz) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.rotX = rx;
    it->second.rotY = ry;
    it->second.rotZ = rz;
    refreshLocked(id);
    return true;
}

bool ItemDisplayManager::setScale(int64_t id, std::string const& scale) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.scale = scale;
    refreshLocked(id);
    return true;
}

bool ItemDisplayManager::setExtend(int64_t id, std::string const& scale, std::string const& rx, std::string const& ry) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.extendScale = scale;
    it->second.extendRotX  = rx;
    it->second.extendRotY  = ry;
    refreshLocked(id);
    return true;
}

bool ItemDisplayManager::setMode(int64_t id, int mode) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.mode = mode;
    refreshLocked(id);
    return true;
}

bool ItemDisplayManager::setEnabled(int64_t id, bool enabled) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.enabled = enabled;
    syncVisibilityLocked();
    return true;
}

bool ItemDisplayManager::setViewDistance(int64_t id, double dist) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.viewDistance = dist;
    syncVisibilityLocked();
    return true;
}

bool ItemDisplayManager::rotateY(int64_t id, float delta) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;

    // 尝试将现有 rotY（常量）叠加增量; 表达式时包一层加法
    auto& ry     = it->second.rotY;
    float  current{};
    auto   parsed = std::from_chars(ry.data(), ry.data() + ry.size(), current);
    if (parsed.ec == std::errc{} && parsed.ptr == ry.data() + ry.size()) {
        ry = std::format("{}", current + delta);
    } else {
        ry = std::format("({})+{}", ry, delta);
    }
    refreshLocked(id);
    return true;
}

std::vector<int64_t> ItemDisplayManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    std::vector<int64_t> ids;
    ids.reserve(mConfigs.size());
    for (auto const& [id, cfg] : mConfigs) ids.push_back(id);
    return ids;
}

int64_t ItemDisplayManager::findNearest(float x, float y, float z, int dim, double maxDist) const {
    std::lock_guard lock(mMutex);
    int64_t best      = -1;
    double  bestDist2 = (maxDist > 0) ? maxDist * maxDist : 1e300;
    for (auto const& [id, cfg] : mConfigs) {
        if (cfg.dimension != dim) continue;
        double const dx   = cfg.x - x;
        double const dy   = cfg.y - y;
        double const dz   = cfg.z - z;
        double const dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            best      = id;
        }
    }
    return best;
}

void ItemDisplayManager::refreshLocked(int64_t id) {
    // 参数变化后刷新所有已见玩家（despawn → respawn 原子替换）
    auto it  = mConfigs.find(id);
    auto rit = mRuntimes.find(id);
    if (it == mConfigs.end() || rit == mRuntimes.end()) return;

    auto uuids = std::vector<mce::UUID>{rit->second.shownPlayers.begin(), rit->second.shownPlayers.end()};
    for (auto const& uuid : uuids) {
        auto* player = findPlayerByUuid(uuid);
        if (!player) {
            rit->second.shownPlayers.erase(uuid);
            continue;
        }
        despawnForPlayer(rit->second, *player);
        rit->second.shownPlayers.erase(uuid);
        if (it->second.enabled) {
            if (spawnForPlayer(id, it->second, rit->second, *player)) {
                rit->second.shownPlayers.insert(uuid);
            }
        }
    }
}

void ItemDisplayManager::syncVisibilityLocked() {
    auto level = ll::service::getLevel();
    if (!level) return;

    level->forEachPlayer([this](Player& player) {
        auto const uuid = player.getUuid();
        if (!mInitializedPlayers.contains(uuid)) return true;

        auto const dimId = player.getDimensionId();
        auto const& ppos = player.getPosition();

        for (auto& [id, data] : mConfigs) {
            auto rit = mRuntimes.find(id);
            if (rit == mRuntimes.end()) continue;
            auto& rt = rit->second;

            bool const visible =
                data.enabled && dimId == DimensionType(data.dimension)
                && (data.viewDistance <= 0
                    || ((ppos.x - data.x) * (ppos.x - data.x) + (ppos.y - data.y) * (ppos.y - data.y)
                        + (ppos.z - data.z) * (ppos.z - data.z))
                           <= data.viewDistance * data.viewDistance);

            if (visible && !rt.shownPlayers.contains(uuid)) {
                if (spawnForPlayer(id, data, rt, player)) rt.shownPlayers.insert(uuid);
            } else if (!visible && rt.shownPlayers.contains(uuid)) {
                despawnForPlayer(rt, player);
                rt.shownPlayers.erase(uuid);
            }
        }
        return true;
    });
}

// ── Tick Hook：动画队列 flush + 周期同步 ──

// Manager 私有访问桥（friend 声明于 Manager 头文件）
struct ItemDisplayTickHookAccess {
    static void sync(ItemDisplayManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.syncVisibilityLocked();
    }
};

LL_TYPE_INSTANCE_HOOK(ItemDisplayTickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();

    auto const now = currentTick();
    auto&      q   = animQueue();
    auto       it  = q.begin();
    while (it != q.end() && it->first <= now) {
        auto& entry = it->second;
        if (auto* player = findPlayerByUuid(entry.playerUuid)) {
            sculk::protocol::AnimateEntityPacket pkt;
            pkt.mAnimation                   = entry.animation;
            pkt.mNextState                   = "none";
            pkt.mStopExpression              = entry.stopExpression;
            pkt.mStopExpressionMolangVersion = sculk::protocol::MolangVersion::Initial;
            pkt.mController                  = entry.controller;
            pkt.mBlendOutTime                = 0;
            pkt.mRuntimeIds                  = {entry.runtimeId};
            sendSculkToPlayer(*player, pkt);
        }
        it = q.erase(it);
    }

    static std::uint64_t lastSyncTick = 0;
    if (now - lastSyncTick >= 20) {
        lastSyncTick = now;
        ItemDisplayTickHookAccess::sync(ItemDisplayManager::getInstance());
    }
}

// hook 生命周期（静态注册即可; 无显示时为空转 no-op）
static ll::memory::HookRegistrar<ItemDisplayTickHook> gTickHookRegistrar;

} // namespace debugshape_export
