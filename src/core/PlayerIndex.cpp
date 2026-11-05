#include "core/PlayerIndex.h"

#include "core/EventIds.h"

#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {
namespace core {

PlayerIndex& PlayerIndex::getInstance() {
    static PlayerIndex instance;
    return instance;
}

void PlayerIndex::track(::Player& player) {
    std::string      name = player.getRealName();
    auto             id   = player.getNetworkIdentifier();
    std::unique_lock lock(mMutex);
    mByNetId.insert_or_assign(id, &player);
    mNames.insert_or_assign(&player, std::move(name));
}

void PlayerIndex::forget(::Player& player) {
    std::unique_lock lock(mMutex);
    mByNetId.erase(player.getNetworkIdentifier());
    mNames.erase(&player);
}

::Player* PlayerIndex::byNetworkId(::NetworkIdentifier const& id) const {
    std::shared_lock lock(mMutex);
    auto const       it = mByNetId.find(id);
    return it == mByNetId.end() ? nullptr : it->second;
}

std::string_view PlayerIndex::nameOf(::Player const& player) const {
    {
        std::shared_lock lock(mMutex);
        auto const       it = mNames.find(&player);
        if (it != mNames.end()) {
            return it->second;
        }
    }
    // 未命中：补一次（名字基本不变，一次分配换掉后续所有 getRealName 拷贝）
    auto             name = player.getRealName();
    std::unique_lock lock(mMutex);
    auto [it, inserted] = mNames.try_emplace(&player, std::move(name));
    return it->second;
}

void PlayerIndex::install() {
    if (mJoinListener != nullptr) {
        return;
    }

    mJoinListener = core::listen<ll::event::PlayerJoinEvent>(core::kPlayerJoinId, [this](ll::event::PlayerJoinEvent& ev) {
        track(ev.self());
    });
    mLeaveListener =
        core::listen<ll::event::PlayerDisconnectEvent>(core::kPlayerDisconnectId, [this](ll::event::PlayerDisconnectEvent& ev) {
            forget(ev.self());
        });

    // 兜底：install 之前已经在线的玩家（插件热重载/晚启用）
    if (auto level = ::ll::service::getLevel()) {
        level->forEachPlayer([this](::Player& player) -> bool {
            track(player);
            return true;
        });
    }
}

void PlayerIndex::shutdown() {
    std::unique_lock lock(mMutex);
    mByNetId.clear();
    mNames.clear();
    if (mJoinListener != nullptr) {
        ll::event::EventBus::getInstance().removeListener(mJoinListener);
        mJoinListener = nullptr;
    }
    if (mLeaveListener != nullptr) {
        ll::event::EventBus::getInstance().removeListener(mLeaveListener);
        mLeaveListener = nullptr;
    }
}

} // namespace core
} // namespace debugshape_export
