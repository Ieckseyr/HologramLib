// ItemActorManager.h - 协议层掉落物（26.51.6）
//
// 客户端凭空出现一只掉落物实体：服务端没有它 —— 不参与拾取判定、不进存档、没有物理。
// 三件事: AddItemActor（生成）、SetActorMotion（随时改速度与方向）、TakeItemActor（播放拾取）。
//
// 为什么用 BDS 的包类而不是手写: AddItemActor 里带 NetworkItemStackDescriptor，物品的 NBT 方言
// 和 sculk 那套不同（见 fakeinv/FakeInventoryPackets.h 的说明），手写必错。其余包用 sculk 的。
#pragma once

#include "core/Registry.h"
#include "core/VirtualActorIds.h"    // actorIds::itemActor 号段
#include "hologramlib/HologramLib.h" // ItemActorSpec / IItemActor

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace debugshape_export {

class ItemActorManager {
public:
    static ItemActorManager& getInstance();

    int64_t spawn(hologramlib::ItemActorSpec const& spec);
    bool    setMotion(int64_t id, float vx, float vy, float vz);
    bool    moveTo(int64_t id, float x, float y, float z);
    bool    take(int64_t id, std::string const& playerName);
    bool    destroy(int64_t id);
    void    destroyAll();

    [[nodiscard]] bool                 exists(int64_t id) const;
    [[nodiscard]] std::vector<int64_t> getAllIds() const;

    // 玩家离开/换维度时不需要特别处理: 掉落物按维度发, 客户端走远了自然清。
    void shutdown();

private:
    ItemActorManager() = default;

    struct Actor {
        std::uint64_t uniqueId{0};
        std::uint64_t runtimeId{0};
        std::string   itemId;
        int           aux{0};
        int           count{1};
        float         x{0}, y{0}, z{0};
        int           dimension{0};
    };

    // 发给某维度的全部玩家（掉落物只在该维度可见）
    static void sendToDimension(int dimension, std::string const& bytes);

    mutable std::mutex                  mMutex;
    core::Registry<Actor, core::NoLock> mActors;
    std::uint64_t                       mNextUniqueId{0x7000000000000001ULL};
    std::uint64_t                       mNextRuntimeId{core::actorIds::itemActor.first};
};

} // namespace debugshape_export
