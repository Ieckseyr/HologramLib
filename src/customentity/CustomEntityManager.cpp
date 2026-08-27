// CustomEntityManager.cpp - 自定义实体协议层生成实现
//
// spawn 序列: AddActorPacket(identifier + metadata + attributes)
// despawn 序列: RemoveActorPacket
// 可见性: 玩家入服/每 20 tick 同步 + 视距/维度过滤（与 ItemDisplay 同模式）
#include "CustomEntityManager.h"

#include <random>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/network/packet/MobArmorEquipmentPacket.h>
#include <mc/network/packet/MobEquipmentPacket.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateAttributesPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorLinkPacket.hpp>
#include <sculk/protocol/codec/packet/AnimateEntityPacket.hpp>
#include <sculk/protocol/codec/level/MolangVersion.hpp>

#include "SculkPacketSend.h"

#include <algorithm>
#include <string>

namespace debugshape_export {

namespace {
// UpdateAttributesPacket 使用 Attribute（9 字段），与 AddActorPacket 的 SyncedAttribute（4 字段）结构完全不同
sculk::protocol::Attribute makeAttribute(
    std::string name,
    float       minV,
    float       maxV,
    float       curV,
    float       defV = 0.0f,
    std::uint64_t tick = 0
) {
    sculk::protocol::Attribute a;
    a.mName             = std::move(name);
    a.mMinValue         = minV;
    a.mMaxValue         = maxV;
    a.mCurrentValue     = curV;
    a.mDefaultMinValue  = minV;
    a.mDefaultMaxValue  = maxV;
    a.mDefaultValue     = (defV == 0.0f) ? curV : defV;
    a.mModifiers        = {};
    a.mTick             = tick;
    return a;
}
} // namespace

// SculkPacketSend.h 的失败日志落点（模板内 extern 声明, 此处定义）
void logSculkPacketSendFailure(char const* name) {
    static auto log = ll::io::LoggerRegistry::getInstance().getOrCreate("HologramLib");
    log->warn("[CustomEntity] sculk packet validation failed for {}", name);
}

namespace {

auto& logger() {
    static auto log = ll::io::LoggerRegistry::getInstance().getOrCreate("HologramLib");
    return *log;
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

// identifier 规范化: 短名补 minecraft: 前缀; 空串返回空（创建时校验拒绝）
std::string normalizeIdentifier(std::string const& raw) {
    if (raw.empty()) return {};
    if (raw.find(':') != std::string::npos) return raw;
    return "minecraft:" + raw;
}

using Runtime = CustomEntityManager::Runtime;

// 根据 (name, aux, nbt) 构造 BDS 原生 ItemStack；空 name 返回空栈（isNull 判定跳过发包）
::ItemStack buildEquipmentItem(hologramlib::CustomEntityEquipment const& eq) {
    if (eq.name.empty()) return {};
    auto level = ll::service::getLevel();
    if (!level) return {};
    try {
        std::unique_ptr<::CompoundTag> tag;
        if (!eq.nbt.empty()) {
            if (auto parsed = ::CompoundTag::fromSnbt(eq.nbt)) {
                tag = std::make_unique<::CompoundTag>(std::move(*parsed));
            }
        }
        auto tryGet = [&](std::string const& name) -> ::ItemStack {
            auto weak = level->getItemRegistry().getItem(::HashedString(name));
            if (!weak) return {};
            return ::ItemStack(*weak, 1, eq.aux, tag.get());
        };
        ::ItemStack stack = tryGet(eq.name);
        if (stack.isNull()) {
            // 自动补 minecraft: 前缀重试
            auto normalized = eq.name;
            if (normalized.find(':') == std::string::npos) normalized = "minecraft:" + normalized;
            stack = tryGet(std::move(normalized));
        }
        return stack;
    } catch (...) {
        return {};
    }
}

// ── 发包原语 ──

void sendCustomActor(
    Player&                            player,
    CustomEntityConfig const&          data,
    Runtime const&                     rt,
    std::optional<std::uint64_t> const& vehicleUniqueId
) {
    sculk::protocol::AddActorPacket pkt;
    pkt.mActorUniqueId  = static_cast<std::int64_t>(rt.uniqueId);
    pkt.mActorRuntimeId = rt.runtimeId;
    pkt.mIdentifier     = normalizeIdentifier(data.identifier);
    pkt.mPosition       = {data.x, data.y, data.z};
    pkt.mVelocity       = {0, 0, 0};
    pkt.mRotation       = {data.pitch, data.yaw}; // Vec2{pitch, yaw} 与 BDS ActorRotation 同序
    pkt.mYHeadRotation  = data.yaw;
    pkt.mYBodyRotation  = data.yaw;
    pkt.mActorLinks     = {};

    // flags 合成: 原始位掩码 | 隐身便捷位(0x20)
    std::int64_t const flags = data.flags | (data.invisible ? (std::int64_t(1) << 5) : 0);

    pkt.mMetaData.mDataItems.clear();
    pkt.mMetaData.mDataItems.push_back(
        {sculk::protocol::ActorDataIDs::Reserved0, flags}
    );
    if (!data.nametag.empty()) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::Name, data.nametag}
        );
    }
    // NametagAlwaysShow (ID=81):
    //   BDS 实际语义: 0 → 名字牌常显（不看距离/准星）, 1 → 仅准星对准才显示
    //   即协议值与直觉相反（2026-08-27 用户实测复验: 旧映射 true→1 表现为开关反了）
    pkt.mMetaData.mDataItems.push_back(
        {sculk::protocol::ActorDataIDs::NametagAlwaysShow,
         static_cast<std::int32_t>(data.nametagAlwaysShow ? 0 : 1)}
    );
    if (data.variant != 0) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::Variant, static_cast<std::int32_t>(data.variant)}
        );
    }
    if (data.markVariant != 0) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::MarkVariant, static_cast<std::int32_t>(data.markVariant)}
        );
    }
    if (data.colorIndex != 0) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::ColorIndex, static_cast<std::uint8_t>(data.colorIndex)}
        );
    }
    // 盔甲架/玩家姿态（PoseIndex 0..13）
    if (data.pose != 0) {
        pkt.mMetaData.mDataItems.push_back(
            {sculk::protocol::ActorDataIDs::PoseIndex, static_cast<std::int32_t>(data.pose)}
        );
    }

    // attributes: health 基础 + minecraft:scale 缩放（客户端有效域 0.0625~10）
    // SyncedAttribute = {name, mMinValue, mMaxValue, mCurrentValue}
    float const scale = std::clamp(data.scale, 0.0625f, 10.0f);
    pkt.mAttributes = {
        {"minecraft:health", 0.0f,     20.0f,      20.0f},
        {"minecraft:scale",  0.0625f,  10.0f,      scale},
    };
    pkt.mSynchedProperties = {};
    sendSculkPacketToPlayer(player, pkt);

    // ── BUGFIX: AddActor 后的 UpdateAttributesPacket 二次确认
    // BDS 客户端对 AddActorPacket 里携带的 attributes 仅初始化不生效，
    // 需要随后的 UpdateAttributesPacket 再"应用"一次，scale 才会真正渲染。
    {
        float const s = std::clamp(data.scale, 0.0625f, 10.0f);
        sculk::protocol::UpdateAttributesPacket uap;
        uap.mActorRuntimeId = rt.runtimeId;
        uap.mAttributes = {
            makeAttribute("minecraft:health", 0.0f,    20.0f,   20.0f, 20.0f),
            makeAttribute("minecraft:scale",  0.0625f, 10.0f,   s,     1.0f),
        };
        uap.mTick = currentTick();
        sendSculkPacketToPlayer(player, uap);
    }

    // ── AddActor 之后立即下发装备（空手/空槽跳过，避免无意义流量） ──
    //  槽 0=主手 / 1=副手：MobEquipmentPacket（Inventory / Offhand container）
    for (int slot = 0; slot <= 1; ++slot) {
        auto stack = buildEquipmentItem(data.equipment[slot]);
        if (stack.isNull()) continue;
        ::ActorRuntimeID runtimeId{rt.runtimeId};
        // 副手 (slot=1) 使用 Offhand container; 主手 (slot=0) Inventory
        auto cid = (slot == 1) ? ::ContainerID::Offhand : ::ContainerID::Inventory;
        ::MobEquipmentPacket equipPkt{
            runtimeId,
            stack,
            static_cast<unsigned char>(slot), // slotByte
            0,                                  // selectedSlot（默认0即可）
            cid
        };
        equipPkt.sendTo(player);
    }
    //  槽 2=head / 3=chest / 4=legs / 5=feet：MobArmorEquipmentPacket 一次发包四个位置 + body
    {
        auto head  = buildEquipmentItem(data.equipment[2]);
        auto torso = buildEquipmentItem(data.equipment[3]);
        auto legs  = buildEquipmentItem(data.equipment[4]);
        auto feet  = buildEquipmentItem(data.equipment[5]);
        // 四个槽全空 → 省一次发包（body 永久空当前未使用）
        if (!head.isNull() || !torso.isNull() || !legs.isNull() || !feet.isNull()) {
            // 无 6 参构造器: 默认构造（五槽全空）后逐槽赋值（TypedStorage 经 operator= 转发）
            ::MobArmorEquipmentPacket armorPkt;
            armorPkt.mRuntimeId = ::ActorRuntimeID{rt.runtimeId};
            armorPkt.mHead      = ::NetworkItemStackDescriptor{head};
            armorPkt.mTorso     = ::NetworkItemStackDescriptor{torso};
            armorPkt.mLegs      = ::NetworkItemStackDescriptor{legs};
            armorPkt.mFeet      = ::NetworkItemStackDescriptor{feet};
            armorPkt.sendTo(player);
        }
    }

    // ── 骑乘链接（1.12.0）: AddActor 之后重放 SetActorLinkPacket ──
    // 载具 = 玩家或另一自定义实体; ghost 恒为乘客（B 端）
    if (vehicleUniqueId) {
        sculk::protocol::SetActorLinkPacket linkPkt;
        linkPkt.mLink.mType                   = sculk::protocol::ActorLinkType::Riding;
        linkPkt.mLink.mTargetAUniqueId        = static_cast<std::int64_t>(*vehicleUniqueId); // 载具
        linkPkt.mLink.mTargetBUniqueId        = static_cast<std::int64_t>(rt.uniqueId);     // 乘客
        linkPkt.mLink.mImmediate              = true;
        linkPkt.mLink.mPassengerInitiated     = false;
        linkPkt.mLink.mVehicleAngularVelocity = 0.0f;
        sendSculkPacketToPlayer(player, linkPkt);
    }
}

