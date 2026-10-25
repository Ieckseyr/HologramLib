// ItemActorManager.cpp - 协议层掉落物
#include "ItemActorManager.h"

#include "core/PacketSink.h"
#include "core/VirtualActorIds.h"
#include "item/MenuItemStack.h"

#include <ll/api/service/Bedrock.h>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/Item.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/packet/AddItemActorPacket.hpp>
#include <sculk/protocol/codec/packet/MoveActorAbsolutePacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/SetActorMotionPacket.hpp>
#include <sculk/protocol/codec/packet/TakeItemActorPacket.hpp>

namespace debugshape_export {

namespace {

std::uint64_t currentTick() {
    auto level = ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

::Player* findPlayer(std::string const& name) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    return level->getPlayer(name);
}

// 角度（度）→ 协议用的字节角
std::uint8_t rotToByte(float degrees) {
    float const normalized = degrees * (256.0f / 360.0f);
    auto const  value      = static_cast<int>(normalized) % 256;
    return static_cast<std::uint8_t>(value < 0 ? value + 256 : value);
}

} // namespace

ItemActorManager& ItemActorManager::getInstance() {
    static ItemActorManager instance;
    return instance;
}

void ItemActorManager::sendToDimension(int dimension, std::string const& bytes) {
    if (bytes.empty())
        return;
    auto level = ll::service::getLevel();
    if (!level)
        return;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (static_cast<int>(player.getDimensionId()) == dimension) {
            wire::sendRaw(player, bytes);
        }
        return true;
    });
}

int64_t ItemActorManager::spawn(hologramlib::ItemActorSpec const& spec) {
    if (spec.itemId.empty())
        return -1;

    // 物品: 先走库内那条"条目 → ItemStack"（SNBT 方言逐字节对拍过），再从 ItemStack 取数字物品 id。
    // **不带 UserData**: 地上一个普通钻石本来就不需要 NBT，也就绕开了物品描述符的 NBT 方言问题
    // （BDS 那份描述符的 payload 构造是私有的，模组侧造不出来）。
    hologramlib::ContainerMenuItem entry;
    entry.type              = spec.itemId;
    entry.count             = spec.count < 1 ? 1 : spec.count;
    entry.damage            = spec.aux;
    ::ItemStack const stack = item::makeMenuItemStack(entry);

    ::Item* itemPtr = stack.mItem.get();
    if (itemPtr == nullptr)
        return -1;
    short const numericId = itemPtr->mId;

    std::lock_guard lock(mMutex);

    Actor actor;
    actor.uniqueId  = mNextUniqueId++;
    actor.runtimeId = mNextRuntimeId++;
    actor.itemId    = spec.itemId;
    actor.aux       = spec.aux;
    actor.count     = entry.count;
    actor.x         = spec.x;
    actor.y         = spec.y;
    actor.z         = spec.z;
    actor.dimension = spec.dimension;

    // 该维度没人在线就不占 id（发了也没人看见）
    auto level = ll::service::getLevel();
    if (!level)
        return -1;
    bool hasViewer = false;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (static_cast<int>(player.getDimensionId()) == actor.dimension) {
            hasViewer = true;
            return false;
        }
        return true;
    });
    if (!hasViewer)
        return -1;

    sculk::protocol::AddItemActorPacket packet;
    packet.mRuntimeId       = actor.runtimeId;
    packet.mUniqueId        = static_cast<std::int64_t>(actor.uniqueId);
    packet.mItem.mId        = numericId;
    packet.mItem.mStackSize = static_cast<std::uint16_t>(actor.count);
    packet.mItem.mAux       = static_cast<std::uint32_t>(spec.aux);
    packet.mPos             = sculk::protocol::Vec3{actor.x, actor.y, actor.z};
    packet.mVelocity        = sculk::protocol::Vec3{spec.vx, spec.vy, spec.vz};
    packet.mIsFromFishing   = false;

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return -1;

    sendToDimension(actor.dimension, bytes);
    return mActors.insert(std::make_unique<Actor>(std::move(actor)));
}

