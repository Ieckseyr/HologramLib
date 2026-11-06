#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace debugshape_export::blockbatch {

struct Change {
    int         x{};
    int         y{};
    int         z{};
    std::string state;
};

// 一次批量替换的目标位置（用于 clear）
struct Pos {
    int x{};
    int y{};
    int z{};
};

// 单条待发包：已经编好的 body（不含包头）+ 目标玩家 uuid
struct PendingPacket {
    std::string uuid;
    std::string body;
};

class BlockBatchManager {
public:
    static BlockBatchManager& getInstance();

    void init();
    void shutdown();

    // 立刻返回 batchId；immediate=true 时当场发完，否则按 tick 预算排队
    int64_t setMany(std::string const& playerName, std::string const& packed, bool immediate);

    // 批量假方块 + 靠近后换成硫磺立方体（可穿过）；radius 是触发半径（格）
    int64_t setManyNear(std::string const& playerName, std::string const& packed, double radius, bool invisible, bool immediate);
    int64_t setManyAll(int dimId, std::string const& packed, bool immediate);
    bool    clearMany(std::string const& playerName, std::string const& packedPositions);

    int         progress(int64_t batchId) const;

    // 只做解析 / 分组 / 编码，不发包（自带测试与线上排查用）
    std::string selfTest(std::string const& packed) const;
    std::string stats() const;

    void onPlayerLeave(std::string const& playerName);
    void tick();

private:
    BlockBatchManager() = default;

    int64_t submit(std::string const& packed, std::vector<std::string> const& viewers, bool immediate);
    void    flushLocked(std::size_t budget);

    struct Impl;
    Impl* mImpl{};
};

} // namespace debugshape_export::blockbatch
