#pragma once

#include "core/Registry.h"

#include "hologramlib/HologramLib.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace debugshape_export {

// 硫磺立方体展示
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
        int64_t entityId{-1};
    };

    core::Registry<Entry> mEntries;
};

hologramlib::ISulfurDisplay& sulfurDisplayAdapter();

} // namespace debugshape_export
