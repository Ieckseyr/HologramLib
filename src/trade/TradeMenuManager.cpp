// TradeMenuManager.cpp - 村民交易菜单（协议层）实现
#include "trade/TradeMenuManager.h"

#include "DiagLog.h"

#include "container/ContainerMenuManager.h"
#include "customentity/CustomEntityManager.h"
#include "trade/TradeOfferNbt.h"

#include <ll/api/chrono/GameChrono.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>

#include <mc/deps/core/utility/BinaryStream.h>
#include <mc/network/Compressibility.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/entity/components_json_legacy/EconomyTradeableComponent.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemInstance.h>
#include <mc/world/item/trading/MerchantRecipe.h>
#include <mc/world/item/trading/MerchantRecipeList.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerType.hpp>
#include <sculk/protocol/codec/inventory/item/ItemStackResponse.hpp>
#include <sculk/protocol/codec/packet/ContainerClosePacket.hpp>
#include <sculk/protocol/codec/packet/SetActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateTradePacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <variant>
#include <vector>

namespace debugshape_export {

namespace {


Player* findPlayerByName(std::string const& name) {
    auto level = ll::service::getLevel();
    if (!level) return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) -> bool {
        if (p.getRealName() == name) {
            found = &p;
            return false; // 停止遍历
        }
        return true;
    });
    return found;
}

// 交易菜单发包: 与库内通用原语的区别是**不做 BDS 回读硬校验**。
// 合成 offers 可能被 BDS 的读后校验拒绝, 但那不代表客户端收不到（PlayerList/AddPlayer 同此判断）。
template <typename PacketT>
bool sendTradePacket(Player& player, PacketT const& packet) {
    std::vector<std::byte>        body;
    sculk::protocol::BinaryStream bodyStream(body);
    packet.write(bodyStream);
    if (body.empty()) return false;

    BinaryStream out;
    out.writeUnsignedVarInt(
        (static_cast<int>(packet.getId()) & 0x3FF) | ((0 & 3) << 10) | ((0 & 3) << 12),
        nullptr,
        nullptr
    );
    out.mBuffer.append(reinterpret_cast<char const*>(body.data()), body.size());

    auto networkSystem = ll::service::getNetworkSystem();
    if (!networkSystem) return false;
    auto* peer = networkSystem->getPeerForUser(player.getNetworkIdentifier());
    if (peer == nullptr) return false;
    peer->sendPacket(out.mBuffer, NetworkPeer::Reliability::Reliable, Compressibility::Compressible);
    return true;
}

// 公开 API 的显示栏值是 1 基（1=新手 … 5=大师）; wire 上是 0 基（抓包实测）。
int clampTier(int tier) {
    return std::clamp(tier, hologramlib::kTradeTierNovice, hologramlib::kTradeTierMaster);
}
int toWireTier(int tier) { return clampTier(tier) - 1; }

// 纯协议层路径给各条交易分配的配方 id 基准: 第 i 条 = kOfferNetIdBase + i。
// 客户端点条目时会在 CraftRecipe 动作里回传这个 id → 减掉基准即得下标（点击定位的主信号）。
// 3676 取自 BDS 原生村民抓包实测值（tests/check-trade-offers.bat 对拍通过）。
constexpr int kOfferNetIdBase = 3676;

// 公开规格 → Offers 构造器入参。menuWireTier = 菜单的 wire 显示栏值:
// locked 的条目必须"比菜单再高一级"才会被客户端判为未解锁。
std::vector<trade::TradeOffer> toOffers(
    std::vector<hologramlib::TradeMenuOffer> const& in,
    int                                             menuWireTier
) {
    std::vector<trade::TradeOffer> out;
    out.reserve(in.size());
    for (auto const& o : in) {
        trade::TradeOffer e;
        auto const        item = [](hologramlib::TradeMenuItem const& i) {
            return trade::TradeItem{i.type, i.count, i.damage, i.name, i.lore, i.isBlock, i.blockVersion};
        };
        e.buyA = item(o.buyA);
        e.buyB = item(o.buyB);
        e.sell = item(o.sell);
        e.tier = o.locked ? menuWireTier + 1 : toWireTier(o.tier);
        e.maxUses = o.maxUses;
        e.traderExp = o.traderExp;
        out.push_back(std::move(e));
    }
    return out;
}

