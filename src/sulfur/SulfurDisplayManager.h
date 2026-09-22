// SulfurDisplayManager.h - 硫磺立方体展示域（1.23.0）
//
// 实现要点: **全部委托给 CustomEntityManager** —— 立方体就是一只自定义实体
// （identifier = minecraft:sulfur_cube）, "吞下去的方块" = 它的主手装备（MobEquipmentPacket,
// 行为包里立方体正是靠 slot.weapon.mainhand 拿方块）, "外观档位" = changeMobProperty 同步的
// minecraft:sulfur_cube_archetype 属性, 隐身 = 实体 flags 的隐身位。本域只负责: 生成/销毁 与
// 展示 id → 实体 id 的映射, 所以这里没有新的钩子、包体只有那个属性包
// （sulfur/SulfurDisplayPackets.h）, 装备与隐身走库里既有的通路。
#pragma once

#include "hologramlib/HologramLib.h" // SulfurDisplaySpec

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace debugshape_export {

class SulfurDisplayManager {
public:
    static SulfurDisplayManager& getInstance();

    int64_t create(hologramlib::SulfurDisplaySpec const& spec);
    bool    setBlock(int64_t id, hologramlib::ContainerMenuItem const& item);
    bool    setArchetype(int64_t id, std::string const& archetype);
    bool    setInvisible(int64_t id, bool on);
    bool    setScale(int64_t id, float scale);
    bool    destroy(int64_t id);
    void    destroyAll();

    [[nodiscard]] bool                 exists(int64_t id) const;
    [[nodiscard]] std::vector<int64_t> getAllIds() const;
    [[nodiscard]] int64_t              entityIdOf(int64_t id) const;

private:
    SulfurDisplayManager() = default;

    struct Entry {
        int64_t entityId{-1}; // 背后的自定义实体（立方体）
    };

    // 持锁取映射; 不存在返回 nullptr
    Entry const* findLocked(int64_t id) const;

    mutable std::mutex                 mMutex;
    std::unordered_map<int64_t, Entry> mEntries;
    int64_t                            mNextId{1};
};

// 公开接口适配（IHologramLib::sulfurDisplays()）
hologramlib::ISulfurDisplay& sulfurDisplayAdapter();

} // namespace debugshape_export
