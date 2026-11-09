#include "core/c_api.h"

#include "core/c_api_internal.h"

#include "BlockPackets.h"

#include <string_view>

#include "core/ItemNbt.h"

// 方块域里只有"方块实体"这条需要解析 NBT，单独一个 TU，跟 c_api_container.cpp 的
// UpdateTrade(offers NBT) 保持同一种写法：读网络 NBT → 组包 → copyOut。
extern "C" {

// 方块实体（BlockActorData，id 56）。nbt 是已序列化的 network NBT（小端，带根 tag）。
// 解析失败返回 -4；成功时按入参重写 x/y/z（与 LL 版 viewBlockEntity 一致），
// 实体的类型串（id 字段）由调用方在 NBT 字节里给：C ABI 不收类型串，
// endstone 侧在 Python 层补 id（见 platform/endstone/.../view.py）。
int32_t holo_core_encode_block_actor_data(
    int32_t        x,
    int32_t        y,
    int32_t        z,
    const uint8_t* nbt,
    size_t         nbt_len,
    uint8_t**      out_body,
    size_t*        out_len) {
    namespace proto = debugshape_export::core::proto;
    namespace bp    = debugshape_export::core::proto::bp;
    namespace trade = debugshape_export::trade;

    if (nbt == nullptr || nbt_len == 0) {
        return -4;
    }

    std::string_view raw{reinterpret_cast<char const*>(nbt), nbt_len};
    bp::BinaryReader reader{raw};
    auto             named = bp::nbt::readNamedTag(reader);
    if (!named) {
        return -4;
    }
    auto* compound = named->second.template get_if<bp::CompoundTag>();
    if (compound == nullptr) {
        return -4;
    }

    proto::BlockActorDataPacket packet;
    packet.pos       = proto::BlockPos{x, y, z};
    packet.data      = std::move(*compound);
    packet.data["x"] = trade::makeTag(bp::IntTag{x});
    packet.data["y"] = trade::makeTag(bp::IntTag{y});
    packet.data["z"] = trade::makeTag(bp::IntTag{z});

    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

} // extern "C"
