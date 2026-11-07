#include "BlockBatchManager.h"

#include "BlockState.h"

#include "kernel/EventIds.h"

#include "sulfur/SulfurDisplayManager.h"

#include "hologramlib/HologramLib.h"
#include "kernel/PacketSink.h"

#include "core/BlockPackets.h"

#include <ll/api/event/EventBus.h>
#include <exception>
#include <ll/api/event/world/ServerLevelTickEvent.h>
#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <format>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

namespace debugshape_export::blockbatch {

namespace {

constexpr int kMaxPerPacket   = 1024; // 单包最多改动数
constexpr int kPacketsPerTick = 8;    // 每 tick 全局发送预算

struct SubChunkKey {
    int x{};
    int y{};
    int z{};

    bool operator<(SubChunkKey const& other) const {
        if (x != other.x)
            return x < other.x;
        if (y != other.y)
            return y < other.y;
        return z < other.z;
    }
};

std::string trimCopy(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

// "x y z state;x y z state" -> changes
std::vector<Change> parseChanges(std::string const& packed, std::size_t& outBad) {
    std::vector<Change> out;
    std::size_t         pos = 0;
    while (pos <= packed.size()) {
        auto const sep  = packed.find(';', pos);
        auto const item = trimCopy(packed.substr(pos, sep == std::string::npos ? std::string::npos : sep - pos));
        pos             = sep == std::string::npos ? packed.size() + 1 : sep + 1;
        if (item.empty())
            continue;

        int         x = 0;
        int         y = 0;
        int         z = 0;
        std::size_t used = 0;
        try {
            x    = std::stoi(item, &used);
            auto p1 = item.find_first_of(" \t", used);
            y    = std::stoi(item.substr(p1), &used);
            auto p2 = item.find_first_of(" \t", p1 + used);
            z    = std::stoi(item.substr(p2), &used);
            auto state = trimCopy(item.substr(p2 + used));
            if (state.empty()) {
                ++outBad;
                continue;
            }
            out.push_back(Change{x, y, z, state});
        } catch (std::exception const&) {
            ++outBad;
        }
    }
    return out;
}

// "x y z;x y z" -> positions
std::vector<Pos> parsePositions(std::string const& packed) {
    std::vector<Pos> out;
    std::size_t      pos = 0;
    while (pos <= packed.size()) {
        auto const sep  = packed.find(';', pos);
        auto const item = trimCopy(packed.substr(pos, sep == std::string::npos ? std::string::npos : sep - pos));
        pos             = sep == std::string::npos ? packed.size() + 1 : sep + 1;
        if (item.empty())
            continue;
        std::size_t used = 0;
        try {
            int         x = std::stoi(item, &used);
            auto const  p1 = item.find_first_of(" \t", used);
            int         y  = std::stoi(item.substr(p1), &used);
            auto const  p2 = item.find_first_of(" \t", p1 + used);
            int         z  = std::stoi(item.substr(p2 + used), &used);
            out.push_back(Pos{x, y, z});
        } catch (std::exception const&) {
        }
    }
    return out;
}

std::uint64_t packPos(int x, int y, int z) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x) & 0x3FFFFFFu) << 38)
         | (static_cast<std::uint64_t>(static_cast<std::uint32_t>(z) & 0x3FFFFFFu) << 12)
         | (static_cast<std::uint64_t>(static_cast<std::uint32_t>(y) & 0xFFFu));
}

::Player* findPlayer(std::string const& name) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(name) : nullptr;
}

} // namespace

struct BlockBatchManager::Impl {
    mutable std::mutex mutex;

    struct BatchState {
        std::size_t total{};
        std::size_t sent{};
    };

    struct Pending {
        std::string  playerName;
        std::string  bytes;
        std::int64_t batchId{};
    };

    int64_t                                   nextBatch{1};
    std::unordered_map<int64_t, BatchState>   batches;
    std::deque<Pending>                       queue;
    std::unordered_map<std::string, std::unordered_map<std::uint64_t, std::uint32_t>> overlay;

    std::uint64_t packets{};
    std::uint64_t bytes{};
    std::uint64_t dropped{};
    std::uint64_t badEntries{};

    struct NearEntry {
        int           x{};
        int           y{};
        int           z{};
        std::uint32_t runtimeId{};
        std::string   blockName;
    };

    struct NearSet {
        std::string                        playerName;
        double                             radius{4.0};
        bool                               invisible{false};
        std::vector<NearEntry>             blocks;
        std::unordered_map<std::uint64_t, int64_t> active; // 坐标 -> sulfurDisplay id
    };

    std::vector<NearSet> nearSets;
    int                  nearTickCounter{};

    ll::event::ListenerPtr tickListener;

