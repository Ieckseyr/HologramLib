// NpcSkinRegistry.cpp - 皮肤注册表实现
//
// PNG 解码移植自 SCustomNpc loadNpcSkin（GDI+ BGRA→RGBA 转换）;
// 玩家采集走 Player::mSkin → SerializedSkinImpl 字段级映射（全字段: 几何/披风/动画/Persona）。
#include "NpcSkinRegistry.h"

#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/service/Bedrock.h>

#include <mc/platform/UUID.h>
#include <mc/deps/core_graphics/helpers/TintMapColor.h>
#include <mc/world/actor/player/AnimatedImageData.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/actor/player/SerializedPersonaPieceHandle.h>
#include <mc/world/actor/player/SerializedSkinImpl.h>
#include <mc/world/actor/player/SerializedSkinRef.h>
#include <mc/world/actor/player/persona/persona.h>
#include <mc/world/level/Level.h>

#include <Windows.h>
#include <objidl.h>
#include <propidl.h>
// NOMINMAX 下 GDI+ 头内裸调 min/max → 引入 std::min/std::max 到全局可见
#include <algorithm>
using std::max;
using std::min;
#include <gdiplus.h>

#include <filesystem>
#include <format>

namespace debugshape_export {

namespace {

auto& logger() {
    static auto log = ll::io::LoggerRegistry::getInstance().getOrCreate("HologramLib");
    return *log;
}

// mce::Blob（皮肤贴图像素）→ std::string 拷贝
std::string blobToString(mce::Blob const& blob) {
    return std::string(reinterpret_cast<char const*>(blob.data()), blob.size());
}

} // namespace

NpcSkinRegistry& NpcSkinRegistry::getInstance() {
    static NpcSkinRegistry instance;
    return instance;
}

void NpcSkinRegistry::init() {
    std::lock_guard lock(mMutex);
    if (mGdiplusToken != nullptr) return; // 已初始化

    Gdiplus::GdiplusStartupInput startupInput{};
    ULONG_PTR                   token{};
    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) != Gdiplus::Ok) {
        logger().error("[PlayerNpc] GDI+ init failed; PNG skin registration disabled");
        return;
    }
    mGdiplusToken = reinterpret_cast<void*>(token);
}

void NpcSkinRegistry::shutdown() {
    std::lock_guard lock(mMutex);
    if (mGdiplusToken == nullptr) return;
    Gdiplus::GdiplusShutdown(reinterpret_cast<ULONG_PTR>(mGdiplusToken));
    mGdiplusToken = nullptr;
    mSkins.clear();
}