// TypedStorage 在小尺寸/对齐时可能退化成透明别名（没有 .get()）, 两种形态统一取值
template <typename T>
constexpr decltype(auto) unwrap(T const& value) {
    if constexpr (requires { value.get(); }) {
        return value.get();
    } else {
        return (value);
    }
}

} // namespace

TradeMenuManager& TradeMenuManager::getInstance() {
    static TradeMenuManager instance;
    return instance;
}

int TradeMenuManager::allocateContainerId() {
    // 动态容器区间 1..100（BDS 实测每次会话分配新的动态 id）
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
    Player&                              player,
    std::string const&                   playerName,
    hologramlib::TradeMenuSpec const&    spec
) {
    // 玩家会移动, 所以按**打开这一刻**的位置与朝向生成: 身后 5 格
    auto const& pos = player.getPosition();
    float const yaw = player.getRotation().y;
    float const rad = yaw * (std::numbers::pi_v<float> / 180.0f);
    // Bedrock yaw 约定: 前向 = (-sin yaw, cos yaw) → 身后 = (sin yaw, -cos yaw)
    float const bx = pos.x + std::sin(rad) * 5.0f;
    float const bz = pos.z - std::cos(rad) * 5.0f;

    hologramlib::CustomEntityConfig cfg;
    cfg.identifier   = spec.carrierIdentifier.empty() ? "minecraft:villager_v2" : spec.carrierIdentifier;
    cfg.x            = bx;
    cfg.y            = pos.y;
    cfg.z            = bz;
    cfg.dimension    = static_cast<int>(player.getDimensionId());
    cfg.yaw          = yaw;
    cfg.invisible    = true;   // 隐身
    cfg.viewDistance = 32.0;   // 只在近处需要存在
    cfg.enabled      = true;

    auto& entities = CustomEntityManager::getInstance();
    auto  libId    = entities.createRandom(cfg);
    if (libId < 0) {
        HLIB_LOG_WARN("[TradeMenu] 载体实体生成失败（玩家 {}）", playerName);
        return -1;
    }
    // 仅该玩家可见（其余玩家完全收不到出生包）
    entities.setVisiblePlayers(libId, std::vector<std::string>{playerName});
    entities.setInvisible(libId, true);
    return libId;
}

void TradeMenuManager::destroyCarrier(Menu& menu) {
    if (menu.carrierLibId < 0) return;
    CustomEntityManager::getInstance().destroy(menu.carrierLibId);
    menu.carrierLibId = -1;
}

