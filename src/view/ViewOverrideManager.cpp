// ViewOverrideManager.cpp - 客户端视图覆盖: 覆盖表 + 包改写
//
// 改写的总原则: **能原包改的就原包改, 改不了的交 runPostActions 补发**。
//   AddActor      → mActorType（换类型）/ mData（名字牌）/ 整个丢弃（隐藏）
//   AddPlayer     → mUnpack（名字牌）/ 整个丢弃（隐藏）
//   SetActorData  → mPackedItems 里我们负责的那两项（服务端改名不覆盖我们的视图）
//   UpdateBlock*  → mRuntimeId（换成覆盖方块的网络 id）
//   FullChunkData → 不改包; 记下"这一包之后要把覆盖范围内的方块补发一遍"（区块会冲掉覆盖）
//
// 所有改写都是"按 id 找项 → 替换", 天然幂等: 同一条包被多条发包路径各套用一次,
// 结果与套用一次相同（广播逐收件人时靠 Applied 里的原值还原隔离）。

#include "ViewOverrideManager.h"

#include "ViewPackets.h"

#include "customentity/CustomEntityManager.h" // getIdPair（库内接口, 公共头没暴露）

#include "DiagLog.h"

#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/network/MinecraftPacketIds.h>
#include <mc/network/Packet.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/network/packet/AddActorPacketPayload.h>
#include <mc/network/packet/AddPlayerPacketPayload.h>
#include <mc/network/packet/LevelChunkPacketPayload.h>
#include <mc/network/packet/MovePlayerPacketPayload.h>
#include <mc/network/packet/SetActorDataPacketPayload.h>
#include <mc/network/packet/UpdateBlockPacketPayload.h>
#include <mc/network/packet/UpdateBlockSyncedPacketPayload.h>
#include <mc/deps/nbt/CompoundTag.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/ActorFlags.h>
#include <sculk/protocol/codec/packet/ActorEventPacket.hpp>
#include <sculk/protocol/codec/packet/AnimatePacket.hpp>
#include <mc/world/actor/player/Inventory.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/actor/ActorDataIDs.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/block/Block.h>

#include <cmath>
#include <format>
#include <optional>
#include <utility>

namespace debugshape_export::view {

namespace {

// ── 逻辑层写死的数值必须与引擎一致（对不上就编译失败, 不会静默错位）──
static_assert(logic::kAddPlayer == static_cast<int>(::MinecraftPacketIds::AddPlayer));
static_assert(logic::kAddActor == static_cast<int>(::MinecraftPacketIds::AddActor));
static_assert(logic::kRemoveActor == static_cast<int>(::MinecraftPacketIds::RemoveActor));
static_assert(logic::kUpdateBlock == static_cast<int>(::MinecraftPacketIds::UpdateBlock));
static_assert(logic::kSetActorData == static_cast<int>(::MinecraftPacketIds::SetActorData));
static_assert(logic::kMovePlayer == static_cast<int>(::MinecraftPacketIds::MovePlayer));
// 注意: BDS 里 id 58 的枚举名叫 FullChunkData, 类名却叫 LevelChunkPacket
static_assert(logic::kLevelChunk == static_cast<int>(::MinecraftPacketIds::FullChunkData));
static_assert(logic::kUpdateBlockSynced == static_cast<int>(::MinecraftPacketIds::UpdateBlockSynced));

static_assert(static_cast<int>(::ActorDataIDs::Name) == 4);
static_assert(static_cast<int>(::ActorDataIDs::NametagAlwaysShow) == 81);

// 元数据表: 出生包(AddActor/AddPlayer)与元数据包(SetActorData)都是 vector<unique_ptr<DataItem>>
using DataList = std::vector<std::unique_ptr<::DataItem>>;

// BDS 的包类都是 ll::PayloadPacket<T>（公有继承 Packet 与载荷 T）:
// 从 Packet& 拿载荷要"先降到 PayloadPacket<T>, 再升到 T" —— 不能一步 static_cast
// （Packet 到 T 是兄弟基类关系, 编译器不认）。调用方按 packet.getId() 判过类型, 不会认错。
template <typename PayloadT>
PayloadT& payloadOf(::Packet& packet) {
    return static_cast<PayloadT&>(static_cast<::ll::PayloadPacket<PayloadT>&>(packet));
}

DataList& dataListOf(::AddActorPacketPayload& payload) { return unwrap(unwrap(payload.mData).mData); }
DataList& dataListOf(::AddPlayerPacketPayload& payload) { return unwrap(unwrap(payload.mUnpack).mData); }
DataList& dataListOf(::SetActorDataPacketPayload& payload) { return unwrap(payload.mPackedItems); }

// 按 id 覆盖一项（没有就追加）—— 幂等的关键: 认 id 不认位置
template <typename T>
void setItemById(DataList& items, ::ActorDataIDs id, T&& value) {
    auto const wanted = static_cast<::DataItem::ID>(id);
    for (auto& item : items) {
        if (item && item->getId() == wanted) {
            item = ::DataItem::create(id, std::forward<T>(value));
            return;
        }
    }
    items.push_back(::DataItem::create(id, std::forward<T>(value)));
}

std::vector<std::unique_ptr<::DataItem>> cloneItems(DataList const& items) {
    std::vector<std::unique_ptr<::DataItem>> out;
    out.reserve(items.size());
    for (auto const& item : items) out.push_back(item ? item->clone() : nullptr);
    return out;
}

void restoreItems(DataList& items, std::vector<std::unique_ptr<::DataItem>> const& saved) {
    items = cloneItems(saved); // 再克隆一份: 保存的那一份要留给下个收件人还原
}

// 方块类型名 → 网络 id（UpdateBlockPacket 要的是网络 id, 不是名字）
std::optional<std::uint32_t> networkIdOfBlockType(std::string const& type) {
    auto block = ::Block::tryGetFromRegistry(::HashedString{type});
    if (!block) return std::nullopt;
    return static_cast<std::uint32_t>(unwrap(block->mNetworkId));
}

// 该位置真实方块的网络 id（撤销覆盖时把客户端那一格改回真实方块）
std::uint32_t realBlockNetworkIdAt(::Player& player, int x, int y, int z) {
    auto const& block = player.getDimensionBlockSource().getBlock(::BlockPos{x, y, z});
    return static_cast<std::uint32_t>(unwrap(block.mNetworkId));
}

// 遍历这次操作要影响的玩家（playerName 空串 = 所有在线玩家）
template <typename Fn>
void forEachTargetPlayer(std::string const& playerName, Fn&& fn) {
    auto level = ::ll::service::getLevel();
    if (!level) return;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (playerName.empty() || player.getRealName() == playerName) fn(player);
        return true;
    });
}

