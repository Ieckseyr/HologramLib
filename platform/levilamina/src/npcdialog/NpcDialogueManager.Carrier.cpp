#include "npcdialog/NpcDialogueManager.h"
#include "core/HookRegistry.h"
#include "kernel/EventIds.h"

#include "DiagLog.h"
#include "kernel/PacketSink.h"

#include "customentity/CustomEntityManager.h"
#include "npcdialog/NpcCarrierPacket.h"
#include "view/ViewOverrideExporter.h"
#include "view/ViewOverrideLogic.h"

#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/Compressibility.h>
#include <mc/network/NetworkIdentifier.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/NpcRequestPacket.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <format>

#include "core/proto/Version.h"

#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numbers>
#include <vector>

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

namespace {

std::atomic<std::int64_t>  gNextCarrierUid{-850403524570LL};
std::atomic<std::uint64_t> gNextCarrierRid{0x6E200000ULL};

template <typename PacketT>
bool sendPacket(Player& player, PacketT const& packet) {
    auto bytes = wire::encode(packet, /*validate=*/false);
    if (bytes.empty())
        return false;
    return wire::sendRaw(
        player,
        bytes,
        wire::Reliability::Reliable,
        wire::detail::packetName(packet),
        wire::detail::packetId(packet));
}

std::pair<std::uint64_t, std::uint64_t> sendCarrierSpawn(
    Player&                           player,
    std::uint64_t                     uniqueId,
    std::uint64_t                     runtimeId,
    hologramlib::NpcDialogSpec const& spec,
    std::string const&                actionJson) {
    auto const& pos      = player.getPosition();
    float const yaw      = player.getRotation().y;
    float const rad      = yaw * (std::numbers::pi_v<float> / 180.0f);
    auto const  position = proto::Vec3{
        pos.x + std::sin(rad) * 5.0f,
        npcdialog::kCarrierY,
        pos.z - std::cos(rad) * 5.0f};

    std::string identifier = spec.carrierIdentifier;
    std::string title      = spec.npcName;
    if (!spec.avatarViewSpec.empty()) {
        auto const view = parseEntitySpec(spec.avatarViewSpec);
        if (!view.identifier.empty()) {
            auto const normalised = view::logic::normaliseType(view.identifier);
            if (!normalised.empty())
                identifier = normalised;
        }
        if (view.hasNametag && !view.nametag.empty())
            title = view.nametag;
    }

    sendPacket(
        player,
        npcdialog::buildCarrierAddActor(
            uniqueId,
            runtimeId,
            identifier,
            title,
            actionJson,
            position,
            yaw,
            spec.avatarSkinVariant));
    return {uniqueId, runtimeId};
}

} // namespace

std::uint64_t NpcDialogueManager::nextCarrierUniqueId() {
    return static_cast<std::uint64_t>(gNextCarrierUid.fetch_sub(1));
}

std::uint64_t NpcDialogueManager::nextCarrierRuntimeId() {
    return gNextCarrierRid.fetch_add(1);
}

void NpcDialogueManager::removeCarrierEntity(Player& player, std::uint64_t uniqueId) {
    if (uniqueId == 0)
        return;
    proto::RemoveActorPacket packet;
    packet.entity_id = proto::ActorUniqueID{static_cast<std::int64_t>(uniqueId)};
    sendPacket(player, packet);
}

std::uint64_t NpcDialogueManager::ensureCarrier(
    Player&                           player,
    std::string const&                playerName,
    hologramlib::NpcDialogSpec const& spec,
    std::string const&                actionJson) {
    bool         hadPrevious = false;
    std::int64_t previousUid = 0;
    {
        std::lock_guard lock(mMutex);
        auto            it = mCarriers.find(playerName);
        if (it != mCarriers.end()) {
            hadPrevious = true;
            previousUid = it->second.uniqueId;
            mCarriers.erase(it);
        }
    }
    // 拆旧载体要发包，必须在锁外
    if (hadPrevious)
        removeCarrierEntity(player, previousUid);

    auto const uid = nextCarrierUniqueId();
    auto const rid = nextCarrierRuntimeId();

    auto const actual = sendCarrierSpawn(player, uid, rid, spec, actionJson);

    {
        std::lock_guard lock(mMutex);
        mCarriers[playerName] = Carrier{actual.first, actual.second};
    }
    HLIB_LOG_INFO(
        "[NpcDialog] 载体已生成: player={} uid={} rid={} (合成载体, y=-66)",
        playerName,
        actual.first,
        actual.second);
    return actual.first;
}

} // namespace debugshape_export