bool NpcSkinRegistry::registerSkinFromPng(hologramlib::PlayerNpcSkin const& skin, std::string& error) {
    std::lock_guard lock(mMutex);
    if (mGdiplusToken == nullptr) {
        error = "GDI+ not initialized";
        return false;
    }

    // skinId 默认取 PNG 文件名（去扩展名）
    std::string skinId = skin.skinId;
    if (skinId.empty()) {
        auto name = std::filesystem::path(skin.pngPath).stem().string();
        if (name.empty()) {
            error = "empty skinId and pngPath has no stem";
            return false;
        }
        skinId = std::move(name);
    }

    Gdiplus::Bitmap bitmap(std::filesystem::absolute(std::filesystem::path(skin.pngPath)).c_str());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        error = "cannot open skin image: " + skin.pngPath;
        return false;
    }
    auto width  = bitmap.GetWidth();
    auto height = bitmap.GetHeight();
    if ((width != 64 && width != 128) || (height != 64 && height != 128)) {
        error = std::format("unsupported skin size: {}x{}", width, height);
        return false;
    }

    Gdiplus::Rect       rect(0, 0, static_cast<INT>(width), static_cast<INT>(height));
    Gdiplus::BitmapData data{};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok) {
        error = "cannot read skin pixels";
        return false;
    }

    // BGRA(BYTE 序) → RGBA 字节流（SCustomNpc 同款转换）
    std::string rgba;
    rgba.resize(static_cast<std::size_t>(width) * height * 4);
    auto* base = static_cast<std::uint8_t*>(data.Scan0);
    for (std::uint32_t y = 0; y < height; ++y) {
        auto* row = data.Stride >= 0
            ? base + static_cast<std::ptrdiff_t>(y) * data.Stride
            : base + static_cast<std::ptrdiff_t>(height - 1 - y) * (-data.Stride);
        for (std::uint32_t x = 0; x < width; ++x) {
            auto source = static_cast<std::size_t>(x) * 4;
            auto target = (static_cast<std::size_t>(y) * width + x) * 4;
            rgba[target]     = static_cast<char>(row[source + 2]); // R
            rgba[target + 1] = static_cast<char>(row[source + 1]); // G
            rgba[target + 2] = static_cast<char>(row[source]);     // B
            rgba[target + 3] = static_cast<char>(row[source + 3]); // A
        }
    }
    bitmap.UnlockBits(&data);

    sculk::protocol::SerializedSkin proto;
    proto.mId                            = "HoloLibNpcSkin_" + skinId;
    proto.mResourcePatch                 = std::format(R"({{"geometry":{{"default":"{}"}}}})", skin.geometry);
    proto.mSkinImageWidth                 = width;
    proto.mSkinImageHeight                = height;
    proto.mSkinImageBytes                 = std::move(rgba);
    proto.mGeometryDataMinEngineVersion   = "1.21.100";
    proto.mFullId                         = proto.mId;
    proto.mArmSize                        = (skin.armSize == "slim") ? "slim" : "wide";
    proto.mSkinColor                       = "#0";
    proto.mOverridesPlayerAppearance       = true;

    mSkins.insert_or_assign(std::move(skinId), std::move(proto));
    return true;
}

