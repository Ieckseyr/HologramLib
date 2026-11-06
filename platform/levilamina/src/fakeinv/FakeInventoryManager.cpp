#include "core/DiagFlags.h"
#include "kernel/EventIds.h"

#include "FakeInventoryManager.h"

#include "DiagLog.h"
#include "kernel/PacketSink.h"
#include "fakeinv/FakeInventoryPackets.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/event/world/ServerLevelTickEvent.h>
#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/containers/ContainerEnumName.h>
#include <mc/world/level/Level.h>

#include <format>

namespace debugshape_export {

namespace {

Player* findPlayerByName(std::string const& name) {
    auto level = ll::service::getLevel();
    if (!level)
        return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) -> bool {
        if (p.getRealName() == name) {
            found = &p;
            return false;
        }
        return true;
    });
    return found;
}

bool isPlayerInventoryEnum(int name) {
    switch (static_cast<::ContainerEnumName>(name)) {
    case ContainerEnumName::CombinedHotbarAndInventoryContainer:
    case ContainerEnumName::HotbarContainer:
    case ContainerEnumName::InventoryContainer:
        return true;
    default:
        return false;
    }
}

constexpr int kMinRefreshIntervalTicks = 5;

} // namespace

FakeInventoryManager& FakeInventoryManager::getInstance() {
    static FakeInventoryManager instance;
    return instance;
}

FakeInventoryManager::Session* FakeInventoryManager::findLocked(std::string const& playerName) {
    auto it = mSessions.find(playerName);
    return it == mSessions.end() ? nullptr : &it->second;
}

bool FakeInventoryManager::slotIsFaked(Session const& session, int slot) {
    if (slot < 0 || slot >= hologramlib::kFakeInventorySlots)
        return false;
    if (slot >= static_cast<int>(session.spec.items.size()))
        return false;
    return !session.spec.items[slot].type.empty();
}

bool FakeInventoryManager::apply(std::string const& playerName, hologramlib::FakeInventorySpec const& spec) {
    if (hlibDomainDisabled("HLIB_NO_CONTAINER"))
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;

    auto content = fakeinv::makeInventoryContent(spec.items, hologramlib::kFakeInventorySlots);
    (void)wire::send(*player, content);
    {
        std::lock_guard lock(mMutex);
        auto&           session   = mSessions[playerName];
        session.playerName        = playerName;
        session.spec              = spec;
        session.ticksSinceRefresh = 0;
    }
    HLIB_LOG_INFO(
        "[FakeInv] 已下发: player={} 条目={} 刷新间隔={} tick",
        playerName,
        spec.items.size(),
        spec.refreshIntervalTicks);
    return true;
}

bool FakeInventoryManager::setSlot(std::string const& playerName, int slot, hologramlib::ContainerMenuItem const& item) {
    if (hlibDomainDisabled("HLIB_NO_CONTAINER"))
        return false;
    if (slot < 0 || slot >= hologramlib::kFakeInventorySlots)
        return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;
    {
        std::lock_guard lock(mMutex);
        auto*           session = findLocked(playerName);
        if (session == nullptr)
            return false;
        if (slot >= static_cast<int>(session->spec.items.size())) {
            session->spec.items.resize(static_cast<std::size_t>(hologramlib::kFakeInventorySlots));
        }
        session->spec.items[slot]  = item;
        session->ticksSinceRefresh = 0;
    }
    auto slotPacket = fakeinv::makeInventorySlot(slot, item);
    (void)wire::send(*player, slotPacket);
    return true;
}

bool FakeInventoryManager::refresh(std::string const& playerName) {
    if (hlibDomainDisabled("HLIB_NO_CONTAINER"))
        return false;
    hologramlib::FakeInventorySpec spec;
    {
        std::lock_guard lock(mMutex);
        auto*           session = findLocked(playerName);
        if (session == nullptr)
            return false;
        spec                       = session->spec;
        session->ticksSinceRefresh = 0;
    }
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return false;

    auto content = fakeinv::makeInventoryContent(spec.items, hologramlib::kFakeInventorySlots);
    (void)wire::send(*player, content);
    return true;
}

