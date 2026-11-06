#include "PlayerNpcManager.h"

#include "NpcSkinRegistry.h"
#include "DiagLog.h"

#include <cctype>
#include <format>
#include <mutex>
#include <string>

namespace debugshape_export {

bool PlayerNpcManager::registerImageSkin(std::string const& pngPath, std::string const& skinId, float sizeBlocks, bool transparent) {
    return registerImageSkinSized(pngPath, skinId, sizeBlocks, sizeBlocks, transparent);
}

bool PlayerNpcManager::registerImageSkinSized(
    std::string const& pngPath,
    std::string const& skinId,
    float              widthBlocks,
    float              heightBlocks,
    bool               transparent
) {
    if (widthBlocks <= 0.0f)
        widthBlocks = 1.0f;
    if (heightBlocks <= 0.0f)
        heightBlocks = 1.0f;

    // 平面按方块算：模型 16 单位 = 1 方块，宽 2W、高 H，所以 W = 8*宽、H = 16*高
    float const w = 8.0f * widthBlocks;
    float const h = 16.0f * heightBlocks;

    std::string geoKey;
    for (char ch : skinId) {
        geoKey += (std::isalnum(static_cast<unsigned char>(ch)) ? ch : '_');
    }
    std::string const geoId = std::format("geometry.hlib_image_{}", geoKey);

    static constexpr char kTemplate[] =
        R"({"format_version":"1.14.0","minecraft:geometry":[{"description":{"identifier":"%ID%","texture_width":128,"texture_height":128},"bones":[{"name":"root"},{"name":"body","parent":"root","pivot":[0,0,0],"poly_mesh":{"normalized_uvs":true,"positions":[[-%W%,0,-0.01],[%W%,0,-0.01],[%W%,%H%,-0.01],[-%W%,%H%,-0.01],[%W%,0,0.01],[-%W%,0,0.01],[-%W%,%H%,0.01],[%W%,%H%,0.01]],"normals":[[0,0,-1],[0,0,-1],[0,0,-1],[0,0,-1],[0,0,1],[0,0,1],[0,0,1],[0,0,1]],"uvs":[[0,0],[1,0],[1,1],[0,1],[1,0],[0,0],[0,1],[1,1]],"polys":[[[0,0,0],[1,0,1],[2,0,2],[3,0,3]],[[4,1,4],[5,1,5],[6,1,6],[7,1,7]]]}}]}]})";

    auto const num = [](float v) {
        return std::format("{:.6g}", v);
    };
    std::string geo        = kTemplate;
    auto        replaceAll = [&](std::string_view key, std::string_view val) {
        for (auto pos = geo.find(key); pos != std::string::npos; pos = geo.find(key, pos + val.size())) {
            geo.replace(pos, key.size(), val);
        }
    };
    replaceAll("%ID%", geoId);
    replaceAll("%W%", num(w));
    replaceAll("%H%", num(h));

    hologramlib::PlayerNpcSkin skin;
    skin.pngPath      = pngPath;
    skin.skinId       = skinId;
    skin.armSize      = "wide";
    skin.geometryData = std::move(geo);
    std::string error;
    bool const  ok = NpcSkinRegistry::getInstance().registerSkinFromPng(skin, error, false, transparent ? 1 : 2);
    if (!ok)
        HLIB_LOG_WARN("[PlayerNpc] registerImageSkin failed: {}", error);
    return ok;
}

bool PlayerNpcManager::setImageSize(
    int64_t            id,
    std::string const& pngPath,
    float              widthBlocks,
    float              heightBlocks,
    bool               transparent
) {
    std::string baseId;
    {
        std::lock_guard lock(mMutex);
        auto*           cfg = configOf(id);
        if (cfg == nullptr)
            return false;
        baseId = cfg->skinId;
    }

    if (widthBlocks <= 0.0f)
        widthBlocks = 1.0f;
    if (heightBlocks <= 0.0f)
        heightBlocks = 1.0f;

    std::string const sizedId = std::format(
        "{}@{}x{}",
        baseId,
        static_cast<int>(widthBlocks * 100.0f + 0.5f),
        static_cast<int>(heightBlocks * 100.0f + 0.5f)
    );

    if (!NpcSkinRegistry::getInstance().hasSkin(sizedId)) {
        if (!registerImageSkinSized(pngPath, sizedId, widthBlocks, heightBlocks, transparent))
            return false;
    }
    return setSkin(id, sizedId);
}

} // namespace debugshape_export
