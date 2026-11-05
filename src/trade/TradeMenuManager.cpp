#include "trade/TradeMenuManager.h"
#include "TradeMenuRuntimeOps.h"
#include "core/EventIds.h"

#include <ll/api/event/player/PlayerJoinEvent.h>

#include "DiagLog.h"
#include "core/PacketSink.h"

#include "container/ContainerResponse.h"
#include "customentity/CustomEntityManager.h"
#include "trade/TradeOfferNbt.h"
#include <mc/network/packet/InventorySlotPacket.h>

#include <ll/api/chrono/GameChrono.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/Compressibility.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/ServerNetworkHandler.h>

#include <mc/entity/components_json_legacy/EconomyTradeableComponent.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Inventory.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemInstance.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/trading/MerchantRecipe.h>
#include <mc/world/item/trading/MerchantRecipeList.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerEnumName.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerType.hpp>
#include <sculk/protocol/codec/inventory/item/ItemStackResponse.hpp>
#include <sculk/protocol/codec/packet/ContainerClosePacket.hpp>
#include <sculk/protocol/codec/packet/ItemStackResponsePacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateTradePacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <variant>
#include <vector>

namespace debugshape_export {
using namespace trade_ops;

TradeMenuManager& TradeMenuManager::getInstance() {
    static TradeMenuManager instance;
    return instance;
}

int TradeMenuManager::allocateContainerId() {

    for (int i = 0; i < 100; ++i) {
        int const candidate = 1 + (mNextContainerId - 1 + i) % 100;
        bool      used      = false;
        for (auto const& [id, menu] : mMenus) {
            if (menu.containerId == candidate) {
                used = true;
                break;
            }
        }
        if (!used) {
            mNextContainerId = candidate % 100 + 1;
            return candidate;
        }
    }
    return 0;
}

int64_t TradeMenuManager::spawnCarrier(
    Player&                           player,
    std::string const&                playerName,
    hologramlib::TradeMenuSpec const& spec) {

    auto const& pos = player.getPosition();
    float const yaw = player.getRotation().y;
    float const rad = yaw * (std::numbers::pi_v<float> / 180.0f);

    float const bx = pos.x + std::sin(rad) * 5.0f;
    float const bz = pos.z - std::cos(rad) * 5.0f;

    hologramlib::CustomEntityConfig cfg;
    cfg.identifier   = spec.carrierIdentifier.empty() ? "minecraft:villager_v2" : spec.carrierIdentifier;
    cfg.x            = bx;
    cfg.y            = pos.y;
    cfg.z            = bz;
    cfg.dimension    = static_cast<int>(player.getDimensionId());
    cfg.yaw          = yaw;
    cfg.invisible    = true;
    cfg.viewDistance = 32.0;
    cfg.enabled      = true;

    auto& entities = CustomEntityManager::getInstance();
    auto  libId    = entities.createRandom(cfg);
    if (libId < 0) {
        HLIB_LOG_WARN("[TradeMenu] 载体实体生成失败（玩家 {}）", playerName);
        return -1;
    }

    entities.setVisiblePlayers(libId, std::vector<std::string>{playerName});
    entities.setInvisible(libId, true);
    return libId;
}

void TradeMenuManager::destroyCarrier(Menu& menu) {
    if (menu.carrierLibId < 0)
        return;
    CustomEntityManager::getInstance().destroy(menu.carrierLibId);
    menu.carrierLibId = -1;
}

::ItemInstance makeItemInstance(hologramlib::TradeMenuItem const& item) {
    auto const snbt = itemSnbt(item);
    if (snbt.empty())
        return ::ItemInstance::EMPTY_ITEM();
    auto tag = CompoundTag::fromSnbt(snbt);
    if (!tag.has_value())
        return ::ItemInstance::EMPTY_ITEM();
    return ::ItemInstance::fromTag(tag.value());
}

bool TradeMenuManager::installTrades(Player&, Menu const& menu) {
    auto level = ll::service::getLevel();
    if (!level)
        return false;
    auto* actor = level->fetchEntity(::ActorUniqueID(static_cast<std::int64_t>(menu.carrierUniqueId)), false);
    if (actor == nullptr) {
        HLIB_LOG_WARN("[TradeMenu] 装交易表失败: 找不到载体实体 {}", menu.carrierUniqueId);
        return false;
    }
    auto comp = actor->getEntityContext().tryGetComponent<EconomyTradeableComponent>();
    if (!comp) {
        HLIB_LOG_WARN("[TradeMenu] 装交易表失败: 载体没有 EconomyTradeableComponent");
        return false;
    }

    auto list     = std::make_unique<::MerchantRecipeList>();
    int  menuTier = toWireTier(menu.spec.tier);
    int  index    = 0;
    for (auto const& offer : menu.spec.offers) {
        ::MerchantRecipe recipe{makeItemInstance(offer.buyA), makeItemInstance(offer.buyB), makeItemInstance(offer.sell)};

        recipe.mRecipeNetId.get() = ::RecipeNetId{static_cast<uint>(kOfferNetIdBase + index)};
        ++index;
        recipe.mTier             = offer.locked ? menuTier + 1 : toWireTier(offer.tier);
        recipe.mMaxUses          = offer.maxUses;
        recipe.mTraderExp        = static_cast<uint>(std::max(0, offer.traderExp));
        recipe.mRewardExp        = true;
        recipe.mPriceMultiplierA = 0.05f;
        recipe.mPriceMultiplierB = 0.0f;
        list->mRecipeList.get().push_back(std::move(recipe));
    }

    list->mTierExpRequirements.get() = {0, 10, 70, 150, 250};
    comp->mOffers                    = std::move(list);
    HLIB_LOG_INFO(
        "[TradeMenu] 真实交易表的配方 netId: {}..{}（客户端成交会回传其中之一, 库按它反查条目下标）",
        kOfferNetIdBase,
        kOfferNetIdBase + static_cast<int>(menu.spec.offers.size()) - 1);
    HLIB_LOG_INFO(
        "[TradeMenu] 已装真实交易表: player={} 载体={} 条数={} 档位={}",
        menu.playerName,
        menu.carrierUniqueId,
        menu.spec.offers.size(),
        menu.spec.tier);
    return true;
}

void TradeMenuManager::sendUpdateTrade(Player& player, Menu const& menu) {
    sculk::protocol::UpdateTradePacket packet;
    packet.mContainerId       = static_cast<sculk::protocol::ContainerID>(menu.containerId);
    packet.mContainerType     = sculk::protocol::ContainerType::Trade;
    packet.mSize              = 0;
    packet.mTier              = toWireTier(menu.spec.tier);
    packet.mEntityUniqueId    = static_cast<std::int64_t>(menu.carrierUniqueId);
    packet.mLastTradingPlayer = -1;
    packet.mDisplayName       = menu.spec.tradeType;
    packet.mUseNewTradeScreen = true;
    packet.mUseEconomyTrade   = true;
    packet.mOffers            = trade::buildOffers(
        toOffers(menu.spec.offers, toWireTier(menu.spec.tier)),
        trade::defaultTierExpRequirements(),
        menu.netIdBase);
    if (!sendTradePacket(player, packet)) {
        HLIB_LOG_WARN("[TradeMenu] UpdateTrade 发送失败（玩家 {}）", menu.playerName);
    }
}

void TradeMenuManager::sendContainerClose(Player& player, Menu const& menu) {
    sculk::protocol::ContainerClosePacket packet;
    packet.mContainerId          = static_cast<sculk::protocol::ContainerID>(menu.containerId);
    packet.mContainerType        = sculk::protocol::ContainerType::Trade;
    packet.mServerInitiatedClose = true;
    sendTradePacket(player, packet);
}

void TradeMenuManager::sendTradeExp(Player& player, Menu const& menu) {
    if (menu.carrierRuntimeId == 0)
        return;
    sculk::protocol::SetActorDataPacket packet;
    packet.mActorRuntimeId = menu.carrierRuntimeId;
    packet.mMetaData.mDataItems.push_back(
        {sculk::protocol::ActorDataIDs::TradeTier, std::int32_t{toWireTier(menu.spec.tier)}});
    packet.mMetaData.mDataItems.push_back({sculk::protocol::ActorDataIDs::MaxTradeTier, std::int32_t{4}});
    packet.mMetaData.mDataItems.push_back(
        {sculk::protocol::ActorDataIDs::TradeExperience, std::int32_t{menu.spec.experience}});
    packet.mTick = 0;
    sendTradePacket(player, packet);
}

int64_t TradeMenuManager::open(std::string const& playerName, hologramlib::TradeMenuSpec const& spec) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) {
        HLIB_LOG_WARN("[TradeMenu] 打开失败: 玩家 {} 不在线", playerName);
        return -1;
    }

    int64_t previous = -1;
    {
        std::lock_guard lock(mMutex);
        auto            it = mByPlayer.find(playerName);
        if (it != mByPlayer.end())
            previous = it->second;
    }
    if (previous >= 0)
        close(previous);

    int64_t menuId = 0;
    {
        std::lock_guard lock(mMutex);
        menuId = mNextMenuId++;
    }

    if (spec.carrierUniqueIdOverride != 0) {
        Menu menu;
        menu.menuId           = menuId;
        menu.playerName       = playerName;
        menu.carrierLibId     = -1;
        menu.carrierUniqueId  = static_cast<std::uint64_t>(spec.carrierUniqueIdOverride);
        menu.carrierRuntimeId = 0;
        menu.spec             = spec;
        menu.serverTrades     = true;
        menu.settleLocally    = false;
        {
            std::lock_guard lock(mMutex);
            menu.containerId = allocateContainerId();
            mMenus.emplace(menuId, menu);
            mByPlayer[playerName] = menuId;
        }
        player->openTrading(::ActorUniqueID(spec.carrierUniqueIdOverride), true);
        return menuId;
    }

    auto const carrierLibId = spawnCarrier(*player, playerName, spec);
    if (carrierLibId < 0)
        return -1;

    std::uint64_t uniqueId = 0, runtimeId = 0;
    CustomEntityManager::getInstance().getIdPair(carrierLibId, uniqueId, runtimeId);

    Menu menu;
    menu.menuId           = menuId;
    menu.playerName       = playerName;
    menu.carrierLibId     = carrierLibId;
    menu.carrierUniqueId  = uniqueId;
    menu.carrierRuntimeId = runtimeId;
    menu.spec             = spec;
    menu.settleLocally    = spec.settleLocally;
    {
        std::lock_guard lock(mMutex);
        menu.containerId = allocateContainerId();
        if (menu.containerId == 0) {

            CustomEntityManager::getInstance().destroy(carrierLibId);
            HLIB_LOG_WARN("[TradeMenu] 动态容器 id 已用尽, 打开失败（玩家 {}）", playerName);
            return -1;
        }
        mMenus.emplace(menuId, menu);
        mByPlayer[playerName] = menuId;
    }

    if (menu.spec.usePacketOffers) {

        auto&             executor       = ll::thread::ServerThreadExecutor::getDefault();
        std::string const playerNameCopy = playerName;
        int64_t const     menuIdCopy     = menu.menuId;
        executor.executeAfter(
            [playerNameCopy, menuIdCopy]() {
                TradeMenuManager::getInstance().laterSendTrade(playerNameCopy, menuIdCopy);
            },
            ll::chrono::game::ticks(4));
        executor.executeAfter(
            [playerNameCopy, menuIdCopy]() {
                TradeMenuManager::getInstance().laterSendTrade(playerNameCopy, menuIdCopy);
            },
            ll::chrono::game::ticks(12));
    } else {

        if (installTrades(*player, menu)) {
            player->openTrading(::ActorUniqueID(static_cast<std::int64_t>(menu.carrierUniqueId)), true);
            menu.serverTrades = true;
            {
                std::lock_guard lock(mMutex);
                if (auto it = mMenus.find(menu.menuId); it != mMenus.end())
                    it->second.serverTrades = true;
            }
        } else {
            HLIB_LOG_WARN("[TradeMenu] 装交易表失败, 回退到分包路径");
            sendUpdateTrade(*player, menu);
        }
        sendTradeExp(*player, menu);
    }
    HLIB_LOG_INFO(
        "[TradeMenu] 已打开: player={} menuId={} containerId={} tier={} offers={} type='{}' 路径={}",
        playerName,
        menuId,
        menu.containerId,
        spec.tier,
        spec.offers.size(),
        spec.tradeType,
        menu.serverTrades ? "真实交易表(BDS openTrading)" : "纯协议层(自建UpdateTrade)");
    return menuId;
}

