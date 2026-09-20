// PlayerSensingManager.cpp - 感知域实现
#include "sensing/PlayerSensingManager.h"

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

// 玩家名 -> 客户端 InputMode 枚举值(26.40: 0=Undefined 1=Mouse 2=Touch 3=GamePad 4=MotionController)
hologramlib::ClientInputDevice mapDevice(int inputMode) {
    switch (inputMode) {
        case 1: return hologramlib::ClientInputDevice::KeyboardMouse;
        case 2: return hologramlib::ClientInputDevice::Touch;
        case 3: return hologramlib::ClientInputDevice::Gamepad;
        case 4: return hologramlib::ClientInputDevice::MotionController;
        default: return hologramlib::ClientInputDevice::Unknown;
    }
}

Player* findPlayerByNetworkId(NetworkIdentifier const& source) {
    auto level = ll::service::getLevel();
    if (!level) return nullptr;
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
    auto const device = mapDevice(inputMode);
    std::lock_guard lock(mMutex);
    // Unknown 不覆盖旧值: AuthInput 早段可能报 Undefined, 别把已知设备冲掉
    if (device == hologramlib::ClientInputDevice::Unknown) return;
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

// ── AuthInput 挂钩: 逐包捕获 InputMode ──
// AuthInput 每玩家每 tick 一包, 这里只做一次 map 写入, 开销可忽略。
// 只观察, 不拦。
LL_TYPE_INSTANCE_HOOK(
    PlayerSensingHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&     source,
    PlayerAuthInputPacket const& packet
) {
    origin(source, packet);

    if (auto* player = findPlayerByNetworkId(source)) {
        PlayerSensingManager::getInstance().record(player->getRealName(), static_cast<int>(packet.mInputMode));
    }
}

static ll::memory::HookRegistrar<PlayerSensingHook> gPlayerSensingHookRegistrar;

// 玩家离线: 清掉记录(重新上线后首个 AuthInput 会立刻补上)
static ll::event::ListenerPtr gSensingDisconnectListener;

struct SensingListenerInit {
    SensingListenerInit() {
        gSensingDisconnectListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
            [](ll::event::PlayerDisconnectEvent& ev) {
                PlayerSensingManager::getInstance().forget(ev.self().getRealName());
            }
        );
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
