// FakeInventoryManager.cpp - 背包虚容器实现（协议层手写包）
#include "FakeInventoryManager.h"

#include "DiagLog.h"
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
    if (!level) return nullptr;
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

// 本域接受的容器枚举: 客户端把玩家自己的背包归在这几个名下
//   12 = CombinedHotbarAndInventoryContainer（主背包+快捷栏一起看时）
//   28 = HotbarContainer / 29 = InventoryContainer（分开看时）
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

// 周期重发的最快间隔（tick）: 别让调用方用 1 tick 把客户端刷爆
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
    if (slot < 0 || slot >= hologramlib::kFakeInventorySlots) return false;
    if (slot >= static_cast<int>(session.spec.items.size())) return false;
    return !session.spec.items[slot].type.empty();
}

bool FakeInventoryManager::apply(std::string const& playerName, hologramlib::FakeInventorySpec const& spec) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;

    // 用 BDS 自己的包类: 物品描述符的 User Data 是另一套 NBT 方言, 手写必错（见 FakeInventoryPackets.h）
    auto content = fakeinv::makeInventoryContent(spec.items, hologramlib::kFakeInventorySlots);
    player->sendNetworkPacket(content);
    {
        std::lock_guard lock(mMutex);
        auto&           session = mSessions[playerName];
        session.playerName      = playerName;
        session.spec            = spec;
        session.ticksSinceRefresh = 0;
    }
    HLIB_LOG_INFO(
        "[FakeInv] 已下发: player={} 条目={} 刷新间隔={} tick",
        playerName,
        spec.items.size(),
        spec.refreshIntervalTicks
    );
    return true;
}

bool FakeInventoryManager::setSlot(std::string const& playerName, int slot, hologramlib::ContainerMenuItem const& item) {
    if (slot < 0 || slot >= hologramlib::kFakeInventorySlots) return false;
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;
    {
        std::lock_guard lock(mMutex);
        auto*           session = findLocked(playerName);
        if (session == nullptr) return false; // 还没 apply 过: 先 apply 再改格
        if (slot >= static_cast<int>(session->spec.items.size())) {
            session->spec.items.resize(static_cast<std::size_t>(hologramlib::kFakeInventorySlots));
        }
        session->spec.items[slot] = item;
        session->ticksSinceRefresh = 0;
    }
    auto slotPacket = fakeinv::makeInventorySlot(slot, item);
    player->sendNetworkPacket(slotPacket);
    return true;
}

bool FakeInventoryManager::refresh(std::string const& playerName) {
    hologramlib::FakeInventorySpec spec;
    {
        std::lock_guard lock(mMutex);
        auto*           session = findLocked(playerName);
        if (session == nullptr) return false;
        spec                      = session->spec;
        session->ticksSinceRefresh = 0;
    }
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;
    // 用 BDS 自己的包类: 物品描述符的 User Data 是另一套 NBT 方言, 手写必错（见 FakeInventoryPackets.h）
    auto content = fakeinv::makeInventoryContent(spec.items, hologramlib::kFakeInventorySlots);
    player->sendNetworkPacket(content);
    return true;
}

bool FakeInventoryManager::clear(std::string const& playerName) {
    bool had = false;
    {
        std::lock_guard lock(mMutex);
        had = mSessions.erase(playerName) > 0;
    }
    if (!had) return false;
    // 恢复真相: 让 BDS 把真实背包内容重发一遍（与本库"只在自己伪造时手写包"的约定一致）
    if (auto* player = findPlayerByName(playerName)) player->sendInventory(false);
    HLIB_LOG_INFO("[FakeInv] 已撤销: player={}", playerName);
    return true;
}

void FakeInventoryManager::clearAll() {
    std::vector<std::string> names;
    {
        std::lock_guard lock(mMutex);
        for (auto const& [name, session] : mSessions) names.push_back(name);
    }
    for (auto const& name : names) clear(name);
}

bool FakeInventoryManager::isActive(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    return mSessions.contains(playerName);
}

std::vector<std::string> FakeInventoryManager::getActivePlayers() const {
    std::lock_guard           lock(mMutex);
    std::vector<std::string>  out;
    out.reserve(mSessions.size());
    for (auto const& [name, session] : mSessions) out.push_back(name);
    return out;
}

