#pragma once

// holo_core 的 C ABI —— endstone（Python）侧通过 ctypes 调用。
// 只做字节：输入形状描述，输出可直接交给 Player.send_packet 的 body。
// 任何引擎对象都不出现在这个边界上，因此同一份 core 既能给 LeviLamina 用，也能给 endstone 用。

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#    define HOLO_CORE_EXPORT __declspec(dllexport)
#else
#    define HOLO_CORE_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define HOLO_CORE_API_VERSION 1u

// 形状类型编号沿用 lib/exports.json 的 shapeType（0 文本, 1 线, 2 盒, 3 圆, 4 球, 5 箭头,
// 7 圆柱, 8 金字塔, 9 椭球, 10 圆锥）。可选字段用 has_* 开关控制是否写进包。
typedef struct holo_shape_desc {
    uint64_t    network_id;
    int32_t     type;

    int32_t     has_location;
    float       x, y, z;

    int32_t     has_end;      // line / arrow 的终点；box 的 bound 也用这组
    float       ex, ey, ez;

    int32_t     has_scale;
    float       scale;

    int32_t     has_rotation;
    float       rx, ry, rz;

    int32_t     has_total_time_left;
    float       total_time_left;

    int32_t     has_max_render_distance;
    float       max_render_distance;

    int32_t     has_color;
    int32_t     color;        // ARGB 打包

    int32_t     has_attached_entity;
    int64_t     attached_entity_id;

    int32_t     has_dimension;
    int32_t     dimension_id;

    // 形状专属参数
    float       width, height, depth;   // pyramid
    float       radius_x, radius_z;     // cylinder / cone
    int32_t     segments;               // sphere / cylinder / cone / ellipsoid

    // 文本专属
    const char* text;
    int32_t     text_use_rotation;
    int32_t     has_text_background;
    int32_t     text_background_color;
    float       text_line_gap_height;
    int32_t     text_depth_test;
    int32_t     text_show_backface;
    int32_t     text_show_text_backface;
} holo_shape_desc;

