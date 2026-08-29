// NpcSkinRegistry.cpp - 皮肤注册表实现
//
// PNG 解码移植自 SCustomNpc loadNpcSkin（GDI+ BGRA→RGBA 转换）;
// 玩家采集走 Player::mSkin → SerializedSkinImpl 字段级映射（全字段: 几何/披风/动画/Persona）。
// 1.18.0: 目录批量导入 + blob 导出/注册 API（库不落盘, 持久化由消费方负责）。
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

#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>

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

// 从 geometry JSON 提取首个模型 identifier（"identifier": "geometry.xxx"）
// 轻量扫描（不引入 JSON 依赖）; 找不到返回空 → 调用方回退 geometry 名
std::string parseGeometryIdentifier(std::string const& json) {
    std::size_t pos = 0;
    while ((pos = json.find("identifier", pos)) != std::string::npos) {
        auto colon = json.find(":", pos + 10);
        if (colon == std::string::npos) break;
        auto q1 = json.find("\"", colon + 1);
        if (q1 == std::string::npos) break;
        auto q2 = json.find("\"", q1 + 1);
        if (q2 == std::string::npos) break;
        auto id = json.substr(q1 + 1, q2 - q1 - 1);
        if (!id.empty()) return id; // "minecraft:geometry" 等 key 命中时值为空串 → 继续找
        pos = q2 + 1;
    }
    return {};
}

// ── blob 序列化格式（getSkinBlob / registerSkinFromBlob 配对使用）──
// 二进制（小端, length-prefixed）: "HLNS" | u32 version | skinId | SerializedSkin 全字段
// 设计目标: 全字段快照 → 消费方存盘后重启恢复, 皮肤成为采集时刻的永久副本
constexpr char          kSkinFileMagic[4] = {'H', 'L', 'N', 'S'};
constexpr std::uint32_t kSkinFileVersion   = 1;

void putU32(std::string& out, std::uint32_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
    out.push_back(static_cast<char>((v >> 16) & 0xFF));
    out.push_back(static_cast<char>((v >> 24) & 0xFF));
}

void putU8(std::string& out, std::uint8_t v) { out.push_back(static_cast<char>(v)); }

void putF32(std::string& out, float v) {
    static_assert(sizeof(float) == sizeof(std::uint32_t), "IEEE 754 expected");
    std::uint32_t bits{};
    std::memcpy(&bits, &v, sizeof(bits));
    putU32(out, bits);
}

void putStr(std::string& out, std::string const& s) {
    putU32(out, static_cast<std::uint32_t>(s.size()));
    out.append(s);
}

std::string serializeSkin(std::string const& skinId, sculk::protocol::SerializedSkin const& skin) {
    std::string out;
    out.reserve(64 * 1024 + 512);
    out.append(kSkinFileMagic, sizeof(kSkinFileMagic));
    putU32(out, kSkinFileVersion);
    putStr(out, skinId);

    // 标量/字符串字段
    putStr(out, skin.mId);
    putStr(out, skin.mPlayFabId);
    putStr(out, skin.mResourcePatch);
    putStr(out, skin.mFullId);

    // 主贴图
    putU32(out, skin.mSkinImageWidth);
    putU32(out, skin.mSkinImageHeight);
    putStr(out, skin.mSkinImageBytes);

    // 动画贴图
    putU32(out, static_cast<std::uint32_t>(skin.mAnimations.size()));
    for (auto const& a : skin.mAnimations) {
        putU32(out, a.mWidth);
        putU32(out, a.mHeight);
        putStr(out, a.mSkinImageBytes);
        putU32(out, static_cast<std::uint32_t>(a.mAnimationType));
        putF32(out, a.mFrameCount);
        putU32(out, static_cast<std::uint32_t>(a.mAnimationExpression));
    }

    // 披风
    putU32(out, skin.mCapeImageWidth);
    putU32(out, skin.mCapeImageHeight);
    putStr(out, skin.mCapeImageBytes);

    // 几何/动画数据/其余标量
    putStr(out, skin.mGeometryData);
    putStr(out, skin.mGeometryDataMinEngineVersion);
    putStr(out, skin.mAnimationData);
    putStr(out, skin.mCapeId);
    putStr(out, skin.mArmSize);
    putStr(out, skin.mSkinColor);

    // Persona 部件
    putU32(out, static_cast<std::uint32_t>(skin.mPersonaPieces.size()));
    for (auto const& p : skin.mPersonaPieces) {
        putStr(out, p.mPieceId);
        putStr(out, p.mPieceType);
        putStr(out, p.mPackId);
        putU8(out, p.mIsDefaultPiece ? 1 : 0);
        putStr(out, p.mProductId);
    }

    // 部件染色
    putU32(out, static_cast<std::uint32_t>(skin.mPieceTintColors.size()));
    for (auto const& t : skin.mPieceTintColors) {
        putStr(out, t.mPieceType);
        putU32(out, static_cast<std::uint32_t>(t.mPieceTintColors.size()));
        for (auto const& c : t.mPieceTintColors) putStr(out, c);
    }

    // 布尔标记
    putU8(out, skin.mIsPremiumSkin ? 1 : 0);
    putU8(out, skin.mIsPersonaSkin ? 1 : 0);
    putU8(out, skin.mIsPersonaCapeOnClassicSkin ? 1 : 0);
    putU8(out, skin.mIsPrimaryUser ? 1 : 0);
    putU8(out, skin.mOverridesPlayerAppearance ? 1 : 0);
    return out;
}

