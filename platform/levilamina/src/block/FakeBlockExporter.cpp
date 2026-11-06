#include "FakeBlockExporter.h"

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

namespace {

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

std::string unquote(std::string s) {
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front()) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// 解析 "minecraft:oak_stairs[waterlogged=true,upside_down_bit=false]" 这种状态串
bool parseBlockState(
    std::string const&                       text,
    std::string&                             outName,
    ::Block::BlockStatesType&                outStates
) {
    auto const        open  = text.find('[');
    std::string const name  = trim(open == std::string::npos ? text : text.substr(0, open));
    if (name.empty())
        return false;
    outName = name;

    if (open == std::string::npos)
        return true;

    auto const close = text.rfind(']');
    if (close == std::string::npos || close <= open)
        return false;

    std::string body = text.substr(open + 1, close - open - 1);
    std::vector<std::string> parts;
    std::string              current;
    bool                     inQuote = false;
    char                     quoteChar = 0;
    for (char ch : body) {
        if (inQuote) {
            current += ch;
            if (ch == quoteChar)
                inQuote = false;
            continue;
        }
        if (ch == '"' || ch == '\'') {
            inQuote   = true;
            quoteChar = ch;
            current += ch;
            continue;
        }
        if (ch == ',') {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current += ch;
    }
    if (!current.empty())
        parts.push_back(current);

    for (auto const& raw : parts) {
        auto const part = trim(raw);
        if (part.empty())
            continue;
        auto const eq = part.find('=');
        if (eq == std::string::npos)
            return false;
        auto const key = trim(part.substr(0, eq));
        auto const val = unquote(trim(part.substr(eq + 1)));
        if (key.empty())
            return false;

        if (val == "true" || val == "false") {
            outStates.emplace_back(key, val == "true");
            continue;
        }

        char* end      = nullptr;
        long  intValue = std::strtol(val.c_str(), &end, 10);
        if (end != nullptr && *end == '\0' && !val.empty()) {
            outStates.emplace_back(key, static_cast<int>(intValue));
            continue;
        }

        double floatValue = std::strtod(val.c_str(), &end);
        if (end != nullptr && *end == '\0' && !val.empty()) {
            outStates.emplace_back(key, static_cast<float>(floatValue));
            continue;
        }

        outStates.emplace_back(key, val);
    }
    return true;
}

bool resolveNetworkId(std::string const& stateText, std::uint32_t& outId) {
    std::string              name;
    ::Block::BlockStatesType states;
    if (!parseBlockState(stateText, name, states)) {
        return false;
    }
    auto block = ::Block::tryGetFromRegistry(::HashedString(name), states);
    if (!block) {
        return false;
    }
    outId = static_cast<std::uint32_t>(block->mNetworkId);
    return true;
}

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
