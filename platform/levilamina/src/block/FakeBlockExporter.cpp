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

#include "core/proto/Version.h"

#include <mc/deps/nbt/CompoundTag.h>

#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

static constexpr const char* NAMESPACE = "HologramLib";

using blockstate::parseBlockState;
using blockstate::resolveNetworkId;
using blockstate::resolveNetworkIdFromSnbt;

namespace {

bool sendUpdateBlock(::Player& player, int x, int y, int z, std::uint32_t runtimeId) {
    proto::UpdateBlockPacket packet;
    packet.pos          = proto::BlockPos{x, y, z};
    packet.runtime_id   = proto::BlockRuntimeId{runtimeId};
    packet.update_flags = 3;
    packet.layer        = 0;

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;
    return wire::sendRaw(player, bytes);
}

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}


// mc NBT → 协议层 bp NBT（走网络 NBT 二进制）
bool sendBlockActorData(::Player& player, int x, int y, int z, ::CompoundTag const& tag) {
    auto binary = tag.toNetworkNbt();
    if (binary.empty())
        return false;
    std::string_view raw{binary};
    proto::bp::BinaryReader reader{raw};
    auto named = proto::bp::nbt::readNamedTag(reader);
    if (!named)
        return false;
    auto* compound = named->second.template get_if<proto::bp::CompoundTag>();
    if (compound == nullptr)
        return false;

    proto::BlockActorDataPacket packet;
    packet.pos  = proto::BlockPos{x, y, z};
    packet.data = std::move(*compound);

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;
    return wire::sendRaw(player, bytes);
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
        "blockFakeResolveNbt",
        [](std::string const& snbt) -> int64_t {
            std::uint32_t id = 0;
            return resolveNetworkIdFromSnbt(snbt, id) ? static_cast<int64_t>(id) : -1;
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockFakeSetNbt",
        [](std::string const& playerName, int x, int y, int z, std::string const& snbt) -> bool {
            auto* player = findPlayer(playerName);
            if (player == nullptr)
                return false;
            auto parsed = ::CompoundTag::fromSnbt(snbt);
            if (!parsed)
                return false;
            std::uint32_t id = 0;
            if (!resolveNetworkIdFromSnbt(snbt, id))
                return false;
            if (!sendUpdateBlock(*player, x, y, z, id))
                return false;
            // 带 "id" 的是方块实体（Banner / Sign / Chest ...），补一个 BlockActorData
            if (parsed->contains("id"))
                sendBlockActorData(*player, x, y, z, *parsed);
            return true;
        });


    // 方块实体数据（告示牌文字 / 旗帜图案 / 箱子内容）：snbt 是 BlockEntity.getNbt().toSNBT()
    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockFakeSetBlockEntity",
        [](std::string const& playerName, int x, int y, int z, std::string const& snbt) -> bool {
            auto* player = findPlayer(playerName);
            if (player == nullptr)
                return false;
            auto parsed = ::CompoundTag::fromSnbt(snbt);
            if (!parsed)
                return false;
            if (!parsed->contains("id"))
                return false;
            // 坐标以参数为准，覆盖 NBT 里的 x/y/z
            (*parsed)["x"] = ::IntTag{x};
            (*parsed)["y"] = ::IntTag{y};
            (*parsed)["z"] = ::IntTag{z};
            return sendBlockActorData(*player, x, y, z, *parsed);
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
