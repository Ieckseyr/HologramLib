# ABI 符号清单

`holo_core.dll` 导出 **68** 个 C 符号（全部 `extern "C"`、`__declspec(dllexport)`），
声明在仓库 `core/c_api.h`。本页由 `tools/gen_abi_docs.py` 从该头文件生成，不手抄。

| 组 | 数量 | 用途 |
|---|---|---|
| 包 id | 30 | 把包 id 从 core 取出来，避免在消费方硬编码 |
| 编码 / 构造 | 35 | 把参数编成包体（body，不含包头） |
| 生命周期与版本 | 3 | 版本号、ABI 版本、释放 core 分配的内存 |

## 包 id（30）

| 符号 | 返回 | 参数 |
|---|---|---|
| `holo_core_packet_id_primitive_shapes` | `uint32_t` | `void` |
| `holo_core_packet_id_spawn_particle` | `uint32_t` | `void` |
| `holo_core_packet_id_boss_event` | `uint32_t` | `void` |
| `holo_core_packet_id_set_score` | `uint32_t` | `void` |
| `holo_core_packet_id_set_display_objective` | `uint32_t` | `void` |
| `holo_core_packet_id_remove_objective` | `uint32_t` | `void` |
| `holo_core_packet_id_set_hud` | `uint32_t` | `void` |
| `holo_core_packet_id_mob_effect` | `uint32_t` | `void` |
| `holo_core_packet_id_actor_event` | `uint32_t` | `void` |
| `holo_core_packet_id_animate_entity` | `uint32_t` | `void` |
| `holo_core_packet_id_update_block` | `uint32_t` | `void` |
| `holo_core_packet_id_update_subchunk_blocks` | `uint32_t` | `void` |
| `holo_core_packet_id_set_actor_data` | `uint32_t` | `void` |
| `holo_core_packet_id_respawn` | `uint32_t` | `void` |
| `holo_core_packet_id_add_item_actor` | `uint32_t` | `void` |
| `holo_core_packet_id_remove_actor` | `uint32_t` | `void` |
| `holo_core_packet_id_move_actor_absolute` | `uint32_t` | `void` |
| `holo_core_packet_id_set_actor_motion` | `uint32_t` | `void` |
| `holo_core_packet_id_take_item_actor` | `uint32_t` | `void` |
| `holo_core_packet_id_add_actor` | `uint32_t` | `void` |
| `holo_core_packet_id_mob_equipment` | `uint32_t` | `void` |
| `holo_core_packet_id_container_open` | `uint32_t` | `void` |
| `holo_core_packet_id_container_close` | `uint32_t` | `void` |
| `holo_core_packet_id_inventory_content` | `uint32_t` | `void` |
| `holo_core_packet_id_update_trade` | `uint32_t` | `void` |
| `holo_core_packet_id_npc_dialogue` | `uint32_t` | `void` |
| `holo_core_packet_id_player_list` | `uint32_t` | `void` |
| `holo_core_packet_id_add_player` | `uint32_t` | `void` |
| `holo_core_packet_id_emote` | `uint32_t` | `void` |
| `holo_core_packet_id_set_actor_link` | `uint32_t` | `void` |

## 编码 / 构造（35）

