// trade_offers_fixture.cpp - 用 BDS 原始交易数据构造 Offers NBT, 落盘供逐字节对拍
//
// 数据来源: logs/fullpkts/pkt80_194951_007.bin（26.40 原生屠夫村民的一次交易界面）
// 的逐字段解码结果。校验: tests/check-trade-offers.py 把本程序产出的字节与抓包里
// Data 段的原始字节逐字节比较 —— 一致才说明 trade::buildOffers 与 BDS 输出等价。
//
// 用法: trade_offers_fixture <输出路径>
#include "trade/TradeOfferNbt.h"

#include <sculk/protocol/utility/BinaryStream.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <vector>

using namespace debugshape_export::trade;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: trade_offers_fixture <output.bin>\n";
        return 2;
    }

    std::vector<TradeOffer> offers{
// 由 logs/fullpkts/pkt80_194951_007.bin 的解码结果生成 (屠夫村民的真实 offers, 含 Block 键)
        {"minecraft:porkchop", 7, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 0, 16, 0, 3676, 2, 0, 0.05f, 0.0f, true},
        {"minecraft:emerald", 1, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:rabbit_stew", 1, 0, "", {}, false, 0, 0, 12, 0, 3677, 1, 0, 0.05f, 0.0f, true},
        {"minecraft:coal", 15, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 1, 16, 0, 3678, 10, 0, 0.05f, 0.0f, true},
        {"minecraft:emerald", 1, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:cooked_chicken", 8, 0, "", {}, false, 0, 1, 16, 0, 3679, 5, 0, 0.05f, 0.0f, true},
        {"minecraft:beef", 10, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 2, 16, 0, 3680, 20, 0, 0.05f, 0.0f, true},
        {"minecraft:dried_kelp_block", 10, 0, "", {}, true, 18168865, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 3, 12, 0, 3681, 30, 0, 0.05f, 0.0f, true},
        {"minecraft:sweet_berries", 10, 0, "", {}, false, 0, "", 0, 0, "", {}, false, 0, "minecraft:emerald", 1, 0, "", {}, false, 0, 4, 12, 0, 3682, 30, 0, 0.05f, 0.0f, true},
    };

    auto                   root = buildOffers(offers);
    std::vector<std::byte> out;
    sculk::protocol::BinaryStream stream(out);
    root.write(stream); // 根 compound: 0x0a + 空名 + 条目 + 0x00 —— 即 UpdateTrade 的 Data 段

    if (out.empty()) {
        std::cerr << "offers serialization produced no bytes\n";
        return 1;
    }
    std::ofstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "cannot open output: " << argv[1] << '\n';
        return 1;
    }
    file.write(reinterpret_cast<char const*>(out.data()), static_cast<std::streamsize>(out.size()));
    std::cout << "offers bytes=" << out.size() << '\n';
    return file ? 0 : 1;
}