int dimensionOfPlayer(::Player& player) { return static_cast<int>(player.getDimensionId()); }

// 记录覆盖时目标玩家所在维度（全局覆盖 / 玩家不在线 = 不限）
int dimensionOfTarget(std::string const& playerName) {
    if (playerName.empty()) return logic::kAnyDimension;
    int dim = logic::kAnyDimension;
    forEachTargetPlayer(playerName, [&](::Player& player) { dim = dimensionOfPlayer(player); });
    return dim;
}

::Actor* actorByRuntimeId(std::uint64_t runtimeId) {
    auto level = ::ll::service::getLevel();
    if (!level) return nullptr;
    return level->getRuntimeEntity(::ActorRuntimeID{runtimeId}, false);
}

// 代理生物的 y 锚点。
//
// 关键: **玩家在协议层上报的 y 在眼睛处**（Bedrock 特有; 脚 = y - 眼高, 站着约 1.62）。
// 代理是普通实体, 它的位置就是脚 —— 所以不能直接用玩家的 y。
// 用服务器对象的碰撞箱下沿取真实脚位（站着/潜行/游泳/爬行都能对, 不用猜常量）。
constexpr float kProxyTrim = 0.0f; // 残余微调（实测偏差已经能由脚位解释; 万一还要挪, 改这里）

float feetAnchorY(::Actor const& actor) { return actor.getAABB().min.y + kProxyTrim; }

// 按 uniqueId 在所有在线玩家里找（**不要用 Level::fetchEntity 找玩家** —— 它不保证覆盖玩家,
// 实测每拍都返回空, 表现就是"被替换成生物的玩家完全不动"）。
::Player* playerByUniqueId(std::int64_t uniqueId) {
    ::Player* found = nullptr;
    forEachTargetPlayer({}, [&](::Player& p) {
        if (p.getOrCreateUniqueID().rawID == uniqueId) found = &p;
    });
    return found;
}

// 输入包里没有碰撞箱, 只有"眼睛处的 y": 用服务器对象求出"眼到脚"的差值再套上去
float feetAnchorYFromInput(::Actor const& actor, float inputY) {
    float const eye  = actor.getPosition().y;
    float const feet = actor.getAABB().min.y;
    return (eye - feet > 0.01f) ? inputY - (eye - feet) : inputY;
}

// 按 uniqueId 找**离这名玩家够近**的实体（够近 = 客户端上多半有它）。
// 太远的不能凭空给它造一只 —— 那会变成服务端不跟踪的"幽灵实体"。
::Actor* liveActorNear(::Player& player, std::int64_t uniqueId, float maxDistance = 64.0f) {
    auto level = ::ll::service::getLevel();
    if (!level) return nullptr;
    auto* actor = level->fetchEntity(::ActorUniqueID{uniqueId}, false);
    if (actor == nullptr) return nullptr;
    return player.distanceTo(*actor) <= maxDistance ? actor : nullptr;
}

} // namespace

ViewOverrideManager& ViewOverrideManager::getInstance() {
    static ViewOverrideManager instance;
    return instance;
}

// ─────────────────────────────────────────────
// 覆盖表
// ─────────────────────────────────────────────

ViewOverrideManager::Table& ViewOverrideManager::ensureTableLocked(std::string const& playerName) {
    if (playerName.empty()) return mGlobal;
    return mPlayers[playerName];
}

ViewOverrideManager::Table const* ViewOverrideManager::tableOfLocked(std::string const& playerName) const {
    if (playerName.empty()) return &mGlobal;
    auto const it = mPlayers.find(playerName);
    return it == mPlayers.end() ? nullptr : &it->second;
}

ViewOverrideManager::EntityRecord*
ViewOverrideManager::findEntityLocked(std::string const& playerName, std::int64_t uniqueId) {
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto const e = it->second.entities.find(uniqueId); e != it->second.entities.end()) return &e->second;
        }
    }
    if (auto const e = mGlobal.entities.find(uniqueId); e != mGlobal.entities.end()) return &e->second;
    return nullptr;
}

