#include "core/DiagFlags.h"

#include "container/ContainerInternal.h"
#include "container/ContainerMenuManager.h"
#include "item/MenuItemStack.h"

#include "DiagLog.h"
#include "kernel/EventIds.h"
#include "kernel/PacketSink.h"

#include "container/ContainerPackets.h"
#include "core/ClickEvent.h"
#include "customentity/CustomEntityManager.h"
#include <mc/network/packet/BlockEventPacket.h>
#include <mc/network/packet/BlockEventPacketPayload.h>

#include <ll/api/chrono/GameChrono.h>
#include <ll/api/io/LoggerRegistry.h>
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
#include <mc/world/level/BlockPos.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/block/Block.h>
#include <mc/world/level/block/VanillaBlockTypeIds.h>
#include <mc/world/level/block/registry/BlockTypeRegistry.h>
#include <mc/world/level/dimension/Dimension.h>
#include <mc/world/level/dimension/DimensionHeightRange.h>

#include "core/proto/Version.h"
#include "MetaWire.h"

#include <algorithm>
#include <atomic>
#include <format>
#include <memory>

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

namespace {

std::uint32_t chestBlockNetworkId() {
    static std::uint32_t const cached = [] {
        auto&          registry = BlockTypeRegistry::mBlockTypeRegistry().mValue;
        ::Block const& chest    = registry.getDefaultBlockState(VanillaBlockTypeIds::Chest(), true);
        return static_cast<std::uint32_t>(chest.mNetworkId);
    }();
    return cached;
}

std::uint32_t realBlockNetworkId(Player& player, int x, int y, int z) {
    auto const& block = player.getDimensionBlockSource().getBlock(::BlockPos{x, y, z});
    return static_cast<std::uint32_t>(block.mNetworkId);
}

bool isAirAt(Player& player, int x, int y, int z) {
    return player.getDimensionBlockSource().getBlock(::BlockPos{x, y, z}).isAir();
}

void pickCarrierPos(Player& player, int& x, int& y, int& z) {
    auto const feet = player.getFeetBlockPos();
    x               = feet.x;
    z               = feet.z;

    auto const& range     = player.getDimension().mHeightRange;
    short const maxY      = static_cast<short>(range->mMax);
    short const minY      = static_cast<short>(range->mMin);
    int const   preferred = (feet.y + 5 > static_cast<int>(maxY) - 1) ? feet.y - 4 : feet.y + 5;

    int const candidates[] = {preferred, preferred + 1, preferred - 1, preferred + 2, preferred - 2};
    for (int const candidate : candidates) {
        if (candidate < static_cast<int>(minY) + 1 || candidate > static_cast<int>(maxY) - 1)
            continue;
        if (isAirAt(player, x, candidate, z)) {
            y = candidate;
            return;
        }
    }
    y = preferred;
}

std::atomic<std::int64_t>  gNextCarrierUid{-860000000000LL};
std::atomic<std::uint64_t> gNextCarrierRid{0x6E300000ULL};

std::uint64_t nextCarrierUniqueId() {
    return static_cast<std::uint64_t>(gNextCarrierUid.fetch_sub(1));
}
std::uint64_t nextCarrierRuntimeId() {
    return gNextCarrierRid.fetch_add(1);
}

} // namespace

ContainerMenuManager& ContainerMenuManager::getInstance() {
    static ContainerMenuManager instance;
    return instance;
}

int ContainerMenuManager::allocateContainerId() {
    auto const now = container_util::nowTick();

    for (auto it = mIdFreedTick.begin(); it != mIdFreedTick.end();) {
        if (now - it->second > 200)
            it = mIdFreedTick.erase(it);
        else
            ++it;
    }

    for (int i = 0; i < 99; ++i) {
        int const candidate = mNextContainerId;
        mNextContainerId    = (mNextContainerId >= 199) ? 101 : (mNextContainerId + 1);
        bool used           = mIdFreedTick.contains(candidate);
        if (!used) {
            mMenus.forEachLocked([&](int64_t, Menu const& menu) {
                if (menu.containerId == candidate)
                    used = true;
            });
        }
        if (!used)
            return candidate;
    }
    return 0;
}

void ContainerMenuManager::sendChestBlocks(Player& player, Menu const& menu) {
    auto const runtimeId = chestBlockNetworkId();

    auto sendOne = [&](int x, int y, int z, bool secondHalf) {
        container_util::sendPacket(player, container::makeUpdateBlock(x, y, z, runtimeId));
        container_util::sendPacket(
            player,
            container::makeBlockActorData(
                x,
                y,
                z,
                container::buildChestNbt(x, y, z, secondHalf, menu.spec.title, menu.spec.rows, menu.spec.items, menu.enchantsBySlot)));
    };

    sendOne(menu.posX, menu.posY, menu.posZ, false);
    if (menu.isBig())
        sendOne(menu.posX + 1, menu.posY, menu.posZ, true);
}