void sendRemove(Player& player, Runtime const& rt) {
    sculk::protocol::RemoveActorPacket pkt;
    pkt.mActorUniqueId = static_cast<std::int64_t>(rt.uniqueId);
    sendSculkPacketToPlayer(player, pkt);
}

// ── 生成/销毁完整序列 ──

bool spawnForPlayer(
    int64_t                                   id,
    CustomEntityConfig const&                 data,
    Runtime&                                  rt,
    Player&                                   player,
    std::optional<std::uint64_t> const&       vehicleUniqueId
) {
    // Level 未就绪: 不缓存不告警, 下个 tick 重试
    if (!ll::service::getLevel()) return false;

    sendCustomActor(player, data, rt, vehicleUniqueId);
    logger().debug(
        "[CustomEntity] spawned #{} '{}' at ({:.1f},{:.1f},{:.1f}) dim={} scale={:.3f} tag='{}'",
        id,
        data.identifier,
        data.x,
        data.y,
        data.z,
        data.dimension,
        data.scale,
        data.nametag
    );
    return true;
}

void despawnForPlayer(Runtime const& rt, Player& player) { sendRemove(player, rt); }

} // namespace

// ── Manager 公开实现 ──

CustomEntityManager& CustomEntityManager::getInstance() {
    static CustomEntityManager instance;
    return instance;
}

