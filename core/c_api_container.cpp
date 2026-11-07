#include "core/c_api.h"

#include "core/c_api_internal.h"

#include "BlockPackets.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "core/ItemNbt.h"
#include "core/ProtocolShape.h"
#include "core/ShapeWire.h"


namespace {

using namespace debugshape_export;

} // namespace

extern "C" {

int32_t holo_core_encode_add_item_actor(
    uint64_t runtime_id,
    int64_t  unique_id,
    int16_t  item_id,
    uint16_t stack_size,
    uint32_t aux,
    float x, float y, float z,
    float vx, float vy, float vz,
    uint8_t** out_body,
    size_t*   out_len) {
    // 1.26.30+ (protocol >= 2168) 的物品字段是 SerializedNetworkItemStackDescriptor：
    // id 为 int16 定长、stack_size 为 uint16 定长，尾随长度前缀 blob。
    debugshape_export::core::proto::AddItemActorPacket packet;
    packet.id                     = debugshape_export::core::proto::ActorUniqueID{unique_id};
    packet.runtime_id             = debugshape_export::core::proto::bp::ActorRuntimeID{runtime_id};
    packet.item.id                = item_id;
    packet.item.stack_size        = stack_size;
    packet.item.aux_value         = aux;
    packet.pos                    = debugshape_export::core::proto::bp::Vec3{x, y, z};
    packet.velocity               = debugshape_export::core::proto::bp::Vec3{vx, vy, vz};
    packet.is_from_fishing        = false;

    return debugshape_export::copyOut(debugshape_export::core::proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_take_item_actor(
    uint64_t  item_runtime_id,
    uint64_t  actor_runtime_id,
    uint8_t** out_body,
    size_t*   out_len
) {
    debugshape_export::core::proto::TakeItemActorPacket packet;
    packet.item_id  = debugshape_export::core::proto::bp::ActorRuntimeID{item_runtime_id};
    packet.actor_id = debugshape_export::core::proto::bp::ActorRuntimeID{actor_runtime_id};
    return debugshape_export::copyOut(debugshape_export::core::proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_container_open(
    uint8_t container_id,
    uint8_t container_type,
    int32_t x, int32_t y, int32_t z,
    int64_t   target_actor_id,
    uint8_t** out_body,
    size_t*   out_len) {
    debugshape_export::core::proto::ContainerOpenPacket packet;
    packet.container_id      = static_cast<debugshape_export::core::proto::ContainerIDType>(container_id);
    packet.type              = static_cast<debugshape_export::core::proto::ContainerTypeType>(container_type);
    packet.pos               = debugshape_export::core::proto::NetworkBlockPosition{x, y, z};
    packet.entity_unique_id  = debugshape_export::core::proto::ActorUniqueID{target_actor_id};

    return debugshape_export::copyOut(debugshape_export::core::proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_container_close(
    uint8_t   container_id,
    uint8_t   container_type,
    uint8_t   server_initiated,
    uint8_t** out_body,
    size_t*   out_len) {
    debugshape_export::core::proto::ContainerClosePacket packet;
    packet.container_id           = static_cast<debugshape_export::core::proto::ContainerIDType>(container_id);
    packet.container_type         = static_cast<debugshape_export::core::proto::ContainerTypeType>(container_type);
    packet.server_initiated_close = server_initiated != 0;

    return debugshape_export::copyOut(debugshape_export::core::proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_inventory_content(
    uint32_t              container_id,
    const holo_item_slot* slots,
    size_t                count,
    uint8_t**             out_body,
    size_t*               out_len) {
    debugshape_export::core::proto::InventoryContentPacket packet;
    packet.inventory_id =
        static_cast<debugshape_export::core::proto::ContainerIDType>(container_id);
    packet.slots.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        debugshape_export::core::proto::InventoryItemDescriptor item;
        item.id          = slots[i].item_id;
        item.stack_size  = slots[i].stack_size;
        item.aux_value   = slots[i].aux;
        if (slots[i].user_data != nullptr && slots[i].user_data_len > 0) {
            item.user_data_buffer.assign(slots[i].user_data, slots[i].user_data_len);
        }
        packet.slots.push_back(std::move(item));
    }

    return debugshape_export::copyOut(debugshape_export::core::proto::encode(packet), out_body, out_len);
}

int32_t holo_core_encode_update_trade(
    uint8_t        container_id,
    uint8_t        container_type,
    int32_t        size,
    int32_t        tier,
    int64_t        entity_unique_id,
    int64_t        last_trading_player,
    const char*    display_name,
    uint8_t        use_new_screen,
    uint8_t        use_economy_trade,
    const uint8_t* offers_nbt,
    size_t         offers_nbt_len,
    uint8_t**      out_body,
    size_t*        out_len) {
    namespace proto = debugshape_export::core::proto;
    namespace bp    = debugshape_export::core::proto::bp;

    proto::UpdateTradePacket packet;
    packet.container_id        = static_cast<proto::ContainerIDType>(container_id);
    packet.type                = static_cast<proto::TradeContainerTypeType>(container_type);
    packet.size                = size;
    packet.trader_tier         = tier;
    packet.entity_unique_id    = proto::ActorUniqueID{entity_unique_id};
    packet.last_trading_player = proto::ActorUniqueID{last_trading_player};
    packet.display_name        = display_name != nullptr ? display_name : "";
    packet.use_new_trade_screen = use_new_screen != 0;
    packet.using_economy_trade  = use_economy_trade != 0;

    if (offers_nbt != nullptr && offers_nbt_len > 0) {
        std::string_view raw{reinterpret_cast<char const*>(offers_nbt), offers_nbt_len};
        bp::BinaryReader reader{raw};
        if (auto named = bp::nbt::readNamedTag(reader); named) {
            if (auto* compound = named->second.template get_if<bp::CompoundTag>(); compound != nullptr) {
                packet.data = std::move(*compound);
            }
        }
    }

    return debugshape_export::copyOut(proto::encode(packet), out_body, out_len);
}

} // extern "C"
