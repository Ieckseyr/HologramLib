// LocatorBarManager.cpp - 方位条航点
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

LocatorBarManager& LocatorBarManager::getInstance() {
    static LocatorBarManager instance;
    return instance;
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

    Handle handle;
    bool   existed = false;
    {
        std::lock_guard lock(mMutex);
        auto&           perPlayer = mWaypoints[playerName];
        auto            it        = perPlayer.find(key);
        if (it != perPlayer.end()) {
            handle  = it->second;
            existed = true;
        } else {
            handle = Handle{fnv1a(key, 1469598103934665603ULL), fnv1a(key, 1099511628211ULL)};
            perPlayer.emplace(key, handle);
        }
    }

    using Payload = sculk::protocol::LocatorBarServerWaypointPayload;
    using Flags   = Payload::UpdateFlags;

    sculk::protocol::LocatorBarWaypointPayload payload;
    payload.mGroupHandle.mUUID.mMostSignificantBits  = handle.msb;
    payload.mGroupHandle.mUUID.mLeastSignificantBits = handle.lsb;
    payload.mActionFlag                              = existed ? sculk::protocol::LocatorBarAction::Update
                                                               : sculk::protocol::LocatorBarAction::Add;

    auto& server          = payload.mServerWaypointPayload;
    auto  flags           = static_cast<std::uint8_t>(Flags::Position) | static_cast<std::uint8_t>(Flags::Visibility);
    server.mWorldPosition = sculk::protocol::LocatorBarWorldPosition{
        sculk::protocol::Vec3{x, y, z},
        dimension,
    };
    server.mIsVisible = visible;
    if (color >= 0) {
        server.mColor = color;
        flags         = static_cast<std::uint8_t>(flags) | static_cast<std::uint8_t>(Flags::Color);
    }
    server.mUpdateFlag = static_cast<Flags>(flags);

    sculk::protocol::LocatorBarPacket packet;
    packet.mWaypoints.push_back(payload);

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return false;
    wire::sendRaw(*player, bytes);
    return true;
}

bool LocatorBarManager::hideFromOthers(std::string const& playerName, bool hidden) {
    // 一套实现：转给"玩家出站包控制"域（那边有完整的掩码 / 隐身体系）
    if (!PlayerVisibilityManager::getInstance().setHidden(playerName, hidden))
        return false;

    std::lock_guard lock(mMutex);
    if (hidden)
        mHiddenFromOthers.insert(playerName);
    else
        mHiddenFromOthers.erase(playerName);
    return true;
}

bool LocatorBarManager::isHiddenFromOthers(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    return mHiddenFromOthers.contains(playerName);
}

bool LocatorBarManager::removeWaypoint(std::string const& playerName, std::string const& key) {
    Handle handle;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return false;
        auto it = outer->second.find(key);
        if (it == outer->second.end())
            return false;
        handle = it->second;
        outer->second.erase(it);
    }

    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return true; // 记录已清；客户端那边随下线消失

    sculk::protocol::LocatorBarWaypointPayload payload;
    payload.mGroupHandle.mUUID.mMostSignificantBits  = handle.msb;
    payload.mGroupHandle.mUUID.mLeastSignificantBits = handle.lsb;
    payload.mActionFlag                              = sculk::protocol::LocatorBarAction::Remove;
    payload.mServerWaypointPayload.mUpdateFlag       = static_cast<sculk::protocol::LocatorBarServerWaypointPayload::UpdateFlags>(0);

    sculk::protocol::LocatorBarPacket packet;
    packet.mWaypoints.push_back(payload);

    if (auto bytes = wire::encodeChecked(packet); !bytes.empty()) {
        wire::sendRaw(*player, bytes);
    }
    return true;
}

int LocatorBarManager::clearWaypoints(std::string const& playerName) {
    std::vector<std::string> keys;
    {
        std::lock_guard lock(mMutex);
        auto            outer = mWaypoints.find(playerName);
        if (outer == mWaypoints.end())
            return 0;
        for (auto const& [key, handle] : outer->second)
            keys.push_back(key);
    }

    int removed = 0;
    for (auto const& key : keys) {
        if (removeWaypoint(playerName, key))
            ++removed;
    }
    return removed;
}

int LocatorBarManager::waypointCount(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            outer = mWaypoints.find(playerName);
    if (outer == mWaypoints.end())
        return 0;
    return static_cast<int>(outer->second.size());
}

void LocatorBarManager::forget(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    mWaypoints.erase(playerName);
    mHiddenFromOthers.erase(playerName); // 人已下线，客户端那边自然消失，不用再发撤销
}

void LocatorBarManager::shutdown() {
    std::lock_guard lock(mMutex);
    mWaypoints.clear();
    mHiddenFromOthers.clear();
}

} // namespace debugshape_export
