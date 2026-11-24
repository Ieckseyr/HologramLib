#pragma once

#include "hologramlib/HologramLib.h"
#include "item/MenuItemStack.h"

#include <utility>
#include <vector>

#include <mc/network/packet/InventoryContentPacket.h>
#include <mc/network/packet/InventorySlotPacket.h>
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

    // 1.9.7：默认构造与 NetworkItemStackDescriptor 拷贝构造都没有导出，只能走引擎的
    // 构造函数（它内部完成 ItemStack → NetworkItemStackDescriptor 的转换）。
    return ::InventoryContentPacket{
        ::ContainerID::Inventory,
        stacks,
        ::FullContainerName{},
        ::ItemStack{}};
}

inline ::InventorySlotPacket makeInventorySlot(int slot, hologramlib::ContainerMenuItem const& entry) {
    // ItemStack 不可赋值 → 走包自带的 (容器, 槽, 物品) 构造函数
    ::InventorySlotPacket packet{
        ::ContainerID::Inventory,
        static_cast<uint>(slot),
        makeItemStack(entry)};
    return packet;
}

} // namespace debugshape_export::fakeinv
