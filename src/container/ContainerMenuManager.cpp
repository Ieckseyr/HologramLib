// ContainerMenuManager.cpp - 虚拟容器（列表）界面实现
//
// 方案逐条复刻参考实现 GMLIB 的 ChestUI（src-shared/gmlib/gm/ui/ChestUI.cpp）:
//   updateBlock      → UpdateBlockPacket(mRuntimeId = 箱子方块的 Block::mNetworkId, flag = 3)
//   updateBlockActor → BlockActorDataPacket(id=Chest / CustomName / x,y,z / pair* / Items)
//   ContainerOpen    → 容器类型 0(Container) + 方块坐标 + 目标实体 -1（GMLIB 写死 114 号容器 id）
//   close            → 把真方块改回去 + ContainerClose
// 差别: GMLIB 走 BDS 的 updateClientBlock/updateClientBlockActor（内部就是这两个包）, 这里直接
// 手写包; 块信息从 BDS 只读查询（方块的网络 id / 该位置的真实方块）, 不调用任何发送型 API。
#include "container/ContainerMenuManager.h"
#include "item/MenuItemStack.h"

#include "DiagLog.h"

#include "container/ContainerPackets.h" // 三个包 + 箱子方块实体 NBT（与离线 fixture 共用同一份）
#include "customentity/CustomEntityManager.h"

#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/chrono/GameChrono.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/thread/ServerThreadExecutor.h>

#include <mc/deps/core/string/HashedString.h>
#include <mc/network/Compressibility.h>
#include <mc/network/NetworkPeer.h>
#include <mc/network/NetworkSystem.h>
#include <mc/network/packet/InventorySlotPacket.h>
#include <mc/network/packet/InventorySlotPacketPayload.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/BlockPos.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/block/Block.h>
#include <mc/world/level/block/VanillaBlockTypeIds.h>
#include <mc/world/level/block/registry/BlockTypeRegistry.h>
#include <mc/world/level/dimension/Dimension.h>
#include <mc/world/level/dimension/DimensionHeightRange.h>

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerID.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerType.hpp>
#include <sculk/protocol/codec/level/block/BlockPos.hpp>
#include <sculk/protocol/codec/nbt/CompoundTag.hpp>
#include <sculk/protocol/codec/nbt/ListTag.hpp>
#include <sculk/protocol/codec/nbt/TagType.hpp>
#include <sculk/protocol/codec/nbt/TagVariant.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>
#include <sculk/protocol/codec/packet/BlockActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/ContainerClosePacket.hpp>
#include <sculk/protocol/codec/packet/ContainerOpenPacket.hpp>
#include <sculk/protocol/codec/packet/RemoveActorPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateBlockPacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <algorithm>
#include <atomic>
#include <format>
#include <memory>

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

