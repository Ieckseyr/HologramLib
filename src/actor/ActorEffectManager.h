#pragma once

#include <cstdint>
#include <string>

namespace debugshape_export {

// 实体表现事件发包
class ActorEffectManager {
public:
    static ActorEffectManager& getInstance();

    bool playHurt(int64_t entityRuntimeId);
    bool playDeath(int64_t entityRuntimeId);
    bool playInstantDeath(int64_t entityRuntimeId);

    bool play(int64_t entityRuntimeId, int eventId, int data);

    bool playFor(std::string const& playerName, int64_t entityRuntimeId, int eventId, int data);

private:
    ActorEffectManager() = default;

    bool dispatch(std::string const& playerName, int64_t entityRuntimeId, int eventId, int data);
};

} // namespace debugshape_export
