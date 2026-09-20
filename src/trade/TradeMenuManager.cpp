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
#include <mc/deps/core/utility/ReadOnlyBinaryStream.h>
#include <mc/network/Compressibility.h>
#include <mc/network/MinecraftPackets.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/ContainerClosePacket.h>
#include <mc/network/packet/ItemStackRequestPacket.h>
#include <mc/network/packet/PlayerAuthInputPacket.h>
#include <mc/network/packet/cerealize/types/item_stack_request_cereal/ItemStackRequestCereal.h>
#include <mc/entity/components_json_legacy/EconomyTradeableComponent.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemInstance.h>
#include <mc/world/item/trading/MerchantRecipe.h>
#include <mc/world/item/trading/MerchantRecipeList.h>
#include <mc/world/level/Level.h>
#include <mc/world/inventory/transaction/ComplexInventoryTransaction.h>
#include <mc/world/inventory/transaction/InventoryAction.h>
#include <mc/world/inventory/transaction/InventorySource.h>
#include <mc/world/containers/ContainerEnumName.h>
#include <mc/world/containers/FullContainerName.h>
#include <mc/world/inventory/network/ItemStackRequestAction.h>
#include <mc/world/inventory/network/ItemStackRequestActionTransferBase.h>
#include <mc/world/inventory/network/ItemStackRequestSlotInfo.h>
#include <mc/world/inventory/network/ItemStackNetResult.h>
#include <mc/world/inventory/transaction/InventoryTransactionError.h>
#include <mc/world/level/Level.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerID.hpp>
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

// 交易相关的容器枚举（客户端把交易界面里的槽位归在这些容器里）
bool isTradeContainerEnum(int name) {
    switch (static_cast<ContainerEnumName>(name)) {
        case ContainerEnumName::LevelEntityContainer:        // 配方列表条目
        case ContainerEnumName::TradeIngredient1Container:   // 当前条目的付费 A
        case ContainerEnumName::TradeIngredient2Container:   // 当前条目的付费 B
        case ContainerEnumName::TradeResultPreviewContainer: // 当前条目的产物
        case ContainerEnumName::Trade2Ingredient1Container:
        case ContainerEnumName::Trade2Ingredient2Container:
        case ContainerEnumName::Trade2ResultPreviewContainer:
        // 客户端把成交产物落到"合成产物容器"再搬进背包（实测: 一次成交的动作序列是
        // CraftRecipeAuto → Consume(背包) → CraftResults → Place(60 → 背包)）。
        // 不把它算进来的话, 这段动作会被报成 trade=false, 看起来像"跟交易无关"。
        case ContainerEnumName::CreatedOutputContainer:
            return true;
        default:
            return false;
    }
}

// 付费槽（31/32 = 单付费界面 A/B, 47/48 = 新交易界面 A/B）:
// "把付费物品放进交易槽"就是这些容器上的动作。纯协议层路径下这一步会被 BDS 拒掉, 客户端的
// 交易界面于是进不到可成交状态 —— 库需要自己把它接住（见 TradeMenuSpec::acceptPaymentPlacement）。
bool isTradeIngredientEnum(int name) {
    switch (static_cast<ContainerEnumName>(name)) {
        case ContainerEnumName::TradeIngredient1Container:
        case ContainerEnumName::TradeIngredient2Container:
        case ContainerEnumName::Trade2Ingredient1Container:
        case ContainerEnumName::Trade2Ingredient2Container:
            return true;
        default:
            return false;
    }
}

// 产物槽 = 真正的成交动作（取出产物）; displayOnly 只拦这一步, 不拦"把付费物品放进槽"
bool isTradeResultEnum(int name) {
    return static_cast<ContainerEnumName>(name) == ContainerEnumName::TradeResultPreviewContainer
        || static_cast<ContainerEnumName>(name) == ContainerEnumName::Trade2ResultPreviewContainer;
}