void CustomEntityManager::init() {
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

void CustomEntityManager::shutdown() {
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
    mDirtyIds.clear();
    mLightDirtyIds.clear();
    mVisibleFilter.clear();
    mAnimQueue.clear();
    mInitializedPlayers.clear();
}

int64_t CustomEntityManager::create(CustomEntityConfig const& config) {
    std::lock_guard lock(mMutex);
    return createLocked(config, mNextId++);
}

int64_t CustomEntityManager::createRandom(CustomEntityConfig const& config) {
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

int64_t CustomEntityManager::createWithId(CustomEntityConfig const& config, int64_t desiredId) {
    if (desiredId <= 0) return -2;
    std::lock_guard lock(mMutex);
    if (mConfigs.contains(desiredId)) return -2;
    // 防自增游标未来撞上该 ID
    if (desiredId >= mNextId && desiredId < 0x10000000) mNextId = desiredId + 1;
    return createLocked(config, desiredId);
}

bool CustomEntityManager::isIdUsed(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mConfigs.contains(id);
}

int64_t CustomEntityManager::createLocked(CustomEntityConfig const& config, int64_t id) {
    // identifier 必须非空（客户端无类型可渲染的实体会被丢弃）
    if (config.identifier.empty()) {
        logger().warn("[CustomEntity] create rejected: empty identifier");
        return -1;
    }

    // 生成本次运行的唯一 Actor ID（0x6E 高位段, 与真实实体及 0x6D 物品域隔离）
    Runtime rt{};
    rt.uniqueId  = allocUniqueIdLocked();
    rt.runtimeId = allocRuntimeIdLocked();

    mConfigs.emplace(id, config);
    mRuntimes.emplace(id, std::move(rt));

    syncVisibilityLocked();
    return id;
}

std::uint64_t CustomEntityManager::allocUniqueIdLocked() { return mNextActorUniqueId++; }

std::uint64_t CustomEntityManager::allocRuntimeIdLocked() { return mNextRuntimeId++; }

bool CustomEntityManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);

    mDirtyIds.erase(id);
    mLightDirtyIds.erase(id);
    mVisibleFilter.erase(id);
    // 清理该实体残留在动画队列中的条目（防 despawn 后 AnimateEntityPacket 迟到）
    for (auto it = mAnimQueue.begin(); it != mAnimQueue.end();) {
        if (it->second.entityId == id) it = mAnimQueue.erase(it);
        else ++it;
    }
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

void CustomEntityManager::destroyAll() {
    std::lock_guard lock(mMutex);
    mDirtyIds.clear();
    mLightDirtyIds.clear();
    mVisibleFilter.clear();
    mAnimQueue.clear();
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

bool CustomEntityManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mConfigs.contains(id);
}

bool CustomEntityManager::get(int64_t id, CustomEntityConfig& out) const {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    out = it->second;
    return true;
}

bool CustomEntityManager::setIdentifier(int64_t id, std::string const& identifier) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (identifier.empty()) return false; // 空 identifier 拒绝（客户端无法渲染）
    it->second.identifier = identifier;
    mDirtyIds.insert(id); // 类型变更需整体 respawn
    return true;
}