// 读取器（越界/长度异常置 ok=false → 上层报格式错误）
struct SkinBlobReader {
    char const*  p;
    std::size_t   remaining;
    bool         ok{true};

    bool read(void* dst, std::size_t n) {
        if (!ok || n > remaining) {
            ok = false;
            return false;
        }
        std::memcpy(dst, p, n);
        p += n;
        remaining -= n;
        return true;
    }
    std::uint32_t u32() {
        std::uint32_t v{};
        read(&v, sizeof(v));
        return v;
    }
    std::uint8_t u8() {
        std::uint8_t v{};
        read(&v, sizeof(v));
        return v;
    }
    float f32() {
        auto bits = u32();
        float v{};
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }
    std::string str() {
        auto n = u32();
        if (!ok || n > remaining) {
            ok = false;
            return {};
        }
        std::string s(n, '\0');
        if (n > 0 && !read(s.data(), n)) return {};
        return s;
    }
};

bool deserializeSkin(std::string const& blob, std::string& skinId, sculk::protocol::SerializedSkin& skin) {
    if (blob.size() < sizeof(kSkinFileMagic) + sizeof(std::uint32_t)) return false;
    SkinBlobReader r{blob.data(), blob.size()};
    char magic[sizeof(kSkinFileMagic)]{};
    if (!r.read(magic, sizeof(magic)) || std::memcmp(magic, kSkinFileMagic, sizeof(magic)) != 0) return false;
    if (r.u32() != kSkinFileVersion) return false;

    skinId = r.str();
    if (!r.ok) return false;

    skin.mId            = r.str();
    skin.mPlayFabId     = r.str();
    skin.mResourcePatch = r.str();
    skin.mFullId        = r.str();

    skin.mSkinImageWidth  = r.u32();
    skin.mSkinImageHeight = r.u32();
    skin.mSkinImageBytes  = r.str();

    auto animCount = r.u32();
    for (std::uint32_t i = 0; i < animCount && r.ok; ++i) {
        sculk::protocol::SerializedSkin::Animation a{};
        a.mWidth              = r.u32();
        a.mHeight             = r.u32();
        a.mSkinImageBytes     = r.str();
        a.mAnimationType      = static_cast<sculk::protocol::AnimatedTextureType>(r.u32());
        a.mFrameCount         = r.f32();
        a.mAnimationExpression = static_cast<sculk::protocol::AnimationExpression>(r.u32());
        skin.mAnimations.push_back(std::move(a));
    }

    skin.mCapeImageWidth  = r.u32();
    skin.mCapeImageHeight = r.u32();
    skin.mCapeImageBytes  = r.str();

    skin.mGeometryData                 = r.str();
    skin.mGeometryDataMinEngineVersion = r.str();
    skin.mAnimationData                = r.str();
    skin.mCapeId                       = r.str();
    skin.mArmSize                      = r.str();
    skin.mSkinColor                    = r.str();

    auto pieceCount = r.u32();
    for (std::uint32_t i = 0; i < pieceCount && r.ok; ++i) {
        sculk::protocol::SerializedSkin::PersonaPiece p{};
        p.mPieceId         = r.str();
        p.mPieceType       = r.str();
        p.mPackId          = r.str();
        p.mIsDefaultPiece  = r.u8() != 0;
        p.mProductId       = r.str();
        skin.mPersonaPieces.push_back(std::move(p));
    }

    auto tintCount = r.u32();
    for (std::uint32_t i = 0; i < tintCount && r.ok; ++i) {
        sculk::protocol::SerializedSkin::PieceTintColors t{};
        t.mPieceType = r.str();
        auto colorCount = r.u32();
        for (std::uint32_t j = 0; j < colorCount && r.ok; ++j) t.mPieceTintColors.push_back(r.str());
        skin.mPieceTintColors.push_back(std::move(t));
    }

    skin.mIsPremiumSkin              = r.u8() != 0;
    skin.mIsPersonaSkin              = r.u8() != 0;
    skin.mIsPersonaCapeOnClassicSkin = r.u8() != 0;
    skin.mIsPrimaryUser              = r.u8() != 0;
    skin.mOverridesPlayerAppearance  = r.u8() != 0;
    return r.ok;
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
    mSkins.clear(); // 纯内存; 持久化副本由消费方自行保存/恢复
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
    proto.mId             = "HoloLibNpcSkin_" + skinId;
    proto.mSkinImageWidth  = width;
    proto.mSkinImageHeight = height;
    proto.mSkinImageBytes  = std::move(rgba);
    if (!skin.geometryData.empty()) {
        // 自定义模型: identifier 从 JSON 提取（失败回退 geometry 名）; 全量几何随皮肤下发
        auto identifier = parseGeometryIdentifier(skin.geometryData);
        proto.mResourcePatch = std::format(
            R"({{"geometry":{{"default":"{}"}}}})",
            identifier.empty() ? skin.geometry : identifier
        );
        proto.mGeometryData               = skin.geometryData;
        proto.mGeometryDataMinEngineVersion = "1.21.100";
    } else {
        // 标准玩家模型（geometry 名走资源包内置几何, 不下发几何数据）
        proto.mResourcePatch               = std::format(R"({{"geometry":{{"default":"{}"}}}})", skin.geometry);
        proto.mGeometryDataMinEngineVersion = "1.21.100";
    }
    proto.mFullId                   = proto.mId;
    proto.mArmSize                  = (skin.armSize == "slim") ? "slim" : "wide";
    proto.mSkinColor                = "#0";
    proto.mOverridesPlayerAppearance = true;

    mSkins.insert_or_assign(std::move(skinId), std::move(proto));
    return true;
}