Player* findPlayerByNetworkIdImpl(NetworkIdentifier const& source) {
    auto level = ll::service::getLevel();
    if (!level) return nullptr;
    Player* found = nullptr;
    level->forEachPlayer([&](Player& p) -> bool {
        if (p.getNetworkIdentifier() == source) {
            found = &p;
            return false;
        }
        return true;
    });
    return found;
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
        // 才能反查下标（详见 offerIndexFromServerTrades）。不设的话全为默认 0, 无法区分。
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
        "[TradeMenu] 已打开: player={} menuId={} containerId={} tier={} offers={} type='{}' displayOnly={}",
        playerName,
        menuId,
        menu.containerId,
        spec.tier,
        spec.offers.size(),
        spec.tradeType,
        spec.displayOnly
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
        if (!copy.serverTrades) {
            sendContainerClose(*player, copy);
            // 纯协议层路径可能接住过"放进交易槽"（acceptPaymentPlacement）: 那只是客户端侧的
            // 预测, 服务端背包里东西还在。关界面时刷一次背包把它清干净。
            player->refreshInventory();
        }
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

uint64_t TradeMenuManager::addClickListener(std::function<void(hologramlib::TradeClickEvent const&)> listener) {
    if (!listener) return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextListenerToken++;
    mListeners.emplace(token, std::move(listener));
    return token;
}

bool TradeMenuManager::removeClickListener(uint64_t token) {
    std::lock_guard lock(mMutex);
    return mListeners.erase(token) > 0;
}

void TradeMenuManager::dispatch(hologramlib::TradeClickEvent const& event) {
    // 快照后在锁外回调（允许监听器内部再注册/移除）
    std::vector<std::function<void(hologramlib::TradeClickEvent const&)>> snapshot;
    {
        std::lock_guard lock(mMutex);
        snapshot.reserve(mListeners.size());
        for (auto const& [token, fn] : mListeners) snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn) fn(event);
    }
}

bool TradeMenuManager::handleAction(
    std::string const&        playerName,
    hologramlib::TradeSlotRef src,
    hologramlib::TradeSlotRef dst,
    int                       amount,
    bool                      srcIsResult,
    int                       recipeNetId
) {
    hologramlib::TradeClickEvent                      click;
    std::vector<std::function<void(
        std::string const&,
        int64_t,
        hologramlib::TradeSlotRef const&,
        hologramlib::TradeSlotRef const&,
        int
    )>>                                               snapshot;
    bool                                              deny = false;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end()) return false;
        auto& menu = menuIt->second;

        // 报"交易侧"那一个槽位: 玩家把付费物品从背包拖进付费槽时, src 是背包槽、dst 才是交易槽
        // —— 只报 src 的话调用方看到的是背包槽位（历史症状: [CLICK] slot=12 container=12 背包）。
        hologramlib::TradeSlotRef primary = src;
        bool                      primaryIsResult = srcIsResult;
        if (!isTradeContainerEnum(src.container) && isTradeContainerEnum(dst.container)) {
            primary         = dst;
            primaryIsResult = isTradeResultEnum(dst.container);
        }

        // 语义化的点击事件（配方列表条目 → offerIndex; 付费/产物槽 → 靠 recipeNetId 定位）
        click.playerName   = playerName;
        click.menuId       = menu.menuId;
        click.slot         = primary.slot;
        click.container    = primary.container;
        click.containerId  = menu.containerId;
        click.recipeNetId  = recipeNetId;
        click.offerIndex   = primary.container == static_cast<int>(ContainerEnumName::LevelEntityContainer)
                               ? primary.slot / 3
                               : -1;
        if (click.offerIndex < 0 && recipeNetId > 0) {
            // 点条目在"新交易界面"里走 CraftRecipe(带配方 id), 槽位信息可能落在付费槽上 ——
            // 用配方 id 定位更准（纯协议层路径下 netId = netIdBase + 条目下标）。
            int const idx = recipeNetId - menu.netIdBase;
            if (idx >= 0 && idx < static_cast<int>(menu.spec.offers.size())) click.offerIndex = idx;
        }
        click.accepted = !menu.spec.displayOnly;
        // displayOnly 的拦截只在**服务端真的会成交**时才需要（真实交易表路径）——那时不拦,
        // BDS 就会把东西换掉。纯协议层路径根本没有对应交易表, 放行后 BDS 自己就会失败, 天然只读。
        // 而"自己回一条失败应答"会让客户端弹错误提示（实测），所以能放行就放行。
        deny = menu.spec.displayOnly && primaryIsResult && menu.serverTrades;

        snapshot.reserve(mActionListeners.size());
        for (auto const& [token, fn] : mActionListeners) snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn) fn(playerName, click.menuId, src, dst, amount);
    }
    dispatch(click);
    return deny;
}

