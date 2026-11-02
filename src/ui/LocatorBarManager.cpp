// LocatorBarManager.cpp - 方位条航点（1.34.0：图标 / 大小 / 绑定实体 / 分组）
#include "LocatorBarManager.h"

#include "../visibility/PlayerVisibilityManager.h" // 隐藏的实现搬到了那边
#include "core/PacketSink.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/actor/player/LocatorBar.hpp>
#include <sculk/protocol/codec/packet/LocatorBarPacket.hpp>

namespace debugshape_export {

namespace {

// key → 稳定的 128 位句柄（FNV-1a 变体挑两个互不相关的种子）。
// 只需要"同一 key 每次得到同一 UUID"，不要求抗碰撞强度。
std::uint64_t fnv1a(std::string const& text, std::uint64_t seed) {
    std::uint64_t hash = seed;
    for (unsigned char const c : text) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

::Player* findPlayer(std::string const& name) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    return level->getPlayer(name);
}

} // namespace

// 协议载荷与其标志位（本文件多处用）
using Payload = sculk::protocol::LocatorBarServerWaypointPayload;

LocatorBarManager& LocatorBarManager::getInstance() {
    static LocatorBarManager instance;
    return instance;
}

bool LocatorBarManager::sendOne(::Player& player, Waypoint const& waypoint, bool isUpdate) {
    sculk::protocol::LocatorBarWaypointPayload payload;
    payload.mGroupHandle.mUUID.mMostSignificantBits  = waypoint.handle.msb;
    payload.mGroupHandle.mUUID.mLeastSignificantBits = waypoint.handle.lsb;
    payload.mActionFlag =
        isUpdate ? sculk::protocol::LocatorBarAction::Update : sculk::protocol::LocatorBarAction::Add;

    auto& server = payload.mServerWaypointPayload;
    auto  flags  = static_cast<std::uint8_t>(Payload::UpdateFlags::Position) | static_cast<std::uint8_t>(Payload::UpdateFlags::Visibility);

    server.mWorldPosition = sculk::protocol::LocatorBarWorldPosition{
        sculk::protocol::Vec3{waypoint.x, waypoint.y, waypoint.z},
        waypoint.dimension,
    };
    server.mIsVisible = waypoint.visible;

    if (waypoint.color >= 0) {
        server.mColor = waypoint.color;
        flags         = static_cast<std::uint8_t>(flags) | static_cast<std::uint8_t>(Payload::UpdateFlags::Color);
    }
    if (!waypoint.texturePath.empty()) {
        server.mTexturePath = waypoint.texturePath;
        flags               = static_cast<std::uint8_t>(flags) | static_cast<std::uint8_t>(Payload::UpdateFlags::TexturePath);
    }
    if (waypoint.hasIconSize) {
        server.mIconSize = sculk::protocol::Vec2{waypoint.iconWidth, waypoint.iconHeight};
        flags            = static_cast<std::uint8_t>(flags) | static_cast<std::uint8_t>(Payload::UpdateFlags::IconSize);
    }
    if (waypoint.actorUniqueId != 0) {
        // 绑定实体：协议里没有专门的 UpdateFlag 位，随位置一起带上
        server.mActorUniqueId = waypoint.actorUniqueId;
    }
    server.mUpdateFlag = static_cast<Payload::UpdateFlags>(flags);

    sculk::protocol::LocatorBarPacket packet;
    packet.mWaypoints.push_back(payload);

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return false;
    wire::sendRaw(player, bytes);
    return true;
}

bool LocatorBarManager::sendMany(::Player& player, std::vector<Waypoint> const& waypoints) {
    if (waypoints.empty())
        return true;

    sculk::protocol::LocatorBarPacket packet;
    packet.mWaypoints.reserve(waypoints.size());

    for (auto const& waypoint : waypoints) {
        sculk::protocol::LocatorBarWaypointPayload payload;
        payload.mGroupHandle.mUUID.mMostSignificantBits  = waypoint.handle.msb;
        payload.mGroupHandle.mUUID.mLeastSignificantBits = waypoint.handle.lsb;
        payload.mActionFlag                              = sculk::protocol::LocatorBarAction::Update;

        auto& server = payload.mServerWaypointPayload;
        auto  flags  = static_cast<std::uint8_t>(Payload::UpdateFlags::Position) | static_cast<std::uint8_t>(Payload::UpdateFlags::Visibility);

        server.mWorldPosition = sculk::protocol::LocatorBarWorldPosition{
            sculk::protocol::Vec3{waypoint.x, waypoint.y, waypoint.z},
            waypoint.dimension,
        };
        server.mIsVisible = waypoint.visible;

        if (waypoint.color >= 0) {
            server.mColor = waypoint.color;
            flags         = static_cast<std::uint8_t>(flags) | static_cast<std::uint8_t>(Payload::UpdateFlags::Color);
        }
        if (!waypoint.texturePath.empty()) {
            server.mTexturePath = waypoint.texturePath;
            flags               = static_cast<std::uint8_t>(flags) | static_cast<std::uint8_t>(Payload::UpdateFlags::TexturePath);
        }
        if (waypoint.hasIconSize) {
            server.mIconSize = sculk::protocol::Vec2{waypoint.iconWidth, waypoint.iconHeight};
            flags            = static_cast<std::uint8_t>(flags) | static_cast<std::uint8_t>(Payload::UpdateFlags::IconSize);
        }
        if (waypoint.actorUniqueId != 0) {
            server.mActorUniqueId = waypoint.actorUniqueId;
        }
        server.mUpdateFlag = static_cast<Payload::UpdateFlags>(flags);

        packet.mWaypoints.push_back(std::move(payload));
    }

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return false;
    wire::sendRaw(player, bytes);
    return true;
}

bool LocatorBarManager::setWaypoint(
    std::string const& playerName,
    std::string const& key,
    float              x,
    float              y,
    float              z,
    int                dimension,
    int                color,
    bool               visible) {
    if (key.empty())
        return false;

    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    Waypoint waypoint;
    bool     existed = false;
    {
        std::lock_guard lock(mMutex);
        auto&           perPlayer = mWaypoints[playerName];
        auto            it        = perPlayer.find(key);
        if (it != perPlayer.end()) {
            waypoint = it->second;
            existed  = true;
        } else {
            waypoint.handle = Handle{fnv1a(key, 1469598103934665603ULL), fnv1a(key, 1099511628211ULL)};
        }
        waypoint.x         = x;
        waypoint.y         = y;
        waypoint.z         = z;
        waypoint.dimension = dimension;
        waypoint.color     = color;
        waypoint.visible   = visible;
        perPlayer[key]     = waypoint;
    }

    return sendOne(*player, waypoint, existed);
}

bool LocatorBarManager::setIcon(std::string const& playerName, std::string const& key, std::string const& texturePath) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    Waypoint copy;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return false;
        auto inner = outer->second.find(key);
        if (inner == outer->second.end())
            return false;
        inner->second.texturePath = texturePath;
        copy                      = inner->second;
    }
    return sendOne(*player, copy, true);
}

