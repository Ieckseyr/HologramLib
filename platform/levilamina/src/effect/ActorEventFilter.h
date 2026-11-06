#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Player;
class Packet;

namespace debugshape_export::effect {

// 表现事件拦截规则
class ActorEventFilter {
public:
    static ActorEventFilter& getInstance();

    void block(int eventId);
    void unblock(int eventId);
    void blockForPlayer(std::string const& playerName, int eventId);
    void unblockForPlayer(std::string const& playerName, int eventId);
    void blockForEntity(int64_t runtimeId, int eventId);
    void unblockForEntity(int64_t runtimeId, int eventId);
    void clear();

    [[nodiscard]] std::string stats() const;

    [[nodiscard]] bool active() const;

    bool shouldDrop(::Player const& recipient, std::uint64_t runtimeId, int eventId);

    static bool readActorEvent(::Packet const& packet, std::uint64_t& runtimeId, int& eventId);

private:
    ActorEventFilter() = default;

    static bool match(std::unordered_set<int> const& set, int eventId);

    mutable std::mutex                                        mMutex;
    std::unordered_set<int>                                   mGlobal;
    std::unordered_map<std::string, std::unordered_set<int>>  mPlayers;
    std::unordered_map<std::int64_t, std::unordered_set<int>> mEntities;
    mutable std::atomic<std::uint64_t>                        mBlocked{0};
    mutable std::atomic<std::uint64_t>                        mPassed{0};
};

} // namespace debugshape_export::effect