// 客户端点了配方列表里的某一条: CraftRecipe 动作回传的 recipeNetId 是"点了哪一条"的主信号。
bool TradeMenuManager::handleOfferClick(std::string const& playerName, int recipeNetId) {
    hologramlib::TradeClickEvent event;
    int                           offerIndex = -1;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end()) return false;
        auto& menu = menuIt->second;

        offerIndex = recipeNetId - menu.netIdBase;
        if (offerIndex < 0 || offerIndex >= static_cast<int>(menu.spec.offers.size())) {
            // 真实交易表路径: netId 由 BDS 分配, 得从载体身上的交易表里反查
            offerIndex = offerIndexFromServerTrades(menu, recipeNetId);
        }

        event.playerName  = playerName;
        event.menuId      = menu.menuId;
        event.recipeNetId = recipeNetId;
        event.offerIndex  = offerIndex;
        event.container   = static_cast<int>(ContainerEnumName::LevelEntityContainer); // 配方列表
        event.slot        = offerIndex >= 0 ? offerIndex * 3 : -1;                     // 每条占 3 槽
        event.containerId = menu.containerId;
        event.accepted    = !menu.spec.displayOnly;
    }
    if (offerIndex < 0) {
        // 定位不到 = 这次点击**不是本菜单的条目**（例如同时开着真实村民的交易界面: 它的
        // netId 由 BDS 分配, 不在本菜单的范围内）。这里不上报 —— 虚拟容器/交易容器的槽位枚举
        // 与真实村民完全一样, 只靠枚举分辨不了, netId 是唯一的可靠判据。
        HLIB_LOG_INFO(
            "[TradeMenu] 配方点击不属于本菜单, 忽略: player={} recipeNetId={}",
            playerName,
            recipeNetId
        );
        return false;
    }
    HLIB_LOG_INFO("[TradeMenu] 条目点击: player={} offerIndex={} recipeNetId={}", playerName, offerIndex, recipeNetId);
    dispatch(event);
    return true;
}

// 真实交易表路径下的反查: 在载体的 MerchantRecipeList 里找 mRecipeNetId 命中的下标。
int TradeMenuManager::offerIndexFromServerTrades(Menu const& menu, int recipeNetId) {
    if (!menu.serverTrades || menu.carrierUniqueId == 0) return -1;
    auto level = ll::service::getLevel();
    if (!level) return -1;
    auto* actor = level->fetchEntity(::ActorUniqueID(static_cast<std::int64_t>(menu.carrierUniqueId)), false);
    if (actor == nullptr) return -1;
    auto comp = actor->getEntityContext().tryGetComponent<EconomyTradeableComponent>();
    if (!comp || !comp->mOffers) return -1;
    auto const& recipes = comp->mOffers->mRecipeList.get();
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        if (static_cast<int>(unwrap(recipes[i].mRecipeNetId).mRawId) == recipeNetId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TradeMenuManager::handleContainerClose(std::string const& playerName, int containerId) {
    int64_t menuId = -1;
    std::vector<std::function<void(
        std::string const&,
        int64_t,
        hologramlib::TradeSlotRef const&,
        hologramlib::TradeSlotRef const&,
        int
    )>> snapshot;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end()) return false;
        // 纯协议层路径: 容器 id 是我们自己发出去的, 按 id 认。
        // 真实交易表路径: 界面由 BDS 打开, 容器 id 是 **BDS 分配的**, 我们那个只是占位 —— 按 id 认
        // 会让"界面结束"哨兵永远不发（消费方收不到收尾事件）。那条路上一个玩家只有这一个菜单,
        // 所以按"有没有菜单"认即可。
        if (!menuIt->second.serverTrades && menuIt->second.containerId != containerId) return false;
        menuId = menuIt->second.menuId;
        snapshot.reserve(mActionListeners.size());
        for (auto const& [token, fn] : mActionListeners) snapshot.push_back(fn);
    }

    // 关闭哨兵: {slot = -1} + amount = -1 —— 与参考实现 GMLIB ChestUI 的约定一致,
    // 让调用方在同一个回调里就能收尾（不需要额外接一个"关闭"回调）。
    hologramlib::TradeSlotRef const none{};
    for (auto& fn : snapshot) {
        if (fn) fn(playerName, menuId, none, none, -1);
    }
    close(menuId); // 结束菜单记录（客户端已经关了, 这里只清服务端状态）
    return true;
}

uint64_t TradeMenuManager::addActionListener(std::function<void(
                                                 std::string const&,
                                                 int64_t,
                                                 hologramlib::TradeSlotRef const&,
                                                 hologramlib::TradeSlotRef const&,
                                                 int
                                             )> listener) {
    if (!listener) return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextListenerToken++;
    mActionListeners.emplace(token, std::move(listener));
    return token;
}

bool TradeMenuManager::removeActionListener(uint64_t token) {
    std::lock_guard lock(mMutex);
    return mActionListeners.erase(token) > 0;
}

void TradeMenuManager::dispatchRawAction(std::string const& playerName, hologramlib::TradeRawAction action) {
    std::vector<hologramlib::TradeRawActionCallback> snapshot;
    int64_t                                          menuId = -1;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) {
            // 虚拟容器没有交易菜单, 但**这次点击也要看得见** —— 原始动作诊断是排障时唯一的
            // 事实来源（"客户端到底发没发、发的是什么容器/槽位"），所以两个域共用它:
            // 容器菜单开着时也照常派发, menuId 取容器的 id。
            auto containerMenuId = debugshape_export::ContainerMenuManager::getInstance().menuIdFor(playerName);
            if (containerMenuId < 0) return;
            menuId = containerMenuId;
        } else {
            auto menuIt = mMenus.find(byPlayer->second);
            if (menuIt == mMenus.end()) return;
            menuId = menuIt->second.menuId;
        }
        snapshot.reserve(mRawActionListeners.size());
        for (auto const& [token, fn] : mRawActionListeners) snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn) fn(playerName, menuId, action);
    }
}