bool FakeInventoryManager::clear(std::string const& playerName) {
    bool had = false;
    {
        std::lock_guard lock(mMutex);
        had = mSessions.erase(playerName) > 0;
    }
    if (!had)
        return false;

    if (auto* player = findPlayerByName(playerName))
        player->sendInventory(false);
    HLIB_LOG_INFO("[FakeInv] 已撤销: player={}", playerName);
    return true;
}

void FakeInventoryManager::clearAll() {
    std::vector<std::string> names;
    {
        std::lock_guard lock(mMutex);
        for (auto const& [name, session] : mSessions)
            names.push_back(name);
    }
    for (auto const& name : names)
        clear(name);
}

bool FakeInventoryManager::isActive(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    return mSessions.contains(playerName);
}

std::vector<std::string> FakeInventoryManager::getActivePlayers() const {
    std::lock_guard          lock(mMutex);
    std::vector<std::string> out;
    out.reserve(mSessions.size());
    for (auto const& [name, session] : mSessions)
        out.push_back(name);
    return out;
}

uint64_t FakeInventoryManager::addClickListener(
    std::function<void(hologramlib::FakeInventoryClickEvent const&)> listener) {
    if (!listener)
        return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextListenerToken++;
    mListeners.emplace(token, std::move(listener));
    return token;
}

bool FakeInventoryManager::removeClickListener(uint64_t token) {
    std::lock_guard lock(mMutex);
    return mListeners.erase(token) > 0;
}

std::vector<hologramlib::FakeInventoryClickEvent> FakeInventoryManager::pollClicks() {
    std::vector<hologramlib::FakeInventoryClickEvent> out;
    std::lock_guard                                   lock(mMutex);
    out.reserve(mClickQueue.size());
    while (!mClickQueue.empty()) {
        out.push_back(std::move(mClickQueue.front()));
        mClickQueue.pop_front();
    }
    return out;
}

void FakeInventoryManager::clearClicks() {
    std::lock_guard lock(mMutex);
    mClickQueue.clear();
}

std::string FakeInventoryManager::formatClick(hologramlib::FakeInventoryClickEvent const& event) {
    return std::format("player={} slot={}", event.playerName, event.slot);
}

bool FakeInventoryManager::handleInventoryAction(std::string const& playerName, int containerEnum, int slot) {
    if (!isPlayerInventoryEnum(containerEnum))
        return false;

    hologramlib::ContainerMenuItem item;
    {
        std::lock_guard lock(mMutex);
        auto*           session = findLocked(playerName);
        if (session == nullptr)
            return false;
        if (!slotIsFaked(*session, slot))
            return false;
        item = session->spec.items[slot];
    }

    hologramlib::FakeInventoryClickEvent event;
    event.playerName = playerName;
    event.slot       = slot;

    std::vector<std::function<void(hologramlib::FakeInventoryClickEvent const&)>> snapshot;
    {
        std::lock_guard lock(mMutex);
        if (mClickQueue.size() >= kClickQueueCap) {
            mClickQueue.pop_front();
            mDroppedClicks.fetch_add(1, std::memory_order_relaxed);
        }
        mClickQueue.push_back(event);
        snapshot.reserve(mListeners.size());
        for (auto const& [token, fn] : mListeners)
            snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn)
            fn(event);
    }

    if (auto* player = findPlayerByName(playerName)) {
        auto slotPacket = fakeinv::makeInventorySlot(slot, item);
        (void)wire::send(*player, slotPacket);
    }
    HLIB_LOG_INFO("[FakeInv] 点击: player={} slot={}", playerName, slot);
    return true;
}

void FakeInventoryManager::onPlayerLeave(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    mSessions.erase(playerName);
}

std::uint64_t FakeInventoryManager::droppedClicks() const {
    return mDroppedClicks.load(std::memory_order_relaxed);
}

void FakeInventoryManager::shutdown() {
    std::vector<std::string> names;
    {
        std::lock_guard lock(mMutex);
        for (auto const& [name, session] : mSessions)
            names.push_back(name);
    }
    for (auto const& name : names)
        clear(name);
}

