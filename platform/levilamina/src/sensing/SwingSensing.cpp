#include "PlayerSensingManager.h"

#include "core/HookRegistry.h"
#include "kernel/PlayerIndex.h"

#include <ll/api/memory/Hook.h>

#include <mc/network/NetworkIdentifier.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/AnimatePacket.h>
#include <mc/world/actor/player/Player.h>

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

namespace debugshape_export {

namespace {

std::mutex                                     gMutex;
std::unordered_map<std::string, std::uint64_t> gSwings;
std::atomic<std::uint64_t>                     gTotal{0};

void noteSwing(::Player& player) {
    std::lock_guard lock(gMutex);
    ++gSwings[player.getRealName()];
    ++gTotal;
}

} // namespace

// 客户端「空手挥动 / 攻击空气」走服务端 AnimatePacket(action = Swing)。
// 这里只做计数与查询，不改变任何服务端行为。
LL_TYPE_INSTANCE_HOOK(
    SwingPacketHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    ::NetworkIdentifier const& source,
    ::AnimatePacket const&     packet) {
    if (debugshape_export::core::live() && packet.mAction == ::AnimatePacket::Action::Swing) {
        if (auto* player = debugshape_export::core::PlayerIndex::getInstance().byNetworkId(source); player != nullptr) {
            noteSwing(*player);
        }
    }
    origin(source, packet);
}

static ll::memory::HookRegistrar<SwingPacketHook> gSwingPacketHook;
static debugshape_export::core::HookRegistrar     gHookReg_swing{
    "sensing-swing",
    [] {
        decltype(gSwingPacketHook)::hook();
    },
    [] {
        decltype(gSwingPacketHook)::unhook();
    },
};

std::uint64_t swingCount(std::string const& playerName) {
    std::lock_guard lock(gMutex);
    auto const      it = gSwings.find(playerName);
    return it != gSwings.end() ? it->second : 0;
}

std::uint64_t swingTotal() {
    return gTotal.load(std::memory_order_relaxed);
}

void swingReset() {
    std::lock_guard lock(gMutex);
    gSwings.clear();
    gTotal.store(0, std::memory_order_relaxed);
}

void forgetSwings(std::string const& playerName) {
    std::lock_guard lock(gMutex);
    gSwings.erase(playerName);
}

} // namespace debugshape_export