// ── 目录批量导入: 一个子文件夹 = 一套皮肤 ──
// 约定: 子文件夹名 = skinId; 文件夹内 PNG 贴图（必需）+ .json 几何（可选 = 默认玩家模型）
// 排序遍历保证多次导入结果确定; 单个文件夹失败仅跳过（warn 日志）, 不影响其余

int NpcSkinRegistry::importSkinsFromDir(std::string const& dirPath, std::string& error) {
    namespace fs = std::filesystem;
    std::error_code ec;
    auto root = fs::path(dirPath);
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        error = "not a directory: " + dirPath;
        return -1;
    }

    // 子文件夹收集 + 排序（目录序不确定 → 导入顺序/日志顺序稳定）
    std::vector<fs::path> subs;
    for (auto const& entry : fs::directory_iterator(root, ec)) {
        if (ec) break;
        if (entry.is_directory(ec) && !ec) subs.push_back(entry.path());
    }
    std::sort(subs.begin(), subs.end());

    int imported = 0;
    for (auto const& sub : subs) {
        auto skinId = sub.filename().string();
        if (skinId.empty() || skinId == "." || skinId == "..") continue;

        // 文件夹内找 PNG（贴图, 必需）与 JSON（几何, 可选）; 多个时取排序后第一个
        std::string pngPath;
        std::string geometryData;
        {
            std::vector<fs::path> files;
            std::error_code      ec2;
            for (auto const& f : fs::directory_iterator(sub, ec2)) {
                if (ec2) break;
                if (f.is_regular_file(ec2) && !ec2) files.push_back(f.path());
            }
            std::sort(files.begin(), files.end());
            for (auto const& p : files) {
                auto ext = p.extension().string();
                for (char& c : ext) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
                if (ext == ".png" && pngPath.empty()) {
                    pngPath = p.string();
                } else if (ext == ".json" && geometryData.empty()) {
                    std::ifstream in(p, std::ios::binary);
                    if (in) {
                        geometryData = std::string{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
                    }
                }
            }
        }

        if (pngPath.empty()) {
            logger().warn("[PlayerNpc] import '{}': no .png found, skipped", skinId);
            continue;
        }

        hologramlib::PlayerNpcSkin skin;
        skin.pngPath       = pngPath;
        skin.skinId       = skinId;
        skin.geometryData = std::move(geometryData); // 空 = 标准玩家模型
        std::string err;
        if (registerSkinFromPng(skin, err)) { // 消费方如需持久化: 导入成功后 getSkinBlob 落盘
            ++imported;
        } else {
            logger().warn("[PlayerNpc] import '{}': {}", skinId, err);
        }
    }
    return imported;
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
    // （皮肤贴图/披风/动画贴图/几何/Persona 部件/染色 全量拷贝 → 快照注册, 之后换肤不影响）
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

bool NpcSkinRegistry::getSkinBlob(std::string const& skinId, std::string& out) const {
    std::lock_guard lock(mMutex);
    auto it = mSkins.find(skinId);
    if (it == mSkins.end()) return false;
    out = serializeSkin(it->first, it->second);
    return true;
}

bool NpcSkinRegistry::registerSkinFromBlob(std::string const& blob, std::string& error) {
    std::string skinId;
    sculk::protocol::SerializedSkin skin;
    if (!deserializeSkin(blob, skinId, skin)) {
        error = "invalid skin blob (bad magic/version/corrupted)";
        return false;
    }
    if (skinId.empty()) {
        error = "invalid skin blob (empty skinId)";
        return false;
    }
    std::lock_guard lock(mMutex);
    mSkins.insert_or_assign(std::move(skinId), std::move(skin));
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