void FakeInventoryManager::tick() {

    std::vector<hologramlib::FakeInventorySpec> due;
    std::vector<std::string>                    dueNames;
    {
        std::lock_guard lock(mMutex);
        for (auto& [name, session] : mSessions) {
            int interval = session.spec.refreshIntervalTicks;
            if (interval <= 0)
                continue;
            if (interval < kMinRefreshIntervalTicks)
                interval = kMinRefreshIntervalTicks;
            if (++session.ticksSinceRefresh < interval)
                continue;
            session.ticksSinceRefresh = 0;
            due.push_back(session.spec);
            dueNames.push_back(name);
        }
    }
    for (std::size_t i = 0; i < due.size(); ++i) {
        auto* player = findPlayerByName(dueNames[i]);
        if (player == nullptr)
            continue;
        auto content = fakeinv::makeInventoryContent(due[i].items, hologramlib::kFakeInventorySlots);
        (void)wire::send(*player, content);
    }
}

namespace {

struct FakeInventoryTickDriver {
    ll::event::ListenerPtr listener;
    FakeInventoryTickDriver() {
        listener = debugshape_export::core::listen<ll::event::ServerLevelTickEvent>(core::kServerLevelTickId, [](ll::event::ServerLevelTickEvent const&) {
            FakeInventoryManager::getInstance().tick();
        },
                                                                                    debugshape_export::core::ListenerScope::Process);
    }
};

FakeInventoryTickDriver const& fakeInventoryTickDriver() {
    static FakeInventoryTickDriver instance;
    return instance;
}

struct FakeInventoryTickDriverBootstrap {
    FakeInventoryTickDriverBootstrap() { (void)fakeInventoryTickDriver(); }
};

FakeInventoryTickDriverBootstrap const gFakeInventoryTickDriverBootstrap;

struct FakeInventoryDisconnectListener {
    ll::event::ListenerPtr listener;
    FakeInventoryDisconnectListener() {
        listener = debugshape_export::core::listen<ll::event::PlayerDisconnectEvent>(core::kPlayerDisconnectId, [](ll::event::PlayerDisconnectEvent& ev) {
            FakeInventoryManager::getInstance().onPlayerLeave(ev.self().getRealName());
        },
                                                                                     debugshape_export::core::ListenerScope::Process);
    }
};

FakeInventoryDisconnectListener const& fakeInventoryDisconnectListener() {
    static FakeInventoryDisconnectListener instance;
    return instance;
}

struct FakeInventoryDisconnectBootstrap {
    FakeInventoryDisconnectBootstrap() { (void)fakeInventoryDisconnectListener(); }
};

FakeInventoryDisconnectBootstrap const gFakeInventoryDisconnectBootstrap;

} // namespace

namespace {
class FakeInventoryAdapter final : public hologramlib::IFakeInventory {
public:
    bool apply(std::string const& playerName, hologramlib::FakeInventorySpec const& spec) override {
        return FakeInventoryManager::getInstance().apply(playerName, spec);
    }
    bool setSlot(std::string const& playerName, int slot, hologramlib::ContainerMenuItem const& item) override {
        return FakeInventoryManager::getInstance().setSlot(playerName, slot, item);
    }
    bool refresh(std::string const& playerName) override {
        return FakeInventoryManager::getInstance().refresh(playerName);
    }
    bool clear(std::string const& playerName) override {
        return FakeInventoryManager::getInstance().clear(playerName);
    }
    void               clearAll() override { FakeInventoryManager::getInstance().clearAll(); }
    [[nodiscard]] bool isActive(std::string const& playerName) const override {
        return FakeInventoryManager::getInstance().isActive(playerName);
    }
    [[nodiscard]] std::vector<std::string> getActivePlayers() const override {
        return FakeInventoryManager::getInstance().getActivePlayers();
    }
    uint64_t addClickListener(std::function<void(hologramlib::FakeInventoryClickEvent const&)> listener) override {
        return FakeInventoryManager::getInstance().addClickListener(std::move(listener));
    }
    bool removeClickListener(uint64_t token) override {
        return FakeInventoryManager::getInstance().removeClickListener(token);
    }
};
} // namespace

hologramlib::IFakeInventory& fakeInventoryAdapter() {
    static FakeInventoryAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
