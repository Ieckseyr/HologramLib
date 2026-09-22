// SulfurDisplayExporter.cpp - 硫磺立方体展示 LSE 导出实现
//
// 脚本侧用法:
//   const id = sulfurCreate(100.5, 65, -200.5, 0, "minecraft:bookshelf", 1, 0, "§6书架", "regular", true);
//   sulfurSetArchetype(id, "sticky");                       // 换外观档位（不用重建）
//   sulfurSetBlock(id, "minecraft:gold_block", 1, 0, "");   // 换吞下去的方块
//   sulfurDestroy(id);
#include "SulfurDisplayExporter.h"
#include "SulfurDisplayManager.h"

#include "lse/LseBridge.h"

#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void SulfurDisplayExporter::exportAll() {
    auto& mgr = SulfurDisplayManager::getInstance();

    // sulfurCreate(x, y, z, dim, blockType, blockCount, blockDamage, blockName, archetype, invisible) -> id
    //   archetype: 外观档位 minecraft:sulfur_cube_archetype（"regular"/"sticky"/"hot"/... ; 空串 = 不下发）
    //   invisible: 立方体本体是否隐身（默认 true —— 实测隐身时"吞下去的方块"照常渲染, 于是只看到那个方块）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "sulfurCreate",
        [&mgr](
            float              x,
            float              y,
            float              z,
            int                dim,
            std::string const& blockType,
            int                blockCount,
            int                blockDamage,
            std::string const& blockName,
            std::string const& archetype,
            bool               invisible
        ) -> int64_t {
            hologramlib::SulfurDisplaySpec spec;
            spec.x         = x;
            spec.y         = y;
            spec.z         = z;
            spec.dim       = dim;
                    spec.block.type   = blockType;
            spec.block.count  = blockCount < 1 ? 1 : blockCount;
            spec.block.damage = blockDamage;
            spec.block.name   = blockName;
            spec.archetype = archetype;
            spec.invisible = invisible;
            return mgr.create(spec);
        });

    // sulfurSetBlock(id, type, count, damage, name) -> bool（换吞下去的方块/物品; 会重建一次实体）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "sulfurSetBlock",
        [&mgr](
            int64_t            id,
            std::string const& type,
            int                count,
            int                damage,
            std::string const& name
        ) -> bool {
            hologramlib::ContainerMenuItem item;
            item.type   = type;
            item.count  = count < 1 ? 1 : count;
            item.damage = damage;
            item.name   = name;
            return mgr.setBlock(id, item);
        });

    // sulfurSetArchetype(id, archetype) -> bool（空串 = 清掉属性）
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurSetArchetype", [&mgr](int64_t id, std::string const& archetype) -> bool {
            return mgr.setArchetype(id, archetype);
        });

    // sulfurSetInvisible(id, on) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurSetInvisible", [&mgr](int64_t id, bool on) -> bool {
            return mgr.setInvisible(id, on);
        });

    // sulfurSetScale(id, scale) -> bool（0.0625~10, 自动钳制）
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurSetScale", [&mgr](int64_t id, float scale) -> bool {
            return mgr.setScale(id, scale);
        });

    // sulfurDestroy(id) -> bool / sulfurDestroyAll() -> void
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurDestroy", [&mgr](int64_t id) -> bool { return mgr.destroy(id); });
    hologramlib::lse::exportAs(NAMESPACE, "sulfurDestroyAll", [&mgr]() -> void { mgr.destroyAll(); });

    // sulfurExists(id) -> bool / sulfurGetIds() -> [i] / sulfurGetEntityId(id) -> i（诊断: 背后的自定义实体 id）
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurExists", [&mgr](int64_t id) -> bool { return mgr.exists(id); });
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurGetIds", [&mgr]() -> std::vector<int64_t> { return mgr.getAllIds(); });
    hologramlib::lse::exportAs(
        NAMESPACE, "sulfurGetEntityId", [&mgr](int64_t id) -> int64_t { return mgr.entityIdOf(id); });
}

} // namespace debugshape_export
