#include "container/ContainerPreview.h"

#include "container/ContainerMenuManager.h"

#include <ll/api/service/Bedrock.h>

#include <mc/world/Container.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/level/BlockPos.h>
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/ChunkPos.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/block/actor/ChestBlockActor.h>
#include <mc/world/level/chunk/ChunkSource.h>
#include <mc/world/level/dimension/Dimension.h>
#include <mc/world/level/dimension/DimensionType.h>

#include <algorithm>
#include <cmath>

namespace debugshape_export::container {

namespace {

::Container* containerOf(::BlockActor* be) {
    if (be == nullptr) {
        return nullptr;
    }
    if (auto* chest = dynamic_cast<::ChestBlockActor*>(be)) {
        return chest->getContainer();
    }
    if (auto* self = dynamic_cast<::Container*>(be)) {
        return self;
    }
    return nullptr;
}

} // namespace

int fillSpecFromBlock(
    int                             dim,
    int                             x,
    int                             y,
    int                             z,
    hologramlib::ContainerMenuSpec& spec,
    bool                            allowChunkLoad) {
    auto level = ::ll::service::getLevel();
    if (!level) {
        return -1;
    }

    auto         dimRef    = level->getDimension(::DimensionType{dim});
    ::Dimension* dimension = dimRef.lock().get();
    if (dimension == nullptr) {
        return -1;
    }

    ::BlockSource& bs = dimension->getBlockSourceFromMainChunkSource();
    ::BlockPos     pos{x, y, z};

    // 未加载区块拿不到方块实体，按需加载一次（readOnly，不写回存档）
    ::BlockActor* be = bs.getBlockEntity(pos);
    if (be == nullptr && allowChunkLoad) {

        auto& cs = bs.getChunkSource();
        cs.getOrLoadChunk(::ChunkPos{pos}, ::ChunkSource::LoadMode::None, true);
        be = bs.getBlockEntity(pos);
    }
    if (be == nullptr) {
        return -2;
    }

    ::Container* container = containerOf(be);
    if (container == nullptr) {
        return -3;
    }

    int const size = container->getContainerSize();
    if (size <= 0) {
        return -3;
    }

    int rows  = std::clamp(static_cast<int>(std::ceil(static_cast<double>(size) / 9.0)), 3, 6);
    spec.rows = rows;
    spec.items.assign(static_cast<std::size_t>(rows) * 9, hologramlib::ContainerMenuItem{});

    for (int i = 0; i < size && i < rows * 9; ++i) {
        ::ItemStack const& stack = container->getItem(i);
        if (stack.isNull() || stack.mCount == 0) {
            continue;
        }
        auto& item  = spec.items[static_cast<std::size_t>(i)];
        item.type   = stack.getTypeName();
        item.count  = static_cast<int>(stack.mCount);
        item.damage = static_cast<int>(stack.getAuxValue());
        item.name   = stack.getCustomName();
        item.lore   = stack.getCustomLore();
    }
    return size;
}

int64_t previewAt(
    std::string const& playerName,
    int                dim,
    int                x,
    int                y,
    int                z,
    std::string const& title) {
    auto level = ::ll::service::getLevel();
    if (!level) {
        return -1;
    }
    if (level->getPlayer(playerName) == nullptr) {
        return -1;
    }

    hologramlib::ContainerMenuSpec spec;
    spec.title = title.empty() ? std::string{"容器预览"} : title;

    int const filled = fillSpecFromBlock(dim, x, y, z, spec, true);
    if (filled < 0) {
        return filled;
    }

    return ContainerMenuManager::getInstance().open(playerName, spec);
}

} // namespace debugshape_export::container
