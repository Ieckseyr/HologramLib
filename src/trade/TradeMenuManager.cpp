// TradeMenuManager.cpp - 村民交易菜单（协议层）实现
#include "trade/TradeMenuManager.h"

#include "DiagLog.h"

#include "customentity/CustomEntityManager.h"
#include "container/ContainerResponse.h" // 空 containers 的成功应答
#include <mc/network/packet/InventorySlotPacket.h>
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
#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>

#include <mc/entity/components_json_legacy/EconomyTradeableComponent.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Inventory.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemInstance.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/trading/MerchantRecipe.h>
#include <mc/world/item/trading/MerchantRecipeList.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerType.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerEnumName.hpp>
#include <sculk/protocol/codec/inventory/item/ItemStackResponse.hpp>
#include <sculk/protocol/codec/packet/ItemStackResponsePacket.hpp>
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

// 条目 → SNBT（BDS 头里没有公开的物品名构造）; 空串 = 无效条目（type 为空）
std::string itemSnbt(hologramlib::TradeMenuItem const& item) {
    if (item.type.empty()) return {};
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
    return snbt;
}

::ItemInstance makeItemInstance(hologramlib::TradeMenuItem const& item) {
    auto const snbt = itemSnbt(item);
    if (snbt.empty()) return ::ItemInstance{};
    auto tag = CompoundTag::fromSnbt(snbt);
    if (!tag.has_value()) return ::ItemInstance{};
    return ::ItemInstance::fromTag(tag.value());
}