ViewOverrideManager::EntityRecord*
ViewOverrideManager::findEntityByRuntimeLocked(std::string const& playerName, std::uint64_t runtimeId) {
    auto lookup = [&](Table& table) -> EntityRecord* {
        auto const idx = table.byRuntime.find(runtimeId);
        if (idx == table.byRuntime.end()) return nullptr;
        auto const e = table.entities.find(idx->second);
        if (e == table.entities.end()) return nullptr;
        // 运行时 id 会被复用（实体死亡 / 玩家换会话）—— 认错了就会把覆盖套到别的实体身上,
        // 所以这里必须回问世界: 这个 runtimeId 现在是不是该 uniqueId 的那只实体
        auto* actor = actorByRuntimeId(runtimeId);
        if (actor == nullptr || actor->getOrCreateUniqueID().rawID != idx->second) return nullptr;
        return &e->second;
    };
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto* rec = lookup(it->second)) return rec;
        }
    }
    return lookup(mGlobal);
}

ViewOverrideManager::BlockRecord* ViewOverrideManager::findBlockLocked(std::string const& playerName, std::int64_t posKey) {
    if (!playerName.empty()) {
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            if (auto const b = it->second.blocks.find(posKey); b != it->second.blocks.end()) return &b->second;
        }
    }
    if (auto const b = mGlobal.blocks.find(posKey); b != mGlobal.blocks.end()) return &b->second;
    return nullptr;
}

void ViewOverrideManager::refreshActiveLocked() {
    // 注意: 这里决定的是"心跳要不要干活" —— 背包镜像与操控**不依赖覆盖表**, 必须一起算进来,
    // 否则它们会因为没有实体/方块覆盖而永远不被驱动（首版就是这个 bug: 操控完全没反应）。
    bool any = !mGlobal.empty();
    if (!any) {
        for (auto const& [name, table] : mPlayers) {
            if (!table.empty()) {
                any = true;
                break;
            }
        }
    }
    mActive.store(any, std::memory_order_relaxed);
}

bool ViewOverrideManager::hasOverridesFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    if (!mGlobal.empty()) return true;
    auto const it = mPlayers.find(playerName);
    return it != mPlayers.end() && !it->second.empty();
}

bool ViewOverrideManager::overrideEntity(
    std::string const&                playerName,
    std::int64_t                      uniqueId,
    hologramlib::EntityView const&    view
) {
    auto normalised        = view;
    normalised.identifier  = logic::normaliseType(view.identifier);
    if (normalised.identifier.empty() && !normalised.hidden && !normalised.hasNametag) {
        // 三项都没设 —— 当作"撤销这个实体的覆盖", 免得留一条什么都不做的记录
        return clearEntity(playerName, uniqueId);
    }

    EntityRecord record;
    bool         pushNametagNow = false;
    bool         needRuntimeId  = false;
    {
        std::lock_guard lock(mMutex);
        auto&           table = ensureTableLocked(playerName);
        auto&           rec   = table.entities[uniqueId];
        rec.uniqueId          = uniqueId;
        rec.view              = normalised;
        if (rec.runtimeKnown) {
            table.byRuntime[rec.runtimeId] = uniqueId;
            pushNametagNow                 = !normalised.hidden && normalised.hasNametag;
        }
        needRuntimeId = !rec.runtimeKnown;
        record        = rec;
        refreshActiveLocked();
    }

    // 还不知道运行时 id（这个实体在这名玩家身上还没出过生）: 直接问一次世界 ——
    // 知道了才能立刻隐藏（RemoveActor）/立刻推名字牌（SetActorData）。问不到就等出生包。
    if (needRuntimeId) {
        if (auto level = ::ll::service::getLevel(); level != nullptr) {
            if (auto* actor = level->fetchEntity(::ActorUniqueID{uniqueId}, false); actor != nullptr) {
                auto const runtimeId = static_cast<std::uint64_t>(actor->getRuntimeID());
                std::lock_guard lock(mMutex);
                auto&           table = ensureTableLocked(playerName);
                auto&           rec   = table.entities[uniqueId];
                rec.runtimeKnown      = true;
                rec.runtimeId         = runtimeId;
                table.byRuntime[runtimeId] = uniqueId;
                record                = rec;
                pushNametagNow        = !normalised.hidden && normalised.hasNametag;
            }
        }
    }

    if (normalised.hidden) {
        // RemoveActor 对"客户端上没有这只实体"是无害的（客户端按 uniqueId 找不到就忽略）,
        // 所以不需要先确认它在不在 —— 直接发, 隐藏立刻生效
        forEachTargetPlayer(playerName, [&](::Player& player) { sendRemoveActor(player, uniqueId); });
    } else if (!normalised.asPlayer.empty() || !normalised.identifier.empty()) {
        forEachTargetPlayer(playerName, [&](::Player& player) {
            auto* actor = liveActorNear(player, uniqueId);
            // 玩家实体: 皮肤走"原版皮肤更新"那条路; 变生物走代理（玩家模型渲染不了生物）
            if (actor != nullptr && actor->isPlayer()) {
                auto& target = *static_cast<::Player*>(actor);
                if (!normalised.asPlayer.empty()) {
                    if (!sendPlayerSkin(player, target, normalised.asPlayer)) {
                        HLIB_LOG_WARN("HologramLib: 换皮肤失败（源玩家不在线?）: {}", normalised.asPlayer);
                    }
                    return;
                }
                if (normalised.identifier != "minecraft:player") {
                    // 替换路线: 把真身从这名观看者客户端移除, 立刻用**同一个 runtimeId/uniqueId**
                    // 发一只该类型的实体; 之后它出生/位移的包都按"替换"处理（见 kAddPlayer / kMovePlayer）。
                    {
                        std::lock_guard lock(mMutex);
                        auto&           table = ensureTableLocked(playerName);
                        auto&           rec   = table.entities[uniqueId];
                        rec.substituted       = true;
                    }
                    sendRemoveActor(player, uniqueId);
                    // 位置/朝向一律来自他的输入包（眼位 − 1.62 = 脚位）; 他还没发过输入就先不发,
                    // 等他下一次出生/输入到位时会由 kAddPlayer 那条分支接住。
                    InputSnapshot snap{};
                    if (inputSnapshotOf(uniqueId, snap)) {
                        sendActorSpawnFromInput(
                            player,
                            snap.runtimeId,
                            uniqueId,
                            normalised.identifier,
                            snap.x,
                            snap.y,
                            snap.z,
                            snap.pitch,
                            snap.yaw,
                            snap.headYaw,
                            normalised.nametag,
                            normalised.hasNametag,
                            normalised.nametagAlwaysShow
                        );
                    }
                    return;
                }
                if (normalised.hasNametag && record.runtimeKnown) {
                    sendNametag(player, record.runtimeId, normalised.nametag, normalised.nametagAlwaysShow);
                }
                return;
            }
            // 非玩家实体: 移除 + 按目标状态重发一只（老路子）
            if (normalised.hidden) {
                sendRemoveActor(player, uniqueId);
            } else if (!normalised.identifier.empty()) {
                if (actor != nullptr) {
                    respawnForPlayer(
                        player,
                        *actor,
                        normalised.identifier,
                        normalised.nametag,
                        normalised.hasNametag,
                        normalised.nametagAlwaysShow
                    );
                } else if (normalised.hasNametag && record.runtimeKnown) {
                    sendNametag(player, record.runtimeId, normalised.nametag, normalised.nametagAlwaysShow);
                }
            } else if (normalised.hasNametag && record.runtimeKnown) {
                sendNametag(player, record.runtimeId, normalised.nametag, normalised.nametagAlwaysShow);
            }
        });
    } else if (pushNametagNow) {
        forEachTargetPlayer(playerName, [&](::Player& player) {
            sendNametag(player, record.runtimeId, normalised.nametag, normalised.nametagAlwaysShow);
        });
    }
    return true;
}