// 条目 → ItemInstance（SNBT: BDS 头里没有公开的物品名构造）
// SNBT 字符串转义（与 jsonEscape 不同: SNBT 只转义引号与反斜杠）
std::string snbtEscape(std::string const& in) {
    std::string out;
    out.reserve(in.size() + 8);
    for (char const c : in) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

::ItemInstance makeItemInstance(hologramlib::TradeMenuItem const& item) {
    if (item.type.empty()) return ::ItemInstance{};
    std::string snbt = std::format(
        "{{\"Count\":{}s,\"Damage\":{}s,\"Name\":\"{}\",\"WasPickedUp\":0b",
        item.count,
        item.damage,
        snbtEscape(item.type)
    );
    if (!item.name.empty() || !item.lore.empty()) {
        snbt += ",\"tag\":{\"display\":{";
        bool first = true;
        if (!item.name.empty()) {
            snbt += std::format("\"Name\":\"{}\"", snbtEscape(item.name));
            first = false;
        }
        if (!item.lore.empty()) {
            if (!first) snbt += ',';
            snbt += "\"Lore\":[";
            for (std::size_t i = 0; i < item.lore.size(); ++i) {
                if (i) snbt += ',';
                snbt += std::format("\"{}\"", snbtEscape(item.lore[i]));
            }
            snbt += ']';
        }
        snbt += "}}";
    }
    snbt += '}';
    auto tag = CompoundTag::fromSnbt(snbt);
    if (!tag.has_value()) return ::ItemInstance{};
    return ::ItemInstance::fromTag(tag.value());
}

// 把公开规格装成**服务端的真实交易表**。
// 这是"物品能放进交易槽 / 能成交"的关键: 我们自己发的 UpdateTrade 只改客户端, 服务端持有的
// 仍是载体自己的（空）交易表, 客户端按我们的表发出的放入请求会被 BDS 校验拒掉。
bool TradeMenuManager::installTrades(Player& /*player*/, Menu const& menu) {
    auto level = ll::service::getLevel();
    if (!level) return false;
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

    auto list      = std::make_unique<::MerchantRecipeList>();
    int  menuTier  = toWireTier(menu.spec.tier);
    int  index     = 0;
    for (auto const& offer : menu.spec.offers) {
        ::MerchantRecipe recipe{makeItemInstance(offer.buyA), makeItemInstance(offer.buyB), makeItemInstance(offer.sell)};
        // 每条给一个独立的配方 netId: 客户端成交时会把"点了哪条"的 netId 回传, 只有它唯一
        // 才能保证客户端与服务端逐条对得上（全为默认 0 时无法区分）。
        recipe.mRecipeNetId.get() = ::RecipeNetId{static_cast<uint>(kOfferNetIdBase + index)};
        ++index;
        recipe.mTier            = offer.locked ? menuTier + 1 : toWireTier(offer.tier);
        recipe.mMaxUses         = offer.maxUses;
        recipe.mTraderExp       = static_cast<uint>(std::max(0, offer.traderExp));
        recipe.mRewardExp       = true;
        recipe.mPriceMultiplierA = 0.05f;
        recipe.mPriceMultiplierB = 0.0f;
        list->mRecipeList.get().push_back(std::move(recipe));
    }
    // 原版村民的档位经验需求（新手→大师）
    list->mTierExpRequirements.get() = {0, 10, 70, 150, 250};
    comp->mOffers                    = std::move(list);
    HLIB_LOG_INFO(
        "[TradeMenu] 真实交易表的配方 netId: {}..{}（客户端成交会回传其中之一, 库按它反查条目下标）",
        kOfferNetIdBase,
        kOfferNetIdBase + static_cast<int>(menu.spec.offers.size()) - 1
    );
    HLIB_LOG_INFO(
        "[TradeMenu] 已装真实交易表: player={} 载体={} 条数={} 档位={}",
        menu.playerName,
        menu.carrierUniqueId,
        menu.spec.offers.size(),
        menu.spec.tier
    );
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
    packet.mOffers = trade::buildOffers(
        toOffers(menu.spec.offers, toWireTier(menu.spec.tier)),
        trade::defaultTierExpRequirements(),
        menu.netIdBase
    );
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
    if (menu.carrierRuntimeId == 0) return;
    sculk::protocol::SetActorDataPacket packet;
    packet.mActorRuntimeId = menu.carrierRuntimeId;
    packet.mMetaData.mDataItems.push_back(
        {sculk::protocol::ActorDataIDs::TradeTier, std::int32_t{toWireTier(menu.spec.tier)}}
    );
    packet.mMetaData.mDataItems.push_back({sculk::protocol::ActorDataIDs::MaxTradeTier, std::int32_t{4}});
    packet.mMetaData.mDataItems.push_back(
        {sculk::protocol::ActorDataIDs::TradeExperience, std::int32_t{menu.spec.experience}}
    );
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
        if (it != mByPlayer.end()) previous = it->second; // 同一玩家先关旧的
    }
    if (previous >= 0) close(previous);

    int64_t menuId = 0;
    {
        std::lock_guard lock(mMutex);
        menuId = mNextMenuId++;
    }

    // 覆盖模式: 直接用调用方给的真实实体, 交给 BDS 自己的开交易流程 —— 客户端与服务端
    // 持有同一份交易表, 客户端的"放入交易槽"才会被校验通过（自建 offers 只改客户端, 对不上）。
    if (spec.carrierUniqueIdOverride != 0) {
        Menu menu;
        menu.menuId               = menuId;
        menu.playerName           = playerName;
        menu.carrierLibId         = -1;
        menu.carrierUniqueId      = static_cast<std::uint64_t>(spec.carrierUniqueIdOverride);
        menu.carrierRuntimeId     = 0;
        menu.spec                 = spec;
        menu.serverTrades         = true;
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
    if (carrierLibId < 0) return -1;

    std::uint64_t uniqueId = 0, runtimeId = 0;
    CustomEntityManager::getInstance().getIdPair(carrierLibId, uniqueId, runtimeId);

    Menu menu;
    menu.menuId          = menuId;
    menu.playerName      = playerName;
    menu.carrierLibId    = carrierLibId;
    menu.carrierUniqueId = uniqueId;
    menu.carrierRuntimeId = runtimeId;
    menu.spec            = spec;
    {
        std::lock_guard lock(mMutex);
        menu.containerId = allocateContainerId();
        if (menu.containerId == 0) {
            // 容器 id 用尽（100 个动态槽位全占）—— 极少见, 直接放弃并回收载体
            CustomEntityManager::getInstance().destroy(carrierLibId);
            HLIB_LOG_WARN("[TradeMenu] 动态容器 id 已用尽, 打开失败（玩家 {}）", playerName);
            return -1;
        }
        mMenus.emplace(menuId, menu);
        mByPlayer[playerName] = menuId;
    }

    if (menu.spec.usePacketOffers) {
        // 默认路径（纯协议层）: 界面完全由我们自己构造的 UpdateTrade 打开。
        // 载体实体是异步发给客户端的, 背靠背发 UpdateTrade 时客户端还不知道那个实体,
        // 交易界面就绑不上去（= 界面上什么都没出现）。所以延后几 tick 再发, 并在 12 tick
        // 补一次刷新 —— 客户端已就绪时第二次只是把同一份 offers 再推一遍, 无副作用。
        auto&             executor       = ll::thread::ServerThreadExecutor::getDefault();
        std::string const playerNameCopy = playerName;
        int64_t const     menuIdCopy     = menu.menuId;
        executor.executeAfter(
            [playerNameCopy, menuIdCopy]() {
                TradeMenuManager::getInstance().laterSendTrade(playerNameCopy, menuIdCopy);
            },
            ll::chrono::game::ticks(4)
        );
        executor.executeAfter(
            [playerNameCopy, menuIdCopy]() {
                TradeMenuManager::getInstance().laterSendTrade(playerNameCopy, menuIdCopy);
            },
            ll::chrono::game::ticks(12)
        );
    } else {
        // 真实交易表路径: 给载体装真实交易表, 再用 BDS 自己的 openTrading 打开 ——
        // 客户端与服务端持有同一份交易表, 放入/成交由 BDS 完成。
        if (installTrades(*player, menu)) {
            player->openTrading(::ActorUniqueID(static_cast<std::int64_t>(menu.carrierUniqueId)), true);
            menu.serverTrades = true;
            {
                std::lock_guard lock(mMutex);
                if (auto it = mMenus.find(menu.menuId); it != mMenus.end()) it->second.serverTrades = true;
            }
        } else {
            HLIB_LOG_WARN("[TradeMenu] 装交易表失败, 回退到分包路径");
            sendUpdateTrade(*player, menu);
        }
        sendTradeExp(*player, menu); // 经验条: 两条路径都用 ActorData 推（BDS 不回写这个）
    }
    HLIB_LOG_INFO(
        "[TradeMenu] 已打开: player={} menuId={} containerId={} tier={} offers={} type='{}' 路径={}",
        playerName,
        menuId,
        menu.containerId,
        spec.tier,
        spec.offers.size(),
        spec.tradeType,
        menu.serverTrades ? "真实交易表(BDS openTrading)" : "纯协议层(自建UpdateTrade)"
    );
    return menuId;
}

bool TradeMenuManager::update(int64_t menuId, hologramlib::TradeMenuSpec const& spec) {
    Menu  copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return false;
        it->second.spec = spec;
        copy            = it->second;
    }
    auto* player = findPlayerByName(copy.playerName);
    if (player == nullptr) return false;
    if (copy.serverTrades) {
        // 真实交易表: 重新装表并让 BDS 重开界面（容器 id 由 BDS 分配, 我们不再自己发 UpdateTrade）
        installTrades(*player, copy);
        player->openTrading(::ActorUniqueID(static_cast<std::int64_t>(copy.carrierUniqueId)), true);
    } else {
        sendUpdateTrade(*player, copy);
    }
    sendTradeExp(*player, copy);
    return true;
}