// 同一份 SNBT 方言的 ItemStack 版本（真结算要往背包里写, 而背包 API 用 ItemStack ——
// ItemInstance 与 ItemStack 是兄弟类型, 不能互转）
::ItemStack makeItemStackOf(hologramlib::TradeMenuItem const& item) {
    auto const snbt = itemSnbt(item);
    if (snbt.empty()) return ::ItemStack{};
    auto tag = CompoundTag::fromSnbt(snbt);
    if (!tag.has_value()) return ::ItemStack{};
    return ::ItemStack::fromTag(tag.value());
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
        menu.settleLocally        = false; // 真实交易表路径由 BDS 结算, 不走自结算
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
    menu.settleLocally   = spec.settleLocally; // 真结算只在纯协议层路径下有意义（下面按 serverTrades 再夹一次）
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
        it->second.spec          = spec;
        it->second.settleLocally = spec.settleLocally && !it->second.serverTrades;
        copy                     = it->second;
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

namespace {
// 真结算的落地原语定义在本文件下半部分（1.24.0 区块）, 而 close()/onPlayerDisconnect() 比它们早
// —— 先声明（同一匿名命名空间 = 内部链接, 只是把可见性提前）。
int takeFromSlot(::Player& player, int slot, int count);
int giveToInventory(::Player& player, ::ItemStack const& stack, int preferSlot);
} // namespace

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
        // 真结算的账本: 关界面时把暂存的付费还回背包（玩家关掉界面/走出范围都没成交, 不该吃亏）
        // 账本里没花掉的付费 + 还"拿在手上"的那份, 一起退回背包
        bool refunded = false;
        if (!copy.cursor.active && copy.ledger.empty()) refunded = false;
        if (copy.cursor.active) {
            ::ItemStack back = copy.cursor.stack;
            if (!back.isNull() && giveToInventory(*player, back, -1) >= 0) {
                HLIB_LOG_INFO(
                    "[TradeMenu] 关界面退还手里那份: player={} {} x{}",
                    copy.playerName,
                    back.getTypeName(),
                    static_cast<int>(back.mCount)
                );
                refunded = true;
            }
        }
        if (!copy.ledger.empty()) {
            if (refundHeld(*player, copy.ledger) > 0) refunded = true;
        }
        if (refunded) player->sendInventory(false);
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

// ─────────────────────────────────────────────
// 1.24.0: 纯协议层真结算
//
// 客户端在交易界面上的物品操作全部走 ItemStackRequest(147)（实测: 付费拖放、手柄成交都在里面;
// AuthInput 那条通道同样挂着）。三条腿（见 TradeSettlementLogic.h 的分类）:
//   ① 付费放进交易槽 → 真的从背包扣下来, 记进账本
//   ② 成交（CraftRecipe 带自建配方 id）→ 校验账本够不够, 扣账本 + 真的把产物写进背包
//   ③ 付费取回 → 把账本里的还回背包
// 服务端没有这个容器, 所以每一步都得自己落地; 全做完才回成功应答 —— 客户端保留自己的预测,
// 于是它看到的就是"真的成交了"。关界面/下线时把账本里剩的付费还回去（不让玩家吃亏）。
//
// 已知边界（实测/设计取舍, 原型阶段写明）:
//   · 触屏在纯协议层到不了成交（付费放不进槽、客户端反复弹回）—— 触屏请走真实交易表路径或容器 UI
//   · 账本按"类型 + 数量"扣（不按交易槽位细分）: 同一付费类型的多条交易共用料槽时, 扣的是先放的
//   · 产物优先写进客户端预测的那一格（ResultOut 的 dst）, 否则找空位; 包里带的 numCrafts 不展开
// ─────────────────────────────────────────────

namespace {
// 从背包某槽扣掉 count 个（返回实际扣掉的个数; -1 = 该槽空/越界）
int takeFromSlot(::Player& player, int slot, int count) {
    auto&     inv  = player.getInventory();
    int const size = inv.getContainerSize();
    if (slot < 0 || slot >= size) return -1;
    ::ItemStack cur = inv.getItem(slot);
    if (cur.isNull()) return -1;
    int const have = static_cast<int>(cur.mCount);
    int const take = std::min(count < 1 ? have : count, have);
    if (take <= 0) return -1;
    if (take >= have) {
        inv.setItem(slot, ::ItemStack{});
    } else {
        cur.mCount = static_cast<unsigned char>(have - take);
        inv.setItem(slot, cur);
    }
    return take;
}

// 把一件物品写进背包: 先试 preferSlot（客户端预测的那一格）, 再同类叠加, 再找空格。
// 返回写入的槽位; -1 = 背包满（调用方按失败处理）
int giveToInventory(::Player& player, ::ItemStack const& stack, int preferSlot) {
    if (stack.isNull()) return -1;
    auto&     inv  = player.getInventory();
    int const size = inv.getContainerSize();
    auto      trySlot = [&](int slot) -> int {
        if (slot < 0 || slot >= size) return -1;
        ::ItemStack cur = inv.getItem(slot);
        if (cur.isNull()) {
            inv.setItem(slot, stack);
            return slot;
        }
        if (cur.getTypeName() == stack.getTypeName()) {
            int const total = static_cast<int>(cur.mCount) + static_cast<int>(stack.mCount);
            if (total <= 127) { // Count 是 Byte 域
                cur.mCount = static_cast<unsigned char>(total);
                inv.setItem(slot, cur);
                return slot;
            }
        }
        return -1;
    };
    if (int const slot = trySlot(preferSlot); slot >= 0) return slot;
    for (int slot = 0; slot < size; ++slot) {
        ::ItemStack cur = inv.getItem(slot);
        if (!cur.isNull() && cur.getTypeName() == stack.getTypeName()) {
            if (int const s = trySlot(slot); s >= 0) return s;
        }
    }
    for (int slot = 0; slot < size; ++slot) {
        if (inv.getItem(slot).isNull()) {
            inv.setItem(slot, stack);
            return slot;
        }
    }
    return -1;
}
} // namespace

void TradeMenuManager::emit(hologramlib::TradeSettlementEvent const& event) {
    std::vector<std::function<void(hologramlib::TradeSettlementEvent const&)>> snapshot;
    {
        std::lock_guard lock(mMutex);
        // LSE 侧拿不到 C++ 监听器 → 同一份事件也进轮询队列（tradePollSettlements 取走并清空）
        mSettlementQueue.push_back(event);
        snapshot.reserve(mSettlementListeners.size());
        for (auto const& [token, fn] : mSettlementListeners) snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn) fn(event);
    }
}

std::string TradeMenuManager::formatSettlement(hologramlib::TradeSettlementEvent const& event) {
    return std::format(
        "player={} menuId={} offer={} ok={} reason={}",
        event.playerName,
        event.menuId,
        event.offerIndex,
        event.ok ? 1 : 0,
        event.reason
    );
}