// 与交易域同款: 自己拼字节 + 经 NetworkPeer 发, 不做 BDS 回读硬校验
template <typename PacketT>
bool sendPacket(Player& player, PacketT const& packet) {
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

// 箱子方块的**网络 id**（UpdateBlockPacket 要的是它, 不是方块名）
std::uint32_t chestBlockNetworkId() {
    static std::uint32_t const cached = [] {
        auto&        registry = BlockTypeRegistry::mBlockTypeRegistry().mValue;
        ::Block const& chest  = registry.getDefaultBlockState(VanillaBlockTypeIds::Chest(), true);
        return static_cast<std::uint32_t>(chest.mNetworkId);
    }();
    return cached;
}

// 该位置真实方块的网络 id（关闭时用来把客户端那一格改回去）
std::uint32_t realBlockNetworkId(Player& player, int x, int y, int z) {
    auto const& block = player.getDimensionBlockSource().getBlock(::BlockPos{x, y, z});
    return static_cast<std::uint32_t>(block.mNetworkId);
}

bool isAirAt(Player& player, int x, int y, int z) {
    return player.getDimensionBlockSource().getBlock(::BlockPos{x, y, z}).isAir();
}

// 客户端侧方块位置: GMLIB 用"脚上方 5 格"（超出世界高度则下移 4 格）; 这里再优先挑空气位,
// 免得把真实方块/方块实体盖掉（合成箱子只是给客户端看的, 位置本身无所谓）。
void pickCarrierPos(Player& player, int& x, int& y, int& z) {
    auto const feet = player.getFeetBlockPos();
    x               = feet.x;
    z               = feet.z;

    // 世界高度范围（GMLIB 也是这么取的: TypedStorage 对 DimensionHeightRange 走 operator->）
    auto const&  range = player.getDimension().mHeightRange;
    short const  maxY  = static_cast<short>(range->mMax);
    short const  minY  = static_cast<short>(range->mMin);
    int const preferred = (feet.y + 5 > static_cast<int>(maxY) - 1) ? feet.y - 4 : feet.y + 5;

    int const candidates[] = {preferred, preferred + 1, preferred - 1, preferred + 2, preferred - 2};
    for (int const candidate : candidates) {
        if (candidate < static_cast<int>(minY) + 1 || candidate > static_cast<int>(maxY) - 1) continue;
        if (isAirAt(player, x, candidate, z)) {
            y = candidate;
            return;
        }
    }
    y = preferred; // 全是实体方块: 仍然开在 GMLIB 的位置上
}

// SNBT 字符串转义（只转义引号与反斜杠）。反斜杠用字符码 92 写, 源码里不出现转义层。
// ── 旧路径（useMinecart=true）专用: 合成一只只发给该玩家的箱子矿车 ──
std::atomic<std::int64_t>  gNextCarrierUid{-860000000000LL};
std::atomic<std::uint64_t> gNextCarrierRid{0x6E300000ULL};

std::uint64_t nextCarrierUniqueId() { return static_cast<std::uint64_t>(gNextCarrierUid.fetch_sub(1)); }
std::uint64_t nextCarrierRuntimeId() { return gNextCarrierRid.fetch_add(1); }

} // namespace

ContainerMenuManager& ContainerMenuManager::getInstance() {
    static ContainerMenuManager instance;
    return instance;
}

int ContainerMenuManager::allocateContainerId() {
    // 显示区间 101..199 循环; 同时占用则顺延, 全占则失败
    for (int i = 0; i < 99; ++i) {
        int const candidate = mNextContainerId;
        mNextContainerId    = (mNextContainerId >= 199) ? 101 : (mNextContainerId + 1);
        bool used = false;
        for (auto const& [id, menu] : mMenus) {
            if (menu.containerId == candidate) {
                used = true;
                break;
            }
        }
        if (!used) return candidate;
    }
    return 0;
}

// ── 步骤 1+2: 客户端侧箱子方块 + 方块实体 ──
void ContainerMenuManager::sendChestBlocks(Player& player, Menu const& menu) {
    auto const runtimeId = chestBlockNetworkId();

    auto sendOne = [&](int x, int y, int z, bool secondHalf) {
        sendPacket(player, container::makeUpdateBlock(x, y, z, runtimeId));
        sendPacket(
            player,
            container::makeBlockActorData(
                x,
                y,
                z,
                container::buildChestNbt(x, y, z, secondHalf, menu.spec.title, menu.spec.rows, menu.spec.items)
            )
        );
    };

    sendOne(menu.posX, menu.posY, menu.posZ, false);
    if (menu.isBig()) sendOne(menu.posX + 1, menu.posY, menu.posZ, true);
}

// ── 步骤 3: ContainerOpen（类型 0 = Container, 绑方块坐标, 目标实体 -1）──
void ContainerMenuManager::sendContainerOpen(Player& player, Menu const& menu) {
    sendPacket(player, container::makeContainerOpen(menu.containerId, menu.posX, menu.posY, menu.posZ));
}

// ── 关闭: 把那一格（大箱子是两格）改回真方块 ──
void ContainerMenuManager::restoreRealBlocks(Player& player, Menu const& menu) {
    auto sendOne = [&](int x, int y, int z) {
        sendPacket(player, container::makeUpdateBlock(x, y, z, realBlockNetworkId(player, x, y, z)));
    };
    sendOne(menu.posX, menu.posY, menu.posZ);
    if (menu.isBig()) sendOne(menu.posX + 1, menu.posY, menu.posZ);
}