void ContainerMenuManager::sendContainerOpen(Player& player, Menu const& menu) {
    container_util::sendPacket(player, container::makeContainerOpen(menu.containerId, menu.posX, menu.posY, menu.posZ));
}

void ContainerMenuManager::restoreRealBlocks(Player& player, Menu const& menu) {
    auto sendOne = [&](int x, int y, int z) {
        container_util::sendPacket(player, container::makeUpdateBlock(x, y, z, realBlockNetworkId(player, x, y, z)));
    };
    sendOne(menu.posX, menu.posY, menu.posZ);
    if (menu.isBig())
        sendOne(menu.posX + 1, menu.posY, menu.posZ);
}

void ContainerMenuManager::sendMinecartCarrier(Player& player, Menu const& menu) {
    auto const uid = menu.carrierUniqueId;
    auto const rid = menu.carrierRuntimeId;

    using ids = proto::ActorDataIDs;

    proto::AddActorPacket spawn;
    spawn.entity_id        = proto::ActorUniqueID{static_cast<std::int64_t>(uid)};
    spawn.runtime_id       = proto::bp::ActorRuntimeID{rid};
    spawn.actor_type       = "minecraft:chest_minecart";
    spawn.pos              = proto::Vec3{
        static_cast<float>(menu.posX),
        static_cast<float>(menu.posY),
        static_cast<float>(menu.posZ)};
    spawn.velocity         = proto::Vec3{0.0f, 0.0f, 0.0f};
    spawn.rot              = proto::Vec2{0.0f, 0.0f};
    spawn.y_head_rotation  = 0.0f;
    spawn.y_body_rotation  = 0.0f;
    auto& meta             = spawn.data.data;
    meta.push_back(meta_wire::string(ids::Name, std::string(menu.spec.title)));
    meta.push_back(meta_wire::int64(ids::Reserved0, static_cast<std::int64_t>(1) << 5));
    container_util::sendPacket(player, spawn);

    proto::ContainerOpenPacket open;
    open.container_id      = static_cast<proto::ContainerIDType>(menu.containerId);
    open.type              = proto::bp::ContainerType::MinecartChest;
    open.pos               = proto::BlockPos{menu.posX, menu.posY, menu.posZ};
    open.entity_unique_id  = proto::ActorUniqueID{static_cast<std::int64_t>(uid)};
    container_util::sendPacket(player, open);
}

bool ContainerMenuManager::snapshotMenu(int64_t menuId, Menu& out) const {
    std::lock_guard lock(mMutex);
    return mMenus.withObject(menuId, [&](Menu const& menu) {
        out = menu;
    });
}

bool ContainerMenuManager::isCurrentEpoch(Menu const& menu) const {
    std::lock_guard lock(mMutex);
    auto const      it = mPlayerEpoch.find(menu.playerName);
    return it != mPlayerEpoch.end() && it->second == menu.ownerEpoch;
}

void ContainerMenuManager::laterSendBlocks(std::string playerName, int64_t menuId) {
    auto* player = container_util::findPlayerByName(playerName);
    if (player == nullptr)
        return;
    Menu copy;
    if (!snapshotMenu(menuId, copy))
        return;
    if (!isCurrentEpoch(copy))
        return;
    sendChestBlocks(*player, copy);
}

void ContainerMenuManager::laterSendOpen(std::string playerName, int64_t menuId) {
    auto* player = container_util::findPlayerByName(playerName);
    if (player == nullptr)
        return;
    Menu copy;
    if (!snapshotMenu(menuId, copy))
        return;
    if (!isCurrentEpoch(copy))
        return;
    sendContainerOpen(*player, copy);
    std::lock_guard lock(mMutex);
    mMenus.withObject(menuId, [&](Menu& menu) {
        menu.uiOpen = true;
    });
}

void ContainerMenuManager::laterMinecartOpen(std::string playerName, int64_t menuId) {
    auto* player = container_util::findPlayerByName(playerName);
    if (player == nullptr)
        return;
    Menu copy;
    if (!snapshotMenu(menuId, copy))
        return;
    if (!isCurrentEpoch(copy))
        return;
    sendMinecartCarrier(*player, copy);
    std::lock_guard lock(mMutex);
    mMenus.withObject(menuId, [&](Menu& menu) {
        menu.uiOpen = true;
    });
}