bool LocatorBarManager::setIconSize(std::string const& playerName, std::string const& key, float width, float height) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    Waypoint copy;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return false;
        auto inner = outer->second.find(key);
        if (inner == outer->second.end())
            return false;
        if (width <= 0.0f || height <= 0.0f) {
            inner->second.hasIconSize = false;
        } else {
            inner->second.hasIconSize = true;
            inner->second.iconWidth   = width;
            inner->second.iconHeight  = height;
        }
        copy = inner->second;
    }
    return sendOne(*player, copy, true);
}

bool LocatorBarManager::attachEntity(std::string const& playerName, std::string const& key, int64_t entityUniqueId) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    Waypoint copy;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return false;
        auto inner = outer->second.find(key);
        if (inner == outer->second.end())
            return false;
        inner->second.actorUniqueId = entityUniqueId;
        copy                        = inner->second;
    }
    return sendOne(*player, copy, true);
}

bool LocatorBarManager::clearAttach(std::string const& playerName, std::string const& key) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    Waypoint copy;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return false;
        auto inner = outer->second.find(key);
        if (inner == outer->second.end())
            return false;
        inner->second.actorUniqueId = 0;
        copy                        = inner->second;
    }
    return sendOne(*player, copy, true);
}

bool LocatorBarManager::setGroup(std::string const& playerName, std::string const& key, std::string const& groupKey) {
    std::lock_guard lock(mMutex);
    auto            outer = mWaypoints.find(playerName);
    if (outer == mWaypoints.end())
        return false;
    auto inner = outer->second.find(key);
    if (inner == outer->second.end())
        return false;
    inner->second.groupKey = groupKey;
    return true;
}