bool ViewOverrideManager::overrideBlock(
    std::string const&             playerName,
    int                            x,
    int                            y,
    int                            z,
    hologramlib::BlockView const&  view
) {
    auto const type = logic::normaliseType(view.type);
    if (type.empty()) return clearBlock(playerName, x, y, z);

    auto const networkId = networkIdOfBlockType(type);
    if (!networkId) {
        HLIB_LOG_WARN("HologramLib: 方块覆盖失败, 类型名不在注册表: {}", type);
        return false;
    }

    auto const key = logic::packPos(x, y, z);
    auto const dim = dimensionOfTarget(playerName);
    {
        std::lock_guard lock(mMutex);
        auto&           table = ensureTableLocked(playerName);
        auto&           rec   = table.blocks[key];
        rec.view              = hologramlib::BlockView{type};
        rec.viewId            = *networkId;
        rec.dim               = dim;
        refreshActiveLocked();
    }

    // 立刻生效: 客户端现在大概率还显示着真实方块, 直接把覆盖方块推给它
    // （玩家不在线时什么都不用做 —— 他进来收到区块时会自动补发）
    forEachTargetPlayer(playerName, [&](::Player& player) {
        sendBlockUpdate(player, x, y, z, *networkId);
    });
    return true;
}

bool ViewOverrideManager::clearEntity(std::string const& playerName, std::int64_t uniqueId) {
    EntityRecord record;
    bool         existed = false;
    {
        std::lock_guard lock(mMutex);
        auto            erase = [&](Table& table) {
            auto const it = table.entities.find(uniqueId);
            if (it == table.entities.end()) return;
            if (it->second.runtimeKnown) table.byRuntime.erase(it->second.runtimeId);
            record  = it->second;
            table.entities.erase(it);
            existed = true;
        };
        if (!playerName.empty()) {
            if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) erase(it->second);
        }
        erase(mGlobal);
        refreshActiveLocked();
    }
    // 撤销不是"把表删了就完事": 客户端上那只实体可能已经被我们移除/换了类型, 得把它恢复原样
    if (existed) {
        forEachTargetPlayer(playerName, [&](::Player& player) { restoreEntityForPlayer(player, uniqueId, record); });
    }
    return existed;
}

bool ViewOverrideManager::clearBlock(std::string const& playerName, int x, int y, int z) {
    auto const key = logic::packPos(x, y, z);
    bool       existed = false;
    {
        std::lock_guard lock(mMutex);
        auto            erase = [&](Table& table) {
            if (table.blocks.erase(key) > 0) existed = true;
        };
        if (!playerName.empty()) {
            if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) erase(it->second);
        }
        erase(mGlobal);
        refreshActiveLocked();
    }
    if (!existed) return false;

    // 把真实方块推回去（按每个玩家自己的维度读, 数值以世界为准）
    forEachTargetPlayer(playerName, [&](::Player& player) {
        sendBlockUpdate(player, x, y, z, realBlockNetworkIdAt(player, x, y, z));
    });
    return true;
}