int64_t ContainerMenuManager::open(std::string const& playerName, hologramlib::ContainerMenuSpec const& spec) {
    if (hlibDomainDisabled("HLIB_NO_CONTAINER"))
        return -1;
    auto* player = container_util::findPlayerByName(playerName);
    if (player == nullptr) {
        HLIB_LOG_WARN("[ContainerMenu] 打开失败: 玩家 {} 不在线", playerName);
        return -1;
    }

    int64_t previous = -1;
    {
        std::lock_guard lock(mMutex);
        if (auto it = mByPlayer.find(playerName); it != mByPlayer.end())
            previous = it->second;
    }
    if (previous >= 0)
        close(previous);

    Menu menu;
    menu.playerName = playerName;
    menu.spec       = spec;
    pickCarrierPos(*player, menu.posX, menu.posY, menu.posZ);
    if (spec.useMinecart) {
        menu.posY             = static_cast<int>(player->getPosition().y) + 5;
        menu.carrierUniqueId  = nextCarrierUniqueId();
        menu.carrierRuntimeId = nextCarrierRuntimeId();
    }
    {
        std::lock_guard lock(mMutex);
        menu.containerId = allocateContainerId();
        menu.ownerEpoch  = ++mPlayerEpoch[playerName];
        if (menu.containerId == 0) {
            HLIB_LOG_WARN("[ContainerMenu] 容器 id 用尽（玩家 {}）", playerName);
            return -1;
        }
        int64_t const menuId = mMenus.insert(std::make_unique<Menu>(menu));
        menu.menuId          = menuId;

        mMenus.withObject(menuId, [&](Menu& stored) {
            stored.menuId = menuId;
        });
        mByPlayer[playerName] = menuId;
    }

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
            ll::chrono::game::ticks(std::max(1, spec.openDelayTicks)));
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
        spec.useMinecart ? "矿车(旧)" : "方块(GMLIB)");
    return menu.menuId;
}

void ContainerMenuManager::resendBlockActor(Player& player, Menu const& menu) {
    auto const title = menu.spec.title;
    container_util::sendPacket(
        player,
        container::makeBlockActorData(
            menu.posX,
            menu.posY,
            menu.posZ,
            container::buildChestNbt(menu.posX, menu.posY, menu.posZ, false, title, menu.spec.rows, menu.spec.items, menu.enchantsBySlot)));
    if (menu.isBig()) {
        container_util::sendPacket(
            player,
            container::makeBlockActorData(
                menu.posX + 1,
                menu.posY,
                menu.posZ,
                container::buildChestNbt(menu.posX + 1, menu.posY, menu.posZ, true, title, menu.spec.rows, menu.spec.items, menu.enchantsBySlot)));
    }
}

bool ContainerMenuManager::getSpec(int64_t menuId, hologramlib::ContainerMenuSpec& out) const {
    std::lock_guard lock(mMutex);
    return mMenus.withObject(menuId, [&](Menu const& menu) {
        out = menu.spec;
    });
}

bool ContainerMenuManager::update(int64_t menuId, hologramlib::ContainerMenuSpec const& spec) {
    if (hlibDomainDisabled("HLIB_NO_CONTAINER"))
        return false;
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        if (!mMenus.withObject(menuId, [&](Menu& menu) {
                menu.spec = spec;
                copy      = menu;
            })) {
            return false;
        }
    }
    auto* player = container_util::findPlayerByName(copy.playerName);
    if (player == nullptr)
        return false;
    if (!isCurrentEpoch(copy))
        return false;

    if (!copy.uiOpen)
        return true;

    resendBlockActor(*player, copy);

    int const slots = std::min<int>(copy.slotCount(), static_cast<int>(copy.spec.items.size()));
    for (int slot = 0; slot < slots; ++slot) {
        auto                  stack = item::makeMenuItemStack(copy.spec.items[slot]);
        ::InventorySlotPacket packet{::InventorySlotPacketPayload(
            static_cast<::ContainerID>(copy.containerId),
            static_cast<uint>(slot),
            stack)};
        (void)wire::send(*player, packet);
    }
    HLIB_LOG_INFO(
        "[ContainerMenu] 就地换内容(仅开着的UI): player={} menuId={} 条目={} rows={}",
        copy.playerName,
        menuId,
        spec.items.size(),
        spec.rows);
    return true;
}

bool ContainerMenuManager::setItem(int64_t menuId, int slot, hologramlib::ContainerMenuItem const& item) {
    return setItemEnchanted(menuId, slot, item, {});
}

bool ContainerMenuManager::setTitle(int64_t menuId, std::string const& title) {
    hologramlib::ContainerMenuSpec spec;
    std::string                    playerName;
    {
        std::lock_guard lock(mMutex);
        if (!mMenus.withObject(menuId, [&](Menu& menu) {
                menu.spec.title = title;
                spec            = menu.spec;
                playerName      = menu.playerName;
            })) {
            return false;
        }
    }
    auto* player = container_util::findPlayerByName(playerName);
    if (player == nullptr)
        return false;

    return update(menuId, spec);
}

