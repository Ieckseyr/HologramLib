#pragma once

#include "hologramlib/HologramLib.h"
#include "item/MenuItemStack.h"

#include <utility>
#include <vector>

#include <mc/network/packet/InventoryContentPacket.h>
#include <mc/network/packet/InventoryContentPacketPayload.h>
#include <mc/network/packet/InventorySlotPacket.h>
#include <mc/network/packet/InventorySlotPacketPayload.h>
#include <mc/world/ContainerID.h>
#include <mc/world/containers/FullContainerName.h>
#include <mc/world/item/ItemStack.h>

namespace debugshape_export::fakeinv {

inline ::ItemStack makeItemStack(hologramlib::ContainerMenuItem const& entry) {
    if (entry.type.empty())
        return ::ItemStack{};
    return item::makeMenuItemStack(entry);
}

inline ::InventoryContentPacket makeInventoryContent(
    std::vector<hologramlib::ContainerMenuItem> const& items,
    int                                                slotCount) {
    static hologramlib::ContainerMenuItem const kEmpty{};
    std::vector<::ItemStack>                    stacks;
    int const                                   total = slotCount < 0 ? 0 : slotCount;
    stacks.reserve(static_cast<std::size_t>(total));
    for (int slot = 0; slot < total; ++slot) {
        auto const& entry = (slot < static_cast<int>(items.size())) ? items[slot] : kEmpty;
        stacks.push_back(makeItemStack(entry));
    }

    ::InventoryContentPacketPayload payload{
        ::ContainerID::Inventory,
        stacks,
        ::FullContainerName{},
        ::ItemStack{}};
    return ::InventoryContentPacket{std::move(payload)};
}

inline ::InventorySlotPacket makeInventorySlot(int slot, hologramlib::ContainerMenuItem const& entry) {
    ::InventorySlotPacketPayload payload{
        ::ContainerID::Inventory,
        slot,
        makeItemStack(entry),
        ::FullContainerName{},
        ::ItemStack{}};
    return ::InventorySlotPacket{std::move(payload)};
}

} // namespace debugshape_export::fakeinv