uint64_t TradeMenuManager::addRawActionListener(hologramlib::TradeRawActionCallback listener) {
    if (!listener) return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextListenerToken++;
    mRawActionListeners.emplace(token, std::move(listener));
    return token;
}

bool TradeMenuManager::removeRawActionListener(uint64_t token) {
    std::lock_guard lock(mMutex);
    return mRawActionListeners.erase(token) > 0;
}

bool TradeMenuManager::handleTransaction(
    std::string const& playerName,
    int                containerId,
    int                slot,
    bool&              outReject
) {
    hologramlib::TradeClickEvent event;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end()) return false;
        auto& menu = menuIt->second;
        if (menu.containerId != containerId) return false; // 不是本菜单的容器

        outReject        = menu.spec.displayOnly;
        event.playerName = playerName;
        event.menuId     = menu.menuId;
        event.slot       = slot;
        event.offerIndex = -1; // 槽位→条目映射待实测确定（见头文件说明）
        event.accepted   = !menu.spec.displayOnly;
    }
    dispatch(event);
    return true;
}

bool TradeMenuManager::acceptsPlacementLocally(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            it = mByPlayer.find(playerName);
    if (it == mByPlayer.end()) return false;
    auto menuIt = mMenus.find(it->second);
    if (menuIt == mMenus.end()) return false;
    // 只在纯协议层路径下需要（真实交易表路径放进交易槽本来就会被 BDS 接受）
    return !menuIt->second.serverTrades && menuIt->second.spec.acceptPaymentPlacement;
}

bool TradeMenuManager::isDisplayOnlyFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            it = mByPlayer.find(playerName);
    if (it == mByPlayer.end()) return false;
    auto menuIt = mMenus.find(it->second);
    return menuIt != mMenus.end() && menuIt->second.spec.displayOnly;
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
// 读包里的槽位信息（ItemStackRequestCereal::SlotInfoData）, 命中交易容器就回传一次点击。
// 强类型直读（包里的动作本来就是序列化数据结构）, 不做运行期下转型。
// 放这里而不是管理器成员: 免于把 cereal 头文件拉进 TradeMenuManager.h。
// 从包里的动作数据结构里抽出槽位引用（强类型直读, 不做运行期下转型）
int slotRefDynamicId(::ItemStackRequestCereal::SlotInfoData const& slotInfo) {
    auto const& opt = unwrap(unwrap(slotInfo.mFullContainerName).mDynamicId);
    return opt.has_value() ? static_cast<int>(*opt) : -1;
}

void fillSlotRef(::ItemStackRequestCereal::SlotInfoData const& slotInfo, hologramlib::TradeSlotRef& out) {
    auto const& fcn = unwrap(slotInfo.mFullContainerName);
    out.container   = static_cast<int>(unwrap(fcn.mName));
    out.slot        = static_cast<int>(slotInfo.mSlot);
}

} // namespace debugshape_export

// ── 库存事务钩子: 交易点击走这条路径（客户端"买不起"时不发包, 故只有可成交的点击会到达）
// 注意: 与 LegacyScriptEngine 的同址钩子叠加为 2 层, 在其记录的崩服阈值（>2）之内。
LL_TYPE_INSTANCE_HOOK(
    TradeMenuTxnHook,
    ll::memory::HookPriority::Normal,
    ComplexInventoryTransaction,
    &ComplexInventoryTransaction::$handle,
    InventoryTransactionError,
    ::Player& player,
    bool      isSenderAuthority
) {
    bool        hit    = false;
    bool        reject = false;
    std::string summary;
    for (auto const& [source, actions] : mTransaction->mActions.get()) {
        for (auto const& action : actions) {
            summary += std::format(
                " [srcType={} cid={} slot={}]",
                static_cast<int>(source.mType),
                static_cast<int>(source.mContainerId),
                static_cast<int>(action.mSlot)
            );
        }
        if (source.mType != InventorySourceType::ContainerInventory) continue;
        for (auto const& action : actions) {
            if (debugshape_export::TradeMenuManager::getInstance().handleTransaction(
                    player.getRealName(),
                    static_cast<int>(source.mContainerId),
                    static_cast<int>(action.mSlot),
                    reject
                )) {
                hit = true;
            }
        }
    }
    // 诊断: 菜单打开期间把**所有**事务（含没匹配上的）记下来 ——
    // 用来分辨"客户端因为买不起根本没发" 与 "发了但我们没匹配到容器 id"
    if (auto const& mgr = debugshape_export::TradeMenuManager::getInstance();
        mgr.hasMenuFor(player.getRealName())) {
        HLIB_LOG_INFO(
            "[TradeMenu] 菜单打开期间收到事务: player={} txnType={} matched={}{}",
            player.getRealName(),
            static_cast<int>(mType),
            hit,
            summary
        );
    }
    // 只观察, 不在这里拒绝: 交易点击实测全部走 ItemStackRequest(147), 这条事务通道对交易
    // 从未触发过（历史日志里"收到事务"0 行）; 而它一旦触发就是整笔拒绝, 会把"把付费物品放进
    // 交易槽"这种正常操作一起挡掉（实测症状: 东西放不进交易槽、产物槽恒红）。
    // displayOnly 的拦截放在 147 通道里按"从产物槽取出"精确判定。
    (void)reject;
    return origin(player, isSenderAuthority);
}

