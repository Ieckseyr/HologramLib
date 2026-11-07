#pragma once

// PrimitiveShapes 拼包与编码；数据模型见 core/ProtocolShape.h。

#include <cstdint>
#include <string>
#include <vector>

#include "core/PacketCodec.h"
#include "core/ProtocolShape.h"

#include "core/proto/Version.h"

namespace debugshape_export::core {

inline constexpr std::uint32_t kPrimitiveShapesPacketId =
    static_cast<std::uint32_t>(core::proto::PrimitiveShapesPacket::Id);

[[nodiscard]] std::string buildPrimitiveShapesBody(std::vector<DebugShape> const& shapes);

[[nodiscard]] std::string buildPrimitiveShapesPacket(std::vector<DebugShape> const& shapes);

} // namespace debugshape_export::core