// 纯协议层路径的延时分步发送: 在服务器线程上重取快照再发（玩家可能已离线、菜单可能已关）
void TradeMenuManager::laterSendTrade(std::string playerName, int64_t menuId) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return;
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return; // 已关闭
        copy = it->second;
    }
    if (copy.serverTrades) return; // 已切到 BDS 自己的开交易流程, 不再发我们的 UpdateTrade
    sendUpdateTrade(*player, copy);
    // 经验条走载体的 ActorData: 也得等客户端认识这个实体之后才收得下, 所以和 UpdateTrade 一起延后
    sendTradeExp(*player, copy);
    HLIB_LOG_INFO("[TradeMenu] UpdateTrade 已下发（延迟）: player={} menuId={}", playerName, menuId);
}

bool TradeMenuManager::close(int64_t menuId) {    Menu  copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return false;
        copy = it->second;
        mMenus.erase(it);
        mByPlayer.erase(copy.playerName);
    }
    if (auto* player = findPlayerByName(copy.playerName)) {
        // 界面由 BDS 打开（真实交易表）时: 不发 ContainerClose, 直接销毁载体 —— 移除商人后
        // 客户端界面自行关闭, 且 BDS 那边也不残留容器状态。
        if (!copy.serverTrades) sendContainerClose(*player, copy);
    }
    destroyCarrier(copy); // 关菜单即删载体实体
    HLIB_LOG_INFO("[TradeMenu] 已关闭: player={} menuId={}", copy.playerName, menuId);
    return true;
}