void ViewOverrideManager::clearAll(std::string const& playerName) {
    std::vector<std::pair<std::int64_t, EntityRecord>> entities;
    std::vector<std::int64_t>                          blockKeys;
    {
        std::lock_guard lock(mMutex);
        auto            collect = [&](Table& table) {
            for (auto const& [uniqueId, record] : table.entities) entities.emplace_back(uniqueId, record);
            for (auto const& [key, record] : table.blocks) blockKeys.push_back(key);
        };
        if (playerName.empty()) {
            collect(mGlobal);
            for (auto& [name, table] : mPlayers) collect(table);
            mPlayers.clear();
            mPulse.clear();
            mGlobal = Table{};
        } else if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) {
            collect(it->second);
            mPlayers.erase(it);
            mPulse.erase(playerName);
            collect(mGlobal); // 全局那份对这一名玩家同样生效, 撤销时也要恢复
        }
        refreshActiveLocked();
    }
    restoreCleared(playerName, entities, blockKeys);
}

void ViewOverrideManager::restoreCleared(
    std::string const&                                        playerName,
    std::vector<std::pair<std::int64_t, EntityRecord>> const& entities,
    std::vector<std::int64_t> const&                          blockKeys
) {
    if (entities.empty() && blockKeys.empty()) return;
    forEachTargetPlayer(playerName, [&](::Player& player) {
        for (auto const key : blockKeys) {
            auto const x = logic::unpackX(key);
            auto const y = logic::unpackY(key);
            auto const z = logic::unpackZ(key);
            sendBlockUpdate(player, x, y, z, realBlockNetworkIdAt(player, x, y, z));
        }
        for (auto const& [uniqueId, record] : entities) restoreEntityForPlayer(player, uniqueId, record);
    });
}

void ViewOverrideManager::respawnForPlayer(
    ::Player&          player,
    ::Actor&           actor,
    std::string const& identifier,
    std::string const& nametag,
    bool               hasNametag,
    bool               alwaysShow
) {
    if (actor.isPlayer()) return; // 玩家出生包（AddPlayer）重建不了: 见 HologramLib.h 的域注释

    auto const uniqueId = actor.getOrCreateUniqueID().rawID;
    if (actor.getTypeName() == identifier) {
        // 本来就是这个类型（撤销时常见）: 不用重建, 只把名字牌状态推回去
        if (hasNametag) {
            sendNametag(player, static_cast<std::uint64_t>(actor.getRuntimeID()), nametag, alwaysShow);
        }
        return;
    }
    sendRemoveActor(player, uniqueId);
    view::sendActorSpawn(player, actor, identifier, nametag, hasNametag, alwaysShow);
}

void ViewOverrideManager::restoreEntityForPlayer(
    ::Player&           player,
    std::int64_t        uniqueId,
    EntityRecord const& record
) {
    // 被"替换成生物"的玩家: 撤销时先把那只假生物从这名观看者客户端移除
    // （真身的出生包不会再补发, 所以他要等重进/换维度才重新看到真实形态 —— 见 HologramLib.h 域注释）
    if (record.substituted) sendRemoveActor(player, uniqueId);

    auto* actor = liveActorNear(player, uniqueId);

    // 换过皮肤: 用他自己的皮肤再发一条, 立刻恢复（这条不依赖真身在不在附近之外的东西）
    if (!record.view.asPlayer.empty() && actor != nullptr && actor->isPlayer()) {
        sendPlayerSkin(player, *static_cast<::Player*>(actor), static_cast<::Player*>(actor)->getRealName());
        return;
    }

    // 隐藏过 / 换过类型: 客户端上这只实体此刻是我们改过的那一版 —— 重发一次真实的
    if (record.view.hidden || !record.view.identifier.empty()) {
        if (actor == nullptr) return; // 已经不在附近了: 客户端上也没有, 等它下次出生自然就是原样
        respawnForPlayer(player, *actor, actor->getTypeName(), actor->getNameTag(), true, false);
        return;
    }

    // 只改过名字牌: 把真实名字牌推回去（真实值从实体上读; 常显状态没有读口, 按"不常显"发,
    // 服务端下次因别的原因重发元数据时会带回真实值）
    if (record.view.hasNametag && actor != nullptr) {
        sendNametag(
            player,
            static_cast<std::uint64_t>(actor->getRuntimeID()),
            actor->getNameTag(),
            false
        );
    }
}


bool ViewOverrideManager::inputSnapshotOf(std::int64_t uniqueId, InputSnapshot& out) const {
    std::lock_guard lock(mMutex);
    for (auto const& [name, snap] : mInputs) {
        if (snap.uniqueId == uniqueId && mServerTick - snap.tick <= 20) {
            out = snap;
            return true;
        }
    }
    return false;
}

void ViewOverrideManager::noteAuthInput(std::string const& playerName, InputSnapshot const& snapshot) {
    // **必须打时间戳**: 驱动逻辑靠"1 秒内的新鲜输入"判断这份快照还能不能用。
    // 首版忘了这一步 → tick 恒为 0 → 每拍都判定过期 → 操控一次都没生效。
    auto stamped = snapshot;
    stamped.tick = mServerTick;
    std::lock_guard lock(mMutex);
    mInputs[playerName] = stamped;
}

void ViewOverrideManager::tickPulse() {
    ++mServerTick;
    if (!mActive.load(std::memory_order_relaxed)) return; // 没用到这个功能: 每 tick 一次原子读, 就这么多
    auto level = ::ll::service::getLevel();
    if (!level) return;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (hasOverridesFor(player.getRealName())) pulseForPlayer(player);
        return true;
    });
}

