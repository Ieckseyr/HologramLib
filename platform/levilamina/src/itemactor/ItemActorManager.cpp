#include "ItemActorManager.h"

#include "kernel/PacketSink.h"
#include "core/VirtualActorIds.h"
#include "item/MenuItemStack.h"

#include <ll/api/service/Bedrock.h>

#include <mc/legacy/ActorRuntimeID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/Item.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>

#include "core/proto/Version.h"

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

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

    proto::AddItemActorPacket packet;
    packet.runtime_id        = proto::bp::ActorRuntimeID{actor.runtimeId};
    packet.id                = proto::ActorUniqueID{static_cast<std::int64_t>(actor.uniqueId)};
    packet.item.id           = numericId;
    packet.item.stack_size   = static_cast<std::uint16_t>(actor.count);
    packet.item.aux_value    = static_cast<std::uint32_t>(spec.aux);
    packet.pos               = proto::Vec3{actor.x, actor.y, actor.z};
    packet.velocity          = proto::Vec3{spec.vx, spec.vy, spec.vz};
    packet.is_from_fishing   = false;

    auto bytes = wire::encode(packet);
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

    proto::SetActorMotionPacket packet;
    packet.runtime_id = proto::bp::ActorRuntimeID{actor->runtimeId};
    packet.motion     = proto::Vec3{vx, vy, vz};
    packet.tick       = proto::bp::PlayerInputTick{currentTick()};

    auto bytes = wire::encode(packet);
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

    proto::MoveActorAbsolutePacket packet;
    packet.move_data.runtime_id = proto::bp::ActorRuntimeID{actor->runtimeId};
    packet.move_data.header     = proto::move_flags::kTeleport;
    packet.move_data.pos        = proto::Vec3{x, y, z};
    packet.move_data.rot_x      = rotToByte(0.0f);
    packet.move_data.rot_y      = rotToByte(0.0f);
    packet.move_data.rot_y_head = rotToByte(0.0f);

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;

    sendToDimension(actor->dimension, bytes);
    return true;
}

bool ItemActorManager::take(int64_t id, std::string const& playerName) {
    // 锁内只取数据（拷成值），编码与发送放到锁外（P04）；erase 会析构记录，锁外不能再碰指针
    std::uint64_t snapshotUniqueId  = 0;
    std::uint64_t snapshotRuntimeId = 0;
    int           snapshotDimension = 0;
    ::Player*     player            = nullptr;
    {
        std::lock_guard lock(mMutex);
        auto*           snapshot = mActors.peek(id);
        if (snapshot == nullptr)
            return false;
        player = findPlayer(playerName);
        if (player == nullptr)
            return false;
        snapshotUniqueId  = snapshot->uniqueId;
        snapshotRuntimeId = snapshot->runtimeId;
        snapshotDimension = snapshot->dimension;
        mActors.erase(id);
    }

    proto::TakeItemActorPacket take;
    take.item_id  = proto::bp::ActorRuntimeID{snapshotRuntimeId};
    take.actor_id = proto::bp::ActorRuntimeID{player->getRuntimeID().rawID};
    if (auto bytes = wire::encode(take); !bytes.empty()) {
        wire::sendRaw(*player, bytes);
    }

    proto::RemoveActorPacket remove;
    remove.entity_id = proto::ActorUniqueID{static_cast<std::int64_t>(snapshotUniqueId)};
    if (auto bytes = wire::encode(remove); !bytes.empty()) {
        sendToDimension(snapshotDimension, bytes);
    }
    return true;
}

bool ItemActorManager::destroy(int64_t id) {
    std::uint64_t uniqueId  = 0;
    int           dimension = 0;
    {
        std::lock_guard lock(mMutex);
        auto*           actor = mActors.peek(id);
        if (actor == nullptr)
            return false;
        uniqueId  = actor->uniqueId;
        dimension = actor->dimension;
        mActors.erase(id);
    }

    proto::RemoveActorPacket remove;
    remove.entity_id = proto::ActorUniqueID{static_cast<std::int64_t>(uniqueId)};
    if (auto bytes = wire::encode(remove); !bytes.empty()) {
        sendToDimension(dimension, bytes);
    }
    return true;
}

void ItemActorManager::destroyAll() {
    std::vector<std::pair<std::uint64_t, int>> victims;
    {
        std::lock_guard lock(mMutex);
        mActors.forEachLocked([&](int64_t, Actor const& actor) {
            victims.emplace_back(actor.uniqueId, actor.dimension);
        });
        mActors.clear();
    }
    for (auto const& [uniqueId, dimension] : victims) {
        proto::RemoveActorPacket remove;
        remove.entity_id = proto::ActorUniqueID{static_cast<std::int64_t>(uniqueId)};
        if (auto bytes = wire::encode(remove); !bytes.empty()) {
            sendToDimension(dimension, bytes);
        }
    }
}

std::uint64_t ItemActorManager::uniqueIdOf(int64_t id) const {
    std::uint64_t   uniqueId = 0;
    std::lock_guard lock(mMutex);
    mActors.withObject(id, [&](Actor const& actor) {
        uniqueId = actor.uniqueId;
    });
    return uniqueId;
}

bool ItemActorManager::exists(int64_t id) const {
    std::lock_guard lock(mMutex);
    return mActors.peek(id) != nullptr;
}

std::uint64_t ItemActorManager::runtimeIdOf(int64_t id) const {
    std::lock_guard lock(mMutex);
    auto*           a = mActors.peek(id);
    return a == nullptr ? 0 : a->runtimeId;
}

std::vector<int64_t> ItemActorManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    return mActors.snapshotAll();
}

void ItemActorManager::shutdown() {
    destroyAll();
}

} // namespace debugshape_export
