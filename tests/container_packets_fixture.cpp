// container_packets_fixture.cpp - 构造虚拟容器用到的全部包, 落盘供离线逐字段核对
//
// 覆盖 GMLIB 方案的四步:
//   ① UpdateBlockPacket      客户端侧箱子方块
//   ② BlockActorDataPacket   方块实体 NBT（含 Items: 单箱子 27 格 / 大箱子两个配对半区）
//   ③ ContainerOpenPacket    开容器（类型 0 = Container + 方块坐标 + 目标实体 -1）
//   ④ ContainerClosePacket   关容器
//
// 用的是库内同一份 src/container/ContainerPackets.h —— 测的是真代码, 不是重写一遍的副本。
// 全部输入都是固定值, 所以产出是确定性的; 其中 container_open.bin 的取值特意照抄一条
// **真实抓包**（logs/fullpkts/pkt46_165803_011.bin）, 于是它可以与抓包做整包逐字节对拍。
//
// 用法: container_packets_fixture <输出目录>
#include "container/ContainerPackets.h"

#include <sculk/protocol/utility/BinaryStream.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace debugshape_export;

namespace {

// 与抓包 pkt46_165803_011.bin 完全一致的输入（容器 102 号, 方块 (376,-66,288)）
constexpr int kRefContainerId = 102;
constexpr int kRefX           = 376;
constexpr int kRefY           = -66;
constexpr int kRefZ           = 288;

// UpdateBlock 的运行 id 需要查 BDS 方块表, 离线拿不到 → 用占位值, 只验结构（pos/id/flags/layer）
constexpr std::uint32_t kPlaceholderBlockId = 12345;

std::vector<hologramlib::ContainerMenuItem> smallChestItems() {
    std::vector<hologramlib::ContainerMenuItem> items(27);
    items[0]  = {"minecraft:paper", 1, 0, "§e任务目标: 清剿僵尸", {"§7击杀 5 只僵尸", "§8点我看看有没有回调"}};
    items[2]  = {"minecraft:emerald", 1, 0, "§a任务目标: 收集绿宝石", {"§7收集 16 个绿宝石"}};
    items[13] = {"minecraft:barrier", 1, 0, "§c关闭", {"§7点这里关掉列表"}};
    return items;
}

std::vector<hologramlib::ContainerMenuItem> bigChestItems() {
    std::vector<hologramlib::ContainerMenuItem> items(54);
    for (int slot = 0; slot < 54; ++slot) {
        if (slot % 9 == 0) {
            items[static_cast<std::size_t>(slot)] =
                {"minecraft:stone", 64, 0, "§7第 " + std::to_string(slot) + " 格", {}};
        }
    }
    return items;
}

template <typename PacketT>
bool writePacket(std::filesystem::path const& path, PacketT const& packet) {
    std::vector<std::byte>        body;
    sculk::protocol::BinaryStream stream(body);
    packet.write(stream); // 不含包头（发送时由发送方补）

    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<char const*>(body.data()), static_cast<std::streamsize>(body.size()));
    std::cout << path.filename().string() << " bytes=" << body.size() << '\n';
    return static_cast<bool>(file);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: container_packets_fixture <output-dir>\n";
        return 2;
    }
    std::filesystem::path const dir = argv[1];
    std::error_code             ec;
    std::filesystem::create_directories(dir, ec);

    // ① 客户端侧箱子方块（单箱子与大箱子用同一个方块 id, 只有坐标不同）
    if (!writePacket(dir / "update_block.bin", container::makeUpdateBlock(kRefX, kRefY, kRefZ, kPlaceholderBlockId))) {
        return 1;
    }

    // ② 方块实体 NBT
    if (!writePacket(
            dir / "block_actor_small.bin",
            container::makeBlockActorData(
                kRefX,
                kRefY,
                kRefZ,
                container::buildChestNbt(kRefX, kRefY, kRefZ, false, "任务列表", 3, smallChestItems())
            )
        )) {
        return 1;
    }
    auto const bigItems = bigChestItems();
    if (!writePacket(
            dir / "block_actor_big_lead.bin",
            container::makeBlockActorData(
                kRefX,
                kRefY,
                kRefZ,
                container::buildChestNbt(kRefX, kRefY, kRefZ, false, "大箱子", 6, bigItems)
            )
        )) {
        return 1;
    }
    if (!writePacket(
            dir / "block_actor_big_second.bin",
            container::makeBlockActorData(
                kRefX + 1,
                kRefY,
                kRefZ,
                container::buildChestNbt(kRefX + 1, kRefY, kRefZ, true, "大箱子", 6, bigItems)
            )
        )) {
        return 1;
    }

    // ③ 开容器（字节级锚定到真实抓包）
    if (!writePacket(dir / "container_open.bin", container::makeContainerOpen(kRefContainerId, kRefX, kRefY, kRefZ))) {
        return 1;
    }

    // ④ 关容器
    if (!writePacket(dir / "container_close.bin", container::makeContainerClose(kRefContainerId))) {
        return 1;
    }
    return 0;
}
