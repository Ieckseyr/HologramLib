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

bool ContainerMenuManager::setItemEnchanted(
    int64_t                                 menuId,
    int                                     slot,
    hologramlib::ContainerMenuItem const&   item,
    std::vector<std::pair<int, int>> const& enchants) {
    if (hlibDomainDisabled("HLIB_NO_CONTAINER"))
        return false;
    Menu copy;
    {
        std::lock_guard lock(mMutex);
        bool const      patched = mMenus.withObject(menuId, [&](Menu& menu) -> bool {
            if (slot < 0 || slot >= static_cast<int>(menu.spec.items.size()))
                return false;
            if (slot < static_cast<int>(menu.spec.items.size()))
                menu.spec.items[slot] = item;
            if (enchants.empty())
                menu.enchantsBySlot.erase(slot);
            else
                menu.enchantsBySlot[slot] = enchants;
            copy = menu;
            return true;
        });
        if (!patched)
            return false;
    }
    auto* player = container_util::findPlayerByName(copy.playerName);
    if (player == nullptr)
        return false;
    if (!isCurrentEpoch(copy))
        return true;

    if (!copy.uiOpen)
        return true;

    auto                  stack = item::makeMenuItemStack(item, enchants);
    ::InventorySlotPacket packet{::InventorySlotPacketPayload(
        static_cast<::ContainerID>(copy.containerId),
        static_cast<uint>(slot),
        stack)};
    (void)wire::send(*player, packet);
    HLIB_LOG_INFO("[ContainerMenu] 按槽刷新(UI开着): player={} menuId={} slot={} 物品={}", copy.playerName, menuId, slot, item.type);
    return true;
}

void ContainerMenuManager::applyInteractiveSlots(
    Menu&                           menu,
    std::vector<RequestSlot> const& slots,
    int                             amount) {
    auto isEmpty = [](hologramlib::ContainerMenuItem const& item) {
        return item.type.empty();
    };
    int move = amount < 1 ? 1 : amount;

    for (std::size_t i = 0; i < slots.size(); ++i) {
        auto const& ref = slots[i];
        if (ref.slot < 0 || ref.slot >= static_cast<int>(menu.spec.items.size()))
            continue;
        auto& slotItem = menu.spec.items[static_cast<std::size_t>(ref.slot)];

        bool const isSource = (i % 2 == 0);
        if (isSource) {
            if (isEmpty(slotItem))
                continue;
            int const taken   = std::min(move, slotItem.count < 1 ? 1 : slotItem.count);
            menu.cursor       = slotItem;
            menu.cursor.count = taken;
            slotItem.count -= taken;
            if (slotItem.count <= 0)
                slotItem = hologramlib::ContainerMenuItem{};
        } else {
            if (isEmpty(menu.cursor))
                continue;
            if (isEmpty(slotItem) || slotItem.type == menu.cursor.type) {

                int const putCount = std::min(move, menu.cursor.count < 1 ? 1 : menu.cursor.count);
                if (isEmpty(slotItem)) {
                    slotItem       = menu.cursor;
                    slotItem.count = putCount;
                } else {
                    slotItem.count += putCount;
                }
                menu.cursor.count -= putCount;
                if (menu.cursor.count <= 0)
                    menu.cursor = hologramlib::ContainerMenuItem{};
            } else {

                std::swap(slotItem, menu.cursor);
            }
        }
    }
}

bool ContainerMenuManager::handleInteractiveRequest(
    std::string const&              playerName,
    std::vector<RequestSlot> const& slots,
    int                             amount) {
    if (slots.empty())
        return false;

    std::vector<hologramlib::ContainerClickEvent> events;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end())
            return false;
        bool const handled = mMenus.withObject(byPlayer->second, [&](Menu& menu) -> bool {
            if (!menu.spec.interactive)
                return false;

            int const slotCount = static_cast<int>(menu.spec.items.size());
            for (auto const& ref : slots) {
                bool const inContainer = (ref.container == static_cast<int>(ContainerEnumName::LevelEntityContainer)) || (ref.containerId >= 101 && ref.containerId <= 199);
                if (!inContainer)
                    return false;
                if (ref.slot < 0 || ref.slot >= slotCount)
                    return false;
            }

            applyInteractiveSlots(menu, slots, amount);

            for (std::size_t i = 0; i < slots.size(); ++i) {
                hologramlib::ContainerClickEvent event;
                event.playerName = playerName;
                event.menuId     = menu.menuId;
                event.slot       = slots[i].slot;
                events.push_back(event);
            }
            return true;
        });
        if (!handled)
            return false;
    }
    for (auto const& event : events)
        dispatch(event);
    HLIB_LOG_INFO(
        "[ContainerMenu] 可交互模式接住请求: player={} 槽位数={} amount={}",
        playerName,
        slots.size(),
        amount);
    return true;
}

bool ContainerMenuManager::handleContainerClose(std::string const& playerName, int containerId) {
    hologramlib::ContainerClickEvent event;
    Menu                             copy;
    {
        std::lock_guard lock(mMutex);
        auto            byPlayer = mByPlayer.find(playerName);
        if (byPlayer == mByPlayer.end())
            return false;
        bool const hit = mMenus.withObject(byPlayer->second, [&](Menu const& menu) -> bool {
            if (menu.containerId != containerId)
                return false;
            copy = menu;
            return true;
        });
        if (!hit)
            return false;
        event.playerName               = playerName;
        event.menuId                   = copy.menuId;
        event.closed                   = true;
        mIdFreedTick[copy.containerId] = container_util::nowTick();
        mMenus.erase(byPlayer->second);
        mByPlayer.erase(byPlayer);
    }
    dispatch(event);

    if (auto* player = container_util::findPlayerByName(playerName)) {
        if (copy.spec.useMinecart) {
            proto::RemoveActorPacket remove;
            remove.entity_id = proto::ActorUniqueID{static_cast<std::int64_t>(copy.carrierUniqueId)};
            container_util::sendPacket(*player, remove);
        } else {
            restoreRealBlocks(*player, copy);
        }
    }
    return true;
}

} // namespace debugshape_export