void ViewOverrideManager::pulseForPlayer(::Player& player) {
    // 客户端视野里"算它有这只实体"的距离 —— 取保守值: 比服务端下发范围小, 免得给客户端凭空造出
    // 一只它本来不该看到的实体（覆盖只在看得见的东西上才有意义）
    constexpr float kVisibleRange = 40.0f;

    std::string const playerName = player.getRealName();

    // 刚进服的玩家先晾一会儿（约 2 秒）: 客户端这时还在收区块、建世界, 塞实体包最容易出事。
    // 宽限期内连"视野基线"都不更新, 好让宽限结束后的第一拍仍然算作"重新出现"。
    constexpr std::uint32_t kJoinGraceTicks = 40;
    {
        std::lock_guard lock(mMutex);
        auto&           state = mPulse[playerName];
        if (state.ticks < kJoinGraceTicks) {
            ++state.ticks;
            return;
        }
    }

    // 锁内只做表操作: 拷出这一拍要看的实体 + （换了区块时）要重推的方块; 查世界/发包都在锁外
    std::vector<std::pair<std::int64_t, EntityRecord>> entities;
    std::vector<std::pair<std::int64_t, std::uint32_t>> repushBlocks;
    {
        std::lock_guard lock(mMutex);
        auto const      feet   = player.getFeetBlockPos();
        auto&           state  = mPulse[playerName];
        bool const      moved  = !state.hasChunk || state.chunkX != (feet.x >> 4) || state.chunkZ != (feet.z >> 4);
        state.hasChunk         = true;
        state.chunkX           = feet.x >> 4;
        state.chunkZ           = feet.z >> 4;
        auto const      dim    = dimensionOfPlayer(player);

        auto collect = [&](Table const& table) {
            for (auto const& [uniqueId, record] : table.entities) entities.emplace_back(uniqueId, record);
            if (!moved) return;
            // 换了区块 = 客户端手里的区块数据重来过, 方块覆盖会被冲掉 —— 全部重推一遍
            for (auto const& [key, record] : table.blocks) {
                if (logic::dimensionMatches(record.dim, dim)) repushBlocks.emplace_back(key, record.viewId);
            }
        };
        if (auto const it = mPlayers.find(playerName); it != mPlayers.end()) collect(it->second);
        collect(mGlobal);
    }

    // 实体: 只有"从看不见 → 看得见"这一拍才动手（重新套用覆盖）
    for (auto const& [uniqueId, record] : entities) {
        bool const nowVisible = liveActorNear(player, uniqueId, kVisibleRange) != nullptr;
        bool       wasVisible = false;
        {
            std::lock_guard lock(mMutex);
            auto&           state = mPulse[playerName];
            auto const      it    = state.visible.find(uniqueId);
            wasVisible            = it != state.visible.end() && it->second;
            state.visible[uniqueId] = nowVisible;
        }
        if (!nowVisible || wasVisible) continue;
    }

    // 方块: 换了区块就把覆盖推回去
    for (auto const& [key, viewId] : repushBlocks) {
        sendBlockUpdate(player, logic::unpackX(key), logic::unpackY(key), logic::unpackZ(key), viewId);
    }
}

std::string ViewOverrideManager::describeFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    std::size_t     entities = mGlobal.entities.size();
    std::size_t     blocks   = mGlobal.blocks.size();
    if (auto const* table = tableOfLocked(playerName)) {
        entities += table->entities.size();
        blocks += table->blocks.size();
    }
    auto const counters = hookCounters();
    return std::format(
        "entities={} blocks={} hooks(calls={} spawn={} meta={} block={} chunk={}) sendFail={} ctrlSteps={}",
        entities,
        blocks,
        counters.calls,
        counters.spawn,
        counters.meta,
        counters.block,
        counters.chunk,
        sculkSendFailureCount().load(std::memory_order_relaxed),
        mControlSteps.load(std::memory_order_relaxed)
    );
}

void ViewOverrideManager::noteHookCall() { mHookCalls.fetch_add(1, std::memory_order_relaxed); }

ViewOverrideManager::HookCounters ViewOverrideManager::hookCounters() const {
    HookCounters out;
    out.calls = mHookCalls.load(std::memory_order_relaxed);
    out.spawn = mSpawnSeen.load(std::memory_order_relaxed);
    out.meta  = mMetaSeen.load(std::memory_order_relaxed);
    out.block = mBlockSeen.load(std::memory_order_relaxed);
    out.chunk = mChunkSeen.load(std::memory_order_relaxed);
    return out;
}

void ViewOverrideManager::onPlayerLeave(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    auto const      it = mPlayers.find(playerName);
    if (it == mPlayers.end()) return;
    // 运行时 id 是本次会话的东西, 换了会话会撞上别的实体 —— 索引必须清;
    // 覆盖本身按 uniqueId / 坐标存, 留着, 玩家回来重新出生时自动套用（区块也会自动补发方块）。
    it->second.byRuntime.clear();
    for (auto& [uniqueId, record] : it->second.entities) record.runtimeKnown = false;
    mPulse.erase(playerName);  // 心跳状态同理（下次进服重新判定视野）
    mInputs.erase(playerName); // 输入快照也是会话内的东西
}

void ViewOverrideManager::shutdown() {
    std::lock_guard lock(mMutex);
    mPlayers.clear();
    mPulse.clear();
    mInputs.clear();
    mGlobal = Table{};
    refreshActiveLocked();
}