    bool flushOne(Pending& pending) {
        auto* player = findPlayer(pending.playerName);
        if (player == nullptr) {
            ++dropped;
            return false;
        }
        wire::sendRaw(*player, pending.bytes, wire::Reliability::Reliable);
        ++packets;
        bytes += pending.bytes.size();
        return true;
    }
};

namespace {

void sendOne(::Player& player, int x, int y, int z, std::uint32_t runtimeId) {
    std::string body = debugshape_export::core::blocks::buildUpdateBlockBody(x, y, z, runtimeId, 3, 0);
    if (body.empty())
        return;
    std::string payload = wire::withHeader(debugshape_export::core::blocks::updateBlockPacketId(), std::move(body));
    wire::sendRaw(player, payload, wire::Reliability::Reliable);
}

std::uint32_t airRuntimeId() {
    static std::uint32_t cached = 0;
    if (cached == 0) {
        std::uint32_t id = 0;
        if (blockstate::resolveNetworkId("minecraft:air", id))
            cached = id;
    }
    return cached;
}

// 方块状态串（可能带 []）-> 方块名
std::string blockNameOf(std::string const& state) {
    auto const open = state.find('[');
    return open == std::string::npos ? state : state.substr(0, open);
}

} // namespace

BlockBatchManager::BlockBatchManager() = default;

BlockBatchManager::~BlockBatchManager() = default;

BlockBatchManager& BlockBatchManager::getInstance() {
    static BlockBatchManager instance;
    return instance;
}

void BlockBatchManager::init() {
    if (mImpl != nullptr)
        return;
    mImpl = std::make_unique<Impl>();
    mImpl->tickListener = core::listen<::ll::event::ServerLevelTickEvent>(
        core::kServerLevelTickId,
        [](::ll::event::ServerLevelTickEvent const&) { BlockBatchManager::getInstance().tick(); },
        core::ListenerScope::Process
    );
}

void BlockBatchManager::shutdown() {
    if (mImpl == nullptr)
        return;
    if (mImpl->tickListener) {
        ::ll::event::EventBus::getInstance().removeListener(mImpl->tickListener);
        mImpl->tickListener = nullptr;
    }
    mImpl.reset();
}

int64_t BlockBatchManager::submit(std::string const& packed, std::vector<std::string> const& viewers, bool immediate) {
    if (mImpl == nullptr || viewers.empty())
        return -1;

    std::size_t         bad = 0;
    std::vector<Change> changes = parseChanges(packed, bad);
    if (changes.empty())
        return -1;

    // 分组：子区块 -> 改动
    std::map<SubChunkKey, std::vector<core::blocks::ChangeEntry>> groups;
    std::size_t                                          skipped = 0;
    for (auto const& change : changes) {
        std::uint32_t runtimeId = 0;
        if (!blockstate::resolveNetworkId(change.state, runtimeId)) {
            ++skipped;
            continue;
        }
        SubChunkKey const key{change.x & ~15, change.y & ~15, change.z & ~15};
        core::blocks::ChangeEntry entry{};
        entry.x         = change.x;
        entry.y         = change.y;
        entry.z         = change.z;
        entry.runtimeId = runtimeId;
        entry.flags     = 0;
        groups[key].push_back(entry);
    }
    if (groups.empty())
        return -1;

    std::lock_guard lock(mImpl->mutex);
    mImpl->badEntries += bad + skipped;

    std::int64_t const batchId = mImpl->nextBatch++;
    auto&              state   = mImpl->batches[batchId];

    for (auto const& [key, entries] : groups) {
        for (std::size_t offset = 0; offset < entries.size(); offset += kMaxPerPacket) {
            std::size_t const count = std::min<std::size_t>(kMaxPerPacket, entries.size() - offset);
            std::vector<core::blocks::ChangeEntry> slice(entries.begin() + offset, entries.begin() + offset + count);
            std::string body = core::blocks::buildUpdateSubChunkBlocksBody(key.x, key.y, key.z, slice);
            if (body.empty()) {
                ++mImpl->dropped;
                continue;
            }
            std::string payload = wire::withHeader(core::blocks::updateSubChunkBlocksPacketId(), std::move(body));

            for (auto const& viewer : viewers) {
                mImpl->queue.push_back(Impl::Pending{viewer, payload, batchId});
                ++state.total;
            }
        }
    }

    if (immediate) {
        flushLocked(mImpl->queue.size());
    }
    return state.total == 0 ? -1 : batchId;
}

void BlockBatchManager::flushLocked(std::size_t budget) {
    if (mImpl == nullptr)
        return;
    std::size_t sent = 0;
    while (!mImpl->queue.empty() && sent < budget) {
        auto pending = std::move(mImpl->queue.front());
        mImpl->queue.pop_front();
        bool ok = mImpl->flushOne(pending);
        if (ok) {
            auto it = mImpl->batches.find(pending.batchId);
            if (it != mImpl->batches.end())
                ++it->second.sent;
        }
        ++sent;
    }
}

