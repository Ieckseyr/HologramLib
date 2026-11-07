#include "core/c_api.h"

#include "BlockPackets.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "core/ItemNbt.h"
#include "core/ProtocolShape.h"
#include "core/ShapeWire.h"


uint32_t holo_core_packet_id_primitive_shapes(void) {
    return debugshape_export::core::kPrimitiveShapesPacketId;
}

uint32_t holo_core_packet_id_spawn_particle(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::SpawnParticleEffectPacket::Id);
}

uint32_t holo_core_packet_id_boss_event(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::BossEventPacket::Id);
}

uint32_t holo_core_packet_id_set_score(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::SetScorePacket::Id);
}

uint32_t holo_core_packet_id_set_display_objective(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::SetDisplayObjectivePacket::Id);
}

uint32_t holo_core_packet_id_remove_objective(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::RemoveObjectivePacket::Id);
}

uint32_t holo_core_packet_id_set_hud(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::SetHudPacket::Id);
}

uint32_t holo_core_packet_id_mob_effect(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::MobEffectPacket::Id);
}

uint32_t holo_core_packet_id_actor_event(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::ActorEventPacket::Id);
}

uint32_t holo_core_packet_id_animate_entity(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::AnimateEntityPacket::Id);
}

uint32_t holo_core_packet_id_update_block(void) {
    return debugshape_export::core::blocks::updateBlockPacketId();
}

uint32_t holo_core_packet_id_update_subchunk_blocks(void) {
    return debugshape_export::core::blocks::updateSubChunkBlocksPacketId();
}

uint32_t holo_core_packet_id_set_actor_data(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::SetActorDataPacket::Id);
}

uint32_t holo_core_packet_id_locator_bar(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::LocatorBarPacket::Id);
}

uint32_t holo_core_packet_id_respawn(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::RespawnPacket::Id);
}

uint32_t holo_core_packet_id_add_item_actor(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::AddItemActorPacket::Id);
}

uint32_t holo_core_packet_id_remove_actor(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::RemoveActorPacket::Id);
}

uint32_t holo_core_packet_id_move_actor_absolute(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::MoveActorAbsolutePacket::Id);
}

uint32_t holo_core_packet_id_set_actor_motion(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::SetActorMotionPacket::Id);
}

uint32_t holo_core_packet_id_take_item_actor(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::TakeItemActorPacket::Id);
}

uint32_t holo_core_packet_id_add_actor(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::AddActorPacket::Id);
}

uint32_t holo_core_packet_id_mob_equipment(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::MobEquipmentPacket::Id);
}

uint32_t holo_core_packet_id_container_open(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::ContainerOpenPacket::Id);
}

uint32_t holo_core_packet_id_container_close(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::ContainerClosePacket::Id);
}

uint32_t holo_core_packet_id_inventory_content(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::InventoryContentPacket::Id);
}

uint32_t holo_core_packet_id_update_trade(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::UpdateTradePacket::Id);
}

uint32_t holo_core_packet_id_npc_dialogue(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::NpcDialoguePacket::Id);
}

uint32_t holo_core_packet_id_player_list(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::PlayerListPacket::Id);
}

uint32_t holo_core_packet_id_add_player(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::AddPlayerPacket::Id);
}

uint32_t holo_core_packet_id_emote(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::EmotePacket::Id);
}

uint32_t holo_core_packet_id_set_actor_link(void) {
    return static_cast<uint32_t>(debugshape_export::core::proto::bp::SetActorLinkPacket::Id);
}
