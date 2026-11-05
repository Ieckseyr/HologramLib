#pragma once

#include <ll/api/event/EventBus.h>
#include <ll/api/event/ListenerBase.h>

#include <mc/network/NetworkIdentifier.h>

#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

class Player;

namespace debugshape_export {
namespace core {

// 玩家索引
class PlayerIndex {
public:
    static PlayerIndex& getInstance();

    void install();
    void shutdown();

    [[nodiscard]] ::Player*        byNetworkId(::NetworkIdentifier const& id) const;
    [[nodiscard]] std::string_view nameOf(::Player const& player) const;

    void track(::Player& player);
    void forget(::Player& player);

private:
    PlayerIndex() = default;

    mutable std::shared_mutex                                  mMutex;
    mutable std::unordered_map<::NetworkIdentifier, ::Player*> mByNetId;
    mutable std::unordered_map<::Player const*, std::string>   mNames;
    ll::event::ListenerPtr                                     mJoinListener;
    ll::event::ListenerPtr                                     mLeaveListener;
};

} // namespace core
} // namespace debugshape_export
