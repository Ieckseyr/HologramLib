#include "BlockBatchExporter.h"

#include "BlockBatchManager.h"

#include "lse/LseBridge.h"

#include <cstdint>
#include <string>

namespace debugshape_export::blockbatch {

static constexpr const char* NAMESPACE = "HologramLib";

void BlockBatchExporter::exportAll() {
    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockBatchSet",
        [](std::string const& playerName, std::string const& packed, bool immediate) -> int64_t {
            return BlockBatchManager::getInstance().setMany(playerName, packed, immediate);
        }
    );

    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockBatchSetAll",
        [](int dimId, std::string const& packed, bool immediate) -> int64_t {
            return BlockBatchManager::getInstance().setManyAll(dimId, packed, immediate);
        }
    );

    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockBatchClear",
        [](std::string const& playerName, std::string const& packedPositions) -> bool {
            return BlockBatchManager::getInstance().clearMany(playerName, packedPositions);
        }
    );

    hologramlib::lse::exportAs(NAMESPACE, "blockBatchStats", []() -> std::string {
        return BlockBatchManager::getInstance().stats();
    });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "blockBatchSetNear",
        [](std::string const& playerName, std::string const& packed, float radius, bool invisible, bool immediate) -> int64_t {
            return blockbatch::BlockBatchManager::getInstance().setManyNear(playerName, packed, radius, invisible, immediate);
        }
    );

    hologramlib::lse::exportAs(NAMESPACE, "blockBatchSelfTest", [](std::string const& packed) -> std::string {
        return BlockBatchManager::getInstance().selfTest(packed);
    });

    hologramlib::lse::exportAs(NAMESPACE, "blockBatchProgress", [](int64_t batchId) -> int64_t {
        return BlockBatchManager::getInstance().progress(batchId);
    });
}

} // namespace debugshape_export::blockbatch