// ── 旧路径: 合成箱子矿车 + ContainerOpen(MinecartChest) ──
void ContainerMenuManager::sendMinecartCarrier(Player& player, Menu const& menu) {
    auto const uid = menu.carrierUniqueId;
    auto const rid = menu.carrierRuntimeId;

    sculk::protocol::AddActorPacket spawn;
    spawn.mActorUniqueId  = static_cast<std::int64_t>(uid);
    spawn.mActorRuntimeId = rid;
    spawn.mIdentifier     = "minecraft:chest_minecart";
    spawn.mPosition       = sculk::protocol::Vec3{
        static_cast<float>(menu.posX),
        static_cast<float>(menu.posY),
        static_cast<float>(menu.posZ)
    };
    spawn.mVelocity       = sculk::protocol::Vec3{0.0f, 0.0f, 0.0f};
    spawn.mRotation       = sculk::protocol::Vec2{0.0f, 0.0f};
    spawn.mYHeadRotation  = 0.0f;
    spawn.mYBodyRotation  = 0.0f;
    auto& meta = spawn.mMetaData.mDataItems;
    meta.push_back({sculk::protocol::ActorDataIDs::Name, std::string(menu.spec.title)});
    meta.push_back({sculk::protocol::ActorDataIDs::Reserved0, static_cast<std::int64_t>(1) << 5});
    sendPacket(player, spawn);

    sculk::protocol::ContainerOpenPacket open;
    open.mContainerId   = static_cast<sculk::protocol::ContainerID>(menu.containerId);
    open.mContainerType = sculk::protocol::ContainerType::MinecartChest;
    open.mPosition      = sculk::protocol::BlockPos{menu.posX, menu.posY, menu.posZ};
    open.mTargetActorId = static_cast<std::int64_t>(uid);
    sendPacket(player, open);
}

bool ContainerMenuManager::snapshotMenu(int64_t menuId, Menu& out) const {
    std::lock_guard lock(mMutex);
    auto            it = mMenus.find(menuId);
    if (it == mMenus.end()) return false;
    out = it->second;
    return true;
}

void ContainerMenuManager::laterSendBlocks(std::string playerName, int64_t menuId) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return;
    Menu copy;
    if (!snapshotMenu(menuId, copy)) return; // 已关闭
    sendChestBlocks(*player, copy);
}

void ContainerMenuManager::laterSendOpen(std::string playerName, int64_t menuId) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return;
    Menu copy;
    if (!snapshotMenu(menuId, copy)) return;
    sendContainerOpen(*player, copy);
}

void ContainerMenuManager::laterMinecartOpen(std::string playerName, int64_t menuId) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return;
    Menu copy;
    if (!snapshotMenu(menuId, copy)) return;
    sendMinecartCarrier(*player, copy);
}

int64_t ContainerMenuManager::open(std::string const& playerName, hologramlib::ContainerMenuSpec const& spec) {
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) {
        HLIB_LOG_WARN("[ContainerMenu] 打开失败: 玩家 {} 不在线", playerName);
        return -1;
    }

    int64_t previous = -1;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mByPlayer.find(playerName); it != mByPlayer.end()) previous = it->second;
    }
    if (previous >= 0) close(previous);

    Menu menu;
    menu.playerName = playerName;
    menu.spec       = spec;
    pickCarrierPos(*player, menu.posX, menu.posY, menu.posZ);
    if (spec.useMinecart) {
        menu.posY            = static_cast<int>(player->getPosition().y) + 5;
        menu.carrierUniqueId = nextCarrierUniqueId();
        menu.carrierRuntimeId = nextCarrierRuntimeId();
    }
    {
        std::lock_guard lock(mMutex);
        menu.menuId      = mNextMenuId++;
        menu.containerId = allocateContainerId();
        if (menu.containerId == 0) {
            HLIB_LOG_WARN("[ContainerMenu] 容器 id 用尽（玩家 {}）", playerName);
            return -1;
        }
        mMenus.emplace(menu.menuId, menu);
        mByPlayer[playerName] = menu.menuId;
    }

    // 节奏与参考实现一致: ① 先摆方块 + 方块实体（客户端要先知道有这个容器）→ ② 等 10 tick
    // 再发 ContainerOpen。背靠背一次发完客户端会打不开界面。
    std::string const playerNameCopy = playerName;
    int64_t const     menuIdCopy     = menu.menuId;
    auto&             executor       = ll::thread::ServerThreadExecutor::getDefault();
    if (spec.useMinecart) {
        executor.execute([playerNameCopy, menuIdCopy]() {
            ContainerMenuManager::getInstance().laterMinecartOpen(playerNameCopy, menuIdCopy);
        });
    } else {
        executor.execute([playerNameCopy, menuIdCopy]() {
            ContainerMenuManager::getInstance().laterSendBlocks(playerNameCopy, menuIdCopy);
        });
        executor.executeAfter(
            [playerNameCopy, menuIdCopy]() {
                ContainerMenuManager::getInstance().laterSendOpen(playerNameCopy, menuIdCopy);
            },
            ll::chrono::game::ticks(std::max(1, spec.openDelayTicks))
        );
    }
    HLIB_LOG_INFO(
        "[ContainerMenu] 已打开: player={} menuId={} containerId={} 行数={} 条目={} 方块=({},{},{}) 路径={}",
        playerName,
        menu.menuId,
        menu.containerId,
        spec.rows,
        spec.items.size(),
        menu.posX,
        menu.posY,
        menu.posZ,
        spec.useMinecart ? "矿车(旧)" : "方块(GMLIB)"
    );
    return menu.menuId;
}