// 付费放进交易槽: 真的从背包扣下来暂存（客户端预测物品离开了背包, 服务端于是也真的离开 ——
// 两边一致, 这正是后面能"真的成交"的前提）
bool TradeMenuManager::applyPayIn(
    ::Player&                 player,
    Menu&                     menu,
    ClassifiedAction const&   action,
    std::string&              reason
) {
    auto&          inv      = player.getInventory();
    int const      size     = inv.getContainerSize();
    int const      srcSlot  = trade::settlement::inventorySlotOf(action.srcContainer, action.slot);
    if (srcSlot < 0 || srcSlot >= size) {
        reason = "slot-out-of-range";
        return false;
    }
    ::ItemStack cur = inv.getItem(srcSlot);
    if (cur.isNull()) {
        reason = "slot-empty";
        return false;
    }
    int const have = static_cast<int>(cur.mCount);
    int const take = std::min(action.amount < 1 ? have : action.amount, have);
    if (take <= 0) {
        reason = "amount-zero";
        return false;
    }
    std::string const type = cur.getTypeName();
    if (takeFromSlot(player, srcSlot, take) != take) {
        reason = "take-failed";
        return false;
    }
    // 账本该条目累计（同一个交易槽分多次放料）
    bool merged = false;
    for (auto& e : menu.ledger) {
        if (e.logical.tradeContainer == action.tradeContainer && e.logical.tradeSlot == action.tradeSlot
            && e.logical.item.type == type) {
            e.logical.item.count += take;
            e.stack.mCount = static_cast<unsigned char>(e.logical.item.count);
            merged = true;
            break;
        }
    }
    if (!merged) {
        LedgerEntry e;
        e.logical.tradeContainer = action.tradeContainer;
        e.logical.tradeSlot      = action.tradeSlot;
        e.logical.item.type      = type;
        e.logical.item.count     = take;
        e.logical.item.damage    = static_cast<int>(cur.getDamageValue());
        e.stack                  = cur; // 原样那一份（含 NBT）
        e.stack.mCount           = static_cast<unsigned char>(take);
        menu.ledger.push_back(std::move(e));
    }
    HLIB_LOG_INFO(
        "[TradeMenu] 付费已入账: player={} 槽={} 类型={} 数量={} 暂存槽=(c{},{})",
        menu.playerName,
        action.slot,
        type,
        take,
        action.tradeContainer,
        action.tradeSlot
    );
    return true;
}

// "点一下拿起": 只记下手里那份（服务端背包不动 —— 它还在 fromSlot, 等于"影子"）
bool TradeMenuManager::applyPickUp(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason
) {
    auto&     inv     = player.getInventory();
    int const size    = inv.getContainerSize();
    int const srcSlot = trade::settlement::inventorySlotOf(action.srcContainer, action.slot);
    if (srcSlot < 0 || srcSlot >= size) {
        reason = "slot-out-of-range";
        return false;
    }
    ::ItemStack cur = inv.getItem(srcSlot);
    if (cur.isNull()) {
        reason = "slot-empty";
        return false;
    }
    menu.cursor.active   = true;
    menu.cursor.stack    = cur;
    menu.cursor.fromSlot = srcSlot; // 服务端背包槽号（客户端槽号已换算过）
    HLIB_LOG_INFO(
        "[TradeMenu] 光标托管: player={} 拿起 {}x{}（背包槽 {}, 服务端暂不动）",
        menu.playerName,
        cur.getTypeName(),
        static_cast<int>(cur.mCount),
        action.slot
    );
    return true;
}