bool NpcSkinRegistry::captureSkin(std::string const& skinId, std::string const& playerName) {
    if (skinId.empty()) return false;
    auto level = ll::service::getLevel();
    if (!level) return false;

    Player* player = level->getPlayer(playerName);
    if (!player) return false;

    // Player::mSkin → SerializedSkinRef → ThreadOwner → SerializedSkinImpl（BDS 原生皮肤全字段）
    auto const& skinRef = player->mSkin;
    if (!skinRef) return false;
    auto const& skinImpl = skinRef->mSkinImpl;
    if (!skinImpl) return false;
    ::SerializedSkinImpl const& impl = skinImpl->mObject;

    // BDS 原生 SerializedSkinImpl → sculk::protocol::SerializedSkin 字段级映射
    // （皮肤贴图/披风/动画贴图/几何/Persona 部件/染色 全量拷贝 → 永久注册副本）
    sculk::protocol::SerializedSkin proto;
    proto.mId            = impl.mId;
    proto.mPlayFabId     = impl.mPlayFabId;
    proto.mResourcePatch = impl.mResourcePatch;
    proto.mFullId        = impl.mFullId.get().empty() ? proto.mId : impl.mFullId.get();

    // 主皮肤贴图
    proto.mSkinImageWidth  = impl.mSkinImage.get().mWidth;
    proto.mSkinImageHeight = impl.mSkinImage.get().mHeight;
    proto.mSkinImageBytes  = blobToString(impl.mSkinImage.get().mImageBytes);

    // 动画贴图（表情 32x32 / 全身 128x128 动画皮肤）
    for (auto const& anim : impl.mSkinAnimatedImages.get()) {
        sculk::protocol::SerializedSkin::Animation out{};
        out.mWidth             = anim.mImage.get().mWidth;
        out.mHeight            = anim.mImage.get().mHeight;
        out.mSkinImageBytes    = blobToString(anim.mImage.get().mImageBytes);
        out.mAnimationType =
            static_cast<sculk::protocol::AnimatedTextureType>(static_cast<std::uint32_t>(anim.mType));
        out.mFrameCount        = anim.mFrames;
        out.mAnimationExpression =
            static_cast<sculk::protocol::AnimationExpression>(static_cast<std::uint32_t>(anim.mAnimationExpression));
        proto.mAnimations.push_back(std::move(out));
    }

    // 披风贴图
    proto.mCapeImageWidth  = impl.mCapeImage.get().mWidth;
    proto.mCapeImageHeight = impl.mCapeImage.get().mHeight;
    proto.mCapeImageBytes  = blobToString(impl.mCapeImage.get().mImageBytes);

    // 几何数据（自定义模型; Json → 字符串, 空/Null → 空串 = 走资源包内置几何）
    auto const& geometry = impl.mGeometryData.get();
    proto.mGeometryData = geometry.isNull() ? std::string{} : geometry.toStyledString();
    proto.mGeometryDataMinEngineVersion = impl.mGeometryDataMinEngineVersion.get().mSemVersion.get().asString();

    proto.mAnimationData = impl.mAnimationData;
    proto.mCapeId        = impl.mCapeId;
    proto.mArmSize       = (impl.mArmSizeType == ::persona::ArmSize::Type::Slim) ? "slim" : "wide";
    proto.mSkinColor     = impl.mSkinColor.get().toHexString();

    // Persona 部件（市场皮肤; 普通自定义皮肤为空）
    for (auto const& piece : impl.mPersonaPieces.get()) {
        sculk::protocol::SerializedSkin::PersonaPiece out{};
        out.mPieceId         = piece.mPieceId;
        out.mPieceType       = ::persona::stringFromPieceType(piece.mPieceType, piece.mIsDefaultPiece);
        out.mPackId          = piece.mPackId.get().asString();
        out.mIsDefaultPiece = piece.mIsDefaultPiece;
        out.mProductId       = piece.mProductId;
        proto.mPersonaPieces.push_back(std::move(out));
    }

    // 部件染色映射
    for (auto const& [pieceType, tints] : impl.mPieceTintColors.get()) {
        sculk::protocol::SerializedSkin::PieceTintColors out{};
        out.mPieceType = ::persona::stringFromPieceType(pieceType, false);
        out.mPieceTintColors.reserve(tints.colors.get().size());
        for (auto const& color : tints.colors.get()) {
            out.mPieceTintColors.push_back(color.toHexString());
        }
        proto.mPieceTintColors.push_back(std::move(out));
    }

    proto.mIsPremiumSkin              = impl.mIsPremium;
    proto.mIsPersonaSkin              = impl.mIsPersona;
    proto.mIsPersonaCapeOnClassicSkin = impl.mIsPersonaCapeOnClassicSkin;
    proto.mIsPrimaryUser              = impl.mIsPrimaryUser;
    proto.mOverridesPlayerAppearance = impl.mOverridesPlayerAppearance;

    std::lock_guard lock(mMutex);
    mSkins.insert_or_assign(skinId, std::move(proto));
    return true;
}

bool NpcSkinRegistry::hasSkin(std::string const& skinId) const {
    std::lock_guard lock(mMutex);
    return mSkins.contains(skinId);
}

bool NpcSkinRegistry::unregisterSkin(std::string const& skinId) {
    std::lock_guard lock(mMutex);
    return mSkins.erase(skinId) > 0;
}

std::vector<std::string> NpcSkinRegistry::getSkinIds() const {
    std::lock_guard lock(mMutex);
    std::vector<std::string> ids;
    ids.reserve(mSkins.size());
    for (auto const& [id, skin] : mSkins) ids.push_back(id);
    return ids;
}

bool NpcSkinRegistry::getSkin(std::string const& skinId, sculk::protocol::SerializedSkin& out) const {
    std::lock_guard lock(mMutex);
    auto it = mSkins.find(skinId);
    if (it == mSkins.end()) return false;
    out = it->second;
    return true;
}

} // namespace debugshape_export