// ── 就地换内容: 不拆界面 ──
// 翻页/进子菜单/返回都走这条: 载体方块还在客户端那边, 所以只把新的方块实体 NBT 重发一次
// （Items/标题就在里面）, 再把 ContainerOpen 重发一次让客户端重新读这个容器。
// 中间**不等待**: 没有"先摆方块再等它生效"这一步。
void ContainerMenuManager::resendBlockActor(Player& player, Menu const& menu) {
    auto const title = menu.spec.title;
    sendPacket(
        player,
        container::makeBlockActorData(
            menu.posX,
            menu.posY,
            menu.posZ,
            container::buildChestNbt(menu.posX, menu.posY, menu.posZ, false, title, menu.spec.rows, menu.spec.items)
        )
    );
    if (menu.isBig()) {
        sendPacket(
            player,
            container::makeBlockActorData(
                menu.posX + 1,
                menu.posY,
                menu.posZ,
                container::buildChestNbt(menu.posX + 1, menu.posY, menu.posZ, true, title, menu.spec.rows, menu.spec.items)
            )
        );
    }
}

bool ContainerMenuManager::update(int64_t menuId, hologramlib::ContainerMenuSpec const& spec) {
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

    resendBlockActor(*player, copy);                        // 新内容
    sendPacket(*player, container::makeContainerOpen(copy.containerId, copy.posX, copy.posY, copy.posZ)); // 让客户端重读
    HLIB_LOG_INFO(
        "[ContainerMenu] 就地换内容: player={} menuId={} 条目={} rows={}",
        copy.playerName,
        menuId,
        spec.items.size(),
        spec.rows
    );
    return true;
}

// 按槽刷新: 一条 InventorySlot 换掉一格（真实箱子同步内容用的就是这种包）——无延迟、无闪烁
bool ContainerMenuManager::setItem(int64_t menuId, int slot, hologramlib::ContainerMenuItem const& item) {
    Menu  copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return false;
        if (slot < 0 || slot >= it->second.slotCount()) return false;
        if (slot < static_cast<int>(it->second.spec.items.size())) it->second.spec.items[slot] = item;
        copy = it->second;
    }
    auto* player = findPlayerByName(copy.playerName);
    if (player == nullptr) return false;

    auto stack = item::makeMenuItemStack(item);
    ::InventorySlotPacket packet{::InventorySlotPacketPayload(
        static_cast<::ContainerID>(copy.containerId),
        static_cast<uint>(slot),
        stack
    )};
    player->sendNetworkPacket(packet);
    HLIB_LOG_INFO("[ContainerMenu] 按槽刷新: player={} menuId={} slot={} 物品={}", copy.playerName, menuId, slot, item.type);
    return true;
}

bool ContainerMenuManager::setTitle(int64_t menuId, std::string const& title) {
    hologramlib::ContainerMenuSpec spec;
    std::string                  playerName;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return false;
        it->second.spec.title = title;
        spec                  = it->second.spec;
        playerName            = it->second.playerName;
    }
    auto* player = findPlayerByName(playerName);
    if (player == nullptr) return false;
    // 复用 update(): 重发方块实体 NBT（新标题 + 现有条目）+ ContainerOpen 让客户端重读
    return update(menuId, spec);
}