bool TradeMenuManager::getSpec(int64_t menuId, hologramlib::TradeMenuSpec& out) const {
    std::lock_guard lock(mMutex);
    auto            it = mMenus.find(menuId);
    if (it == mMenus.end())
        return false;
    out = it->second.spec;
    return true;
}

bool TradeMenuManager::update(int64_t menuId, hologramlib::TradeMenuSpec const& spec) {
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end())
            return false;
        it->second.spec          = spec;
        it->second.settleLocally = spec.settleLocally && !it->second.serverTrades;
        copy                     = it->second;
    }
    auto* player = findPlayerByName(copy.playerName);
    if (player == nullptr)
        return false;
    if (copy.serverTrades) {

        installTrades(*player, copy);
        player->openTrading(::ActorUniqueID(static_cast<std::int64_t>(copy.carrierUniqueId)), true);
    } else {
        sendUpdateTrade(*player, copy);
    }
    sendTradeExp(*player, copy);
    return true;
}

void TradeMenuManager::laterSendTrade(std::string playerName, int64_t menuId) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr)
        return;
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end())
            return;
        copy = it->second;
    }
    if (copy.serverTrades)
        return;
    sendUpdateTrade(*player, copy);

    sendTradeExp(*player, copy);
    HLIB_LOG_INFO("[TradeMenu] UpdateTrade 已下发（延迟）: player={} menuId={}", playerName, menuId);
}