// "再点一下放进交易槽": 手里那份的 amount 个落进交易槽 —— 在 fromSlot 上真扣, 记进账本
bool TradeMenuManager::applyCursorPayIn(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason
) {
    if (!menu.cursor.active) {
        reason = "no-cursor-item";
        return false;
    }
    int const onCursor = static_cast<int>(menu.cursor.stack.mCount);
    int const take     = action.amount < 1 ? onCursor : action.amount;
    if (take <= 0 || take > onCursor) {
        reason = "cursor-amount-mismatch";
        return false;
    }
    if (takeFromSlot(player, menu.cursor.fromSlot, take) != take) {
        reason = "slot-empty";
        return false;
    }
    std::string const type   = menu.cursor.stack.getTypeName();
    bool              merged = false;
    for (auto& e : menu.ledger) {
        if (e.logical.tradeContainer == action.tradeContainer && e.logical.tradeSlot == action.tradeSlot
            && e.logical.item.type == type) {
            e.logical.item.count += take;
            e.stack.mCount = static_cast<unsigned char>(e.logical.item.count);
            merged = true;
            break;
        }
    }
    if (!merged) {
        LedgerEntry e;
        e.logical.tradeContainer = action.tradeContainer;
        e.logical.tradeSlot      = action.tradeSlot;
        e.logical.item.type      = type;
        e.logical.item.count     = take;
        e.logical.item.damage    = static_cast<int>(menu.cursor.stack.getDamageValue());
        e.stack                  = menu.cursor.stack;
        e.stack.mCount           = static_cast<unsigned char>(take);
        menu.ledger.push_back(std::move(e));
    }
    int const left = onCursor - take;
    if (left <= 0) {
        menu.cursor = CursorShadow{};
    } else {
        menu.cursor.stack.mCount = static_cast<unsigned char>(left);
    }
    HLIB_LOG_INFO(
        "[TradeMenu] 付费已入账（光标）: player={} {}x{} → 交易槽(c{},{})",
        menu.playerName,
        type,
        take,
        action.tradeContainer,
        action.tradeSlot
    );
    return true;
}

// 放回背包（取消拿起）: 把 fromSlot 那份搬到客户端预测的那一格
bool TradeMenuManager::applyCursorBack(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason
) {
    if (!menu.cursor.active) {
        reason = "no-cursor-item";
        return false;
    }
    auto&     inv     = player.getInventory();
    int const size    = inv.getContainerSize();
    int const dstSlot = trade::settlement::inventorySlotOf(action.srcContainer, action.slot);
    if (dstSlot < 0 || dstSlot >= size) {
        reason = "slot-out-of-range";
        return false;
    }
    int const count = static_cast<int>(menu.cursor.stack.mCount);
    if (dstSlot == menu.cursor.fromSlot) {
        menu.cursor = CursorShadow{}; // 原地放回: 什么都不用动
        return true;
    }
    ::ItemStack dst = inv.getItem(dstSlot);
    if (dst.isNull()) {
        if (takeFromSlot(player, menu.cursor.fromSlot, count) != count) {
            reason = "slot-empty";
            return false;
        }
        inv.setItem(dstSlot, menu.cursor.stack);
    } else if (dst.getTypeName() == menu.cursor.stack.getTypeName()) {
        int const total = static_cast<int>(dst.mCount) + count;
        if (total > 127) {
            reason = "slot-full";
            return false;
        }
        if (takeFromSlot(player, menu.cursor.fromSlot, count) != count) {
            reason = "slot-empty";
            return false;
        }
        dst.mCount = static_cast<unsigned char>(total);
        inv.setItem(dstSlot, dst);
    } else {
        reason = "slot-occupied";
        return false;
    }
    menu.cursor = CursorShadow{};
    return true;
}

// 付费取回: 把账本里暂存的还回背包
bool TradeMenuManager::applyPayOut(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    std::string&            reason
) {
    auto it = std::find_if(menu.ledger.begin(), menu.ledger.end(), [&](LedgerEntry const& e) {
        return e.logical.tradeContainer == action.tradeContainer && e.logical.tradeSlot == action.tradeSlot;
    });
    if (it == menu.ledger.end()) {
        reason = "nothing-held";
        return false;
    }
    int const have = it->logical.item.count;
    int const take = std::min(action.amount < 1 ? have : action.amount, have);
    if (take <= 0) {
        reason = "amount-zero";
        return false;
    }
    ::ItemStack back = it->stack; // 原样还回去（含 NBT）
    back.mCount      = static_cast<unsigned char>(take);
    int const dstSlot = trade::settlement::inventorySlotOf(action.srcContainer, action.slot);
    if (giveToInventory(player, back, dstSlot) < 0) {
        reason = "inventory-full";
        return false;
    }
    it->logical.item.count -= take;
    it->stack.mCount = static_cast<unsigned char>(it->logical.item.count);
    if (it->logical.item.count <= 0) menu.ledger.erase(it);
    return true;
}