uint64_t FakeInventoryManager::addClickListener(
    std::function<void(hologramlib::FakeInventoryClickEvent const&)> listener
) {
    if (!listener) return 0;
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
    if (!isPlayerInventoryEnum(containerEnum)) return false;

    hologramlib::ContainerMenuItem item;
    {
        std::lock_guard lock(mMutex);
        auto*           session = findLocked(playerName);
        if (session == nullptr) return false;
        if (!slotIsFaked(*session, slot)) return false; // 只有"伪造且非空"的格子才算功能项
        item = session->spec.items[slot];
    }

    hologramlib::FakeInventoryClickEvent event;
    event.playerName = playerName;
    event.slot       = slot;

    std::vector<std::function<void(hologramlib::FakeInventoryClickEvent const&)>> snapshot;
    {
        std::lock_guard lock(mMutex);
        mClickQueue.push_back(event); // LSE 侧同一份事件
        snapshot.reserve(mListeners.size());
        for (auto const& [token, fn] : mListeners) snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn) fn(event);
    }

    // BDS 会因为"服务端那格对不上"而把客户端那格回滚成真实物品 —— 把伪造内容重发一次盖回去,
    // 这样界面上那一格不会在点击后"变成别的东西"（与虚拟容器点击后物品闪回原位的表现一致）。
    if (auto* player = findPlayerByName(playerName)) {
        auto slotPacket = fakeinv::makeInventorySlot(slot, item);
    player->sendNetworkPacket(slotPacket);
    }
    HLIB_LOG_INFO("[FakeInv] 点击: player={} slot={}", playerName, slot);
    return true;
}

void FakeInventoryManager::onPlayerLeave(std::string const& playerName) {
    std::lock_guard lock(mMutex);
    mSessions.erase(playerName);
}

void FakeInventoryManager::shutdown() {
    std::vector<std::string> names;
    {
        std::lock_guard lock(mMutex);
        for (auto const& [name, session] : mSessions) names.push_back(name);
    }
    for (auto const& name : names) clear(name);
}

// ── tick 驱动: 周期重发伪造内容（BDS 的背包同步会把伪造内容覆盖掉）──
void FakeInventoryManager::tick() {
    // 先在锁内挑出到期的会话（快照 spec, 锁外发包）
    std::vector<hologramlib::FakeInventorySpec> due;
    std::vector<std::string>                    dueNames;
    {
        std::lock_guard lock(mMutex);
        for (auto& [name, session] : mSessions) {
            int interval = session.spec.refreshIntervalTicks;
            if (interval <= 0) continue;   // 关掉周期重发
            if (interval < kMinRefreshIntervalTicks) interval = kMinRefreshIntervalTicks; // 下限: 最快 5 tick
            if (++session.ticksSinceRefresh < interval) continue;
            session.ticksSinceRefresh = 0;
            due.push_back(session.spec);
            dueNames.push_back(name);
        }
    }
    for (std::size_t i = 0; i < due.size(); ++i) {
        auto* player = findPlayerByName(dueNames[i]);
        if (player == nullptr) continue;
        auto content = fakeinv::makeInventoryContent(due[i].items, hologramlib::kFakeInventorySlots);
        player->sendNetworkPacket(content);
    }
}

namespace {

struct FakeInventoryTickDriver {
    ll::event::ListenerPtr listener;
    FakeInventoryTickDriver() {
        listener = ll::event::EventBus::getInstance().emplaceListener<ll::event::ServerLevelTickEvent>(
            [](ll::event::ServerLevelTickEvent const&) { FakeInventoryManager::getInstance().tick(); }
        );
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

// ── 掉线清理: 会话必须随玩家离开销毁 ──
// 漏了这一步的实测后果: 会话留在内存里, 玩家重连后 tick 驱动立刻又给他重发一遍伪造内容
// （当时那个包的物品 NBT 方言是错的）→ 客户端在进服瞬间就卡死, 表现为"进不去服务器"。
struct FakeInventoryDisconnectListener {
    ll::event::ListenerPtr listener;
    FakeInventoryDisconnectListener() {
        listener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
            [](ll::event::PlayerDisconnectEvent& ev) {
                FakeInventoryManager::getInstance().onPlayerLeave(ev.self().getRealName());
            }
        );
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

// ── 公开接口适配 ──
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
    void clearAll() override { FakeInventoryManager::getInstance().clearAll(); }
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
