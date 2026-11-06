#pragma once

#include <ll/api/event/EventBus.h>
#include <ll/api/event/EventId.h>
#include <ll/api/event/Listener.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>

#include <functional>
#include <memory>
#include <string>

namespace debugshape_export::core {

// 事件 ID
inline constexpr ll::event::EventIdView kPlayerJoinId{"ll::event::PlayerJoinEvent"};
inline constexpr ll::event::EventIdView kPlayerDisconnectId{"ll::event::PlayerDisconnectEvent"};
inline constexpr ll::event::EventIdView kServerStartedId{"ll::event::ServerStartedEvent"};
inline constexpr ll::event::EventIdView kServerLevelTickId{"ll::event::ServerLevelTickEvent"};

// 库是否启用
bool live();

void trackListener(std::weak_ptr<ll::event::ListenerBase> listener);
void removeAllListeners();

enum class ListenerScope { Domain,
                           Process };

template <class Event, class Fn>
ll::event::ListenerPtr listen(ll::event::EventIdView id, Fn&& fn, ListenerScope scope = ListenerScope::Domain) {
    auto inner   = std::forward<Fn>(fn);
    auto guarded = [inner = std::move(inner)](Event& ev) mutable {
        if (!live()) {
            return;
        }
        inner(ev);
    };
    auto listener = ll::event::Listener<Event>::create(std::move(guarded));
    ll::event::EventBus::getInstance().addListener(listener, id);
    if (scope == ListenerScope::Domain) {
        trackListener(listener);
    }
    return listener;
}

// 玩家下线清理挂点。进程级：disable 时不摘（摘了重新 enable 也不会补回来），
// 回调本身走 listen() 的 live() 门控，库停用时不会执行。
class PlayerLeaveHook {
public:
    explicit PlayerLeaveHook(std::function<void(std::string const&)> onLeave);

    PlayerLeaveHook(PlayerLeaveHook const&)            = delete;
    PlayerLeaveHook& operator=(PlayerLeaveHook const&) = delete;

private:
    ll::event::ListenerPtr mListener;
};

} // namespace debugshape_export::core
