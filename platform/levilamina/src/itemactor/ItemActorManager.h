#pragma once

#include "core/Registry.h"
#include "core/VirtualActorIds.h"
#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace debugshape_export {

// 虚拟掉落物
class ItemActorManager {
public:
    static ItemActorManager& getInstance();

    int64_t spawn(hologramlib::ItemActorSpec const& spec);
    bool    setMotion(int64_t id, float vx, float vy, float vz);
    bool    moveTo(int64_t id, float x, float y, float z);
    bool    take(int64_t id, std::string const& playerName);
    bool    destroy(int64_t id);
    void    destroyAll();

    [[nodiscard]] bool exists(int64_t id) const;

    [[nodiscard]] std::uint64_t        uniqueIdOf(int64_t id) const;
    [[nodiscard]] std::vector<int64_t> getAllIds() const;

    std::uint64_t runtimeIdOf(int64_t id) const;

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

    static void sendToDimension(int dimension, std::string const& bytes);

    mutable std::mutex                  mMutex;
    core::Registry<Actor, core::NoLock> mActors;
    std::uint64_t                       mNextUniqueId{0x7000000000000001ULL};
    std::uint64_t                       mNextRuntimeId{core::actorIds::itemActor.first};
};

} // namespace debugshape_export