bool TradeMenuManager::close(int64_t menuId) {
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end())
            return false;
        copy = it->second;
        mMenus.erase(it);
        mByPlayer.erase(copy.playerName);
    }
    // 玩家不在线也要把账本转成待退还（关服 / reload / 掉线后 close 都走这里），否则押在交易框里的物品直接丢
    auto* player   = findPlayerByName(copy.playerName);
    bool  refunded = false;

    if (copy.cursor.active && player != nullptr) {
        ::ItemStack back = copy.cursor.stack;
        if (!back.isNull() && giveToInventory(*player, back, -1) >= 0) {
            HLIB_LOG_INFO(
                "[TradeMenu] 关界面退还手里那份: player={} {} x{}",
                copy.playerName,
                back.getTypeName(),
                static_cast<int>(back.mCount));
            refunded = true;
        }
    }
    if (!copy.ledger.empty()) {
        if (player != nullptr && refundHeld(*player, copy.ledger) > 0)
            refunded = true;
        if (!copy.ledger.empty()) {
            // 背包放不下或人不在线：不能随菜单一起丢，记成待退还，进服时重试
            stashPendingRefund(copy.playerName, copy.ledger);
        }
    }
    if (player != nullptr) {
        if (refunded)
            player->sendInventory(false);

        if (!copy.serverTrades)
            sendContainerClose(*player, copy);
    }
    destroyCarrier(copy);
    HLIB_LOG_INFO("[TradeMenu] 已关闭: player={} menuId={}", copy.playerName, menuId);
    return true;
}