bool CustomEntityManager::setPosition(int64_t id, float x, float y, float z, int dim) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.x = x;
    it->second.y = y;
    it->second.z = z;
    bool const dimChanged = (dim >= 0 && it->second.dimension != dim);
    if (dim >= 0) it->second.dimension = dim;
    // 维度变更影响可见性 → 重脏(走 respawn+可见性重算)；否则仅坐标→轻脏(增量 MoveActorAbsolute)
    if (dimChanged) mDirtyIds.insert(id);
    else            mLightDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setRotation(int64_t id, float yaw, float pitch) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.yaw   = yaw;
    it->second.pitch = pitch;
    mLightDirtyIds.insert(id); // 增量 MoveActorAbsolute 可直接刷新朝向
    return true;
}

bool CustomEntityManager::setNametag(int64_t id, std::string const& text) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.nametag = text;
    mLightDirtyIds.insert(id); // 增量 SetActorData 刷新 Name 元数据
    return true;
}

bool CustomEntityManager::setScale(int64_t id, float scale) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (scale <= 0) return false;
    it->second.scale = scale;
    mLightDirtyIds.insert(id); // 增量 UpdateAttributesPacket 应用 scale
    return true;
}

bool CustomEntityManager::setVariant(int64_t id, int variant) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.variant = variant;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setMarkVariant(int64_t id, int markVariant) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.markVariant = markVariant;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setColorIndex(int64_t id, int colorIndex) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.colorIndex = colorIndex;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setFlags(int64_t id, int64_t flags) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.flags = flags;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setInvisible(int64_t id, bool on) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (it->second.invisible == on) return true; // 幂等
    it->second.invisible = on;
    mLightDirtyIds.insert(id); // 增量 SetActorData 刷新 Reserved0 flags
    return true;
}

bool CustomEntityManager::setEnabled(int64_t id, bool enabled) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.enabled = enabled;
    mDirtyIds.insert(id); // 开关影响可见性, tick 脏刷新时统一重算
    return true;
}

bool CustomEntityManager::setViewDistance(int64_t id, double dist) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    it->second.viewDistance = dist;
    mDirtyIds.insert(id); // 视距影响可见性, tick 脏刷新时统一重算
    return true;
}

bool CustomEntityManager::setPose(int64_t id, int pose) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (pose < 0) pose = 0;
    if (pose > 13) pose = 13;
    it->second.pose = pose;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::setEquipmentSlot(int64_t id, int slot, std::string const& name, int aux, std::string const& nbt) {
    std::lock_guard lock(mMutex);
    if (slot < 0 || slot > 5) return false;
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (name.empty()) {
        it->second.equipment[slot] = {};
    } else {
        it->second.equipment[slot].name = name;
        it->second.equipment[slot].aux  = aux;
        it->second.equipment[slot].nbt  = nbt;
    }
    mDirtyIds.insert(id);
    return true;
}

// ── 1.12.0 追加 ──