int LocatorBarManager::syncGroup(std::string const& playerName, std::string const& groupKey) {
    if (groupKey.empty())
        return 0;

    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return 0;

    std::vector<Waypoint> batch;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return 0;
        for (auto const& [key, waypoint] : outer->second) {
            if (waypoint.groupKey == groupKey)
                batch.push_back(waypoint);
        }
    }
    if (batch.empty())
        return 0;
    if (!sendMany(*player, batch))
        return 0;
    return static_cast<int>(batch.size());
}

bool LocatorBarManager::hideFromOthers(std::string const& playerName, bool hidden) {
    return PlayerVisibilityManager::getInstance().setHidden(playerName, hidden);
}

bool LocatorBarManager::isHiddenFromOthers(std::string const& playerName) const {
    return PlayerVisibilityManager::getInstance().isHidden(playerName);
}

bool LocatorBarManager::removeWaypoint(std::string const& playerName, std::string const& key) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    Waypoint copy;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return false;
        auto inner = outer->second.find(key);
        if (inner == outer->second.end())
            return false;
        copy = inner->second;
        outer->second.erase(inner);
    }

    sculk::protocol::LocatorBarWaypointPayload payload;
    payload.mGroupHandle.mUUID.mMostSignificantBits  = copy.handle.msb;
    payload.mGroupHandle.mUUID.mLeastSignificantBits = copy.handle.lsb;
    payload.mActionFlag                              = sculk::protocol::LocatorBarAction::Remove;

    sculk::protocol::LocatorBarPacket packet;
    packet.mWaypoints.push_back(payload);

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return false;
    wire::sendRaw(*player, bytes);
    return true;
}

int LocatorBarManager::clearWaypoints(std::string const& playerName) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return 0;

    std::vector<Waypoint> victims;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return 0;
        for (auto const& [key, waypoint] : outer->second)
            victims.push_back(waypoint);
        outer->second.clear();
    }
    if (victims.empty())
        return 0;

    sculk::protocol::LocatorBarPacket packet;
    packet.mWaypoints.reserve(victims.size());
    for (auto const& waypoint : victims) {
        sculk::protocol::LocatorBarWaypointPayload payload;
        payload.mGroupHandle.mUUID.mMostSignificantBits  = waypoint.handle.msb;
        payload.mGroupHandle.mUUID.mLeastSignificantBits = waypoint.handle.lsb;
        payload.mActionFlag                              = sculk::protocol::LocatorBarAction::Remove;
        packet.mWaypoints.push_back(std::move(payload));
    }

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return 0;
    wire::sendRaw(*player, bytes);
    return static_cast<int>(victims.size());
}

int LocatorBarManager::waypointCount(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            outer = mWaypoints.find(playerName);
    return outer == mWaypoints.end() ? 0 : static_cast<int>(outer->second.size());
}

void LocatorBarManager::forget(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    mWaypoints.erase(playerName);
}

void LocatorBarManager::shutdown() {
    std::lock_guard lock(mMutex);
    mWaypoints.clear();
}

} // namespace debugshape_export