void TradeMenuManager::closeAll() {
    std::vector<int64_t> ids;
    {
        std::lock_guard lock(mMutex);
        for (auto const& [id, menu] : mMenus)
            ids.push_back(id);
    }
    for (auto const id : ids)
        close(id);
}

bool TradeMenuManager::isOpen(int64_t menuId) const {
    std::lock_guard lock(mMutex);
    return mMenus.contains(menuId);
}

std::vector<int64_t> TradeMenuManager::getAllIds() const {
    std::lock_guard      lock(mMutex);
    std::vector<int64_t> out;
    out.reserve(mMenus.size());
    for (auto const& [id, menu] : mMenus)
        out.push_back(id);
    return out;
}

bool TradeMenuManager::addOffer(int64_t menuId, hologramlib::TradeMenuOffer const& offer) {
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end())
            return false;
        it->second.spec.offers.push_back(offer);
    }
    return resendTradeTable(menuId);
}

bool TradeMenuManager::setTier(int64_t menuId, int tier, int experience) {
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end())
            return false;
        it->second.spec.tier       = tier;
        it->second.spec.experience = experience;
    }
    return resendTradeTable(menuId);
}

bool TradeMenuManager::resendTradeTable(int64_t menuId) {
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end())
            return false;
        copy = it->second;
    }
    auto* player = findPlayerByName(copy.playerName);
    if (player == nullptr)
        return false;
    if (copy.serverTrades) {
        installTrades(*player, copy);
        player->openTrading(::ActorUniqueID(static_cast<std::int64_t>(copy.carrierUniqueId)), true);
    } else {
        sendUpdateTrade(*player, copy);
    }
    sendTradeExp(*player, copy);
    return true;
}