int64_t BlockBatchManager::setMany(std::string const& playerName, std::string const& packed, bool immediate) {
    if (findPlayer(playerName) == nullptr)
        return -1;
    return submit(packed, {playerName}, immediate);
}

int64_t BlockBatchManager::setManyAll(int dimId, std::string const& packed, bool immediate) {
    std::vector<std::string> viewers;
    auto                     level = ::ll::service::getLevel();
    if (!level)
        return -1;
    level->forEachPlayer([&](::Player& player) -> bool {
        if (static_cast<int>(player.getDimensionId()) == dimId)
            viewers.push_back(player.getRealName());
        return true;
    });
    if (viewers.empty())
        return -1;
    return submit(packed, viewers, immediate);
}

int64_t BlockBatchManager::setManyNear(
    std::string const& playerName,
    std::string const& packed,
    double             radius,
    bool               invisible,
    bool               immediate
) {
    int64_t const batchId = setMany(playerName, packed, immediate);
    if (batchId < 0 || mImpl == nullptr)
        return batchId;

    std::size_t         bad = 0;
    std::vector<Change> changes = parseChanges(packed, bad);

    std::lock_guard lock(mImpl->mutex);
    for (auto& set : mImpl->nearSets) {
        if (set.playerName != playerName)
            continue;
        for (auto const& [key, sulfurId] : set.active)
            SulfurDisplayManager::getInstance().destroy(sulfurId);
        set.active.clear();
    }
    mImpl->nearSets.erase(
        std::remove_if(
            mImpl->nearSets.begin(),
            mImpl->nearSets.end(),
            [&](Impl::NearSet const& set) { return set.playerName == playerName; }
        ),
        mImpl->nearSets.end()
    );

    Impl::NearSet set;
    set.playerName = playerName;
    set.radius     = radius <= 0.0 ? 4.0 : radius;
    set.invisible  = invisible;
    for (auto const& change : changes) {
        std::uint32_t runtimeId = 0;
        if (!blockstate::resolveNetworkId(change.state, runtimeId))
            continue;
        set.blocks.push_back(Impl::NearEntry{change.x, change.y, change.z, runtimeId, blockNameOf(change.state)});
    }
    if (set.blocks.empty())
        return -1;
    mImpl->nearSets.push_back(std::move(set));
    return batchId;
}

bool BlockBatchManager::clearMany(std::string const& playerName, std::string const& packedPositions) {
    if (mImpl == nullptr)
        return false;

    {
        std::lock_guard lock(mImpl->mutex);
        for (auto& set : mImpl->nearSets) {
            if (set.playerName != playerName)
                continue;
            for (auto const& [key, sulfurId] : set.active)
                SulfurDisplayManager::getInstance().destroy(sulfurId);
            set.active.clear();
        }
        mImpl->nearSets.erase(
            std::remove_if(
                mImpl->nearSets.begin(),
                mImpl->nearSets.end(),
                [&](Impl::NearSet const& set) { return set.playerName == playerName; }
            ),
            mImpl->nearSets.end()
        );
    }
    auto* player = findPlayer(playerName);
    if (player == nullptr)
        return false;

    auto positions = parsePositions(packedPositions);
    if (positions.empty())
        return false;

    bool any = false;
    for (auto const& pos : positions) {
        auto const& block = player->getDimensionBlockSource().getBlock(::BlockPos{pos.x, pos.y, pos.z});
        std::string body =
            core::blocks::buildUpdateBlockBody(pos.x, pos.y, pos.z, static_cast<std::uint32_t>(block.mNetworkId), 3, 0);
        if (body.empty())
            continue;
        std::string payload = wire::withHeader(core::blocks::updateBlockPacketId(), std::move(body));
        any = wire::sendRaw(*player, payload, wire::Reliability::Reliable) || any;
    }
    return any;
}

int BlockBatchManager::progress(int64_t batchId) const {
    if (mImpl == nullptr)
        return -1;
    std::lock_guard lock(mImpl->mutex);
    auto            it = mImpl->batches.find(batchId);
    if (it == mImpl->batches.end() || it->second.total == 0)
        return -1;
    return static_cast<int>(it->second.sent * 100 / it->second.total);
}

std::string BlockBatchManager::stats() const {
    if (mImpl == nullptr)
        return "blockbatch: 未初始化";
    std::lock_guard lock(mImpl->mutex);
    return std::format(
        "batch={} queue={} packets={} bytes={} dropped={} bad={}",
        mImpl->nextBatch - 1,
        mImpl->queue.size(),
        mImpl->packets,
        mImpl->bytes,
        mImpl->dropped,
        mImpl->badEntries
    ) + std::format(" nearSets={}", mImpl->nearSets.size());
}

