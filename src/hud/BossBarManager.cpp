// BossBarManager.cpp - Boss 条
#include "BossBarManager.h"

#include "core/PacketSink.h"
#include "core/VirtualActorIds.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/packet/BossEventPacket.hpp>

namespace debugshape_export {

namespace {

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

} // namespace

BossBarManager& BossBarManager::getInstance() {
    static BossBarManager instance;
    return instance;
}

std::int64_t BossBarManager::nextTargetId() {
    // 虚拟号段里自增（不参与 ghost 路由，服务端没有对应实体）
    static std::atomic<std::uint64_t> counter{core::actorIds::bossBar.first};
    return static_cast<std::int64_t>(counter.fetch_add(1, std::memory_order_relaxed));
}

bool BossBarManager::send(::Player& player, Bar const& bar, int eventType) {
    sculk::protocol::BossEventPacket packet;
    packet.mTargetActorID = bar.targetId;
    packet.mType          = static_cast<sculk::protocol::BossEventPacket::EventType>(eventType);
    packet.mName          = bar.title;
    packet.mFilteredName  = bar.title;
    packet.mPercentage    = bar.percent;
    packet.mColor         = static_cast<std::uint32_t>(bar.color);
    packet.mOverlay       = static_cast<std::uint32_t>(bar.overlay);

    auto bytes = wire::encodeChecked(packet);
    if (bytes.empty())
        return false;

    wire::sendRaw(player, bytes);
    return true;
}

int64_t BossBarManager::add(
    std::string const&          playerName,
    std::string const&          title,
    float                       healthPercent,
    hologramlib::BossBarColor   color,
    hologramlib::BossBarOverlay overlay) {
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return -1;

    Bar bar;
    bar.playerName = playerName;
    bar.targetId   = nextTargetId();
    bar.title      = title;
    bar.percent    = healthPercent < 0.0f ? 0.0f : (healthPercent > 1.0f ? 1.0f : healthPercent);
    bar.color      = color;
    bar.overlay    = overlay;

    if (!send(*player, bar, static_cast<int>(sculk::protocol::BossEventPacket::EventType::Add))) {
        return -1;
    }

    std::lock_guard lock(mMutex);
    bar.id = static_cast<int64_t>(bar.targetId); // 对外 id 直接用虚拟 actor id，简且唯一
    mBars.emplace(bar.id, bar);
    return bar.id;
}

bool BossBarManager::setTitle(int64_t id, std::string const& title) {
    Bar copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(id);
        if (it == mBars.end())
            return false;
        it->second.title = title;
        copy             = it->second;
    }
    ::Player* player = findPlayer(copy.playerName);
    if (player == nullptr)
        return false;
    return send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::UpdateName));
}

bool BossBarManager::setHealth(int64_t id, float healthPercent) {
    Bar copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(id);
        if (it == mBars.end())
            return false;
        it->second.percent = healthPercent < 0.0f ? 0.0f : (healthPercent > 1.0f ? 1.0f : healthPercent);
        copy               = it->second;
    }
    ::Player* player = findPlayer(copy.playerName);
    if (player == nullptr)
        return false;
    return send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::UpdatePercent));
}

bool BossBarManager::setStyle(int64_t id, hologramlib::BossBarColor color, hologramlib::BossBarOverlay overlay) {
    Bar copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(id);
        if (it == mBars.end())
            return false;
        it->second.color   = color;
        it->second.overlay = overlay;
        copy               = it->second;
    }
    ::Player* player = findPlayer(copy.playerName);
    if (player == nullptr)
        return false;
    return send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::UpdateStyle));
}

bool BossBarManager::remove(int64_t id) {
    Bar copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(id);
        if (it == mBars.end())
            return false;
        copy = it->second;
        mBars.erase(it);
    }
    ::Player* player = findPlayer(copy.playerName);
    if (player == nullptr)
        return true; // 人已下线，客户端那边早没了
    send(*player, copy, static_cast<int>(sculk::protocol::BossEventPacket::EventType::Remove));
    return true;
}

void BossBarManager::removeAll(std::string const& playerName) {
    std::vector<Bar> victims;
    {
        std::lock_guard lock(mMutex);
        for (auto it = mBars.begin(); it != mBars.end();) {
            if (it->second.playerName == playerName) {
                victims.push_back(it->second);
                it = mBars.erase(it);
            } else {
                ++it;
            }
        }
    }
    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return;
    for (auto const& bar : victims) {
        send(*player, bar, static_cast<int>(sculk::protocol::BossEventPacket::EventType::Remove));
    }
}

int BossBarManager::count(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    int             n = 0;
    for (auto const& [id, bar] : mBars) {
        if (bar.playerName == playerName)
            ++n;
    }
    return n;
}

void BossBarManager::onPlayerLeave(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    for (auto it = mBars.begin(); it != mBars.end();) {
        if (it->second.playerName == playerName)
            it = mBars.erase(it);
        else
            ++it;
    }
}

void BossBarManager::shutdown() {
    std::lock_guard lock(mMutex);
    mBars.clear();
}

} // namespace debugshape_export
