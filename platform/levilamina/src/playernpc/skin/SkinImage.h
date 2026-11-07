#pragma once

#include <cstdint>
#include <string>

namespace debugshape_export::skin {

enum class AlphaMode {
    Keep       = 0,
    Binary     = 1,
    BlendWhite = 2,
};

struct RgbaImage {
    std::uint32_t width{};
    std::uint32_t height{};
    std::string   rgba;
};

bool startup(std::string& error);
void shutdown();

bool loadPng(
    std::string const& path,
    AlphaMode          alphaMode,
    bool               forceOpaque,
    RgbaImage&         out,
    std::string&       error);

} // namespace debugshape_export::skin