bool ItemActorManager::setMotion(int64_t id, float vx, float vy, float vz) {
    std::lock_guard lock(mMutex);
    Actor*          actor = mActors.peek(id);
    if (actor == nullptr)
        return false;

    sculk::protocol::SetActorMotionPacket packet;
    packet.mActorRuntimeId = actor->runtimeId;
    packet.mMotion         = sculk::protocol::Vec3{vx, vy, vz};
    packet.mTick           = currentTick();

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return false;

    sendToDimension(actor->dimension, bytes);
    return true;
}

bool ItemActorManager::moveTo(int64_t id, float x, float y, float z) {
    std::lock_guard lock(mMutex);
    Actor*          actor = mActors.peek(id);
    if (actor == nullptr)
        return false;

    actor->x = x;
    actor->y = y;
    actor->z = z;

    sculk::protocol::MoveActorAbsolutePacket packet;
    packet.mActorRuntimeId = actor->runtimeId;
    packet.mFlags          = sculk::protocol::MoveActorAbsolutePacket::Flags::Teleport;
    packet.mPosition       = sculk::protocol::Vec3{x, y, z};
    packet.mRotationX      = rotToByte(0.0f);
    packet.mRotationY      = rotToByte(0.0f);
    packet.mRotationYHead  = rotToByte(0.0f);

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return false;

    sendToDimension(actor->dimension, bytes);
    return true;
}

bool ItemActorManager::take(int64_t id, std::string const& playerName) {
    std::lock_guard lock(mMutex);
    Actor*          actor = mActors.peek(id);
    if (actor == nullptr)
        return false;

    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    // 1) 只给这一名玩家播"飞向自己"的动画
    sculk::protocol::TakeItemActorPacket take;
    take.mItemRuntimeId  = actor->runtimeId;
    take.mActorRuntimeId = player->getRuntimeID().rawID;
    if (auto bytes = wire::encodeChecked(take); !bytes.empty()) {
        wire::sendRaw(*player, bytes);
    }

    // 2) 对所有人生效地删掉
    sculk::protocol::RemoveActorPacket remove;
    remove.mActorUniqueId = static_cast<std::int64_t>(actor->uniqueId);
    if (auto bytes = wire::encodeChecked(remove); !bytes.empty()) {
        sendToDimension(actor->dimension, bytes);
    }

    mActors.erase(id);
    return true;
}

bool ItemActorManager::destroy(int64_t id) {
    std::lock_guard lock(mMutex);
    Actor*          actor = mActors.peek(id);
    if (actor == nullptr)
        return false;

    sculk::protocol::RemoveActorPacket remove;
    remove.mActorUniqueId = static_cast<std::int64_t>(actor->uniqueId);
    if (auto bytes = wire::encodeChecked(remove); !bytes.empty()) {
        sendToDimension(actor->dimension, bytes);
    }

    mActors.erase(id);
    return true;
}

void ItemActorManager::destroyAll() {
    std::lock_guard                            lock(mMutex);
    std::vector<std::pair<std::uint64_t, int>> victims;
    mActors.forEachLocked([&](int64_t, Actor const& actor) {
        victims.emplace_back(actor.uniqueId, actor.dimension);
    });
    for (auto const& [uniqueId, dimension] : victims) {
        sculk::protocol::RemoveActorPacket remove;
        remove.mActorUniqueId = static_cast<std::int64_t>(uniqueId);
        if (auto bytes = wire::encodeChecked(remove); !bytes.empty()) {
            sendToDimension(dimension, bytes);
        }
    }
    mActors.clear();
}

bool ItemActorManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mActors.peek(id) != nullptr;
}

std::vector<int64_t> ItemActorManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    return mActors.snapshotAll();
}

void ItemActorManager::shutdown() {
    destroyAll();
}

} // namespace debugshape_export