bool TradeMenuManager::setSettleLocally(int64_t menuId, bool on) {
    std::lock_guard lock(mMutex);
    auto            it = mMenus.find(menuId);
    if (it == mMenus.end())
        return false;
    it->second.settleLocally = on;
    return true;
}

bool TradeMenuManager::isSettleLocally(int64_t menuId) const {
    std::lock_guard lock(mMutex);
    auto            it = mMenus.find(menuId);
    return it != mMenus.end() && it->second.settleLocally;
}

bool TradeMenuManager::hasSettlingMenuFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            byPlayer = mByPlayer.find(playerName);
    if (byPlayer == mByPlayer.end())
        return false;
    auto it = mMenus.find(byPlayer->second);
    return it != mMenus.end() && it->second.settleLocally && !it->second.serverTrades;
}

uint64_t TradeMenuManager::addSettlementListener(
    std::function<void(hologramlib::TradeSettlementEvent const&)> listener) {
    if (!listener)
        return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextSettlementToken++;
    mSettlementListeners.emplace(token, std::move(listener));
    return token;
}

bool TradeMenuManager::removeSettlementListener(uint64_t token) {
    std::lock_guard lock(mMutex);
    return mSettlementListeners.erase(token) > 0;
}

std::vector<std::string> TradeMenuManager::pollSettlements() {
    std::vector<std::string> out;
    std::lock_guard          lock(mMutex);
    out.reserve(mSettlementQueue.size());
    while (!mSettlementQueue.empty()) {
        out.push_back(formatSettlement(mSettlementQueue.front()));
        mSettlementQueue.pop_front();
    }
    return out;
}

bool TradeMenuManager::handleContainerClose(std::string const& playerName, int containerId) {
    int64_t menuId = -1;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end())
            return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end())
            return false;

        if (!menuIt->second.serverTrades && menuIt->second.containerId != containerId)
            return false;
        menuId = menuIt->second.menuId;
    }
    close(menuId);
    return true;
}

bool TradeMenuManager::hasMenuFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    return mByPlayer.contains(playerName);
}

void TradeMenuManager::onPlayerLeave(std::string const& playerName) {
    int64_t menuId = -1;
    {
        std::lock_guard lock(mMutex);
        auto            it = mByPlayer.find(playerName);
        if (it != mByPlayer.end())
            menuId = it->second;
    }
    if (menuId >= 0)
        close(menuId);
}