static ll::memory::HookRegistrar<TradeMenuTxnHook> gTradeMenuTxnHookRegistrar;

// 回一个"该请求不允许"的应答。
// 关键: 拦截**不能只丢包** —— 客户端在等这次 ItemStackRequest 的应答, 不回就卡在预测状态
// （实测症状: 物品放不进交易槽、产物槽恒红）。参考实现 GMLIB 干脆不拦（放行 + 覆盖客户端显示）;
// 我们要拦, 就必须自己把这应答补上。
//
// 为什么不用 BDS 的 ItemStackResponsePacket: 它含 ItemStackRequestId（TypedClientNetId 有虚函数）,
// 该类型在 LeviLamina 26.40 未导出符号, 一构造就链接失败。而 sculk 的应答里 requestId 就是 int32,
// 且我们本来就是自己拼字节经 NetworkPeer 发 —— 走 sculk 反而更直接。
// 3 = ItemStackNetResult::ActionRequestNotAllowed（sculk 的枚举只列了 0/1, 按线格式取原值）。
void rejectRequests(::Player& player, std::vector<std::int32_t> const& requestIds) {
    if (requestIds.empty()) return;
    sculk::protocol::ItemStackResponsePacket packet;
    for (auto const id : requestIds) {
        sculk::protocol::ItemStackResponseInfo info;
        info.mResult    = static_cast<sculk::protocol::ItemStackNetResult>(3);
        info.mRequestId = id;
        packet.mResponse.mResponses.push_back(std::move(info));
    }
    debugshape_export::sendTradePacket(player, packet);
}