void TradeMenuManager::closeAll() {
    std::vector<int64_t> ids;
    {
        std::lock_guard lock(mMutex);
        for (auto const& [id, menu] : mMenus) ids.push_back(id);
    }
    for (auto const id : ids) close(id);
}

bool TradeMenuManager::isOpen(int64_t menuId) const {
    std::lock_guard lock(mMutex);
    return mMenus.contains(menuId);
}

std::vector<int64_t> TradeMenuManager::getAllIds() const {
    std::lock_guard      lock(mMutex);
    std::vector<int64_t> out;
    out.reserve(mMenus.size());
    for (auto const& [id, menu] : mMenus) out.push_back(id);
    return out;
}

// 追加一条交易并就地重发交易表: 复用同一个界面与载体, 不重开、不等待。
// 纯协议层路径重发我们自己的 UpdateTrade; 真实交易表路径重装服务端交易表并让 BDS 重开界面。
bool TradeMenuManager::addOffer(int64_t menuId, hologramlib::TradeMenuOffer const& offer) {
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return false;
        it->second.spec.offers.push_back(offer);
    }
    return resendTradeTable(menuId);
}

// 改显示栏值 / 经验条（同样就地重发）
bool TradeMenuManager::setTier(int64_t menuId, int tier, int experience) {
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return false;
        it->second.spec.tier       = tier;
        it->second.spec.experience = experience;
    }
    return resendTradeTable(menuId);
}

// 就地重发交易表（玩家可能已离线 / 菜单可能已关 → false）
bool TradeMenuManager::resendTradeTable(int64_t menuId) {
    Menu  copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return false;
        copy = it->second;
    }
    auto* player = findPlayerByName(copy.playerName);
    if (player == nullptr) return false;
    if (copy.serverTrades) {
        installTrades(*player, copy);
        player->openTrading(::ActorUniqueID(static_cast<std::int64_t>(copy.carrierUniqueId)), true);
    } else {
        sendUpdateTrade(*player, copy);
    }
    sendTradeExp(*player, copy);
    return true;
}

bool TradeMenuManager::handleContainerClose(std::string const& playerName, int containerId) {
    int64_t menuId = -1;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end()) return false;
        // 纯协议层路径: 容器 id 是我们自己发出去的, 按 id 认。
        // 真实交易表路径: 界面由 BDS 打开, 容器 id 是 **BDS 分配的**, 我们那个只是占位 —— 按 id 认
        // 会让"界面结束"永远认不出来。那条路上一个玩家只有这一个菜单, 按"有没有菜单"认即可。
        if (!menuIt->second.serverTrades && menuIt->second.containerId != containerId) return false;
        menuId = menuIt->second.menuId;
    }
    close(menuId); // 结束菜单记录（客户端已经关了, 这里只清服务端状态）
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
        if (it != mByPlayer.end()) menuId = it->second;
    }
    if (menuId >= 0) close(menuId);
}

void TradeMenuManager::shutdown() { closeAll(); }

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
    bool close(int64_t menuId) override { return TradeMenuManager::getInstance().close(menuId); }
    void closeAll() override { TradeMenuManager::getInstance().closeAll(); }
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
};
} // namespace

hologramlib::ITradeMenu& tradeMenuAdapter() {
    static TradeMenuAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