// 成交: 账本够 → 扣账本 + 发产物
bool TradeMenuManager::settleDeal(
    ::Player&               player,
    Menu&                   menu,
    ClassifiedAction const& action,
    int                     productSlot,
    std::string&            reason
) {
    int const index = action.offerIndex;
    if (index < 0 || index >= static_cast<int>(menu.spec.offers.size())) {
        reason = "offer-out-of-range";
        return false;
    }
    auto const& offer = menu.spec.offers[static_cast<std::size_t>(index)];

    std::vector<HeldPayment> heldView;
    heldView.reserve(menu.ledger.size());
    for (auto const& e : menu.ledger) heldView.push_back(e.logical);
    std::vector<HeldPayment> taken;
    if (!trade::settlement::planConsumption(heldView, offer, taken)) {
        reason = "payment-missing";
        return false;
    }
    if (offer.sell.type.empty()) {
        reason = "no-product";
        return false;
    }
    ::ItemStack product = makeItemStackOf(offer.sell);
    if (product.isNull()) {
        reason = "bad-product";
        return false;
    }
    // 先确认背包放得下再扣账本（免得扣了又发不出去）
    bool const canPlace =
        [&] {
            auto&     inv  = player.getInventory();
            int const size = inv.getContainerSize();
            if (productSlot >= 0 && productSlot < size) {
                ::ItemStack const& cur = inv.getItem(productSlot);
                if (cur.isNull() || cur.getTypeName() == product.getTypeName()) return true;
            }
            for (int slot = 0; slot < size; ++slot) {
                ::ItemStack const& cur = inv.getItem(slot);
                if (cur.isNull()) return true;
                if (cur.getTypeName() == product.getTypeName()
                    && static_cast<int>(cur.mCount) + static_cast<int>(product.mCount) <= 127) {
                    return true;
                }
            }
            return false;
        }();
    if (!canPlace) {
        reason = "inventory-full";
        return false;
    }

    // **先给货, 再扣账本**: 反过来会出现"账本已扣、货却发不出去"的丢件窗口（canPlace 只是预检）。
    // planConsumption 已经确认账本够, 所以下面扣不到只可能是账本被并发改了。
    int const placed = giveToInventory(player, product, productSlot);
    if (placed < 0) {
        reason = "inventory-full";
        return false;
    }
    for (auto const& t : taken) {
        for (auto& e : menu.ledger) {
            if (e.logical.tradeContainer != t.tradeContainer || e.logical.tradeSlot != t.tradeSlot) continue;
            if (e.logical.item.type != t.item.type || e.logical.item.count <= 0) continue;
            int const cut = std::min(t.item.count, e.logical.item.count);
            e.logical.item.count -= cut;
            e.stack.mCount = static_cast<unsigned char>(e.logical.item.count);
            break;
        }
    }
    menu.ledger.erase(
        std::remove_if(
            menu.ledger.begin(),
            menu.ledger.end(),
            [](LedgerEntry const& e) { return e.logical.item.count <= 0; }
        ),
        menu.ledger.end()
    );
    HLIB_LOG_INFO(
        "[TradeMenu] 真结算完成: player={} 条目={} 产物={}x{} → 背包槽={}",
        menu.playerName,
        index,
        offer.sell.type,
        offer.sell.count,
        placed
    );
    return true;
}

// 把账本里剩下的付费还回背包（关界面 / 下线）
int TradeMenuManager::refundHeld(::Player& player, std::vector<LedgerEntry>& ledger) {
    int refunded = 0;
    for (auto& e : ledger) {
        if (e.logical.item.type.empty() || e.logical.item.count <= 0) continue;
        ::ItemStack back = e.stack; // 原样那一份（含 NBT）
        back.mCount      = static_cast<unsigned char>(e.logical.item.count);
        if (!back.isNull() && giveToInventory(player, back, -1) >= 0) {
            HLIB_LOG_INFO(
                "[TradeMenu] 退还暂存的付费: player={} {} x{}",
                player.getRealName(),
                e.logical.item.type,
                e.logical.item.count
            );
            e.logical.item.count = 0;
            ++refunded;
        } else {
            HLIB_LOG_WARN(
                "[TradeMenu] 退还失败（背包满）: player={} {} x{} —— 保留在账本里, 下次关界面再试",
                player.getRealName(),
                e.logical.item.type,
                e.logical.item.count
            );
        }
    }
    ledger.erase(
        std::remove_if(ledger.begin(), ledger.end(), [](LedgerEntry const& e) { return e.logical.item.count <= 0; }),
        ledger.end()
    );
    return refunded;
}