std::size_t ViewOverrideManager::globalEntityCount() const {
    std::lock_guard lock(mMutex);
    return mGlobal.entities.size();
}

std::size_t ViewOverrideManager::globalBlockCount() const {
    std::lock_guard lock(mMutex);
    return mGlobal.blocks.size();
}

// ─────────────────────────────────────────────
// 出站改写
// ─────────────────────────────────────────────

bool ViewOverrideManager::applyToOutbound(::Player& target, ::Packet& packet, Applied& applied) {
    std::string const& playerName = target.getRealName();

    std::lock_guard lock(mMutex);
    Table*          playerTable = nullptr;
    if (auto const it = mPlayers.find(playerName); it != mPlayers.end() && !it->second.empty()) {
        playerTable = &it->second;
    }
    if (playerTable == nullptr && mGlobal.empty()) return false; // 快路径: 这名玩家没有任何覆盖

    // ── 本函数**只决定拦不拦**, 不再改原包里的任何字段 ──
    // 需要"改"的地方一律: 丢掉原包 + 由库自己手写协议包补发。我们这个库只做协议包,
    // 不掺"改引擎对象再让引擎序列化"那套 —— 每条我们自己发的包内容都完全可控。
    switch (static_cast<int>(packet.getId())) {
    case logic::kAddActor: {
        mSpawnSeen.fetch_add(1, std::memory_order_relaxed);
        auto& payload  = payloadOf<::AddActorPacketPayload>(packet);
        auto  uniqueId = unwrap(payload.mEntityId).rawID;
        auto* record   = findEntityLocked(playerName, uniqueId);
        if (record == nullptr) return false;
        record->runtimeKnown = true;
        record->runtimeId    = static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID);
        if (playerTable != nullptr) playerTable->byRuntime[record->runtimeId] = uniqueId;

        applied.hit = true;
        if (record->view.hidden) {
            applied.drop = true; // 隐藏 = 这条出生包不发给他
            return true;
        }
        if (!record->view.identifier.empty()) {
            // 换类型: 丢掉原出生包, 发我们自己的包（其余字段照抄原包 → 内容齐全, 只换类型字符串）
            sendActorSpawnRewritten(
                target,
                payload,
                record->view.identifier,
                record->view.nametag,
                record->view.hasNametag,
                record->view.nametagAlwaysShow
            );
            applied.drop = true;
            return true;
        }
        if (record->view.hasNametag) {
            applied.nametagPush.emplace_back(uniqueId, record->runtimeId); // 原包照发, 名字牌我们补
        }
        return true;
    }
    case logic::kAddPlayer: {
        mSpawnSeen.fetch_add(1, std::memory_order_relaxed);
        auto& payload   = payloadOf<::AddPlayerPacketPayload>(packet);
        auto  runtimeId = static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID);
        auto* actor     = actorByRuntimeId(runtimeId);
        if (actor == nullptr) return false;
        auto  uniqueId = actor->getOrCreateUniqueID().rawID; // 身份识别
        auto* record   = findEntityLocked(playerName, uniqueId);
        if (record == nullptr) return false;
        record->runtimeKnown = true;
        record->runtimeId    = runtimeId;
        record->playerName   = unwrap(payload.mName); // 协议层: 名字就在出生包里
        if (playerTable != nullptr) playerTable->byRuntime[runtimeId] = uniqueId;

        applied.hit = true;
        if (record->view.hidden) {
            applied.drop = true;
            return true;
        }
        if (!record->view.identifier.empty() && record->view.identifier != "minecraft:player") {
            // 玩家"变生物": 丢掉出生包 + 用**同一个 runtimeId/uniqueId** 发我们自己的实体。
            // 位置/朝向取他发来的 PlayerAuthInput（眼位 − 1.62 = 脚位）; 服务端那边仍是真玩家,
            // 所以打到它身上的攻击由服务端按真身结算。
            InputSnapshot snap{};
            if (inputSnapshotOf(uniqueId, snap)) {
                sendActorSpawnFromInput(
                    target,
                    runtimeId,
                    uniqueId,
                    record->view.identifier,
                    snap.x,
                    snap.y,
                    snap.z,
                    snap.pitch,
                    snap.yaw,
                    snap.headYaw,
                    record->view.nametag,
                    record->view.hasNametag,
                    record->view.nametagAlwaysShow
                );
            }
            record->substituted = true;
            applied.drop        = true;
            return true;
        }
        if (record->view.hasNametag) applied.nametagPush.emplace_back(uniqueId, runtimeId);
        return true;
    }
    case logic::kSetActorData: {
        mMetaSeen.fetch_add(1, std::memory_order_relaxed);
        auto& payload   = payloadOf<::SetActorDataPacketPayload>(packet);
        auto  runtimeId = static_cast<std::uint64_t>(unwrap(payload.mId).rawID);
        auto* record    = findEntityByRuntimeLocked(playerName, runtimeId);
        if (record == nullptr) return false;

        applied.hit = true;
        if (record->view.hidden) {
            applied.drop = true;
            return true;
        }
        // 原包照发（健康/姿态等元数据要留着）, 名字牌由我们自己的包在它之后压过去
        if (record->view.hasNametag) applied.nametagPush.emplace_back(record->uniqueId, runtimeId);
        return true;
    }
    case logic::kUpdateBlock:
    case logic::kUpdateBlockSynced: {
        mBlockSeen.fetch_add(1, std::memory_order_relaxed);
        bool const synced = packet.getId() == ::MinecraftPacketIds::UpdateBlockSynced;
        auto const pos = synced ? unwrap(payloadOf<::UpdateBlockSyncedPacketPayload>(packet).mPos)
                                : unwrap(payloadOf<::UpdateBlockPacketPayload>(packet).mPos);
        auto const current = synced
            ? static_cast<std::uint32_t>(unwrap(payloadOf<::UpdateBlockSyncedPacketPayload>(packet).mRuntimeId))
            : static_cast<std::uint32_t>(unwrap(payloadOf<::UpdateBlockPacketPayload>(packet).mRuntimeId));

        auto* record = findBlockLocked(playerName, logic::packPos(pos.x, pos.y, pos.z));
        if (record == nullptr) return false;
        if (!logic::dimensionMatches(record->dim, dimensionOfPlayer(target))) return false;
        if (!logic::needsRewrite(current, record->viewId)) return false; // 服务端发的就是覆盖方块 → 照发

        // 换方块: 丢掉原包 + 我们自己的 UpdateBlock（覆盖方块的网络 id）
        sendBlockUpdate(target, pos.x, pos.y, pos.z, record->viewId);
        applied.hit  = true;
        applied.drop = true;
        return true;
    }
    case logic::kMovePlayer: {
        auto&   payload   = payloadOf<::MovePlayerPacketPayload>(packet);
        auto    runtimeId = static_cast<std::uint64_t>(unwrap(payload.mPlayerID).rawID);
        auto*   record    = findEntityByRuntimeLocked(playerName, runtimeId);
        if (record == nullptr || !record->substituted) return false;
        // 被替换成生物后, 玩家专属位移包对这名观看者丢掉; 位置由心跳用 MoveActorAbsolute 推
        applied.hit  = true;
        applied.drop = true;
        return true;
    }
    case logic::kLevelChunk: {
        mChunkSeen.fetch_add(1, std::memory_order_relaxed);
        auto&      payload = payloadOf<::LevelChunkPacketPayload>(packet);
        auto const pos     = unwrap(payload.mPos);
        auto const dim     = static_cast<int>(unwrap(payload.mDimensionId));
        auto       collect = [&](Table const& table) {
            for (auto const& [key, record] : table.blocks) {
                if (!logic::inChunk(key, pos.x, pos.z)) continue;
                if (!logic::dimensionMatches(record.dim, dim)) continue;
                applied.repush.emplace_back(key, record.viewId);
            }
        };
        if (playerTable != nullptr) collect(*playerTable);
        collect(mGlobal);
        if (applied.repush.empty()) return false;
        applied.hit = true;
        return true;
    }
    default:
        return false; // 不认识的包一个字段都不碰
    }
}

