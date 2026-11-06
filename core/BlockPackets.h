#pragma once

// 方块相关包的构造（平台无关）：LL 与 holo_core 共用同一份，避免两边各写一遍。

#include "PacketCodec.h"

#include <sculk/protocol/codec/level/block/BlockPos.hpp>
#include <sculk/protocol/codec/level/chunk/SubChunkBlockChangeInfo.hpp>
#include <sculk/protocol/codec/packet/UpdateBlockPacket.hpp>
#include <sculk/protocol/codec/packet/UpdateSubChunkBlocksPacket.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace debugshape_export::core::blocks {

struct ChangeEntry {
    int           x{};
    int           y{};
    int           z{};
    std::uint32_t runtimeId{};
    std::uint32_t flags{};
    std::uint64_t syncEntity{};
    std::uint32_t syncMessage{};
};

inline std::uint32_t updateBlockPacketId() {
    return static_cast<std::uint32_t>(sculk::protocol::MinecraftPacketIds::UpdateBlock);
}

inline std::string buildUpdateBlockBody(
    int           x,
    int           y,
    int           z,
    std::uint32_t runtimeId,
    std::uint32_t flag,
    std::uint32_t layer
) {
    sculk::protocol::UpdateBlockPacket packet;
    packet.mBlockPosition = sculk::protocol::BlockPos{x, y, z};
    packet.mRuntimeId     = runtimeId;
    packet.mFlag          = flag;
    packet.mLayer         = layer;
    return codec::body(packet);
}

inline std::uint32_t updateSubChunkBlocksPacketId() {
    return static_cast<std::uint32_t>(sculk::protocol::MinecraftPacketIds::UpdateSubChunkBlocks);
}

inline std::string buildUpdateSubChunkBlocksBody(
    int                              subX,
    int                              subY,
    int                              subZ,
    std::vector<ChangeEntry> const&  changes
) {
    sculk::protocol::UpdateSubChunkBlocksPacket packet;
    packet.mSubChunkBlockPosition = sculk::protocol::BlockPos{subX, subY, subZ};
    packet.mBlocksChangeStandards.reserve(changes.size());
    for (auto const& change : changes) {
        sculk::protocol::SubChunkBlockChangeInfo info;
        info.mPosition    = sculk::protocol::BlockPos{change.x, change.y, change.z};
        info.mRuntimeId   = change.runtimeId;
        info.mFlags       = change.flags;
        info.mSyncEntity  = change.syncEntity;
        info.mSyncMessage = change.syncMessage;
        packet.mBlocksChangeStandards.push_back(info);
    }
    return codec::body(packet);
}

} // namespace debugshape_export::core::blocks
