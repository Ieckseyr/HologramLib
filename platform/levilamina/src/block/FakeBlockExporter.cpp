#include "FakeBlockExporter.h"

#include "BlockState.h"

#include "kernel/PacketSink.h"

#include "lse/LseBridge.h"

#include <ll/api/service/Bedrock.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/BlockPos.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/block/Block.h>

#include <sculk/protocol/codec/level/block/BlockPos.hpp>
#include <sculk/protocol/codec/packet/UpdateBlockPacket.hpp>

#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

using blockstate::parseBlockState;
using blockstate::resolveNetworkId;

namespace {

bool sendUpdateBlock(::Player& player, int x, int y, int z, std::uint32_t runtimeId) {
    sculk::protocol::UpdateBlockPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mRuntimeId     = runtimeId;
    packet.mFlag          = 3;
    packet.mLayer         = 0;

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;
    return wire::sendRaw(player, bytes);
}

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

} // namespace

void FakeBlockExporter::exportAll() {
    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockFakeResolve",
        [](std::string const& state) -> int64_t {
            std::uint32_t id = 0;
            return resolveNetworkId(state, id) ? static_cast<int64_t>(id) : -1;
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockFakeSet",
        [](std::string const& playerName, int x, int y, int z, std::string const& state) -> bool {
            auto* player = findPlayer(playerName);
            if (player == nullptr)
                return false;
            std::uint32_t id = 0;
            if (!resolveNetworkId(state, id))
                return false;
            return sendUpdateBlock(*player, x, y, z, id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockFakeClear",
        [](std::string const& playerName, int x, int y, int z) -> bool {
            auto* player = findPlayer(playerName);
            if (player == nullptr)
                return false;
            auto const& block = player->getDimensionBlockSource().getBlock(::BlockPos{x, y, z});
            return sendUpdateBlock(*player, x, y, z, static_cast<std::uint32_t>(block.mNetworkId));
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockFakeSetAll",
        [](int x, int y, int z, int dim, std::string const& state) -> int {
            std::uint32_t id = 0;
            if (!resolveNetworkId(state, id))
                return 0;
            auto level = ::ll::service::getLevel();
            if (!level)
                return 0;
            int sent = 0;
            level->forEachPlayer([&](::Player& player) -> bool {
                if (static_cast<int>(player.getDimensionId()) == dim) {
                    if (sendUpdateBlock(player, x, y, z, id))
                        ++sent;
                }
                return true;
            });
            return sent;
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockFakeClearAll",
        [](int x, int y, int z, int dim) -> int {
            auto level = ::ll::service::getLevel();
            if (!level)
                return 0;
            int sent = 0;
            level->forEachPlayer([&](::Player& player) -> bool {
                if (static_cast<int>(player.getDimensionId()) == dim) {
                    auto const& block = player.getDimensionBlockSource().getBlock(::BlockPos{x, y, z});
                    if (sendUpdateBlock(player, x, y, z, static_cast<std::uint32_t>(block.mNetworkId)))
                        ++sent;
                }
                return true;
            });
            return sent;
        });
}

} // namespace debugshape_export
