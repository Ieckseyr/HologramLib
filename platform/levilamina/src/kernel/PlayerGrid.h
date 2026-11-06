#pragma once

#include <ll/api/service/Bedrock.h>
#include <mc/platform/UUID.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace debugshape_export {
namespace core {

inline std::uint64_t serverTick() {
    auto level = ::ll::service::getLevel();
    return level ? level->getCurrentTick().tickID : 0;
}

inline ::Player* findOnlinePlayer(mce::UUID const& uuid) {
    auto level = ::ll::service::getLevel();
    return level ? level->getPlayer(uuid) : nullptr;
}

class PlayerGrid {
public:
    using PlayerList = std::vector<::Player*>;

    static constexpr float kCellSize = 64.0f;

    [[nodiscard]] bool ensureFresh() {
        std::uint64_t const now = serverTick();
        if (mBuilt && mStamp == now)
            return mHasLevel;
        mBuilt    = true;
        mStamp    = now;
        mHasLevel = rebuild();
        return mHasLevel;
    }

    template <class Fn>
    void forEachNear(int dim, float x, float z, float radius, Fn&& fn) const {
        auto const dimIt = mCells.find(dim);
        if (dimIt == mCells.end())
            return;

        auto const& cells = dimIt->second;
        int const   x0    = cellOf(x - radius);
        int const   x1    = cellOf(x + radius);
        int const   z0    = cellOf(z - radius);
        int const   z1    = cellOf(z + radius);
        for (int cx = x0; cx <= x1; ++cx) {
            for (int cz = z0; cz <= z1; ++cz) {
                auto const it = cells.find(pack(cx, cz));
                if (it == cells.end())
                    continue;
                for (auto* player : it->second)
                    fn(*player);
            }
        }
    }

    [[nodiscard]] PlayerList const& inDim(int dim) const {
        static PlayerList const kEmpty;
        auto                    it = mByDim.find(dim);
        return it == mByDim.end() ? kEmpty : it->second;
    }

private:
    using CellMap  = std::unordered_map<std::uint64_t, PlayerList>;
    using DimCells = std::unordered_map<int, CellMap>;
    using DimMates = std::unordered_map<int, PlayerList>;

    bool rebuild() {
        mCells.clear();
        mByDim.clear();

        auto level = ::ll::service::getLevel();
        if (!level)
            return false;

        level->forEachPlayer([this](::Player& player) {
            auto const& pos = player.getPosition();
            int const   dim = static_cast<int>(player.getDimensionId());
            mCells[dim][pack(cellOf(pos.x), cellOf(pos.z))].push_back(&player);
            mByDim[dim].push_back(&player);
            return true;
        });
        return true;
    }

    static int cellOf(float v) { return static_cast<int>(std::floor(v / kCellSize)); }

    static std::uint64_t pack(int cx, int cz) {
        return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 32) | static_cast<std::uint32_t>(cz);
    }

    DimCells      mCells;
    DimMates      mByDim;
    std::uint64_t mStamp{0};
    bool          mBuilt{false};
    bool          mHasLevel{false};
};

} // namespace core
} // namespace debugshape_export