| 符号 | 返回 | 参数 |
|---|---|---|
| `holo_core_encode_primitive_shapes` | `int32_t` | `holo_shape_desc const* shapes, size_t count, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_primitive_shapes_packet` | `int32_t` | `holo_shape_desc const* shapes, size_t count, uint8_t** out_packet, size_t* out_len` |
| `holo_core_encode_spawn_particle` | `int32_t` | `uint8_t dimension_id, int64_t actor_unique_id, float x, float y, float z, const char* effect, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_boss_event` | `int32_t` | `int64_t target_actor_id, int64_t player_id, uint8_t type, const char* name, const char* filtered_name, float percentage, uint32_t color, uint32_t overlay, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_set_display_objective` | `int32_t` | `const char* slot, const char* objective, const char* display_name, const char* criteria, uint8_t sort_order, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_remove_objective` | `int32_t` | `const char* objective, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_set_score` | `int32_t` | `uint8_t packet_type, const holo_score_entry* entries, size_t count, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_set_hud` | `int32_t` | `const uint8_t* elements, size_t count, uint8_t visibility, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_mob_effect` | `int32_t` | `uint64_t runtime_id, uint8_t event_type, int32_t effect_id, int32_t amplifier, uint8_t show_particles, int32_t duration_ticks, uint64_t tick, uint8_t ambient, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_actor_event` | `int32_t` | `uint64_t runtime_id, int32_t event_id, int32_t data, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_animate_entity` | `int32_t` | `const char* animation, const char* next_state, const char* stop_expression, int32_t molang_version, const char* controller, float blend_out_time, const uint64_t* runtime_ids, size_t count, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_update_block` | `int32_t` | `int32_t x, int32_t y, int32_t z, uint32_t runtime_id, uint32_t flag, uint32_t layer, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_block_actor_data` | `int32_t` | `int32_t x, int32_t y, int32_t z, const uint8_t* nbt, size_t nbt_len, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_update_subchunk_blocks` | `int32_t` | `int32_t sub_x, int32_t sub_y, int32_t sub_z, const holo_block_change* blocks, size_t count, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_set_actor_data` | `int32_t` | `uint64_t runtime_id, uint64_t tick, const holo_meta_entry* entries, size_t count, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_respawn` | `int32_t` | `float x, float y, float z, uint8_t state, uint64_t runtime_id, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_add_item_actor` | `int32_t` | `uint64_t runtime_id, int64_t unique_id, int16_t item_id, uint16_t stack_size, uint32_t aux, float x, float y, float z, float vx, float vy, float vz, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_remove_actor` | `int32_t` | `int64_t unique_id, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_move_actor_absolute` | `int32_t` | `uint64_t runtime_id, uint8_t flags, float x, float y, float z, uint8_t rotation_x, uint8_t rotation_y, uint8_t rotation_y_head, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_set_actor_motion` | `int32_t` | `uint64_t runtime_id, float vx, float vy, float vz, uint64_t tick, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_take_item_actor` | `int32_t` | `uint64_t item_runtime_id, uint64_t actor_runtime_id, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_add_actor_ex` | `int32_t` | `const char* identifier, const char* name, uint64_t runtime_id, int64_t unique_id, float x, float y, float z, float yaw, float pitch, float hitbox_width, float hitbox_height, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_mob_equipment` | `int32_t` | `uint64_t runtime_id, int16_t item_id, uint16_t stack_size, uint32_t aux, uint8_t slot, uint8_t selected_slot, uint8_t container_id, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_container_open` | `int32_t` | `uint8_t container_id, uint8_t container_type, int32_t x, int32_t y, int32_t z, int64_t target_actor_id, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_container_close` | `int32_t` | `uint8_t container_id, uint8_t container_type, uint8_t server_initiated, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_inventory_content` | `int32_t` | `uint32_t container_id, const holo_item_slot* slots, size_t count, uint8_t** out_body, size_t* out_len` |
| `holo_core_build_item_nbt` | `int32_t` | `const char* type, int32_t count, int32_t damage, const char* name, const char* lore_csv, const char* ench_csv, int32_t is_block, int32_t block_version, uint8_t** out_body, size_t* out_len` |
| `holo_core_build_offers` | `int32_t` | `const holo_trade_offer* offers, size_t count, const int32_t* tier_exp, size_t tier_count, int32_t base_net_id, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_update_trade` | `int32_t` | `uint8_t container_id, uint8_t container_type, int32_t size, int32_t tier, int64_t entity_unique_id, int64_t last_trading_player, const char* display_name, uint8_t use_new_screen, uint8_t use_economy_trade, const uint8_t* offers_nbt, size_t offers_nbt_len, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_npc_dialogue` | `int32_t` | `uint64_t npc_id, uint8_t action, const char* dialogue, const char* scene_name, const char* npc_name, const char* action_json, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_player_list_add` | `int32_t` | `const uint8_t* uuid16, int64_t actor_unique_id, const char* name, const holo_skin_desc* skin, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_player_list_remove` | `int32_t` | `const uint8_t* uuid16, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_add_player` | `int32_t` | `uint64_t runtime_id, const char* name, const uint8_t* uuid16, const holo_skin_desc* skin, float x, float y, float z, float yaw, float pitch, float scale, const char* nametag, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_emote` | `int32_t` | `uint64_t runtime_id, const char* emote_id, uint32_t length_ticks, uint8_t flags, uint8_t** out_body, size_t* out_len` |
| `holo_core_encode_set_actor_link` | `int32_t` | `uint8_t type, int64_t target_a_unique_id, int64_t target_b_unique_id, uint8_t immediate, uint8_t passenger_initiated, float vehicle_angular_velocity, uint8_t** out_body, size_t* out_len` |

## 生命周期与版本（3）

| 符号 | 返回 | 参数 |
|---|---|---|
| `holo_core_api_version` | `uint32_t` | `void` |
| `holo_core_version` | `const char*` | `void` |
| `holo_free` | `void` | `void* ptr` |

---

完整用法见 [ABI 接口总览](index.md)。