HOLO_CORE_EXPORT uint32_t holo_core_api_version(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_primitive_shapes(void);
HOLO_CORE_EXPORT const char* holo_core_version(void);

// 把一批形状编码成 PrimitiveShapes 的 body（不含包头）。
// 成功返回 0；out_body 由本库分配，调用方用 holo_free 释放。
HOLO_CORE_EXPORT int32_t holo_core_encode_primitive_shapes(
    holo_shape_desc const* shapes,
    size_t                 count,
    uint8_t**              out_body,
    size_t*                out_len);

// 带包头的完整字节串（LL 侧用；endstone 用上面的 body 即可）
HOLO_CORE_EXPORT int32_t holo_core_encode_primitive_shapes_packet(
    holo_shape_desc const* shapes,
    size_t                 count,
    uint8_t**              out_packet,
    size_t*                out_len);

HOLO_CORE_EXPORT uint32_t holo_core_packet_id_spawn_particle(void);

// SpawnParticleEffect 的 body（粒子域：Endstone 侧一个点一包）
HOLO_CORE_EXPORT int32_t holo_core_encode_spawn_particle(
    uint8_t     dimension_id,
    int64_t     actor_unique_id,
    float       x,
    float       y,
    float       z,
    const char* effect,
    uint8_t**   out_body,
    size_t*     out_len);

HOLO_CORE_EXPORT uint32_t holo_core_packet_id_boss_event(void);

// BossEvent 的 body（Boss 条：add/更新/删除都是这一个包，靠 type 区分）
// type: 0 Add, 1 PlayerAdded, 2 Remove, 3 PlayerRemoved, 4 UpdatePercent, 5 UpdateName, 7 UpdateStyle
HOLO_CORE_EXPORT int32_t holo_core_encode_boss_event(
    int64_t     target_actor_id,
    int64_t     player_id,
    uint8_t     type,
    const char* name,
    const char* filtered_name,
    float       percentage,
    uint32_t    color,
    uint32_t    overlay,
    uint8_t**   out_body,
    size_t*     out_len);

// ── 计分板（侧边栏） ─────────────────────────────────────────────────────
typedef struct holo_score_entry {
    int64_t     scoreboard_id;
    int32_t     score_value;
    int32_t     identity_type;   // 0 Invalid, 1 Player, 2 Entity, 3 FakePlayer
    int64_t     actor_unique_id;
    const char* objective;
    const char* fake_player_name;
} holo_score_entry;

HOLO_CORE_EXPORT uint32_t holo_core_packet_id_set_score(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_set_display_objective(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_remove_objective(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_set_hud(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_set_display_objective(
    const char* slot,
    const char* objective,
    const char* display_name,
    const char* criteria,
    uint8_t     sort_order,
    uint8_t**   out_body,
    size_t*     out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_remove_objective(
    const char* objective,
    uint8_t**   out_body,
    size_t*     out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_set_score(
    uint8_t                  packet_type, // 0 Change, 1 Remove
    const holo_score_entry*  entries,
    size_t                   count,
    uint8_t**                out_body,
    size_t*                  out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_set_hud(
    const uint8_t* elements,
    size_t         count,
    uint8_t        visibility, // 0 Hide, 1 Reset
    uint8_t**      out_body,
    size_t*        out_len);

// ── 玩家表现（药水效果 / 动作事件 / 动画） ─────────────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_mob_effect(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_actor_event(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_animate_entity(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_mob_effect(
    uint64_t  runtime_id,
    uint8_t   event_type,        // 0 Add, 1 Update, 2 Remove
    int32_t   effect_id,
    int32_t   amplifier,
    uint8_t   show_particles,
    int32_t   duration_ticks,
    uint64_t  tick,
    uint8_t   ambient,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_actor_event(
    uint64_t  runtime_id,
    int32_t   event_id,
    int32_t   data,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_animate_entity(
    const char*     animation,
    const char*     next_state,
    const char*     stop_expression,
    int32_t         molang_version,
    const char*     controller,
    float           blend_out_time,
    const uint64_t* runtime_ids,
    size_t          count,
    uint8_t**       out_body,
    size_t*         out_len);

// ── 假方块（客户端本地改一个方块，不动服务端世界） ─────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_update_block(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_update_block(
    int32_t   x,
    int32_t   y,
    int32_t   z,
    uint32_t  runtime_id,
    uint32_t  flag,
    uint32_t  layer,
    uint8_t** out_body,
    size_t*   out_len);

// 方块实体（BlockActorData，id 56）。nbt 是已序列化的 network NBT（小端，带根 tag），
// 由调用方用 SNBT→NBT 之类的工具生成；成功时按入参重写 x/y/z。
// 方块实体的类型（NBT 里的 id 字段，如 "Banner"）由调用方写进 NBT —— 本函数不收类型串。
// 解析失败返回负值（当前 -4），成功返回 0；out_body 由本库分配，调用方用 holo_free 释放。
HOLO_CORE_EXPORT int32_t holo_core_encode_block_actor_data(
    int32_t        x,
    int32_t        y,
    int32_t        z,
    const uint8_t* nbt,
    size_t         nbt_len,
    uint8_t**      out_body,
    size_t*        out_len);

// ── 方块批量（一条 UpdateSubChunkBlocks 承载一个子区块的多个改动） ──────
typedef struct holo_block_change {
    int32_t  x;
    int32_t  y;
    int32_t  z;
    uint32_t runtime_id;
    uint32_t flags;
    uint64_t sync_entity;
    uint32_t sync_message;
} holo_block_change;

HOLO_CORE_EXPORT uint32_t holo_core_packet_id_update_subchunk_blocks(void);

// sub_*: 子区块原点的方块坐标（x & ~15），待抓包对拍确认
HOLO_CORE_EXPORT int32_t holo_core_encode_update_subchunk_blocks(
    int32_t                  sub_x,
    int32_t                  sub_y,
    int32_t                  sub_z,
    const holo_block_change* blocks,
    size_t                   count,
    uint8_t**                out_body,
    size_t*                  out_len);

// ── Actor 元数据（姿态 / 冻结强度等） ───────────────────────────────────
// kind: 0 byte, 1 short, 2 int, 3 float, 4 long, 5 vec3
typedef struct holo_meta_entry {
    int32_t id;
    int32_t kind;
    int64_t ival;
    float   fval;
    float   x, y, z;
} holo_meta_entry;

HOLO_CORE_EXPORT uint32_t holo_core_packet_id_set_actor_data(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_set_actor_data(
    uint64_t               runtime_id,
    uint64_t               tick,
    const holo_meta_entry* entries,
    size_t                 count,
    uint8_t**              out_body,
    size_t*                out_len);

// ── 定位条（LocatorBar） ────────────────────────────────────────────────
// action: 0 None, 1 Add, 2 Remove, 3 Update
// flags:  Position=1, Visibility=2, TexturePath=4, IconSize=8, Color=16,
//         ClientPositionAuthority=32
typedef struct holo_waypoint {
    int64_t       msb;
    int64_t       lsb;
    uint8_t       action;
    uint32_t      flags;
    int32_t       has_visible;
    uint8_t       visible;
    int32_t       has_position;
    float         x, y, z;
    int32_t       dimension;
    int32_t       has_texture;
    const char*   texture;
    int32_t       has_icon_size;
    float         icon_w, icon_h;
    int32_t       has_color;
    int32_t       color;
    int32_t       has_authority;
    uint8_t       authority;
    int32_t       has_actor;
    int64_t       actor_uid;
} holo_waypoint;



// ── 复活（Respawn） ─────────────────────────────────────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_respawn(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_respawn(
    float     x,
    float     y,
    float     z,
    uint8_t   state,       // 0 ReadyToSpawn, 1 ReadyToSpawn?, 2 ClientReady (clamp 0..2)
    uint64_t  runtime_id,
    uint8_t** out_body,
    size_t*   out_len);

// ── 物品实体（item actor） ──────────────────────────────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_add_item_actor(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_remove_actor(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_move_actor_absolute(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_set_actor_motion(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_take_item_actor(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_add_item_actor(
    uint64_t  runtime_id,
    int64_t   unique_id,
    int16_t   item_id,        // 数字物品 id（见 platform/endstone/.../data/item_ids.json）
    uint16_t  stack_size,
    uint32_t  aux,
    float     x, float y, float z,
    float     vx, float vy, float vz,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_remove_actor(
    int64_t   unique_id,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_move_actor_absolute(
    uint64_t  runtime_id,
    uint8_t   flags,
    float     x, float y, float z,
    uint8_t   rotation_x,
    uint8_t   rotation_y,
    uint8_t   rotation_y_head,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_set_actor_motion(
    uint64_t  runtime_id,
    float     vx, float vy, float vz,
    uint64_t  tick,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_take_item_actor(
    uint64_t  item_runtime_id,
    uint64_t  actor_runtime_id,
    uint8_t** out_body,
    size_t*   out_len);

// ── 物品展示（fox 载体 + 主手装备） ──────────────────────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_add_actor(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_mob_equipment(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_add_actor_ex(
    const char* identifier,      // 例如 "minecraft:fox" / "minecraft:chest_minecart"
    const char* name,            // 非空则写进 ActorDataIDs::Name（容器标题用），可为 NULL
    uint64_t    runtime_id,
    int64_t     unique_id,
    float       x, float y, float z,
    float       yaw, float pitch,
    float       hitbox_width,
    float       hitbox_height,
    uint8_t**   out_body,
    size_t*     out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_mob_equipment(
    uint64_t  runtime_id,
    int16_t   item_id,
    uint16_t  stack_size,
    uint32_t  aux,
    uint8_t   slot,
    uint8_t   selected_slot,
    uint8_t   container_id,
    uint8_t** out_body,
    size_t*   out_len);

// ── 虚拟容器 ────────────────────────────────────────────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_container_open(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_container_close(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_inventory_content(void);

typedef struct holo_item_slot {
    int16_t     item_id;
    uint16_t    stack_size;
    uint32_t    aux;
    const char* user_data;      // 物品 NBT 字节（可由 holo_core_build_item_nbt 生成），可为 NULL
    size_t      user_data_len;
} holo_item_slot;

HOLO_CORE_EXPORT int32_t holo_core_encode_container_open(
    uint8_t   container_id,
    uint8_t   container_type,
    int32_t   x, int32_t y, int32_t z,
    int64_t   target_actor_id,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_container_close(
    uint8_t   container_id,
    uint8_t   container_type,
    uint8_t   server_initiated,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_inventory_content(
    uint32_t              container_id,
    const holo_item_slot* slots,
    size_t                count,
    uint8_t**             out_body,
    size_t*               out_len);

// 物品 NBT（Name/Count/Damage/WasPickedUp/Block/tag{display{Name,Lore},ench}）序列化成网络 NBT 字节
HOLO_CORE_EXPORT int32_t holo_core_build_item_nbt(
    const char* type,
    int32_t     count,
    int32_t     damage,
    const char* name,        // 可为 NULL
    const char* lore_csv,    // ';' 分隔，可为 NULL
    const char* ench_csv,    // "id:lvl;id:lvl"，可为 NULL
    int32_t     is_block,
    int32_t     block_version,
    uint8_t**   out_body,
    size_t*     out_len);

// ── 交易（UpdateTrade + offers NBT） ────────────────────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_update_trade(void);

typedef struct holo_trade_item {
    const char* type;
    int32_t     count;
    int32_t     damage;
    const char* name;
    const char* lore_csv;      // ';' 分隔
    const char* ench_csv;      // "id:lvl;id:lvl"
    int32_t     is_block;
    int32_t     block_version;
} holo_trade_item;

typedef struct holo_trade_offer {
    holo_trade_item buy_a;
    holo_trade_item buy_b;
    holo_trade_item sell;
    int32_t         tier;
    int32_t         max_uses;
    int32_t         uses;
    int32_t         net_id;
    int32_t         trader_exp;
    int32_t         demand;
    int32_t         reward_exp;
    float           price_multiplier_a;
    float           price_multiplier_b;
} holo_trade_offer;

HOLO_CORE_EXPORT int32_t holo_core_build_offers(
    const holo_trade_offer* offers,
    size_t                  count,
    const int32_t*          tier_exp,
    size_t                  tier_count,
    int32_t                 base_net_id,
    uint8_t**               out_body,
    size_t*                 out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_update_trade(
    uint8_t     container_id,
    uint8_t     container_type,
    int32_t     size,
    int32_t     tier,
    int64_t     entity_unique_id,
    int64_t     last_trading_player,
    const char* display_name,
    uint8_t     use_new_screen,
    uint8_t     use_economy_trade,
    const uint8_t* offers_nbt,
    size_t      offers_nbt_len,
    uint8_t**   out_body,
    size_t*     out_len);

// ── NPC 对话 ────────────────────────────────────────────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_npc_dialogue(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_npc_dialogue(
    uint64_t    npc_id,
    uint8_t     action,        // 0 Open, 1 Close
    const char* dialogue,
    const char* scene_name,
    const char* npc_name,
    const char* action_json,
    uint8_t**   out_body,
    size_t*     out_len);

// ── 玩家形态 NPC（PlayerList + AddPlayer） ──────────────────────────────
typedef struct holo_skin_desc {
    const char* id;
    const char* geometry;      // geometry JSON，可为 NULL
    const char* arm_size;      // "wide" / "slim"，可为 NULL
    uint32_t    width;
    uint32_t    height;
    const char* rgba;
    size_t      rgba_len;
} holo_skin_desc;

HOLO_CORE_EXPORT uint32_t holo_core_packet_id_player_list(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_add_player(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_player_list_add(
    const uint8_t*       uuid16,
    int64_t              actor_unique_id,
    const char*          name,
    const holo_skin_desc* skin,
    uint8_t**            out_body,
    size_t*              out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_player_list_remove(
    const uint8_t* uuid16,
    uint8_t**      out_body,
    size_t*        out_len);

HOLO_CORE_EXPORT int32_t holo_core_encode_add_player(
    uint64_t             runtime_id,
    const char*          name,
    const uint8_t*       uuid16,
    const holo_skin_desc* skin,
    float                x, float y, float z,
    float                yaw, float pitch,
    float                scale,
    const char*          nametag,
    uint8_t**            out_body,
    size_t*              out_len);

// ── 表情 / 骑乘 ─────────────────────────────────────────────────────────
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_emote(void);
HOLO_CORE_EXPORT uint32_t holo_core_packet_id_set_actor_link(void);

HOLO_CORE_EXPORT int32_t holo_core_encode_emote(
    uint64_t    runtime_id,
    const char* emote_id,
    uint32_t    length_ticks,
    uint8_t     flags,
    uint8_t**   out_body,
    size_t*     out_len);

// type: 0 None, 1 Riding, 2 Passenger
HOLO_CORE_EXPORT int32_t holo_core_encode_set_actor_link(
    uint8_t   type,
    int64_t   target_a_unique_id,
    int64_t   target_b_unique_id,
    uint8_t   immediate,
    uint8_t   passenger_initiated,
    float     vehicle_angular_velocity,
    uint8_t** out_body,
    size_t*   out_len);

HOLO_CORE_EXPORT void holo_free(void* ptr);

#ifdef __cplusplus
}
#endif
