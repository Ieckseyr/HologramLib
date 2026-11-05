#pragma once

#include "hologramlib/HologramLib.h"

#include "trade/TradeOfferNbt.h"

#include <cstdint>
#include <string>
#include <vector>

#include <sculk/protocol/codec/inventory/container/ContainerID.hpp>
#include <sculk/protocol/codec/inventory/container/ContainerType.hpp>
#include <sculk/protocol/codec/level/block/BlockPos.hpp>
#include <sculk/protocol/codec/nbt/CompoundTag.hpp>
#include <sculk/protocol/codec/nbt/ListTag.hpp>
#include <sculk/protocol/codec/nbt/TagType.hpp>
#include <sculk/protocol/codec/nbt/TagVariant.hpp>
#include <sculk/protocol/codec/packet/BlockActorDataPacket.hpp>
#include <sculk/protocol/codec/packet/ContainerClosePacket.hpp>
#include <sculk/protocol/codec/packet/ContainerOpenPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateBlockPacket.hpp>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace debugshape_export::container {

using sculk::protocol::ByteTag;
using sculk::protocol::IntTag;
using sculk::protocol::StringTag;

inline bool isBigChest(int rows) {
    return rows >= 6;
}
inline int slotCountForRows(int rows) {
    return isBigChest(rows) ? 54 : 27;
}

using EnchantMap = std::unordered_map<int, std::vector<std::pair<int, int>>>;

inline sculk::protocol::CompoundTag buildChestNbt(
    int                                                x,
    int                                                y,
    int                                                z,
    bool                                               secondHalf,
    std::string const&                                 title,
    int                                                rows,
    std::vector<hologramlib::ContainerMenuItem> const& items,
    EnchantMap const&                                  enchMap = {}) {
    sculk::protocol::CompoundTag out;
    out.mValue["CustomName"] = trade::makeTag(StringTag{title});
    out.mValue["Findable"]   = trade::makeTag(ByteTag{0});
    out.mValue["id"]         = trade::makeTag(StringTag{"Chest"});
    out.mValue["isMovable"]  = trade::makeTag(ByteTag{1});
    out.mValue["x"]          = trade::makeTag(IntTag{x});
    out.mValue["y"]          = trade::makeTag(IntTag{y});
    out.mValue["z"]          = trade::makeTag(IntTag{z});

    bool const big = isBigChest(rows);
    if (big) {
        out.mValue["pairx"]    = trade::makeTag(IntTag{secondHalf ? x - 1 : x + 1});
        out.mValue["pairz"]    = trade::makeTag(IntTag{z});
        out.mValue["pairlead"] = trade::makeTag(ByteTag{static_cast<std::int8_t>(secondHalf ? 0 : 1)});
    }

    sculk::protocol::ListTag list;
    list.mType = sculk::protocol::TagType::Compound;

    int const firstSlot = secondHalf ? 27 : 0;
    int const lastSlot  = secondHalf ? 54 : (big ? 27 : 27);
    for (int slot = firstSlot; slot < lastSlot; ++slot) {
        if (slot < 0 || slot >= static_cast<int>(items.size()))
            break;
        auto const& entry = items[static_cast<std::size_t>(slot)];
        if (entry.type.empty())
            continue;

        trade::TradeItem item;
        item.type   = entry.type;
        item.count  = entry.count;
        item.damage = entry.damage;
        item.name   = entry.name;
        item.lore   = entry.lore;
        auto enchIt = enchMap.find(slot);
        if (enchIt != enchMap.end())
            item.enchants = enchIt->second;

        auto itemNbt           = trade::detail::itemToNbt(item);
        itemNbt.mValue["Slot"] = trade::makeTag(ByteTag{static_cast<std::int8_t>(slot - firstSlot)});
        list.mValue.push_back(trade::makeTag(std::move(itemNbt)));
    }
    out.mValue["Items"] = trade::makeTag(std::move(list));
    return out;
}

inline sculk::protocol::UpdateBlockPacket makeUpdateBlock(
    int           x,
    int           y,
    int           z,
    std::uint32_t runtimeId) {
    sculk::protocol::UpdateBlockPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mRuntimeId     = runtimeId;
    packet.mFlag          = 3;
    packet.mLayer         = 0;
    return packet;
}

inline sculk::protocol::BlockActorDataPacket makeBlockActorData(
    int                          x,
    int                          y,
    int                          z,
    sculk::protocol::CompoundTag nbt) {
    sculk::protocol::BlockActorDataPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mActorDataTags = std::move(nbt);
    return packet;
}

inline sculk::protocol::ContainerOpenPacket makeContainerOpen(int containerId, int x, int y, int z) {
    sculk::protocol::ContainerOpenPacket packet;
    packet.mContainerId   = static_cast<sculk::protocol::ContainerID>(containerId);
    packet.mContainerType = sculk::protocol::ContainerType::Container;
    packet.mPosition      = sculk::protocol::BlockPos{x, y, z};
    packet.mTargetActorId = -1;
    return packet;
}

inline sculk::protocol::ContainerClosePacket makeContainerClose(int containerId) {
    sculk::protocol::ContainerClosePacket packet;
    packet.mContainerId          = static_cast<sculk::protocol::ContainerID>(containerId);
    packet.mContainerType        = sculk::protocol::ContainerType::Container;
    packet.mServerInitiatedClose = true;
    return packet;
}

} // namespace debugshape_export::container