std::string BlockBatchManager::selfTest(std::string const& packed) const {
    std::size_t         bad = 0;
    std::vector<Change> changes = parseChanges(packed, bad);
    if (changes.empty()) {
        return std::format("changes=0 bad={}", bad);
    }

    std::map<SubChunkKey, std::vector<core::blocks::ChangeEntry>> groups;
    std::size_t                                                   unresolved = 0;
    for (auto const& change : changes) {
        std::uint32_t runtimeId = 0;
        if (!blockstate::resolveNetworkId(change.state, runtimeId)) {
            ++unresolved;
            continue;
        }
        SubChunkKey const key{change.x & ~15, change.y & ~15, change.z & ~15};
        core::blocks::ChangeEntry entry{};
        entry.x         = change.x;
        entry.y         = change.y;
        entry.z         = change.z;
        entry.runtimeId = runtimeId;
        groups[key].push_back(entry);
    }

    std::size_t packets = 0;
    std::size_t bytes   = 0;
    for (auto const& [key, entries] : groups) {
        for (std::size_t offset = 0; offset < entries.size(); offset += kMaxPerPacket) {
            std::size_t const count = std::min<std::size_t>(kMaxPerPacket, entries.size() - offset);
            std::vector<core::blocks::ChangeEntry> slice(entries.begin() + offset, entries.begin() + offset + count);
            auto const body = core::blocks::buildUpdateSubChunkBlocksBody(key.x, key.y, key.z, slice);
            if (body.empty())
                continue;
            ++packets;
            bytes += body.size();
        }
    }
    return std::format(
        "changes={} groups={} packets={} bytes={} bad={} unresolved={}",
        changes.size(), groups.size(), packets, bytes, bad, unresolved
    );
}

void BlockBatchManager::onPlayerLeave(std::string const& playerName) {
    if (mImpl == nullptr)
        return;
    std::lock_guard lock(mImpl->mutex);
    mImpl->overlay.erase(playerName);

    for (auto& set : mImpl->nearSets) {
        if (set.playerName != playerName)
            continue;
        for (auto const& [key, sulfurId] : set.active)
            SulfurDisplayManager::getInstance().destroy(sulfurId);
        set.active.clear();
    }
    mImpl->nearSets.erase(
        std::remove_if(
            mImpl->nearSets.begin(), mImpl->nearSets.end(),
            [&](Impl::NearSet const& set) { return set.playerName == playerName; }
        ),
        mImpl->nearSets.end()
    );
    for (auto it = mImpl->queue.begin(); it != mImpl->queue.end();) {
        if (it->playerName == playerName) {
            it = mImpl->queue.erase(it);
        } else {
            ++it;
        }
    }
}

void BlockBatchManager::tick() {
    if (mImpl == nullptr)
        return;
    std::lock_guard lock(mImpl->mutex);
    flushLocked(kPacketsPerTick);

    // 靠近切换每 4 tick 检查一次，避免每 tick 遍历整批方块
    if (++mImpl->nearTickCounter < 4)
        return;
    mImpl->nearTickCounter = 0;

    for (auto& set : mImpl->nearSets) {
        auto* player = findPlayer(set.playerName);
        if (player == nullptr)
            continue;
        auto const  pos       = player->getPosition();
        int const   dim       = static_cast<int>(player->getDimensionId());
        double const nearSq   = set.radius * set.radius;
        double const leaveSq  = (set.radius * 1.3) * (set.radius * 1.3);

        for (auto const& block : set.blocks) {
            double const dx = pos.x - (static_cast<double>(block.x) + 0.5);
            double const dy = pos.y - static_cast<double>(block.y);
            double const dz = pos.z - (static_cast<double>(block.z) + 0.5);
            double const d2 = dx * dx + dy * dy + dz * dz;
            auto const   key = packPos(block.x, block.y, block.z);
            auto         it  = set.active.find(key);

            if (it == set.active.end()) {
                if (d2 > nearSq)
                    continue;
                sendOne(*player, block.x, block.y, block.z, airRuntimeId());
                hologramlib::SulfurDisplaySpec spec;
                spec.x        = static_cast<float>(block.x) + 0.5f;
                spec.y        = static_cast<float>(block.y);
                spec.z        = static_cast<float>(block.z) + 0.5f;
                spec.dim      = dim;
                spec.block.type = block.blockName;
                spec.block.count = 1;
                spec.invisible  = set.invisible;
                spec.visiblePlayers = {set.playerName};
                int64_t const sulfurId = SulfurDisplayManager::getInstance().create(spec);
                if (sulfurId >= 0)
                    set.active.emplace(key, sulfurId);
            } else if (d2 > leaveSq) {
                SulfurDisplayManager::getInstance().destroy(it->second);
                set.active.erase(it);
                sendOne(*player, block.x, block.y, block.z, block.runtimeId);
            }
        }
    }
}

} // namespace debugshape_export::blockbatch
