#pragma once

#include "hologramlib/HologramLib.h"

#include "core/proto/Version.h"
#include "trade/TradeOfferNbt.h"

#include <cstdint>
#include <string>
#include <vector>

#include <unordered_map>
#include <utility>

namespace debugshape_export::container {

namespace proto = ::debugshape_export::core::proto;

using trade::ByteTag;
using trade::CompoundTag;
using trade::IntTag;
using trade::ListTag;
using trade::StringTag;
using trade::TagType;

inline bool isBigChest(int rows) {
    return rows >= 6;
}
inline int slotCountForRows(int rows) {
    return isBigChest(rows) ? 54 : 27;
}

using EnchantMap = std::unordered_map<int, std::vector<std::pair<int, int>>>;

inline CompoundTag buildChestNbt(
    int                                                x,
    int                                                y,
    int                                                z,
    bool                                               secondHalf,
    std::string const&                                 title,
    int                                                rows,
    std::vector<hologramlib::ContainerMenuItem> const& items,
    EnchantMap const&                                  enchMap = {}) {
    CompoundTag out;
    out["CustomName"] = trade::makeTag(StringTag{title});
    out["Findable"]   = trade::makeTag(ByteTag{0});
    out["id"]         = trade::makeTag(StringTag{"Chest"});
    out["isMovable"]  = trade::makeTag(ByteTag{1});
    out["x"]          = trade::makeTag(IntTag{x});
    out["y"]          = trade::makeTag(IntTag{y});
    out["z"]          = trade::makeTag(IntTag{z});

    bool const big = isBigChest(rows);
    if (big) {
        out["pairx"]    = trade::makeTag(IntTag{secondHalf ? x - 1 : x + 1});
        out["pairz"]    = trade::makeTag(IntTag{z});
        out["pairlead"] = trade::makeTag(ByteTag{static_cast<std::int8_t>(secondHalf ? 0 : 1)});
    }

    ListTag list(TagType::Compound);

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

        auto itemNbt       = trade::detail::itemToNbt(item);
        itemNbt["Slot"]    = trade::makeTag(ByteTag{static_cast<std::int8_t>(slot - firstSlot)});
        list.push_back(trade::Tag{std::move(itemNbt)});
    }
    out["Items"] = trade::makeTag(std::move(list));
    return out;
}

inline proto::UpdateBlockPacket makeUpdateBlock(
    int           x,
    int           y,
    int           z,
    std::uint32_t runtimeId) {
    proto::UpdateBlockPacket packet;
    packet.pos          = proto::BlockPos{x, y, z};
    packet.runtime_id   = proto::BlockRuntimeId{runtimeId};
    packet.update_flags = 3;
    packet.layer        = 0;
    return packet;
}

inline proto::BlockActorDataPacket makeBlockActorData(
    int        x,
    int        y,
    int        z,
    CompoundTag nbt) {
    proto::BlockActorDataPacket packet;
    packet.pos  = proto::BlockPos{x, y, z};
    packet.data = std::move(nbt);
    return packet;
}

inline proto::ContainerOpenPacket makeContainerOpen(int containerId, int x, int y, int z) {
    proto::ContainerOpenPacket packet;
    packet.container_id = static_cast<proto::ContainerIDType>(containerId);
    packet.type         = proto::bp::ContainerType::Container;
    packet.pos          = proto::BlockPos{x, y, z};
    packet.entity_unique_id = proto::ActorUniqueID{-1};
    return packet;
}

inline proto::ContainerClosePacket makeContainerClose(int containerId) {
    proto::ContainerClosePacket packet;
    packet.container_id          = static_cast<proto::ContainerIDType>(containerId);
    packet.container_type        = proto::bp::ContainerType::Container;
    packet.server_initiated_close = true;
    return packet;
}

} // namespace debugshape_export::container
