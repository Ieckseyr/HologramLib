#pragma once

#include <hologramlib/HologramLib.h>

#include <cstdint>
#include <string>

namespace debugshape_export::container {

int64_t previewAt(std::string const& playerName, int dim, int x, int y, int z, std::string const& title);

int fillSpecFromBlock(
    int                             dim,
    int                             x,
    int                             y,
    int                             z,
    hologramlib::ContainerMenuSpec& spec,
    bool                            allowChunkLoad);

} // namespace debugshape_export::container
