#include "effect/ActorEventFilter.h"

#include <mc/network/Packet.h>
#include <mc/network/packet/ActorEventPacket.h>
#include <mc/network/packet/ActorEventPacketPayload.h>
#include <mc/world/actor/player/Player.h>

#include <format>

namespace debugshape_export::effect {

namespace {

template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return (value.get());
    } else {
        return (value);
    }
}

template <class PayloadT>
PayloadT const& payloadOf(::Packet const& packet) {
    return static_cast<PayloadT const&>(static_cast<::ll::PayloadPacket<PayloadT> const&>(packet));
}

} // namespace

ActorEventFilter& ActorEventFilter::getInstance() {
    static ActorEventFilter instance;
    return instance;
}

bool ActorEventFilter::match(std::unordered_set<int> const& set, int eventId) {
    return !set.empty() && (set.contains(-1) || set.contains(eventId));
}

void ActorEventFilter::block(int eventId) {
    std::lock_guard lock(mMutex);
    mGlobal.insert(eventId);
}

void ActorEventFilter::unblock(int eventId) {
    std::lock_guard lock(mMutex);
    mGlobal.erase(eventId);
    if (eventId == -1) {
        mGlobal.clear();
    }
}

void ActorEventFilter::blockForPlayer(std::string const& playerName, int eventId) {
    std::lock_guard lock(mMutex);
    mPlayers[playerName].insert(eventId);
}

void ActorEventFilter::unblockForPlayer(std::string const& playerName, int eventId) {
    std::lock_guard lock(mMutex);
    auto            it = mPlayers.find(playerName);
    if (it == mPlayers.end()) {
        return;
    }
    if (eventId == -1) {
        mPlayers.erase(it);
        return;
    }
    it->second.erase(eventId);
    if (it->second.empty()) {
        mPlayers.erase(it);
    }
}

void ActorEventFilter::blockForEntity(int64_t runtimeId, int eventId) {
    std::lock_guard lock(mMutex);
    mEntities[runtimeId].insert(eventId);
}

void ActorEventFilter::unblockForEntity(int64_t runtimeId, int eventId) {
    std::lock_guard lock(mMutex);
    auto            it = mEntities.find(runtimeId);
    if (it == mEntities.end()) {
        return;
    }
    if (eventId == -1) {
        mEntities.erase(it);
        return;
    }
    it->second.erase(eventId);
    if (it->second.empty()) {
        mEntities.erase(it);
    }
}

void ActorEventFilter::clear() {
    std::lock_guard lock(mMutex);
    mGlobal.clear();
    mPlayers.clear();
    mEntities.clear();
}

bool ActorEventFilter::active() const {
    std::lock_guard lock(mMutex);
    return !mGlobal.empty() || !mPlayers.empty() || !mEntities.empty();
}

std::string ActorEventFilter::stats() const {
    std::lock_guard lock(mMutex);
    return std::format(
        "blocked={} passed={} rules=g{}/p{}/e{}",
        mBlocked.load(std::memory_order_relaxed),
        mPassed.load(std::memory_order_relaxed),
        mGlobal.size(),
        mPlayers.size(),
        mEntities.size());
}

// 全局 / 逐玩家 / 逐实体三条规则命中任一即拦
bool ActorEventFilter::shouldDrop(::Player const& recipient, std::uint64_t runtimeId, int eventId) {
    bool drop = false;
    {
        std::lock_guard lock(mMutex);
        drop = match(mGlobal, eventId);
        if (!drop) {
            auto const name = recipient.getRealName();
            auto       pit  = mPlayers.find(name);
            if (pit != mPlayers.end()) {
                drop = match(pit->second, eventId);
            }
        }
        if (!drop) {
            auto eit = mEntities.find(static_cast<std::int64_t>(runtimeId));
            if (eit != mEntities.end()) {
                drop = match(eit->second, eventId);
            }
        }
    }
    if (drop) {
        mBlocked.fetch_add(1, std::memory_order_relaxed);
    } else {
        mPassed.fetch_add(1, std::memory_order_relaxed);
    }
    return drop;
}

bool ActorEventFilter::readActorEvent(::Packet const& packet, std::uint64_t& runtimeId, int& eventId) {
    if (packet.getId() != ::MinecraftPacketIds::ActorEvent) {
        return false;
    }
    auto const& payload = payloadOf<::ActorEventPacketPayload>(packet);
    runtimeId           = static_cast<std::uint64_t>(unwrap(payload.mRuntimeId).rawID);
    eventId             = static_cast<int>(unwrap(payload.mEventId));
    return true;
}

} // namespace debugshape_export::effect
