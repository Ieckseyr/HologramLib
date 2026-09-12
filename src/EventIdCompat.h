// EventIdCompat.h - LeviLamina 26.40 事件 ID 对齐（MSVC 编译的插件必需）
//
// 26.40 把事件类放进了 inline namespace，事件 ID 由编译器的类型名生成：
//   Clang 编译的官方 LeviLamina.dll → "ll::event::PlayerJoinEvent"
//   MSVC 编译的本库                 → "ll::event::player::PlayerJoinEvent"
// 两边对不上，本库注册的监听器就收不到任何事件（表现为实体类完全不向玩家发送）。
// 这里显式指定事件 ID，与官方侧保持一致。
#pragma once

#include <ll/api/event/EventId.h>

#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/event/server/ServerStartedEvent.h>
#include <ll/api/event/world/ServerLevelTickEvent.h>

namespace ll::event {
template <>
constexpr EventIdView getEventId<::ll::event::player::PlayerJoinEvent> =
    EventIdView{"ll::event::PlayerJoinEvent"};

template <>
constexpr EventIdView getEventId<::ll::event::player::PlayerDisconnectEvent> =
    EventIdView{"ll::event::PlayerDisconnectEvent"};

template <>
constexpr EventIdView getEventId<::ll::event::world::ServerLevelTickEvent> =
    EventIdView{"ll::event::ServerLevelTickEvent"};

template <>
constexpr EventIdView getEventId<::ll::event::server::ServerStartedEvent> =
    EventIdView{"ll::event::ServerStartedEvent"};
} // namespace ll::event
