#include "SidebarManager.h"

#include "kernel/EventIds.h"
#include "kernel/PacketSink.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/MinecraftPacketIds.h>
#include <mc/network/packet/ChangeFakePlayerScore.h>
#include <mc/network/packet/RemoveScore.h>
#include <mc/network/packet/SetScorePacket.h>
#include <mc/network/packet/SetScorePacketPayload.h>
#include <mc/world/scores/ScoreboardId.h>

#include <sculk/protocol/codec/packet/RemoveObjectivePacket.hpp>
#include <sculk/protocol/codec/packet/SetDisplayObjectivePacket.hpp>

#include <format>

namespace debugshape_export {

namespace {

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

std::uint64_t fnv1a(std::string const& text, std::uint64_t seed) {
    std::uint64_t hash = seed;
    for (unsigned char const c : text) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

constexpr char const* kSlot     = "sidebar";
constexpr char const* kCriteria = "dummy";

} // namespace

SidebarManager& SidebarManager::getInstance() {
    static SidebarManager instance;
    return instance;
}

std::string SidebarManager::objectiveOf(std::string const& playerName) {

    return std::format("hlib_sb_{:08x}", static_cast<std::uint32_t>(fnv1a(playerName, 1469598103934665603ULL)));
}

std::int64_t SidebarManager::idBaseOf(std::string const& playerName) {

    auto const slot = fnv1a(playerName, 1099511628211ULL) % 4096ULL;
    return 100000LL + static_cast<std::int64_t>(slot) * 4096LL;
}

bool SidebarManager::sendDisplay(::Player& player, std::string const& objective, std::string const& title) {
    sculk::protocol::SetDisplayObjectivePacket packet;
    packet.mDisplaySlotName      = kSlot;
    packet.mObjectiveName        = objective;
    packet.mObjectiveDisplayName = title;
    packet.mCriteriaName         = kCriteria;

    packet.mSortOrder = sculk::protocol::SetDisplayObjectivePacket::ObjectiveSortOrder::Ascending;

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;
    wire::sendRaw(player, bytes);
    return true;
}

bool SidebarManager::sendScores(
    ::Player&                       player,
    std::string const&              objective,
    std::vector<std::string> const& lines,
    std::int64_t                    idBase) {

    ::SetScorePacketPayload payload;
    payload.mScoreInfo->reserve(lines.size());
    for (std::size_t i = 0; i < lines.size(); ++i) {
        ::ChangeFakePlayerScore entry;
        entry.mScoreboardId->mRawID = idBase + static_cast<std::int64_t>(i);
        entry.mObjectiveName        = objective;
        entry.mScoreValue           = static_cast<int>(i + 1);
        entry.mFakePlayerName       = lines[i];
        payload.mScoreInfo->emplace_back(entry);
    }

    ::SetScorePacket packet{payload};
    ::BinaryStream   stream;
    packet.write(stream);
    return wire::sendBody(
        player,
        static_cast<std::uint32_t>(::MinecraftPacketIds::SetScore),
        std::move(stream.mBuffer),
        wire::Reliability::Reliable,
        "SetScorePacket");
}

bool SidebarManager::sendRemoveScores(
    ::Player&          player,
    std::string const& objective,
    int                count,
    std::int64_t       idBase) {
    if (count <= 0)
        return true;

    ::SetScorePacketPayload payload;
    payload.mScoreInfo->reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        ::RemoveScore entry;
        entry.mScoreboardId->mRawID = idBase + static_cast<std::int64_t>(i);
        entry.mObjectiveName        = objective;
        payload.mScoreInfo->emplace_back(entry);
    }

    ::SetScorePacket packet{payload};
    ::BinaryStream   stream;
    packet.write(stream);
    return wire::sendBody(
        player,
        static_cast<std::uint32_t>(::MinecraftPacketIds::SetScore),
        std::move(stream.mBuffer),
        wire::Reliability::Reliable,
        "SetScorePacket(remove)");
}

bool SidebarManager::sendRemoveObjective(::Player& player, std::string const& objective) {
    sculk::protocol::RemoveObjectivePacket packet;
    packet.mObjectiveName = objective;

    auto bytes = wire::encode(packet);
    if (bytes.empty())
        return false;
    wire::sendRaw(player, bytes);
    return true;
}

bool SidebarManager::set(
    std::string const&              playerName,
    std::string const&              title,
    std::vector<std::string> const& lines) {
    if (lines.empty())
        return remove(playerName);

    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    auto const objective = objectiveOf(playerName);
    auto const idBase    = idBaseOf(playerName);

    Bar  previous;
    bool had = false;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(playerName);
        if (it != mBars.end()) {
            previous = it->second;
            had      = true;
        }
        mBars[playerName] = Bar{title, lines};
    }

    if (had)
        sendRemoveScores(*player, objective, static_cast<int>(previous.lines.size()), idBase);
    bool const titleChanged = !had || previous.title != title;
    if (titleChanged) {
        if (had)
            sendRemoveObjective(*player, objective);
        if (!sendDisplay(*player, objective, title))
            return false;
    }
    return sendScores(*player, objective, lines, idBase);
}

bool SidebarManager::setTitle(std::string const& playerName, std::string const& title) {
    std::vector<std::string> lines;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(playerName);
        if (it == mBars.end())
            return false;
        it->second.title = title;
        lines            = it->second.lines;
    }
    return set(playerName, title, lines);
}

bool SidebarManager::setLine(std::string const& playerName, int index, std::string const& text) {
    std::string              title;
    std::vector<std::string> lines;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(playerName);
        if (it == mBars.end())
            return false;
        if (index < 0 || index >= static_cast<int>(it->second.lines.size()))
            return false;
        it->second.lines[static_cast<std::size_t>(index)] = text;
        title                                             = it->second.title;
        lines                                             = it->second.lines;
    }
    return set(playerName, title, lines);
}

bool SidebarManager::remove(std::string const& playerName) {
    Bar  previous;
    bool had = false;
    {
        std::lock_guard lock(mMutex);
        auto            it = mBars.find(playerName);
        if (it == mBars.end())
            return false;
        previous = it->second;
        mBars.erase(it);
        had = true;
    }
    if (!had)
        return false;

    ::Player* player = findPlayer(playerName);
    if (player == nullptr)
        return true;

    auto const objective = objectiveOf(playerName);
    auto const idBase    = idBaseOf(playerName);
    sendRemoveScores(*player, objective, static_cast<int>(previous.lines.size()), idBase);
    sendRemoveObjective(*player, objective);
    return true;
}

void SidebarManager::removeAll() {
    std::vector<std::string> names;
    {
        std::lock_guard lock(mMutex);
        names.reserve(mBars.size());
        for (auto const& [name, bar] : mBars)
            names.push_back(name);
    }
    for (auto const& name : names)
        remove(name);
}

bool SidebarManager::isShown(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    return mBars.contains(playerName);
}

int SidebarManager::lineCount(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            it = mBars.find(playerName);
    return it == mBars.end() ? 0 : static_cast<int>(it->second.lines.size());
}

void SidebarManager::onPlayerLeave(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    mBars.erase(playerName);
}

void SidebarManager::shutdown() {
    std::lock_guard lock(mMutex);
    mBars.clear();
}

} // namespace debugshape_export