// 真结算请求处理（钩子入口）
// 交易槽内容推送: 一条 InventorySlot(50) 把这一格改成指定物品（空物品 = 清空）。
//
// 为什么不走 ItemStackResponse 的 containers 段（更"正统"）:
//   **协议库 sculk 的 ItemStackResponseSlotInfo 与 BDS 的线格式不一致** —— sculk 多写一个
//   FilteredCustomName 字符串、DurabilityCorrection 按 varint 写, 而 BDS 没有那个字符串、该字段是
//   short(定长 2 字节)。空 containers 时两者无差别（所以一直没暴露）, 一旦带内容客户端就解析越界
//   **直接崩**（30.09 实测: 客户端收到后立刻掉线）。ContainerInfo/容器枚举/槽号这几层两者一致,
//   出问题的就是槽条目本身, 所以这里改用容器域已验证的 InventorySlot 通道。
bool TradeMenuManager::sendTradeSlotUpdate(
    ::Player&               player,
    Menu const&             menu,
    int                     tradeSlot,
    ::ItemStack const&      item
) {
    ::InventorySlotPacket packet{::InventorySlotPacketPayload(
        static_cast<::ContainerID>(menu.containerId),
        static_cast<uint>(tradeSlot),
        item,
        ::FullContainerName{},
        ::ItemStack{}
    )};
    player.sendNetworkPacket(packet);
    HLIB_LOG_INFO(
        "[TradeMenu] 交易槽内容已推送: containerId={} slot={} 物品={}x{}",
        menu.containerId,
        tradeSlot,
        item.isNull() ? "(空)" : item.getTypeName(),
        item.isNull() ? 0 : static_cast<int>(item.mCount)
    );
    return true;
}