bool ContainerMenuManager::close(int64_t menuId) {
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        auto            it = mMenus.find(menuId);
        if (it == mMenus.end()) return false;
        copy = it->second;
        mMenus.erase(it);
        if (auto byPlayer = mByPlayer.find(copy.playerName);
            byPlayer != mByPlayer.end() && byPlayer->second == menuId) {
            mByPlayer.erase(byPlayer);
        }
    }
    if (auto* player = findPlayerByName(copy.playerName)) {
        sendPacket(*player, container::makeContainerClose(copy.containerId));
        if (copy.spec.useMinecart) {
            // 协议层合成实体: 关界面即让客户端删掉它
            sculk::protocol::RemoveActorPacket remove;
            remove.mActorUniqueId = static_cast<std::int64_t>(copy.carrierUniqueId);
            sendPacket(*player, remove);
        } else {
            restoreRealBlocks(*player, copy); // 把客户端那一格改回真实方块
        }
    }
    HLIB_LOG_INFO("[ContainerMenu] 已关闭: player={} menuId={}", copy.playerName, menuId);
    return true;
}

void ContainerMenuManager::closeAll() {
    std::vector<int64_t> ids;
    {
        std::lock_guard lock(mMutex);
        for (auto const& [id, menu] : mMenus) ids.push_back(id);
    }
    for (auto const id : ids) close(id);
}

bool ContainerMenuManager::isOpen(int64_t menuId) const {
    std::lock_guard lock(mMutex);
    return mMenus.contains(menuId);
}

std::vector<int64_t> ContainerMenuManager::getAllIds() const {
    std::lock_guard      lock(mMutex);
    std::vector<int64_t> out;
    out.reserve(mMenus.size());
    for (auto const& [id, menu] : mMenus) out.push_back(id);
    return out;
}

uint64_t ContainerMenuManager::addClickListener(
    std::function<void(hologramlib::ContainerClickEvent const&)> listener
) {
    if (!listener) return 0;
    std::lock_guard lock(mMutex);
    auto const      token = mNextListenerToken++;
    mListeners.emplace(token, std::move(listener));
    return token;
}

bool ContainerMenuManager::removeClickListener(uint64_t token) {
    std::lock_guard lock(mMutex);
    return mListeners.erase(token) > 0;
}

void ContainerMenuManager::dispatch(hologramlib::ContainerClickEvent const& event) {
    std::vector<std::function<void(hologramlib::ContainerClickEvent const&)>> snapshot;
    {
        std::lock_guard lock(mMutex);
        // LSE 侧拿不到 C++ 监听器 → 同一份事件也进轮询队列（containerPollClicks 取走并清空）
        mClickQueue.push_back(event);
        snapshot.reserve(mListeners.size());
        for (auto const& [token, fn] : mListeners) snapshot.push_back(fn);
    }
    for (auto& fn : snapshot) {
        if (fn) fn(event);
    }
}

std::vector<hologramlib::ContainerClickEvent> ContainerMenuManager::pollClicks() {
    std::vector<hologramlib::ContainerClickEvent> out;
    std::lock_guard                               lock(mMutex);
    out.reserve(mClickQueue.size());
    while (!mClickQueue.empty()) {
        out.push_back(std::move(mClickQueue.front()));
        mClickQueue.pop_front();
    }
    return out;
}

void ContainerMenuManager::clearClicks() {
    std::lock_guard lock(mMutex);
    mClickQueue.clear();
}

// LSE 轮询条目格式（脚本按空格切即可）:
//   "player=Steve menuId=2 slot=31 closed=0"
std::string ContainerMenuManager::formatClick(hologramlib::ContainerClickEvent const& event) {
    return std::format(
        "player={} menuId={} slot={} closed={}",
        event.playerName,
        event.menuId,
        event.slot,
        event.closed ? 1 : 0
    );
}

bool ContainerMenuManager::hasMenuFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    return mByPlayer.contains(playerName);
}

int64_t ContainerMenuManager::menuIdFor(std::string const& playerName) const {
    std::lock_guard lock(mMutex);
    auto            it = mByPlayer.find(playerName);
    return it == mByPlayer.end() ? -1 : it->second;
}

