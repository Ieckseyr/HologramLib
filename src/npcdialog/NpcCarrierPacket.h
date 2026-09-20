// NpcCarrierPacket.h - 合成 NPC 载体的包构造（纯 sculk 协议, 不依赖 BDS/LeviLamina）
//
// 抽出来单独放的原因: 这里是"客户端能不能认出 NPC 并弹出对话界面"的全部关键数据,
// 必须能离线逐字节对拍 —— tests/npc_carrier_fixture.cpp + tests/check-npc-carrier.py
// 用同一份构造代码产出字节, 再按 BDS 26.40 真实 NPC 生成包（logs/fullpkts/pkt13_*.bin）
// 的结构逐字段校验。改动这里务必重跑那个测试。
//
// 字段顺序与 BDS 2168 schema 一致（sculk::protocol 按 schema 实现）:
//   uniqueId → runtimeId → identifier → pos → vel → rot → yHead → yBody → 属性表 → ActorData → 属性同步 → 链接
#pragma once

#include <sculk/protocol/codec/actor/ActorDataIDs.hpp>
#include <sculk/protocol/codec/actor/MetaData.hpp>
#include <sculk/protocol/codec/math/Vec2.hpp>
#include <sculk/protocol/codec/math/Vec3.hpp>
#include <sculk/protocol/codec/packet/AddActorPacket.hpp>

#include <cstdint>
#include <string>

namespace debugshape_export::npcdialog {

// 载体所在高度: 世界下方。客户端因此看不到实体本身, 但 NPC 界面里的头像照常渲染。
// 不要改用"隐形"标志位 —— 实测那样头像会一起消失（参考实现同样靠远置处理）。
inline constexpr float kCarrierY = -66.0f;

// 客户端识别 NPC / 渲染对话界面所需的 NpcData 元数据（头像取景 + 皮肤选择表）。
// 结构同 GMLIB-Kobe 参考实现与真实 BDS NPC（实测 1042B, 差异仅皮肤表条目）。
inline std::string const& npcDataJson() {
    static std::string const json =
        R"JSON({"picker_offsets":{"scale":[1.70,1.70,1.70],"translate":[0,20,0]},"portrait_offsets":{"scale":[1.750,1.750,1.750],"translate":[-7,50,0]},"skin_list":[{"variant":0},{"variant":1},{"variant":2},{"variant":3},{"variant":4},{"variant":5},{"variant":6},{"variant":7},{"variant":8},{"variant":9},{"variant":10},{"variant":11},{"variant":12},{"variant":13},{"variant":14},{"variant":15},{"variant":16},{"variant":17},{"variant":18},{"variant":19},{"variant":25},{"variant":26},{"variant":27},{"variant":28},{"variant":29},{"variant":30},{"variant":31},{"variant":32},{"variant":33},{"variant":34},{"variant":20},{"variant":21},{"variant":22},{"variant":23},{"variant":24},{"variant":35},{"variant":36},{"variant":37},{"variant":38},{"variant":39},{"variant":40},{"variant":41},{"variant":42},{"variant":43},{"variant":44},{"variant":50},{"variant":51},{"variant":52},{"variant":53},{"variant":54},{"variant":45},{"variant":46},{"variant":47},{"variant":48},{"variant":49},{"variant":55},{"variant":56},{"variant":57},{"variant":58},{"variant":59}]})JSON";
    return json;
}

// 合成载体的 AddActor。identifier 必须是 NPC 家族（客户端把对话界面与该实体类型绑定）。
inline sculk::protocol::AddActorPacket buildCarrierAddActor(
    std::uint64_t               uniqueId,
    std::uint64_t               runtimeId,
    std::string const&          identifier,
    std::string const&          npcName,
    std::string const&          actionJson,
    sculk::protocol::Vec3 const position,
    float                       yaw
) {
    sculk::protocol::AddActorPacket packet;
    packet.mActorUniqueId  = static_cast<std::int64_t>(uniqueId);
    packet.mActorRuntimeId = runtimeId;
    packet.mIdentifier     = identifier.empty() ? std::string{"minecraft:npc"} : identifier;
    packet.mPosition       = position;
    packet.mVelocity       = sculk::protocol::Vec3{0.0f, 0.0f, 0.0f};
    packet.mRotation       = sculk::protocol::Vec2{yaw, 0.0f};
    packet.mYHeadRotation  = 0.0f;
    packet.mYBodyRotation  = 0.0f;

    // 5 条 ActorData = 参考实现的最小充分集合（真实 BDS NPC 有 73 条, 但客户端只靠这几条
    // 就能渲染 NPC 界面与按钮）。Actions 是客户端实际读取按钮的来源。
    auto& items = packet.mMetaData.mDataItems;
    items.reserve(5);
    items.push_back({sculk::protocol::ActorDataIDs::Name, std::string(npcName)});
    items.push_back({sculk::protocol::ActorDataIDs::HasNpc, static_cast<std::uint8_t>(1)});
    items.push_back({sculk::protocol::ActorDataIDs::NpcData, npcDataJson()});
    items.push_back({sculk::protocol::ActorDataIDs::Actions, actionJson});
    items.push_back({sculk::protocol::ActorDataIDs::InteractText, std::string(npcName)});
    return packet;
}

} // namespace debugshape_export::npcdialog
