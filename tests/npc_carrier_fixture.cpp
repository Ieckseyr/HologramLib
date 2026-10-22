// npc_carrier_fixture.cpp - 用 NpcCarrierPacket.h 产出合成载体包, 供逐字段对拍
//
// 校验: tests/check-npc-carrier.py 解码本程序产出的字节, 逐字段核对
//   · 与 BDS 26.40 schema 的字段顺序一致（uid/rid/identifier/pos/…/ActorData/props/links）
//   · ActorData 里 NPC 标记项齐全（Name=4 / HasNpc=39 / NpcData=40 / Actions=41 / InteractText=100）
//   · 位置 y = kCarrierY（世界下方 → 客户端看不到实体但头像照常）
// 并与真实 BDS NPC 生成包（logs/fullpkts/pkt13_*.bin）做交叉核对。
//
// 用法: npc_carrier_fixture <addactor.bin> <npcdialogue.bin>
#include "npcdialog/NpcCarrierPacket.h"

#include <sculk/protocol/codec/packet/NpcDialoguePacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace debugshape_export::npcdialog;

namespace {

void dump(std::string const& path, std::vector<std::byte> const& body, char const* what) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<char const*>(body.data()), static_cast<std::streamsize>(body.size()));
    std::cout << what << " = " << body.size() << " B\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: npc_carrier_fixture <addactor.bin> <npcdialogue.bin>\n";
        return 2;
    }

    constexpr std::uint64_t kUid = static_cast<std::uint64_t>(-850403524570LL);
    constexpr std::uint64_t kRid = 0x6E200001ULL;
    std::string const       npcName    = "任务发布员";
    std::string const       actionJson = R"([{"button_name":"进入下一层","data":[],"mode":0,"text":"","type":1}])";

    auto carrier = buildCarrierAddActor(
        kUid,
        kRid,
        "minecraft:npc",
        npcName,
        actionJson,
        sculk::protocol::Vec3{366.03f, kCarrierY, 286.24f},
        -18.28f
    );
    std::vector<std::byte>       carrierBody;
    sculk::protocol::BinaryStream carrierStream(carrierBody);
    carrier.write(carrierStream);
    dump(argv[1], carrierBody, "addactor");

    sculk::protocol::NpcDialoguePacket dialog;
    dialog.mNpcId      = kUid;
    dialog.mActionType = sculk::protocol::NpcDialoguePacket::ActionType::Open;
    dialog.mDialogue   = "§7这是第一层对话。";
    dialog.mSceneName  = "main";
    dialog.mNpcName    = npcName;
    dialog.mActionJSON = actionJson;
    std::vector<std::byte>        dialogBody;
    sculk::protocol::BinaryStream dialogStream(dialogBody);
    dialog.write(dialogStream);
    dump(argv[2], dialogBody, "npcdialogue");

    return 0;
}
