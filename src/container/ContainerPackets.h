// ContainerPackets.h - 虚拟容器的三个包 + 箱子方块实体 NBT（纯构造, 不碰 BDS）
//
// 抽出来的原因与 trade/TradeOfferNbt.h 一样: 让 tests/check-container-packets.bat 能离线
// 编译同一份构造代码, 把字节解回来逐字段核对 —— 不必让 fixture 重新实现一遍格式。
// 方案逐条对齐参考实现 GMLIB 的 ChestUI（src-shared/gmlib/gm/ui/ChestUI.cpp）:
//   updateBlock      → UpdateBlockPacket(mRuntimeId = 箱子方块的 Block::mNetworkId, flag = 3)
//   updateBlockActor → BlockActorDataPacket(BlockPos + CustomName/x/y/z/pair*/Items 的 NBT)
//   ContainerOpen    → 容器类型 0(Container) + 方块坐标 + 目标实体 -1（GMLIB 用写死的 114 号容器 id,
//                      本库用 101..199 显示区间）
//
// 方块实体 NBT 的键与取值（GMLIB 的 mChestNbt）:
//   id = "Chest"    Findable = 0b   isMovable = 1b   CustomName = 标题
//   x / y / z = 方块坐标
//   大箱子再加 pairx / pairz / pairlead（lead 半: pairlead=1 且 pairx = x+1; 副半相反）
//   Items = List<Compound>, 每个物品是标准 Bedrock 物品 NBT + Slot（各自半区里的 0..26）
#pragma once

#include "hologramlib/HologramLib.h" // ContainerMenuItem

#include "trade/TradeOfferNbt.h" // trade::detail::itemToNbt（同一份逐字节对拍过的物品 NBT 方言）

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

namespace debugshape_export::container {

using sculk::protocol::ByteTag;
using sculk::protocol::IntTag;
using sculk::protocol::StringTag;

// 大箱子判定: rows >= 6 → 54 格（两个配对箱子方块）
inline bool isBigChest(int rows) { return rows >= 6; }
inline int  slotCountForRows(int rows) { return isBigChest(rows) ? 54 : 27; }

// 箱子方块实体 NBT。secondHalf = 大箱子的副半（x-1 那一格; 只装 27..53 号条目）。
inline sculk::protocol::CompoundTag buildChestNbt(
    int                                    x,
    int                                    y,
    int                                    z,
    bool                                   secondHalf,
    std::string const&                     title,
    int                                    rows,
    std::vector<hologramlib::ContainerMenuItem> const& items
) {
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
        if (slot < 0 || slot >= static_cast<int>(items.size())) break;
        auto const& entry = items[static_cast<std::size_t>(slot)];
        if (entry.type.empty()) continue; // 空槽不占位

        trade::TradeItem item;
        item.type   = entry.type;
        item.count  = entry.count;
        item.damage = entry.damage;
        item.name   = entry.name;
        item.lore   = entry.lore;

        auto itemNbt           = trade::detail::itemToNbt(item);
        itemNbt.mValue["Slot"] = trade::makeTag(ByteTag{static_cast<std::int8_t>(slot - firstSlot)});
        list.mValue.push_back(trade::makeTag(std::move(itemNbt)));
    }
    out.mValue["Items"] = trade::makeTag(std::move(list));
    return out;
}

// ① 客户端侧箱子方块（flag = 3 = (1<<0)|(1<<1): 通知邻居 + 立刻重绘, GMLIB 同值）
inline sculk::protocol::UpdateBlockPacket makeUpdateBlock(
    int           x,
    int           y,
    int           z,
    std::uint32_t runtimeId
) {
    sculk::protocol::UpdateBlockPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mRuntimeId     = runtimeId;
    packet.mFlag          = 3;
    packet.mLayer         = 0;
    return packet;
}

// ② 该方块的方块实体数据
inline sculk::protocol::BlockActorDataPacket makeBlockActorData(
    int                         x,
    int                         y,
    int                         z,
    sculk::protocol::CompoundTag nbt
) {
    sculk::protocol::BlockActorDataPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mActorDataTags = std::move(nbt);
    return packet;
}

// ③ 打开容器: 类型 Container(0) + 方块坐标 + 目标实体 -1（绑方块, 不绑实体）
inline sculk::protocol::ContainerOpenPacket makeContainerOpen(int containerId, int x, int y, int z) {
    sculk::protocol::ContainerOpenPacket packet;
    packet.mContainerId   = static_cast<sculk::protocol::ContainerID>(containerId);
    packet.mContainerType = sculk::protocol::ContainerType::Container;
    packet.mPosition      = sculk::protocol::BlockPos{x, y, z};
    packet.mTargetActorId = -1;
    return packet;
}

// ④ 关容器（服务端主动关）
inline sculk::protocol::ContainerClosePacket makeContainerClose(int containerId) {
    sculk::protocol::ContainerClosePacket packet;
    packet.mContainerId          = static_cast<sculk::protocol::ContainerID>(containerId);
    packet.mContainerType        = sculk::protocol::ContainerType::Container;
    packet.mServerInitiatedClose = true;
    return packet;
}

} // namespace debugshape_export::container
