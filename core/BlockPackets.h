#pragma once

// 方块相关包的构造（平台无关）：LL 与 holo_core 共用同一份。
// 协议前置 = EndstoneMC/bedrock-protocol（生成式 codec，按协议版本选形）。

#include "core/proto/Version.h"

#include <cstdint>
#include <string>
#include <vector>

namespace debugshape_export::core::proto {

namespace bp = ::bedrock::protocol;

// 任意生成包 → 包体字节（不含包头）
template <class PacketT>
std::string encode(PacketT const& packet) {
    std::string buffer;
    bp::BinaryWriter writer{buffer};
    bp::serialize(writer, packet);
    return buffer;
}

} // namespace debugshape_export::core::proto

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
    return static_cast<std::uint32_t>(proto::UpdateBlockPacket::Id);
}

inline std::string buildUpdateBlockBody(
    int           x,
    int           y,
    int           z,
    std::uint32_t runtimeId,
    std::uint32_t flag,
    std::uint32_t layer
) {
    proto::UpdateBlockPacket packet;
    packet.pos          = proto::BlockPos{x, y, z};
    packet.runtime_id   = proto::BlockRuntimeId{runtimeId};
    packet.update_flags = static_cast<std::uint8_t>(flag);
    packet.layer        = layer;
    return proto::encode(packet);
}

inline std::uint32_t updateSubChunkBlocksPacketId() {
    return static_cast<std::uint32_t>(proto::UpdateSubChunkBlocksPacket::Id);
}

inline std::string buildUpdateSubChunkBlocksBody(
    int                             subX,
    int                             subY,
    int                             subZ,
    std::vector<ChangeEntry> const& changes
) {
    proto::UpdateSubChunkBlocksPacket packet;
    packet.sub_chunk_block_position = proto::BlockPos{subX, subY, subZ};

    packet.blocks_changed.standards.reserve(changes.size());
    for (auto const& change : changes) {
        proto::UpdateSubChunkNetworkBlockInfo info;
        info.pos          = proto::BlockPos{change.x, change.y, change.z};
        info.runtime_id   = proto::BlockRuntimeId{change.runtimeId};
        info.update_flags = static_cast<std::uint8_t>(change.flags);
        info.entity_unique_id = proto::ActorUniqueID{static_cast<std::int64_t>(change.syncEntity)};
        info.message          = proto::ActorBlockSyncMessage::MessageId{};
        packet.blocks_changed.standards.push_back(info);
    }
    return proto::encode(packet);
}

} // namespace debugshape_export::core::blocks