bool TradeMenuManager::handleItemRequest(
    ::Player&                                player,
    std::vector<TradeRequestAction> const&   actions,
    int                                      requestId
) {
    if (actions.empty()) return false;

    Menu      menu;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(player.getRealName());
        if (byPlayer == mByPlayer.end()) return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end()) return false;
        // 只接管"纯协议层 + 开了真结算"的菜单: 真实交易表路径归 BDS 管; 纯展示路径一个字都不读
        if (!menuIt->second.settleLocally || menuIt->second.serverTrades) return false;
        menu = menuIt->second;
    }

    auto const classified = trade::settlement::classify(
        actions,
        menu.netIdBase,
        static_cast<int>(menu.spec.offers.size())
    );
    // 只有"真的动了东西"的动作才接管: 付费进/出、成交。
    // **纯产物出槽（ResultOut）不接管** —— 那不是一次成交, 接管了却给不出东西, 客户端会留下
    // 一份幻影物品; 交给 BDS 走它自己的失败路径反而是安静的（客户端把预测撤回）。
    bool anyTrade = false;
    bool touchCursor = false;
    for (auto const& c : classified) {
        if (c.touchesCursor) touchCursor = true;
        if (c.kind == ActionKind::PayIn || c.kind == ActionKind::PayOut || c.kind == ActionKind::Deal
            || c.kind == ActionKind::PickUp || c.kind == ActionKind::CursorPayIn
            || c.kind == ActionKind::CursorBack) {
            anyTrade = true;
        }
    }
    if (!anyTrade) {
        // 不认的请求里若有牵涉光标的动作: 那份"手里拿着的"托管作废（BDS 的光标是空的, 它接不住,
        // 客户端会安静地撤回这次预测 —— 物品还在背包原处, 不会丢）
        if (touchCursor) {
            std::lock_guard lock(mMutex);
            if (auto it = mMenus.find(menu.menuId); it != mMenus.end()) it->second.cursor = CursorShadow{};
        }
        return false; // 与交易无关的请求: 放行给 BDS
    }

    // 客户端预测的产物落点（同一请求里的 ResultOut 动作; 没有就交给"找空位"）
    int productSlot = -1;
    for (auto const& c : classified) {
        if (c.kind == ActionKind::ResultOut && c.slot >= 0) {
            // 分类里存的是客户端槽号 → 换成服务端背包槽号（Inventory(29) 要 +9）
            productSlot = trade::settlement::inventorySlotOf(c.srcContainer, c.slot);
            break;
        }
    }

    // 两遍: 先把付费的进出摆对（一条"点条目"请求里通常同时带放料与成交, 顺序不保证）,
    // 再成交。全部在服务器线程, 不持锁（账本在局部副本上改, 完成后写回）。
    bool changed = false;
    // 只有"客户端预测之外的真实改动"才需要重推背包:
    //   · 付费放进交易槽 = 客户端自己预测的那次移动（服务端照做）, 两端一致 → 不要多此一举
    //   · 取回/成交/失败 = 落点或结果与客户端预测未必一致 → 必须把权威内容推回去
    bool needResync = false;
    bool ok         = true;
    std::string reason;
    // 顺序: 拿起 → 直接放料 → 光标放料 → 放回背包 → 取回 → 成交（一条请求里可能混着几种）
    for (auto const& c : classified) {
        if (c.kind != ActionKind::PickUp) continue;
        if (!applyPickUp(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed = true;
    }
    for (auto const& c : classified) {
        if (!ok) break;
        if (c.kind != ActionKind::PayIn) continue;
        if (!applyPayIn(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed = true;
    }
    for (auto const& c : classified) {
        if (!ok) break;
        if (c.kind != ActionKind::CursorPayIn) continue;
        if (!applyCursorPayIn(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed = true;
    }
    for (auto const& c : classified) {
        if (!ok) break;
        if (c.kind != ActionKind::CursorBack) continue;
        if (!applyCursorBack(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed    = true;
        needResync = true; // 落点由我们搬, 未必等于客户端预测的那一格
    }
    for (auto const& c : classified) {
        if (!ok) break;
        if (c.kind != ActionKind::PayOut) continue;
        if (!applyPayOut(player, menu, c, reason)) {
            ok = false;
            break;
        }
        changed    = true;
        needResync = true; // 退还落点是我们挑的空位, 未必等于客户端预测的那一格
    }
    bool dealt         = false;
    int  dealtOffer    = -1;
    for (auto const& c : classified) {
        if (!ok || c.kind != ActionKind::Deal) continue;
        if (!settleDeal(player, menu, c, productSlot, reason)) {
            ok = false;
            break;
        }
        changed    = true;
        needResync = true; // 产物落点同样以服务端为准
        dealt      = true;
        dealtOffer = c.offerIndex;
        emit(hologramlib::TradeSettlementEvent{
            player.getRealName(),
            menu.menuId,
            c.offerIndex,
            true,
            {}
        });
    }

    // 账本/光标写回（其它字段不覆盖: 菜单可能刚被关掉/改过）
    {
        std::lock_guard lock(mMutex);
        if (auto it = mMenus.find(menu.menuId); it != mMenus.end()) {
            it->second.ledger = std::move(menu.ledger);
            it->second.cursor = menu.cursor;
            if (touchCursor) it->second.cursor = CursorShadow{}; // 有没认全的光标动作 → 托管作废
        }
    }

    // 应答还是"空 containers 的 Success"（客户端能正确解析）; 交易槽的内容另用 InventorySlot 推 ——
    // 交易界面要求服务端把槽里的东西摆出来, 否则它会把这次放料的预测撤回（付款弹回、成交点不动）。
    container::sendRequestSuccess(player, {requestId});
    if (ok && changed) {
        for (auto const& c : classified) {
            if (c.kind != ActionKind::PayIn && c.kind != ActionKind::CursorPayIn && c.kind != ActionKind::PayOut) {
                continue;
            }
            // 该交易槽现在攒了什么（写回后的账本里数）
            ::ItemStack slotItem{};
            {
                std::lock_guard lock(mMutex);
                if (auto it = mMenus.find(menu.menuId); it != mMenus.end()) {
                    for (auto const& e : it->second.ledger) {
                        if (e.logical.tradeContainer == c.tradeContainer && e.logical.tradeSlot == c.tradeSlot) {
                            slotItem = e.stack; // 原样那一份
                            break;
                        }
                    }
                }
            }
            sendTradeSlotUpdate(player, menu, c.tradeSlot, slotItem);
        }
    }

    if (!ok) {
        std::string const why = reason.empty() ? "settle-failed" : reason;
        // 注意: 失败时**不回应答、也不接管** —— 交给 BDS 走它自己的失败路径（客户端安静撤回预测）
        // 失败: 把本次动过的账本还回去, **然后不接管** —— 交给 BDS 走它自己的失败路径。
        // 为什么不自己回失败应答: 客户端会弹错误提示; 也不推背包: 那会把客户端的光标/预测状态
        // 一起擦掉（实测症状: 刚拿起来的物品凭空消失、点击像被弹回）。让 BDS 拒掉这一次请求,
        // 客户端只会安静地撤回这一次预测。
        if (changed) refundHeld(player, menu.ledger);
        HLIB_LOG_WARN(
            "[TradeMenu] 真结算未完成: player={} 原因={}（已退还本次扣下的付费, 请求交回 BDS）",
            menu.playerName,
            why
        );
        emit(hologramlib::TradeSettlementEvent{player.getRealName(), menu.menuId, dealtOffer, false, why});
        std::lock_guard lock(mMutex);
        if (auto it = mMenus.find(menu.menuId); it != mMenus.end()) it->second.ledger = std::move(menu.ledger);
        return false; // 不接管
    }
    if (changed && needResync) {
        // 有东西真的动了、且落点可能和客户端预测不同: 推一次权威背包, 两端对齐
        player.sendInventory(false);
    }
    (void)dealt;
    return true; // 接管这条请求（调用方回成功应答、不交给 BDS）
}

bool TradeMenuManager::setSettleLocally(int64_t menuId, bool on) {
    std::lock_guard lock(mMutex);
    auto            it = mMenus.find(menuId);
    if (it == mMenus.end()) return false;
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
    if (byPlayer == mByPlayer.end()) return false;
    auto it = mMenus.find(byPlayer->second);
    return it != mMenus.end() && it->second.settleLocally && !it->second.serverTrades;
}

uint64_t TradeMenuManager::addSettlementListener(
    std::function<void(hologramlib::TradeSettlementEvent const&)> listener
) {
    if (!listener) return 0;
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

// 掉线: 关菜单 + 把账本里暂存的付费还给玩家（用活着的 Player 引用 —— 这一刻按名字查人可能已查不到）
void TradeMenuManager::onPlayerDisconnect(::Player& player) {
    std::string const playerName = player.getRealName();
    int64_t           menuId     = -1;
    {
        std::lock_guard lock(mMutex);
        auto            it = mByPlayer.find(playerName);
        if (it != mByPlayer.end()) menuId = it->second;
    }
    if (menuId < 0) return;

    std::vector<LedgerEntry> ledger;
    CursorShadow             cursor;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mMenus.find(menuId); it != mMenus.end()) {
            ledger.swap(it->second.ledger);
            cursor = it->second.cursor;
            it->second.cursor        = CursorShadow{};
            it->second.settleLocally = false; // 离线瞬间起不再接管请求
        }
    }
    // 手里拿着的那份也要还（服务端背包里它还在 fromSlot, 但客户端认为在手上 —— 关掉菜单就作废,
    // 直接把背包里的原样内容推回客户端即可, 不额外给物品）
    // **不给正在掉线的客户端发包**: 对端已在拆连接, 这时塞包可能把客户端会话状态弄脏
    // （实测怀疑: 掉线瞬间推背包后, 客户端再进服会报错）。物品已经还回服务端背包,
    // 客户端重进时由服务端权威数据自然同步。
    if (!ledger.empty()) {
        int const n = refundHeld(player, ledger);
        HLIB_LOG_INFO("[TradeMenu] 掉线退还暂存的付费: player={} 条目={}（不发包）", playerName, n);
    } else if (cursor.active) {
        HLIB_LOG_INFO(
            "[TradeMenu] 掉线作废光标托管: player={} {} x{}（背包原处未动, 不发包）",
            playerName,
            cursor.stack.getTypeName(),
            static_cast<int>(cursor.stack.mCount)
        );
    }
    close(menuId);
}

void TradeMenuManager::init() {
    if (!mDisconnectListener) {
        mDisconnectListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
            [](ll::event::PlayerDisconnectEvent& ev) {
                TradeMenuManager::getInstance().onPlayerDisconnect(ev.self());
            }
        );
    }
}

void TradeMenuManager::shutdown() {
    if (mDisconnectListener) {
        ll::event::EventBus::getInstance().removeListener(mDisconnectListener);
        mDisconnectListener.reset();
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
    // ── 1.24.0: 纯协议层真结算 ──
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