namespace debugshape_export {
namespace {

// 一次请求的处理结论（两个钩子共用）:
//   Forward       = 照常交给 BDS
//   Deny          = 自己回失败应答并拦下（displayOnly 拦成交）
//   AcceptLocally = 自己回**成功**应答并拦下（把付费放进交易槽 —— 纯协议层路径下服务端没有这个
//                   容器, 放行会被 BDS 拒掉, 客户端把物品弹回、界面永远进不到可成交状态; 回成功
//                   让客户端保留这次放入, 触屏才走得到成交）
enum class StackVerdict { Forward, Deny, AcceptLocally };

// 处理一个物品请求动作（cereal 形态: ItemStackRequestCereal::*ActionData）。
//
// **两条包通道共用这一份逻辑**:
//   ① ItemStackRequestPacket(147) —— 独立的物品请求
//   ② PlayerAuthInputPacket(AuthInput, 144) 里内嵌的 mItemStackRequest —— 菜单界面的交互常走这条;
//      BDS 的 ItemStackRequestCereal::toActionData() 能把它的解析态动作转成同一 cereal 形态。
// 只挂 147 会漏掉 AuthInput 那条（实测: 触屏点交易条目一条回调都没有, 就是因为它走 AuthInput）。
template <typename ActionDataT>
StackVerdict processStackAction(::Player& player, ActionDataT const& data, std::size_t actionIndex, int sourcePacketId) {
    auto& mgr = TradeMenuManager::getInstance();

        using T = std::decay_t<decltype(data)>;
        hologramlib::TradeSlotRef src{};
        hologramlib::TradeSlotRef dst{};
        bool                       hasSrc = false;
        bool                       hasDst = false;
        int                        amount = 1;

        int srcContainerId = -1;
        int dstContainerId = -1;
        if constexpr (requires { data.mSource; }) {
            debugshape_export::fillSlotRef(data.mSource.get(), src);
            srcContainerId = debugshape_export::slotRefDynamicId(data.mSource.get());
            hasSrc         = true;
        }
        if constexpr (requires { data.mDestination; }) {
            debugshape_export::fillSlotRef(data.mDestination.get(), dst);
            dstContainerId = debugshape_export::slotRefDynamicId(data.mDestination.get());
            hasDst         = true;
        }
        if constexpr (requires { data.mAmount; }) {
            amount = static_cast<int>(data.mAmount);
        }

        // CraftRecipe / CraftRecipeAuto / CraftRecipeOptional 三种动作都带 mRecipeNetId
        // —— 这是"玩家点了配方列表里的哪一条"的信号（交易条目在新交易界面里就是配方）。
        // 注意: CraftRepairAndDisenchantActionData 也有个同名成员, 但类型是 ItemStackNetIdVariant,
        // 所以这里必须按具体类型分辨, 不能用 requires { mRecipeNetId }。
        int recipeNetId = -1;
        if constexpr (
            std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeActionData>
            || std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeAutoActionData>
            || std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeOptionalActionData>
        ) {
            recipeNetId = static_cast<int>(debugshape_export::unwrap(data.mRecipeNetId).mRawId);
        }

        // 诊断: 先把原始动作交出去（不筛容器）, 探针据此能看到"客户端到底发了什么"
        hologramlib::TradeRawAction raw;
        raw.actionIndex   = static_cast<int>(actionIndex);
        raw.src           = src;
        raw.dst           = dst;
        raw.amount        = amount;
        raw.hasSrc        = hasSrc;
        raw.hasDst        = hasDst;
        raw.recipeNetId   = recipeNetId;
        raw.sourcePacketId = sourcePacketId;
        raw.srcContainerId = srcContainerId;
        raw.dstContainerId = dstContainerId;
        raw.tradeRelated  = (hasSrc && debugshape_export::isTradeContainerEnum(src.container))
                         || (hasDst && debugshape_export::isTradeContainerEnum(dst.container));
        mgr.dispatchRawAction(player.getRealName(), raw);

        // 点了某一条交易 → 立即回调（用配方 id 定位条目, 不依赖槽位信息 ——
        // 新交易界面里这个动作没有槽位, 槽位信号落在随后的付费/产物槽动作上）
        if (recipeNetId > 0) {
            mgr.handleOfferClick(player.getRealName(), recipeNetId);
        }

        // 虚拟容器（列表）域与交易域共用这一个 147 钩子（同址只挂一层）:
        // 两者的槽位都在实体容器上, 由容器 id 区间区分（虚拟容器固定 101..199）。
        // 虚拟容器先认; 认下了就不再走交易域的实体容器判定（免得同一动作回传两次）。
        // 两侧都试: 取物时槽位在 src, 往容器里放物时在 dst —— 都算"点了这个格子"。
        bool containerClaimed = false;
        if (hasSrc && src.container == static_cast<int>(ContainerEnumName::LevelEntityContainer)) {
            containerClaimed = debugshape_export::ContainerMenuManager::getInstance().handleSlotAction(
                player.getRealName(),
                src.container,
                srcContainerId,
                src.slot
            );
        }
        if (!containerClaimed && hasDst
            && dst.container == static_cast<int>(ContainerEnumName::LevelEntityContainer)) {
            containerClaimed = debugshape_export::ContainerMenuManager::getInstance().handleSlotAction(
                player.getRealName(),
                dst.container,
                dstContainerId,
                dst.slot
            );
        }
        // 虚拟容器命中: **只回传, 不拦**（与参考实现 GMLIB 完全一致 —— 它也是派发回调后
        // origin 放行）。实测教训: 这里若自己回一条失败应答（ItemStackNetResult 3）, 客户端会
        // **弹一个错误提示**; 交给 BDS 走它自己的失败路径反而是安静的（客户端只把预测撤回去）,
        // 物品在界面上闪一下回到原位, 回调照常收到。所以别"自作聪明"地代答。
        (void)containerClaimed;

        // 只有落在交易容器上的动作才回传（背包内部搬东西不该打扰调用方）
        bool const tradeSrc = hasSrc && debugshape_export::isTradeContainerEnum(src.container);
        bool const tradeDst = hasDst && debugshape_export::isTradeContainerEnum(dst.container);
        if (!containerClaimed && (tradeSrc || tradeDst)) {
            if (mgr.handleAction(
                    player.getRealName(),
                    src,
                    dst,
                    amount,
                    tradeSrc && debugshape_export::isTradeResultEnum(src.container),
                    recipeNetId
                )) {
                return StackVerdict::Deny;
            }
        }
        // 付费放进交易槽 → 纯协议层路径下由库自己接住（回成功）, 见 StackVerdict 的说明
        if (hasDst && debugshape_export::isTradeIngredientEnum(dst.container)
            && mgr.acceptsPlacementLocally(player.getRealName())) {
            return StackVerdict::AcceptLocally;
        }
        // 注意: **不能**在这里拦 CraftRecipe 类动作 —— 交易界面是用合成那套机制实现的,
        // 点条目让付费物品进槽走的就是 CraftRecipe 请求; 拦了它物品就进不了交易槽、产物槽恒红
        // （实测症状: "东西放不了交易槽、产物槽一直红"）。真正的成交在同一个请求里带一个
        // "从产物槽取出"的转移动作, 由上面的产物槽判定拦下即可。
        (void)std::is_same_v<T, ::ItemStackRequestCereal::CraftRecipeActionData>;
        return StackVerdict::Forward;
}

} // namespace
} // namespace debugshape_export