bool CustomEntityManager::scaleBy(int64_t id, double factor) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (factor <= 0) return false;
    it->second.scale =
        std::clamp(static_cast<float>(it->second.scale * factor), 0.0625f, 10.0f);
    mLightDirtyIds.insert(id); // 增量 UpdateAttributesPacket 应用 scale
    return true;
}

bool CustomEntityManager::setVisiblePlayers(int64_t id, std::vector<std::string> const& playerNames) {
    std::lock_guard lock(mMutex);
    if (!mConfigs.contains(id)) return false;

    if (playerNames.empty()) {
        mVisibleFilter.erase(id); // 空列表 = 清除限制 = 全员可见
    } else {
        mVisibleFilter.insert_or_assign(
            id, std::unordered_set<std::string>(playerNames.begin(), playerNames.end())
        );
    }

    // 白名单变化只影响"给谁看", 不改变实体参数 —— 直接重算可见性
    // （对名单外已见玩家 despawn, 名单内新玩家 spawn）
    syncVisibilityLocked();
    return true;
}

bool CustomEntityManager::clearVisiblePlayers(int64_t id) {
    std::lock_guard lock(mMutex);
    if (!mConfigs.contains(id)) return false;
    if (mVisibleFilter.erase(id) == 0) return true; // 本就无限制, 视为成功
    syncVisibilityLocked();
    return true;
}

bool CustomEntityManager::setVisiblePlayer(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    if (!mConfigs.contains(id)) return false;

    mVisibleFilter.insert_or_assign(id, std::unordered_set<std::string>{playerName});

    // 与 setVisiblePlayers 一致: 名单变化直接重算可见性
    syncVisibilityLocked();
    return true;
}

std::string CustomEntityManager::getDebugInfo(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return "not_found";

    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return "no_runtime";

    auto fit = mVisibleFilter.find(id);
    std::string filterList;
    if (fit != mVisibleFilter.end()) {
        for (auto const& n : fit->second) {
            if (!filterList.empty()) filterList += ",";
            filterList += n;
        }
    }
    auto& d = it->second;
    int   equipCount = 0;
    for (auto const& eq : d.equipment) {
        if (!eq.name.empty()) ++equipCount;
    }
    std::string ride = "none";
    if (!d.ridePlayerName.empty()) {
        ride = "player:" + d.ridePlayerName;
    } else if (d.rideEntityId != 0) {
        ride = "entity:" + std::to_string(d.rideEntityId);
    }
    return std::format(
        "id={} dim={} pos=({:.1f},{:.1f},{:.1f}) identifier='{}' enabled={} scale={:.3f} pose={} "
        "equip={} view={} filter=[{}] shown={} ride={}",
        id,
        d.dimension,
        d.x,
        d.y,
        d.z,
        d.identifier,
        d.enabled,
        d.scale,
        d.pose,
        equipCount,
        d.viewDistance,
        filterList,
        rit->second.shownPlayers.size(),
        ride
    );
}

bool CustomEntityManager::setRidePlayer(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;

    if (playerName.empty()) {
        // 空名 = 清除链接
        if (it->second.ridePlayerName.empty() && it->second.rideEntityId == 0) return true;
        it->second.ridePlayerName.clear();
        it->second.rideEntityId = 0;
        mDirtyIds.insert(id); // respawn 重放（新实体不带链接）
        return true;
    }

    // 玩家须在线（链接包需要其 ActorUniqueID）
    auto level = ll::service::getLevel();
    if (!level || !level->getPlayer(playerName)) return false;

    it->second.ridePlayerName = playerName;
    it->second.rideEntityId   = 0; // 互斥: 后设者生效
    mDirtyIds.insert(id);          // respawn 换新实体并重放 SetActorLinkPacket
    return true;
}

bool CustomEntityManager::setRideEntity(int64_t id, int64_t vehicleEntityId) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;

    if (vehicleEntityId == 0) {
        // 0 = 清除链接
        if (it->second.ridePlayerName.empty() && it->second.rideEntityId == 0) return true;
        it->second.ridePlayerName.clear();
        it->second.rideEntityId = 0;
        mDirtyIds.insert(id);
        return true;
    }

    if (vehicleEntityId == id) return false;              // 不能骑自己
    if (!mRuntimes.contains(vehicleEntityId)) return false; // 载具实体须存在

    it->second.rideEntityId   = vehicleEntityId;
    it->second.ridePlayerName.clear(); // 互斥: 后设者生效
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::clearRide(int64_t id) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (it->second.ridePlayerName.empty() && it->second.rideEntityId == 0) return true;
    it->second.ridePlayerName.clear();
    it->second.rideEntityId = 0;
    mDirtyIds.insert(id);
    return true;
}

