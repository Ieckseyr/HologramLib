// NpcSkinRegistry.cpp - 皮肤注册表实现
//
// PNG 解码移植自 SCustomNpc loadNpcSkin（GDI+ BGRA→RGBA 转换）;
// 玩家采集走 Player::mSkin → SerializedSkinImpl 字段级映射（全字段: 几何/披风/动画/Persona）。
// 1.17.1: 注册/采集即写盘 <mod目录>/npc_skins/（全字段二进制）, init 批量加载 → 永久快照。
#include "NpcSkinRegistry.h"

#include "ModEntry.h"

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
#include <system_error>

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

// ── 磁盘持久化: 二进制序列化（小端, length-prefixed）──
// 文件布局: "HLNS" | u32 version | skinId | SerializedSkin 全字段
// 设计目标: 全字段快照落盘 → 玩家换肤/重启/源玩家不上线均不影响已注册皮肤
constexpr char           kSkinFileMagic[4]  = {'H', 'L', 'N', 'S'};
constexpr std::uint32_t  kSkinFileVersion   = 1;
// 单文件大小上限（动画皮肤 + 几何 + 披风理论峰值远低于此; 超限视为损坏跳过）
constexpr std::uint64_t  kSkinFileMaxSize   = 16 * 1024 * 1024;

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

// 读取器（越界/长度异常置 ok=false → 上层跳过该文件, 不影响其余皮肤）
struct SkinFileReader {
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
    SkinFileReader r{blob.data(), blob.size()};
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

    skin.mGeometryData                = r.str();
    skin.mGeometryDataMinEngineVersion = r.str();
    skin.mAnimationData               = r.str();
    skin.mCapeId                      = r.str();
    skin.mArmSize                     = r.str();
    skin.mSkinColor                   = r.str();

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

// skinId → 安全文件名（percent 风格: 非 [A-Za-z0-9_.-] → %HH, 不同 skinId 不撞名）
std::string sanitizeFileName(std::string const& skinId) {
    static char const* hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(skinId.size() + 16);
    for (unsigned char c : skinId) {
        bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.'
                 || c == '-';
        if (safe) {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0xF]);
        }
    }
    if (out.empty() || out == "." || out == "..") out = "_empty_";
    return out + ".bin";
}

} // namespace

NpcSkinRegistry& NpcSkinRegistry::getInstance() {
    static NpcSkinRegistry instance;
    return instance;
}

void NpcSkinRegistry::init() {
    std::lock_guard lock(mMutex);
    if (mGdiplusToken != nullptr) return; // 已初始化

    // 持久化皮肤恢复（不依赖 GDI+, 放在 GDI+ 初始化前: GDI+ 失败也不影响已存储皮肤可用）
    loadAllFromDisk();

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
    mSkins.clear(); // 仅清内存; 磁盘快照保留, 下次 init 恢复
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

    auto [it, inserted] = mSkins.insert_or_assign(std::move(skinId), std::move(proto));
    saveSkinToDisk(it->first, it->second); // 永久存储（PNG 源文件可能被删, 落盘后不再依赖）; inserted 只区分新插入/覆盖
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
    auto [it, inserted] = mSkins.insert_or_assign(skinId, std::move(proto));
    // 永久快照落盘: 采集时刻的皮肤全字段 → 此后源玩家换肤/下线/重启均不影响
    saveSkinToDisk(it->first, it->second);
    return true;
}

bool NpcSkinRegistry::hasSkin(std::string const& skinId) const {
    std::lock_guard lock(mMutex);
    return mSkins.contains(skinId);
}

bool NpcSkinRegistry::unregisterSkin(std::string const& skinId) {
    std::lock_guard lock(mMutex);
    if (mSkins.erase(skinId) == 0) return false;
    eraseSkinFromDisk(skinId); // 磁盘快照一并删除（真正卸载）
    return true;
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

// ── 磁盘持久化实现（调用方已持 mMutex）──

std::filesystem::path NpcSkinRegistry::skinsDir() const {
    // ModEntry 单例持有本 mod 的 NativeMod 引用（load 阶段构造, 生命周期内有效）
    return ModEntry::getInstance().getSelf().getModDir() / "npc_skins";
}

std::filesystem::path NpcSkinRegistry::skinFilePath(std::string const& skinId) const {
    return skinsDir() / sanitizeFileName(skinId);
}

void NpcSkinRegistry::saveSkinToDisk(std::string const& skinId, sculk::protocol::SerializedSkin const& skin) const {
    namespace fs = std::filesystem;
    std::error_code ec;
    auto dir = skinsDir();
    fs::create_directories(dir, ec); // 已存在不算错
    auto path = dir / sanitizeFileName(skinId);
    auto blob = serializeSkin(skinId, skin);

    // 先写 .tmp 再原子改名: 崩溃/断电不产生半截文件（半截文件加载时会被校验跳过）
    auto tmp = path;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) {
            logger().warn("[PlayerNpc] cannot open skin file for write: {}", tmp.string());
            return;
        }
        f.write(blob.data(), static_cast<std::streamsize>(blob.size()));
        if (!f) {
            logger().warn("[PlayerNpc] skin file write incomplete: {}", tmp.string());
            fs::remove(tmp, ec);
            return;
        }
    }
    fs::rename(tmp, path, ec); // Windows 上原子覆盖已存在目标
    if (ec) {
        // 兜底: rename 失败（目标被占用等极少见）→ 直接写目标
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (f) {
            f.write(blob.data(), static_cast<std::streamsize>(blob.size()));
            std::error_code ec2;
            fs::remove(tmp, ec2);
            return;
        }
        logger().warn("[PlayerNpc] persist skin '{}' failed: {}", skinId, ec.message());
        std::error_code ec3;
        fs::remove(tmp, ec3);
    }
}

void NpcSkinRegistry::eraseSkinFromDisk(std::string const& skinId) const {
    std::error_code ec;
    std::filesystem::remove(skinFilePath(skinId), ec); // 不存在不算错
    auto tmp = skinFilePath(skinId);
    tmp += ".tmp";
    std::filesystem::remove(tmp, ec);
}

void NpcSkinRegistry::loadAllFromDisk() {
    namespace fs = std::filesystem;
    std::error_code ec;
    auto dir = skinsDir();
    if (!fs::exists(dir, ec) || ec) return;

    std::uint32_t loaded = 0;
    for (auto const& entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec) || ec) continue;
        auto const& path = entry.path();
        if (path.extension() != ".bin") continue;
        auto size = entry.file_size(ec);
        if (ec || size > kSkinFileMaxSize) {
            logger().warn("[PlayerNpc] skip abnormal skin file: {}", path.filename().string());
            continue;
        }

        std::ifstream f(path, std::ios::binary);
        if (!f) continue;
        std::string blob{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};

        std::string skinId;
        sculk::protocol::SerializedSkin skin;
        if (!deserializeSkin(blob, skinId, skin)) {
            logger().warn("[PlayerNpc] skip corrupted skin file: {}", path.filename().string());
            continue;
        }
        // 文件内 skinId 为准（文件名仅是 sanitize 结果）
        mSkins.insert_or_assign(std::move(skinId), std::move(skin));
        ++loaded;
    }
    if (loaded > 0) logger().info("[PlayerNpc] restored {} persistent skin(s) from disk", loaded);
}

} // namespace debugshape_export