void TradeMenuManager::onPlayerDisconnect(::Player& player) {
    std::string const playerName = player.getRealName();
    int64_t           menuId     = -1;
    {
        std::lock_guard lock(mMutex);
        auto            it = mByPlayer.find(playerName);
        if (it != mByPlayer.end())
            menuId = it->second;
    }
    if (menuId < 0)
        return;

    std::vector<LedgerEntry> ledger;
    CursorShadow             cursor;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mMenus.find(menuId); it != mMenus.end()) {
            ledger.swap(it->second.ledger);
            cursor                   = it->second.cursor;
            it->second.cursor        = CursorShadow{};
            it->second.settleLocally = false;
        }
    }

    if (!ledger.empty()) {
        int const n = refundHeld(player, ledger);
        HLIB_LOG_INFO("[TradeMenu] 掉线退还暂存的付费: player={} 条目={}（不发包）", playerName, n);
        if (!ledger.empty()) {
            stashPendingRefund(playerName, ledger);
        }
    } else if (cursor.active) {
        HLIB_LOG_INFO(
            "[TradeMenu] 掉线作废光标托管: player={} {} x{}（背包原处未动, 不发包）",
            playerName,
            cursor.stack.getTypeName(),
            static_cast<int>(cursor.stack.mCount));
    }
    close(menuId);
}

void TradeMenuManager::init() {
    if (!mDisconnectListener) {
        mDisconnectListener = debugshape_export::core::listen<ll::event::PlayerDisconnectEvent>(core::kPlayerDisconnectId,
                                                                                                [](ll::event::PlayerDisconnectEvent& ev) {
                                                                                                    TradeMenuManager::getInstance().onPlayerDisconnect(ev.self());
                                                                                                });
    }
    if (!mJoinListener) {
        mJoinListener = debugshape_export::core::listen<ll::event::PlayerJoinEvent>(core::kPlayerJoinId,
                                                                                    [](ll::event::PlayerJoinEvent& ev) {
                                                                                        TradeMenuManager::getInstance().drainPendingRefunds(ev.self());
                                                                                    });
    }
}

void TradeMenuManager::shutdown() {
    if (mDisconnectListener) {
        ll::event::EventBus::getInstance().removeListener(mDisconnectListener);
        mDisconnectListener.reset();
    }
    if (mJoinListener) {
        ll::event::EventBus::getInstance().removeListener(mJoinListener);
        mJoinListener.reset();
    }
    closeAll();
}

} // namespace debugshape_export

namespace debugshape_export {
namespace {
class TradeMenuAdapter final : public hologramlib::ITradeMenu {
public:
    int64_t open(std::string const& playerName, hologramlib::TradeMenuSpec const& spec) override {
        return TradeMenuManager::getInstance().open(playerName, spec);
    }
    bool update(int64_t menuId, hologramlib::TradeMenuSpec const& spec) override {
        return TradeMenuManager::getInstance().update(menuId, spec);
    }
    bool               close(int64_t menuId) override { return TradeMenuManager::getInstance().close(menuId); }
    void               closeAll() override { TradeMenuManager::getInstance().closeAll(); }
    [[nodiscard]] bool isOpen(int64_t menuId) const override {
        return TradeMenuManager::getInstance().isOpen(menuId);
    }
    [[nodiscard]] std::vector<int64_t> getAllIds() const override {
        return TradeMenuManager::getInstance().getAllIds();
    }
    bool addOffer(int64_t menuId, hologramlib::TradeMenuOffer const& offer) override {
        return TradeMenuManager::getInstance().addOffer(menuId, offer);
    }
    bool setTier(int64_t menuId, int tier, int experience) override {
        return TradeMenuManager::getInstance().setTier(menuId, tier, experience);
    }

    bool setSettleLocally(int64_t menuId, bool on) override {
        return TradeMenuManager::getInstance().setSettleLocally(menuId, on);
    }
    [[nodiscard]] bool isSettleLocally(int64_t menuId) const override {
        return TradeMenuManager::getInstance().isSettleLocally(menuId);
    }
    uint64_t addSettlementListener(std::function<void(hologramlib::TradeSettlementEvent const&)> listener) override {
        return TradeMenuManager::getInstance().addSettlementListener(std::move(listener));
    }
    bool removeSettlementListener(uint64_t token) override {
        return TradeMenuManager::getInstance().removeSettlementListener(token);
    }
    std::vector<std::string> pollSettlements() override {
        return TradeMenuManager::getInstance().pollSettlements();
    }
};
} // namespace

hologramlib::ITradeMenu& tradeMenuAdapter() {
    static TradeMenuAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