bool ContainerMenuManager::handleSlotAction(
    std::string const& playerName,
    int                containerEnum,
    int                containerId,
    int                slot
) {
    // 命中判定（宽松优先; 参考实现 GMLIB 干脆完全不筛 —— 只看"这个玩家有没有开着的箱子"）:
    //   ① 容器枚举是 LevelEntityContainer(7) → 箱子式界面的槽位就在这个枚举下; 或
    //   ② 动态容器 id 落在本域的显示区间 101..199 —— 该区间是刻意避开的（真实容器与 BDS
    //      交易走 1..100）, 所以命中 101..199 必然是我们这个容器。
    // 为什么不硬性要求 id 等于本菜单的 id: 客户端对"块容器"可能报 0 或别的值, 硬要求会让
    // 点击**全部漏掉**（实测症状: 箱子界面开着, 点条目什么回调都没有）。槽位号仍要落在本容器
    // 容量内, 用来挡住误命中。
    if (containerEnum != 7 && !(containerId >= 101 && containerId <= 199)) return false;

    hologramlib::ContainerClickEvent event;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end()) return false;
        auto const& menu = menuIt->second;

        if (slot < 0 || slot >= menu.slotCount()) return false; // 越界（不是本容器的格子）

        event.playerName = playerName;
        event.menuId     = menu.menuId;
        event.slot       = slot;
    }
    dispatch(event);
    return true;
}

bool ContainerMenuManager::handleContainerClose(std::string const& playerName, int containerId) {
    hologramlib::ContainerClickEvent event;
    Menu                             copy;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end()) return false;
        auto menuIt = mMenus.find(byPlayer->second);
        if (menuIt == mMenus.end()) return false;
        if (menuIt->second.containerId != containerId) return false; // 不是本域的容器
        copy             = menuIt->second;
        event.playerName = playerName;
        event.menuId     = copy.menuId;
        event.closed     = true;
        mMenus.erase(menuIt);
        mByPlayer.erase(byPlayer);
    }
    dispatch(event);
    // 客户端已经自己关掉了界面, 但客户端侧那个假箱子方块**还在**（是我们发的 UpdateBlock）。
    // 这里不能走 close()（记录已摘掉）, 所以单独收拾客户端那边:
    //   方块路径 → 把真方块改回去; 矿车路径 → 让客户端删掉合成实体。
    if (auto* player = findPlayerByName(playerName)) {
        if (copy.spec.useMinecart) {
            sculk::protocol::RemoveActorPacket remove;
            remove.mActorUniqueId = static_cast<std::int64_t>(copy.carrierUniqueId);
            sendPacket(*player, remove);
        } else {
            restoreRealBlocks(*player, copy);
        }
    }
    return true;
}

void ContainerMenuManager::onPlayerLeave(std::string const& playerName) {
    int64_t menuId = -1;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mByPlayer.find(playerName); it != mByPlayer.end()) menuId = it->second;
    }
    if (menuId >= 0) close(menuId);
}

void ContainerMenuManager::shutdown() {
    closeAll();
    std::lock_guard lock(mMutex);
    mMenus.clear();
    mByPlayer.clear();
}

namespace {
class ContainerMenuAdapter final : public hologramlib::IContainerMenu {
public:
    int64_t open(std::string const& playerName, hologramlib::ContainerMenuSpec const& spec) override {
        return ContainerMenuManager::getInstance().open(playerName, spec);
    }
    bool update(int64_t menuId, hologramlib::ContainerMenuSpec const& spec) override {
        return ContainerMenuManager::getInstance().update(menuId, spec);
    }
    bool setItem(int64_t menuId, int slot, hologramlib::ContainerMenuItem const& item) override {
        return ContainerMenuManager::getInstance().setItem(menuId, slot, item);
    }
    bool close(int64_t menuId) override { return ContainerMenuManager::getInstance().close(menuId); }
    void closeAll() override { ContainerMenuManager::getInstance().closeAll(); }
    [[nodiscard]] bool isOpen(int64_t menuId) const override {
        return ContainerMenuManager::getInstance().isOpen(menuId);
    }
    [[nodiscard]] std::vector<int64_t> getAllIds() const override {
        return ContainerMenuManager::getInstance().getAllIds();
    }
    uint64_t addClickListener(std::function<void(hologramlib::ContainerClickEvent const&)> listener) override {
        return ContainerMenuManager::getInstance().addClickListener(std::move(listener));
    }
    bool removeClickListener(uint64_t token) override {
        return ContainerMenuManager::getInstance().removeClickListener(token);
    }
};
} // namespace

hologramlib::IContainerMenu& containerMenuAdapter() {
    static ContainerMenuAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