void ViewOverrideManager::runPostActions(::Player& target, Applied const& applied) {
    // ① 名字牌: 在原包之后用我们自己的 SetActorData 压过去（服务端之后改名也压得住）
    for (auto const& [uniqueId, runtimeId] : applied.nametagPush) {
        std::string nametag;
        bool        alwaysShow = false;
        {
            std::lock_guard lock(mMutex);
            auto const      lookup = [&](Table const& t) -> EntityRecord const* {
                auto const e = t.entities.find(uniqueId);
                return e == t.entities.end() ? nullptr : &e->second;
            };
            auto const* record = lookup(mGlobal);
            if (auto const it = mPlayers.find(target.getRealName()); it != mPlayers.end()) {
                if (auto const* local = lookup(it->second); local != nullptr) record = local;
            }
            if (record == nullptr) continue;
            nametag    = record->view.nametag;
            alwaysShow = record->view.nametagAlwaysShow;
        }
        sendNametag(target, runtimeId, nametag, alwaysShow);
    }

    // ② 区块重发后把方块覆盖补回给这名玩家
    for (auto const& [key, viewId] : applied.repush) {
        sendBlockUpdate(target, logic::unpackX(key), logic::unpackY(key), logic::unpackZ(key), viewId);
    }
}


} // namespace debugshape_export::view

// ─────────────────────────────────────────────
// 对外接口适配（IHologramLib::viewOverrides()）
// ─────────────────────────────────────────────
namespace debugshape_export {

namespace {

using view::ViewOverrideManager;

class ViewOverrideAdapter : public hologramlib::IViewOverride {
public:
    bool overrideEntity(
        std::string const&             playerName,
        std::int64_t                   entityUniqueId,
        hologramlib::EntityView const& view
    ) override {
        return ViewOverrideManager::getInstance().overrideEntity(playerName, entityUniqueId, view);
    }

    bool overrideBlock(
        std::string const&            playerName,
        int                           x,
        int                           y,
        int                           z,
        hologramlib::BlockView const& view
    ) override {
        return ViewOverrideManager::getInstance().overrideBlock(playerName, x, y, z, view);
    }

    bool clearEntity(std::string const& playerName, std::int64_t entityUniqueId) override {
        return ViewOverrideManager::getInstance().clearEntity(playerName, entityUniqueId);
    }

    bool clearBlock(std::string const& playerName, int x, int y, int z) override {
        return ViewOverrideManager::getInstance().clearBlock(playerName, x, y, z);
    }

    void clearAll(std::string const& playerName) override {
        ViewOverrideManager::getInstance().clearAll(playerName);
    }

    [[nodiscard]] std::string describeFor(std::string const& playerName) const override {
        return ViewOverrideManager::getInstance().describeFor(playerName);
    }
};

} // namespace

hologramlib::IViewOverride& viewOverrideAdapter() {
    static ViewOverrideAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