bool CustomEntityManager::playAnimation(
    int64_t id, std::string const& animation, std::string const& stopExpression, int durationTicks
) {
    std::lock_guard lock(mMutex);
    auto it = mConfigs.find(id);
    if (it == mConfigs.end()) return false;
    if (animation.empty()) return false;
    auto rit = mRuntimes.find(id);
    if (rit == mRuntimes.end()) return false;
    if (!it->second.enabled || rit->second.shownPlayers.empty()) return false;

    auto const now  = currentTick();
    auto const ctrl = "wiki.customentity." + std::to_string(id); // 稳定名: respawn 同名原地覆盖, 零注册表泄漏
    for (auto const& uuid : rit->second.shownPlayers) {
        mAnimQueue.emplace(
            now + 2, // 与 ItemDisplay 同节奏: 入队 +2 tick 后发出
            EntityAnimEntry{uuid, rit->second.runtimeId, id, animation, ctrl, stopExpression}
        );
    }
    // 限时动画: 到期补发一包 stopExpression="query.any_animation"（立即为真 → 停止）
    if (durationTicks > 0) {
        for (auto const& uuid : rit->second.shownPlayers) {
            mAnimQueue.emplace(
                now + 2 + static_cast<std::uint64_t>(durationTicks),
                EntityAnimEntry{uuid, rit->second.runtimeId, id, animation, ctrl, "query.any_animation"}
            );
        }
    }
    return true;
}

bool CustomEntityManager::findByRuntimeId(std::uint64_t runtimeId, int64_t& outId) const {
    std::lock_guard lock(mMutex);
    for (auto const& [id, rt] : mRuntimes) {
        if (rt.runtimeId == runtimeId) {
            outId = id;
            return true;
        }
    }
    return false;
}

std::vector<int64_t> CustomEntityManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    std::vector<int64_t> ids;
    ids.reserve(mConfigs.size());
    for (auto const& [id, cfg] : mConfigs) ids.push_back(id);
    return ids;
}

int64_t CustomEntityManager::findNearest(float x, float y, float z, int dim, double maxDist) const {
    std::lock_guard lock(mMutex);
    int64_t best      = -1;
    double  bestDist2 = (maxDist > 0) ? maxDist * maxDist : 1e300;
    for (auto const& [id, cfg] : mConfigs) {
        if (cfg.dimension != dim) continue;
        double const dx    = cfg.x - x;
        double const dy    = cfg.y - y;
        double const dz    = cfg.z - z;
        double const dist2 = dx * dx + dy * dy + dz * dz;
        if (dist2 < bestDist2) {
            bestDist2 = dist2;
            best      = id;
        }
    }
    return best;
}

std::optional<std::uint64_t> CustomEntityManager::resolveVehicleUniqueIdLocked(CustomEntityConfig const& data) {
    if (!data.ridePlayerName.empty()) {
        auto level = ll::service::getLevel();
        if (!level) return std::nullopt;
        if (auto* player = level->getPlayer(data.ridePlayerName)) {
            return static_cast<std::uint64_t>(player->getOrCreateUniqueID().rawID);
        }
        return std::nullopt;
    }
    if (data.rideEntityId != 0) {
        auto it = mRuntimes.find(data.rideEntityId);
        if (it != mRuntimes.end()) return it->second.uniqueId;
    }
    return std::nullopt;
}

void CustomEntityManager::flushAnimsLocked() {
    if (mAnimQueue.empty()) return;
    auto const now = currentTick();
    for (auto it = mAnimQueue.begin(); it != mAnimQueue.end() && it->first <= now;) {
        auto& e = it->second;
        // 发送前活性验证: 实体已 despawn / respawn 换 ID 的条目直接丢弃
        bool alive = false;
        auto rit   = mRuntimes.find(e.entityId);
        if (rit != mRuntimes.end()) {
            alive = rit->second.runtimeId == e.runtimeId
                 && rit->second.shownPlayers.contains(e.playerUuid);
        }
        if (alive) {
            if (auto* player = findPlayerByUuid(e.playerUuid)) {
                sculk::protocol::AnimateEntityPacket pkt;
                pkt.mAnimation                   = e.animation;
                pkt.mNextState                   = "none";
                pkt.mStopExpression              = e.stopExpression;
                pkt.mStopExpressionMolangVersion = sculk::protocol::MolangVersion::Initial;
                pkt.mController                  = e.controller;
                pkt.mBlendOutTime                = 0;
                pkt.mRuntimeIds                  = {e.runtimeId};
                sendSculkPacketToPlayer(*player, pkt);
            }
        }
        it = mAnimQueue.erase(it);
    }
}

