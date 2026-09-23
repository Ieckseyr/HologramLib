// ContainerResponse.h - 虚拟容器"可交互"模式用的应答包（手写, sculk）
//
// 客户端发来的 ItemStackRequest 必须被应答, 否则它会卡在预测态（实测: 物品放不进去、格子恒红）。
// 虚拟容器本来**服务端并不存在**, 所以默认是"只回传、不拦"（放行给 BDS, 让它自己失败并把客户端
// 预测撤回 —— 物品闪回原位, 与参考实现 GMLIB 一致）。
//
// 但 `ContainerMenuSpec::interactive = true` 时我们要**真的接受**客户端在容器内部的移动/交换:
// 那就得自己回一条 **Success**。Success 的线上形态比失败多一段 containers 数组（sculk
// ItemStackResponseInfo::write 会写它）; 这里给空数组 = "没有槽位更正", 客户端保留自己的预测
// —— 正是我们要的: 物品留在新格子, 而库这边也把同样的改动记进条目表。
#pragma once

#include "SculkPacketSend.h"

#include <cstdint>
#include <vector>

#include <mc/world/actor/player/Player.h>

#include <sculk/protocol/codec/inventory/item/ItemStackResponse.hpp>
#include <sculk/protocol/codec/packet/ItemStackResponsePacket.hpp>

namespace debugshape_export::container {

// 回一条"这次请求成功了"（requestIds = 本次包里我们接管的那几条请求的 clientRequestId）
inline void sendRequestSuccess(::Player& player, std::vector<std::int32_t> const& requestIds) {
    if (requestIds.empty()) return;
    sculk::protocol::ItemStackResponsePacket packet;
    for (auto const id : requestIds) {
        sculk::protocol::ItemStackResponseInfo info;
        info.mResult    = sculk::protocol::ItemStackNetResult::Success;
        info.mRequestId = id;
        packet.mResponse.mResponses.push_back(std::move(info));
    }
    sendSculkPacketToPlayer(player, packet);
}

} // namespace debugshape_export::container
