#include "SkinImage.h"

#include <Windows.h>
#include <gdiplus.h>
#include <objidl.h>
#include <propidl.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <memory>

namespace debugshape_export::skin {

namespace {

struct GdiplusSession {
    ULONG_PTR token{};
    bool      ok{false};

    ~GdiplusSession() {
        if (ok)
            Gdiplus::GdiplusShutdown(token);
    }
};

GdiplusSession& session() {
    static GdiplusSession instance;
    return instance;
}

bool isPowerOfTwo(std::uint32_t value) {
    return value >= 64 && (value & (value - 1)) == 0;
}

std::uint8_t blendOnWhite(std::uint8_t channel, std::uint8_t alpha) {
    return static_cast<std::uint8_t>((channel * alpha + 255 * (255 - alpha)) / 255);
}

void applyAlpha(std::uint8_t& r, std::uint8_t& g, std::uint8_t& b, std::uint8_t& a, AlphaMode mode, bool forceOpaque) {
    switch (mode) {
    case AlphaMode::Binary:
        a = a >= 128 ? 255 : 0;
        break;
    case AlphaMode::BlendWhite:
        r = blendOnWhite(r, a);
        g = blendOnWhite(g, a);
        b = blendOnWhite(b, a);
        a = 255;
        break;
    case AlphaMode::Keep:
    default:
        if (forceOpaque && a != 0)
            a = 255;
        break;
    }
}

} // namespace

bool startup(std::string& error) {
    auto& s = session();
    if (s.ok)
        return true;

    Gdiplus::GdiplusStartupInput input{};
    if (Gdiplus::GdiplusStartup(&s.token, &input, nullptr) != Gdiplus::Ok) {
        error = "GDI+ init failed";
        return false;
    }
    s.ok = true;
    return true;
}

void shutdown() {
    // GdiplusSession 的析构负责真正的 Shutdown；这里只做显式提前释放。
    auto& s = session();
    if (!s.ok)
        return;
    Gdiplus::GdiplusShutdown(s.token);
    s.ok    = false;
    s.token = 0;
}

bool loadPng(
    std::string const& path,
    AlphaMode          alphaMode,
    bool               forceOpaque,
    RgbaImage&         out,
    std::string&       error) {
    if (!session().ok) {
        error = "GDI+ not initialized";
        return false;
    }

    Gdiplus::Bitmap bitmap(std::filesystem::absolute(std::filesystem::path(path)).c_str());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        error = "cannot open skin image: " + path;
        return false;
    }

    auto const width  = bitmap.GetWidth();
    auto const height = bitmap.GetHeight();
    if (width != height || !isPowerOfTwo(width)) {
        error = std::format(
            "unsupported skin size: {}x{} (需要方形且边长是 64/128/256/512/1024)",
            width,
            height);
        return false;
    }

    Gdiplus::Rect       rect(0, 0, static_cast<INT>(width), static_cast<INT>(height));
    Gdiplus::BitmapData data{};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok) {
        error = "cannot read skin pixels";
        return false;
    }

    RgbaImage image;
    image.width  = width;
    image.height = height;
    image.rgba.resize(static_cast<std::size_t>(width) * height * 4);

    auto* base = static_cast<std::uint8_t*>(data.Scan0);
    for (std::uint32_t y = 0; y < height; ++y) {
        auto* row = data.Stride >= 0
                        ? base + static_cast<std::ptrdiff_t>(y) * data.Stride
                        : base + static_cast<std::ptrdiff_t>(height - 1 - y) * (-data.Stride);
        for (std::uint32_t x = 0; x < width; ++x) {
            auto const source = static_cast<std::size_t>(x) * 4;
            auto const target = (static_cast<std::size_t>(y) * width + x) * 4;

            std::uint8_t r = row[source + 2];
            std::uint8_t g = row[source + 1];
            std::uint8_t b = row[source];
            std::uint8_t a = row[source + 3];
            applyAlpha(r, g, b, a, alphaMode, forceOpaque);

            image.rgba[target]     = static_cast<char>(r);
            image.rgba[target + 1] = static_cast<char>(g);
            image.rgba[target + 2] = static_cast<char>(b);
            image.rgba[target + 3] = static_cast<char>(a);
        }
    }
    bitmap.UnlockBits(&data);

    out = std::move(image);
    return true;
}

} // namespace debugshape_export::skin
