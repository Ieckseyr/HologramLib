// trade_packet_fixture.cpp - 构造完整的 UpdateTrade 包（含全部包级字段 + Offers NBT）, 落盘供逐字节对拍
//
// 与 trade_offers_fixture.cpp 的区别: 那个只验 Data 段, 这个验**整包** ——
// 包级字段（容器 id/类型、size、tier、entityUniqueId、lastTradingPlayer、displayName、两个 flag）
// 的取值与顺序也要与 BDS 抓包完全一致。字段顺序按 .cache/schema-1.26.40/UpdateTradePacket.json。
//
// 用法: trade_packet_fixture <输出路径>
#include "trade_reference_offers.h"

#include <sculk/protocol/codec/packet/UpdateTradePacket.hpp>
#include <sculk/protocol/utility/BinaryStream.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <vector>

using namespace debugshape_export;
using namespace debugshape_export::tests;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: trade_packet_fixture <output.bin>\n";
        return 2;
    }

    // 取值全部照抄参考样本, 这样产出应当与抓包逐字节一致
    sculk::protocol::UpdateTradePacket packet;
    packet.mContainerId       = static_cast<sculk::protocol::ContainerID>(kRefContainerId);
    packet.mContainerType     = static_cast<sculk::protocol::ContainerType>(kRefContainerType);
    packet.mSize              = kRefSize;
    packet.mTier              = kRefTier;
    packet.mEntityUniqueId    = kRefEntityUniqueId;
    packet.mLastTradingPlayer = kRefLastTrader;
    packet.mDisplayName       = kRefDisplayName;
    packet.mUseNewTradeScreen = true;
    packet.mUseEconomyTrade   = true;
    packet.mOffers            = trade::buildOffers(referenceOffers());

    std::vector<std::byte>        out;
    sculk::protocol::BinaryStream stream(out);
    packet.write(stream); // 不含包头（发送时由发送方补）

    if (out.empty()) {
        std::cerr << "UpdateTrade serialization produced no bytes\n";
        return 1;
    }
    std::ofstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "cannot open output: " << argv[1] << '\n';
        return 1;
    }
    file.write(reinterpret_cast<char const*>(out.data()), static_cast<std::streamsize>(out.size()));
    std::cout << "update_trade payload bytes=" << out.size() << '\n';
    return file ? 0 : 1;
}
