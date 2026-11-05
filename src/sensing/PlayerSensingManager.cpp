#include "sensing/PlayerSensingManager.h"
#include "core/EventIds.h"
#include "core/HookRegistry.h"

#include "DiagLog.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/Listener.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>

#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/PlayerAuthInputPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

namespace debugshape_export {

namespace {

hologramlib::ClientInputDevice mapDevice(int inputMode) {
    switch (inputMode) {
    case 1:
        return hologramlib::ClientInputDevice::KeyboardMouse;
    case 2:
        return hologramlib::ClientInputDevice::Touch;
    case 3:
        return hologramlib::ClientInputDevice::Gamepad;
    case 4:
        return hologramlib::ClientInputDevice::MotionController;
    default:
        return hologramlib::ClientInputDevice::Unknown;
    }
}

Player* findPlayerByNetworkId(NetworkIdentifier const& source) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) -> bool {
        if (p.getNetworkIdentifier() == source) {
            found = &p;
            return false;
        }
        return true;
    });
    return found;
}

} // namespace

PlayerSensingManager& PlayerSensingManager::getInstance() {
    static PlayerSensingManager instance;
    return instance;
}

void PlayerSensingManager::record(std::string const& playerName, int inputMode) {
    auto const      device = mapDevice(inputMode);
    std::lock_guard lock(mMutex);

    if (device == hologramlib::ClientInputDevice::Unknown)
        return;
    mDevices[playerName] = device;
}

hologramlib::ClientInputDevice PlayerSensingManager::deviceOf(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            it = mDevices.find(playerName);
    return it == mDevices.end() ? hologramlib::ClientInputDevice::Unknown : it->second;
}

bool PlayerSensingManager::isTouch(std::string const& playerName) const {
    return deviceOf(playerName) == hologramlib::ClientInputDevice::Touch;
}

void PlayerSensingManager::forget(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    mDevices.erase(playerName);
}

void PlayerSensingManager::shutdown() {
    std::lock_guard lock(mMutex);
    mDevices.clear();
}

LL_TYPE_INSTANCE_HOOK(
    PlayerSensingHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&     source,
    PlayerAuthInputPacket const& packet) {
    if (!debugshape_export::core::live()) {
        origin(source, packet);
        return;
    }
    origin(source, packet);

    if (auto* player = findPlayerByNetworkId(source)) {
        PlayerSensingManager::getInstance().record(player->getRealName(), static_cast<int>(packet.mInputMode));
    }
}

static ll::memory::HookRegistrar<PlayerSensingHook> gPlayerSensingHookRegistrar;
static debugshape_export::core::HookRegistrar       gHookReg_sensing_authinput{
    "sensing-authinput",
    [] {
        decltype(gPlayerSensingHookRegistrar)::hook();
    },
    [] {
        decltype(gPlayerSensingHookRegistrar)::unhook();
    },
};

static ll::event::ListenerPtr gSensingDisconnectListener;

struct SensingListenerInit {
    SensingListenerInit() {
        gSensingDisconnectListener = debugshape_export::core::listen<ll::event::PlayerDisconnectEvent>(core::kPlayerDisconnectId, [](ll::event::PlayerDisconnectEvent& ev) {
            PlayerSensingManager::getInstance().forget(ev.self().getRealName());
        },
                                                                                                       debugshape_export::core::ListenerScope::Process);
    }
};
static SensingListenerInit gSensingListenerInit;

namespace {
class PlayerSensingAdapter final : public hologramlib::IPlayerSensing {
public:
    [[nodiscard]] hologramlib::ClientInputDevice inputDeviceOf(std::string const& playerName) const override {
        return PlayerSensingManager::getInstance().deviceOf(playerName);
    }
    [[nodiscard]] bool isTouch(std::string const& playerName) const override {
        return PlayerSensingManager::getInstance().isTouch(playerName);
    }
};
} // namespace

hologramlib::IPlayerSensing& playerSensingAdapter() {
    static PlayerSensingAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