void CustomEntityManager::refreshLocked(int64_t id) {
    // 参数变化后刷新所有已见玩家（despawn → respawn 原子替换）
    //
    // respawn 必须更换全新 uniqueId/runtimeId（与 ItemDisplay 同源结论）:
    // 客户端实体删除是帧末延迟销毁, 同帧/临近帧 RemoveActor(旧ID)+AddActor(同ID)
    // 会触发客户端 runtimeId 重映射未定义行为。换新 ID 后是纯粹的
    // "删旧实体 + 建全新实体", 无 ID 复用。
    auto it  = mConfigs.find(id);
    auto rit = mRuntimes.find(id);
    if (it == mConfigs.end() || rit == mRuntimes.end()) return;

    auto& rt    = rit->second;
    auto  uuids = std::vector<mce::UUID>{rt.shownPlayers.begin(), rt.shownPlayers.end()};

    std::vector<Player*> toSpawn;
    bool                 replaced = false;
    for (auto const& uuid : uuids) {
        auto* player = findPlayerByUuid(uuid);
        if (!player) {
            rt.shownPlayers.erase(uuid);
            continue;
        }
        despawnForPlayer(rt, *player);
        rt.shownPlayers.erase(uuid);
        replaced = true;
        if (it->second.enabled) toSpawn.push_back(player);
    }

    if (replaced) {
        rt.uniqueId  = allocUniqueIdLocked();
        rt.runtimeId = allocRuntimeIdLocked();
    }

    for (auto* player : toSpawn) {
        if (spawnForPlayer(id, it->second, rt, *player, resolveVehicleUniqueIdLocked(it->second))) {
            rt.shownPlayers.insert(player->getUuid());
        }
    }
}

// ── 轻脏增量刷新（不换 uniqueId/runtimeId, 不发 RemoveActor, 客户端零闪烁）
// 适用: 每 tick 动画驱动的 setPosition/setRotation/setScale/setNametag/setInvisible
// 发包: MoveActorAbsolutePacket (坐标+朝向)
//       SetActorDataPacket     (名字牌+flags)
//       UpdateAttributesPacket (scale attribute 应用)
void CustomEntityManager::refreshLightLocked(int64_t id) {
    auto it  = mConfigs.find(id);
    auto rit = mRuntimes.find(id);
    if (it == mConfigs.end() || rit == mRuntimes.end()) return;
    auto const& data = it->second;
    auto&       rt   = rit->second;

    if (!data.enabled || rt.shownPlayers.empty()) return;

    // tick 序号（SetActorData / UpdateAttributes 均需带当前 tick）
    auto const tick = currentTick();

    // 将角度映射到 MoveActorAbsolute 的字节旋转 (0..360° → 0..255)
    auto rotToByte = [](float deg) -> std::uint8_t {
        float d = std::fmod(deg, 360.0f);
        if (d < 0) d += 360.0f;
        return static_cast<std::uint8_t>((d * 256.0f) / 360.0f);
    };

    for (auto const& uuid : rt.shownPlayers) {
        auto* player = findPlayerByUuid(uuid);
        if (!player) continue;

        // 1) 坐标+朝向增量
        //    mHeader 标志位: bit0(0x01)=OnGround bit1(0x02)=Teleport bit2(0x04)=ForceMove
        //    Teleport 使客户端忽略碰撞/插值直接落位（动画逐 tick 覆盖所需的语义）
        {
            sculk::protocol::MoveActorAbsolutePacket maap;
            maap.mActorRuntimeId = rt.runtimeId;
            maap.mHeader         = 0x02;
            maap.mPosition       = {data.x, data.y, data.z};
            maap.mRotationX     = rotToByte(data.pitch);
            maap.mRotationY     = rotToByte(data.yaw);
            maap.mRotationYHead = rotToByte(data.yaw);
            sendSculkPacketToPlayer(*player, maap);
        }

        // 2) 元数据 (Name / NametagAlwaysShow / flags)
        {
            sculk::protocol::SetActorDataPacket sadp;
            sadp.mActorRuntimeId = rt.runtimeId;
            sadp.mTick           = tick;
            std::int64_t const flags = data.flags | (data.invisible ? (std::int64_t(1) << 5) : 0);
            sadp.mMetaData.mDataItems.push_back(
                {sculk::protocol::ActorDataIDs::Reserved0, flags}
            );
            if (!data.nametag.empty()) {
                sadp.mMetaData.mDataItems.push_back(
                    {sculk::protocol::ActorDataIDs::Name, data.nametag}
                );
            }
            sadp.mMetaData.mDataItems.push_back(
                {sculk::protocol::ActorDataIDs::NametagAlwaysShow,
                 static_cast<std::int32_t>(data.nametagAlwaysShow ? 0 : 1)}
            );
            sendSculkPacketToPlayer(*player, sadp);
        }

        // 3) scale attribute 二次应用
        {
            float const s = std::clamp(data.scale, 0.0625f, 10.0f);
            sculk::protocol::UpdateAttributesPacket uap;
            uap.mActorRuntimeId = rt.runtimeId;
            uap.mAttributes = {
                makeAttribute("minecraft:health", 0.0f,    20.0f,   20.0f, 20.0f, tick),
                makeAttribute("minecraft:scale",  0.0625f, 10.0f,   s,     1.0f,  tick),
            };
            uap.mTick = tick;
            sendSculkPacketToPlayer(*player, uap);
        }
    }
}

