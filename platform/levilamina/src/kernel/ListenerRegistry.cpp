#include "kernel/EventIds.h"

#include <ll/api/event/EventBus.h>

#include <mutex>
#include <vector>

namespace debugshape_export::core {

namespace {

std::mutex& registryMutex() {
    static std::mutex m;
    return m;
}

std::vector<std::weak_ptr<ll::event::ListenerBase>>& registry() {
    static std::vector<std::weak_ptr<ll::event::ListenerBase>> v;
    return v;
}

} // namespace

void trackListener(std::weak_ptr<ll::event::ListenerBase> listener) {
    std::lock_guard lock(registryMutex());
    registry().push_back(std::move(listener));
}

void removeAllListeners() {
    std::vector<std::weak_ptr<ll::event::ListenerBase>> snapshot;
    {
        std::lock_guard lock(registryMutex());
        snapshot.swap(registry());
    }
    auto& bus = ll::event::EventBus::getInstance();
    for (auto& weak : snapshot) {
        if (auto listener = weak.lock()) {
            bus.removeListener(listener);
        }
    }
}

PlayerLeaveHook::PlayerLeaveHook(std::function<void(std::string const&)> onLeave) {
    if (!onLeave) {
        return;
    }
    mListener = listen<ll::event::PlayerDisconnectEvent>(
        kPlayerDisconnectId,
        [fn = std::move(onLeave)](ll::event::PlayerDisconnectEvent& ev) { fn(ev.self().getRealName()); },
        ListenerScope::Process);
}

} // namespace debugshape_export::core