// 回一条"这次请求成功了"的应答（与 rejectRequests 对称）。
// Success 的线上形态比失败多一段 containers 数组（sculk ItemStackResponseInfo::write）; 这里给
// 空数组 = "没有槽位更正", 客户端就保留自己的预测 —— 正是要的: 付费留在交易槽里（客户端侧）,
// 成交按钮因此可用。
void acceptRequests(::Player& player, std::vector<std::int32_t> const& requestIds) {
    if (requestIds.empty()) return;
    sculk::protocol::ItemStackResponsePacket packet;
    for (auto const id : requestIds) {
        sculk::protocol::ItemStackResponseInfo info;
        info.mResult    = sculk::protocol::ItemStackNetResult::Success;
        info.mRequestId = id;
        packet.mResponse.mResponses.push_back(std::move(info));
    }
    debugshape_export::sendTradePacket(player, packet);
}

// ── ItemStackRequestPacket(147): 交易点击的**真正通道** ──
// 实测结论（旧日志 + 抓包）: 客户端对交易槽/配方列表的操作全部走这条包, 从不经过
// ComplexInventoryTransaction(30) —— 所以点击回调必须挂在这里; 只挂 30 的结果是
// "点了完全没反应"（历史上 0 条回调记录, 而同一时段 147 请求一条不少）。
//   · 转移类动作（Take/Place/Swap/Drop/Destroy/Consume/Create = 0..6）带 src/dst 槽位信息,
//     可据此判定点了哪个容器/槽位
//   · 成交是 CraftRecipe 类动作（12..19）: 它没有槽位信息, 但同一请求里会带一个
//     "从产物槽取出"的转移动作, 所以按产物槽位拦截即可覆盖成交
// 槽位 → 条目 的映射: 配方列表条目挂在实体容器(LevelEntityContainer)上, 每条占 3 槽
// （buyA/buyB/产物），故 offerIndex = slot / 3 —— 映射待实测钉死, 事件里同时回传原始 slot。
LL_TYPE_INSTANCE_HOOK(
    TradeItemStackRequestHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&      source,
    ItemStackRequestPacket const& packet
) {
    auto* player = debugshape_export::findPlayerByNetworkIdImpl(source);
    if (player == nullptr) {
        origin(source, packet);
        return;
    }
    auto& mgr = debugshape_export::TradeMenuManager::getInstance();
    // 这个 147 钩子是**两个域共用**的: 交易菜单与虚拟容器都靠它收点击。
    // 所以入口条件是"任一域开着" —— 只判交易会让虚拟容器的点击全部漏掉
    // （实测症状: 虚拟容器开着, 点条目什么回调都没有）。
    if (!mgr.hasMenuFor(player->getRealName())
        && !debugshape_export::ContainerMenuManager::getInstance().hasMenuFor(player->getRealName())) {
        origin(source, packet);
        return;
    }

    debugshape_export::StackVerdict verdict      = debugshape_export::StackVerdict::Forward;
    std::vector<std::int32_t>       requestIds;
    for (auto const& request : packet.mRequests.get()) {
        requestIds.push_back(request.mClientRequestId.get().mRawId);
        for (auto const& action : request.mActions.get()) {
            std::visit(
                [&](auto const& data) {
                    auto const v = debugshape_export::processStackAction(*player, data, action.index(), 147);
                    if (v == debugshape_export::StackVerdict::Deny) verdict = v; // Deny 优先
                    else if (v == debugshape_export::StackVerdict::AcceptLocally
                             && verdict == debugshape_export::StackVerdict::Forward) verdict = v;
                },
                action
            );
        }
    }

    if (verdict == debugshape_export::StackVerdict::AcceptLocally) {
        // 自己接住这次"放进交易槽": 回成功、不交给 BDS（交给它只会被拒）
        acceptRequests(*player, requestIds); // 与 rejectRequests 同处全局名字域（钩子宏体内未限定名可用）
        return;
    }
    if (verdict == debugshape_export::StackVerdict::Deny) {
        // 两种情况都走这里: displayOnly 拦下成交, 以及虚拟容器命中（服务端没有这个容器）。
        // 关键是**必须**补一条失败应答 —— 不回就让客户端卡在预测态（物品放不进去、格子恒红）。
        rejectRequests(*player, requestIds);
        return;
    }
    origin(source, packet);
}

static ll::memory::HookRegistrar<TradeItemStackRequestHook> gTradeItemStackRequestHookRegistrar;