void CustomEntityManager::processDirtyLocked() {
    // tick 内合并处理脏实体
    //  重脏(mDirtyIds): respawn 换新 ID (identifier/pose/equipment/flags 等结构变更)
    //  轻脏(mLightDirtyIds): 增量包 (每 tick 动画只走这里, 零闪烁)
    // 同一 ID 同时在重脏集里 → 以 respawn 为准 (respawn 后所有属性是最新 config, 轻脏已覆盖)

    // playAnimation 入队条目（now+2 起步）先于本次脏刷新发出（与 ItemDisplay 同节奏）
    flushAnimsLocked();

    bool needVisibility = false;
    if (!mDirtyIds.empty()) {
        auto dirty = std::move(mDirtyIds);
        mDirtyIds.clear();
        for (auto id : dirty) {
            if (!mConfigs.contains(id)) continue;
            needVisibility = true;
            // 同一 ID 同时也在轻脏里: respawn 后 config 已是最新, 轻脏无需再发
            mLightDirtyIds.erase(id);
            refreshLocked(id);
        }
    }
    if (!mLightDirtyIds.empty()) {
        auto light = std::move(mLightDirtyIds);
        mLightDirtyIds.clear();
        for (auto id : light) {
            if (!mConfigs.contains(id)) continue;
            refreshLightLocked(id);
        }
    }
    if (needVisibility) syncVisibilityLocked();
}

void CustomEntityManager::syncVisibilityLocked() {
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

            // 可见玩家白名单（1.12.0）: 有条目时仅名单内玩家可见（按 Player::getRealName, 即 LSE realName 匹配）
            auto fit = mVisibleFilter.find(id);
            bool const allowedByFilter =
                fit == mVisibleFilter.end() || fit->second.contains(player.getRealName());

            bool const visible =
                data.enabled && allowedByFilter && dimId == DimensionType(data.dimension)
                && (data.viewDistance <= 0
                    || ((ppos.x - data.x) * (ppos.x - data.x) + (ppos.y - data.y) * (ppos.y - data.y)
                        + (ppos.z - data.z) * (ppos.z - data.z))
                           <= data.viewDistance * data.viewDistance);

            if (visible && !rt.shownPlayers.contains(uuid)) {
                if (spawnForPlayer(id, data, rt, player, resolveVehicleUniqueIdLocked(data))) {
                    rt.shownPlayers.insert(uuid);
                }
            } else if (!visible && rt.shownPlayers.contains(uuid)) {
                despawnForPlayer(rt, player);
                rt.shownPlayers.erase(uuid);
            }
        }
        return true;
    });
}

//脏刷新周期同步

// Manager 私有访问桥（friend 声明于 Manager 头文件）
struct CustomEntityTickHookAccess {
    static void sync(CustomEntityManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.syncVisibilityLocked();
    }
    static void processDirty(CustomEntityManager& mgr) {
        std::lock_guard lock(mgr.mMutex);
        mgr.processDirtyLocked();
    }
};

LL_TYPE_INSTANCE_HOOK(CustomEntityTickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();

    // 脏刷新: 本 tick 内累积的 setter 变更合并为单次 respawn
    CustomEntityTickHookAccess::processDirty(CustomEntityManager::getInstance());

    auto const now = currentTick();
    static std::uint64_t lastSyncTick = 0;
    if (now - lastSyncTick >= 20) {
        lastSyncTick = now;
        CustomEntityTickHookAccess::sync(CustomEntityManager::getInstance());
    }
}

// hook 生命周期（静态注册即可; 无实体时为空转 no-op）
static ll::memory::HookRegistrar<CustomEntityTickHook> gCustomEntityTickHookRegistrar;

} // namespace debugshape_export