bool ContainerMenuManager::setInteractive(int64_t menuId, bool on) {
    std::lock_guard lock(mMutex);
    return mMenus.withObject(menuId, [&](Menu& menu) {
        menu.spec.interactive = on;
    });
}

bool ContainerMenuManager::close(int64_t menuId) {
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        if (!mMenus.withObject(menuId, [&](Menu const& menu) {
                copy = menu;
            }))
            return false;
        mMenus.erase(menuId);
        mIdFreedTick[copy.containerId] = container_util::nowTick();
        if (auto byPlayer = mByPlayer.find(copy.playerName);
            byPlayer != mByPlayer.end() && byPlayer->second == menuId) {
            mByPlayer.erase(byPlayer);
        }
    }
    if (auto* player = container_util::findPlayerByName(copy.playerName)) {
        container_util::sendPacket(*player, container::makeContainerClose(copy.containerId));
        if (copy.spec.useMinecart) {

            proto::RemoveActorPacket remove;
            remove.entity_id = proto::ActorUniqueID{static_cast<std::int64_t>(copy.carrierUniqueId)};
            container_util::sendPacket(*player, remove);
        } else {
            restoreRealBlocks(*player, copy);
        }
    }
    HLIB_LOG_INFO("[ContainerMenu] 已关闭: player={} menuId={}", copy.playerName, menuId);
    return true;
}

void ContainerMenuManager::closeAll() {
    std::vector<int64_t> ids;
    {
        std::lock_guard lock(mMutex);
        ids = mMenus.snapshotAll();
    }
    for (auto const id : ids) {
        close(id);
    }
}

bool ContainerMenuManager::blockEvent(int64_t menuId, int b0, int b1) {
    Menu        copy;
    std::string playerName;
    {
        std::lock_guard lock(mMutex);
        if (!mMenus.withObject(menuId, [&](Menu const& menu) {
                copy       = menu;
                playerName = menu.playerName;
            })) {
            return false;
        }
    }

    auto* player = container_util::findPlayerByName(playerName);
    if (player == nullptr)
        return false;

    ::BlockEventPacket packet;
    packet.mPos = ::BlockPos{copy.posX, copy.posY, copy.posZ};
    packet.mB0  = b0;
    packet.mB1  = b1;
    (void)wire::send(*player, packet);
    return true;
}

bool ContainerMenuManager::isOpen(int64_t menuId) const {
    std::lock_guard lock(mMutex);
    return mMenus.exists(menuId);
}

std::vector<int64_t> ContainerMenuManager::getAllIds() const {
    std::lock_guard lock(mMutex);
    return mMenus.snapshotAll();
}

uint64_t ContainerMenuManager::addClickListener(
    std::function<void(hologramlib::ContainerClickEvent const&)> listener) {
    if (!listener)
        return 0;
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

std::string ContainerMenuManager::formatClick(hologramlib::ContainerClickEvent const& event) {

    core::ClickEventLine line;
    line.player = event.playerName;
    line.menuId = event.menuId;
    line.slot   = event.slot;
    line.closed = event.closed;
    return core::formatClickLine(line);
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
    int                slot) {

    if (containerEnum != 7 && !(containerId >= 101 && containerId <= 199))
        return false;

    hologramlib::ContainerClickEvent event;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end())
            return false;
        bool const hit = mMenus.withObject(byPlayer->second, [&](Menu const& menu) -> bool {
            if (slot < 0 || slot >= static_cast<int>(menu.spec.items.size()))
                return false;

            event.playerName = playerName;
            event.menuId     = menu.menuId;
            event.slot       = slot;
            return true;
        });
        if (!hit)
            return false;
    }
    dispatch(event);
    return true;
}

void ContainerMenuManager::onPlayerLeave(std::string const& playerName) {
    int64_t menuId = -1;
    {
        std::lock_guard lock(mMutex);
        ++mPlayerEpoch[playerName];
        if (auto it = mByPlayer.find(playerName); it != mByPlayer.end())
            menuId = it->second;
    }
    if (menuId >= 0)
        close(menuId);
}

std::uint64_t ContainerMenuManager::droppedClicks() const {
    return mDroppedClicks.load(std::memory_order_relaxed);
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
    bool setInteractive(int64_t menuId, bool on) override {
        return ContainerMenuManager::getInstance().setInteractive(menuId, on);
    }
    bool setItem(int64_t menuId, int slot, hologramlib::ContainerMenuItem const& item) override {
        return ContainerMenuManager::getInstance().setItem(menuId, slot, item);
    }
    bool close(int64_t menuId) override { return ContainerMenuManager::getInstance().close(menuId); }
    bool blockEvent(int64_t menuId, int b0, int b1) override {
        return ContainerMenuManager::getInstance().blockEvent(menuId, b0, b1);
    }
    void               closeAll() override { ContainerMenuManager::getInstance().closeAll(); }
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