// ── PlayerAuthInputPacket(144, AuthInput): 点击的**第二条通道** ──
// 客户端把物品请求发上来有两条路: 独立的 ItemStackRequestPacket(147), 以及搭在 AuthInput 里
// 内嵌的 mItemStackRequest（PlayerAuthInputPacketPayload::mItemStackRequest,
// 对应的输入标志是 PerformItemStackRequest = 36）。菜单界面的交互经常走后者 ——
// 实测: 触屏点交易条目时 147 一条都没有, 只看 147 就永远等不到回调。
// 这里把内嵌请求用 BDS 自己的 toActionData() 转成与 147 相同的 cereal 形态, 再喂进同一套
// processStackAction()。**只观察、不拦**: 这个包同时承载玩家移动, 拦下它会把移动一起吞掉;
// 而内嵌请求由 BDS 自己处理（虚拟容器/纯协议层交易在服务端本来就不存在, 它自然失败）。
LL_TYPE_INSTANCE_HOOK(
    TradeAuthInputHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&       source,
    PlayerAuthInputPacket const&   packet
) {
    origin(source, packet); // 移动照常处理

    // 绝大多数 AuthInput 不带物品请求 —— 先做这个指针判断, 免得每 tick 都去查菜单
    if (packet.mItemStackRequest == nullptr) return;

    auto* player = debugshape_export::findPlayerByNetworkIdImpl(source);
    if (player == nullptr) return;
    auto& mgr = debugshape_export::TradeMenuManager::getInstance();
    if (!mgr.hasMenuFor(player->getRealName())
        && !debugshape_export::ContainerMenuManager::getInstance().hasMenuFor(player->getRealName())) {
        return;
    }

    auto const& request = *packet.mItemStackRequest;
    std::size_t actionIndex = 0;
    HLIB_LOG_INFO(
        "[TradeMenu] AuthInput 内嵌物品请求: player={} 动作数={}",
        player->getRealName(),
        request.mActions.get().size()
    );
    for (auto const& actionPtr : request.mActions.get()) {
        if (actionPtr != nullptr) {
            // 解析态 → cereal 形态（BDS 自带转换）, 之后与 147 完全同路
            std::visit(
                [&](auto const& data) {
                    (void)debugshape_export::processStackAction(*player, data, actionIndex, 144);
                },
                ::ItemStackRequestCereal::toActionData(*actionPtr)
            );
        }
        ++actionIndex;
    }
}

static ll::memory::HookRegistrar<TradeAuthInputHook> gTradeAuthInputHookRegistrar;

// ── 容器关闭钩子: 客户端关掉交易界面 ──
// 对齐参考实现 GMLIB 的 ClosedRequestHook: 由它发出"关闭哨兵"回调（{slot=-1} + amount=-1）,
// 调用方在同一个回调里收尾, 不需要单独接一个关闭事件。
LL_TYPE_INSTANCE_HOOK(
    TradeContainerCloseHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::$handle,
    void,
    NetworkIdentifier const&     source,
    ContainerClosePacket const&  packet
) {
    origin(source, packet);

    auto* player = debugshape_export::findPlayerByNetworkIdImpl(source);
    if (player == nullptr) return;
    auto const containerId = static_cast<int>(packet.mContainerId);
    if (debugshape_export::ContainerMenuManager::getInstance().handleContainerClose(
            player->getRealName(),
            containerId
        )) {
        return;
    }
    auto& mgr = debugshape_export::TradeMenuManager::getInstance();
    if (!mgr.hasMenuFor(player->getRealName())) return;
    mgr.handleContainerClose(player->getRealName(), containerId);
}

static ll::memory::HookRegistrar<TradeContainerCloseHook> gTradeContainerCloseHookRegistrar;

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
    uint64_t addClickListener(std::function<void(hologramlib::TradeClickEvent const&)> listener) override {
        return TradeMenuManager::getInstance().addClickListener(std::move(listener));
    }
    bool removeClickListener(uint64_t token) override {
        return TradeMenuManager::getInstance().removeClickListener(token);
    }
    uint64_t addActionListener(hologramlib::TradeActionCallback listener) override {
        return TradeMenuManager::getInstance().addActionListener(std::move(listener));
    }
    bool removeActionListener(uint64_t token) override {
        return TradeMenuManager::getInstance().removeActionListener(token);
    }
    uint64_t addRawActionListener(hologramlib::TradeRawActionCallback listener) override {
        return TradeMenuManager::getInstance().addRawActionListener(std::move(listener));
    }
    bool removeRawActionListener(uint64_t token) override {
        return TradeMenuManager::getInstance().removeRawActionListener(token);
    }
};
} // namespace

hologramlib::ITradeMenu& tradeMenuAdapter() {
    static TradeMenuAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
