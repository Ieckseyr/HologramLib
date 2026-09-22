// FakeInventoryPackets.h - 背包虚容器的两个包（用 BDS 自己的包类构造, 见下方"为什么"）
//
//   InventoryContentPacket(49)  整份背包内容: ContainerId = Inventory(0)
//   InventorySlotPacket(50)     单格改写: 同一容器 id
//
// 为什么这里不像其它域那样手写包体:
//   **物品描述符里的 User Data 用的是另一套 NBT 方言**（u16 名字长度 / 4 字节整数 / u32 列表计数,
//   见 26.40 抓包 logs/fullpkts/pkt49_115316_007.bin），而 sculk 的 CompoundTag 写的是 varint 那套
//   （Offers / BlockActorData 用的才是那套, 两边都逐字节对拍过）。手写描述符因此必然写错 —— 客户端
//   解析越界会直接卡死（实测现象）。BDS 自己的 `NetworkItemStackDescriptor` 才是唯一可靠来源, 所以
//   这里交给它的包类序列化; 其余域（交易菜单 / 虚拟容器 / NPC 对话）仍然全部手写。
//
// 容器名与 storage item 的取值同样以抓包为准: BDS 发玩家背包内容时容器名字节写 0、storage 是空物品。
// 单格包的两个可选段（容器名 / storage）BDS 用存在标志表达, 抓包里都是 0/0。
#pragma once

#include "hologramlib/HologramLib.h" // hologramlib::ContainerMenuItem
#include "item/MenuItemStack.h"      // item::makeMenuItemStack（条目 -> ItemStack）

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

// 条目 -> ItemStack（type 空 = 空物品 = 该格空气）
inline ::ItemStack makeItemStack(hologramlib::ContainerMenuItem const& entry) {
    if (entry.type.empty()) return ::ItemStack{};
    return item::makeMenuItemStack(entry);
}

// 整份背包内容（items[i] 即槽位 i; 不足 slotCount 的补空物品）
inline ::InventoryContentPacket makeInventoryContent(
    std::vector<hologramlib::ContainerMenuItem> const& items,
    int                                                slotCount
) {
    static hologramlib::ContainerMenuItem const kEmpty{};
    std::vector<::ItemStack>                    stacks;
    int const                                   total = slotCount < 0 ? 0 : slotCount;
    stacks.reserve(static_cast<std::size_t>(total));
    for (int slot = 0; slot < total; ++slot) {
        auto const& entry = (slot < static_cast<int>(items.size())) ? items[slot] : kEmpty;
        stacks.push_back(makeItemStack(entry));
    }
    // 容器名用默认值、storage 用空物品 —— 与 BDS 抓包（pkt49: 容器名字节 0 + storage 空描述符）一致
    ::InventoryContentPacketPayload payload{
        ::ContainerID::Inventory,
        stacks,
        ::FullContainerName{},
        ::ItemStack{}
    };
    return ::InventoryContentPacket{std::move(payload)};
}

// 单格改写
inline ::InventorySlotPacket makeInventorySlot(int slot, hologramlib::ContainerMenuItem const& entry) {
    ::InventorySlotPacketPayload payload{
        ::ContainerID::Inventory,
        slot,
        makeItemStack(entry),
        ::FullContainerName{},
        ::ItemStack{}
    };
    return ::InventorySlotPacket{std::move(payload)};
}

} // namespace debugshape_export::fakeinv
