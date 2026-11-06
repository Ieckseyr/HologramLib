#pragma once

// PrimitiveShapes（调试形状）报文的平台无关构造与编码。
// 形状数据模型来自 core/ProtocolShape.h；这里只负责拼包与字节化。

#include <cstdint>
#include <string>
#include <vector>

#include "core/PacketCodec.h"
#include "core/ProtocolShape.h"

#include "sculk/protocol/codec/MinecraftPacketIds.hpp"

namespace debugshape_export::core {

inline constexpr std::uint32_t kPrimitiveShapesPacketId =
    static_cast<std::uint32_t>(sculk::protocol::MinecraftPacketIds::PrimitiveShapes);

[[nodiscard]] std::string buildPrimitiveShapesBody(std::vector<DebugShape> const& shapes);

[[nodiscard]] std::string buildPrimitiveShapesPacket(std::vector<DebugShape> const& shapes);

} // namespace debugshape_export::core
