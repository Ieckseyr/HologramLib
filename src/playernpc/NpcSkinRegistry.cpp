#include "NpcSkinRegistry.h"

#include <sculk/protocol/utility/BinaryStream.hpp>
#include <sculk/protocol/utility/ReadOnlyBinaryStream.hpp>

#include "DiagLog.h"
#include "NpcPlayerList.h"

#include <ll/api/io/Logger.h>
#include <ll/api/io/LoggerRegistry.h>
#include <ll/api/service/Bedrock.h>

#include <ll/api/memory/Hook.h>
#include <mc/deps/core_graphics/helpers/TintMapColor.h>
#include <mc/network/BatchedNetworkPeer.h>
#include <mc/network/NetworkPeer.h>
#include <mc/platform/UUID.h>
#include <mc/world/actor/player/AnimatedImageData.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/actor/player/SerializedPersonaPieceHandle.h>
#include <mc/world/actor/player/SerializedSkinImpl.h>
#include <mc/world/actor/player/SerializedSkinRef.h>
#include <mc/world/actor/player/persona/persona.h>
#include <mc/world/level/Level.h>

#include <Windows.h>
#include <fstream>
#include <iterator>
#include <objidl.h>
#include <propidl.h>

#include <algorithm>
using std::max;
using std::min;
#include <gdiplus.h>

#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <unordered_map>
#include <utility>

namespace debugshape_export {

namespace {

std::string blobToString(mce::Blob const& blob) {
    return std::string(reinterpret_cast<char const*>(blob.data()), blob.size());
}

constexpr char kStandardHumanoidGeometry[] =
    R"({"format_version":"1.12.0","minecraft:geometry":[{"description":{"identifier":"geometry.humanoid.custom","texture_width":64,"texture_height":64,"visible_bounds_width":2,"visible_bounds_height":4,"visible_bounds_offset":[0,1.5,0]},"bones":[{"name":"body","pivot":[0,24,0],"cubes":[{"origin":[-4,12,-2],"size":[8,12,4],"uv":[16,16]}]},{"name":"waist","pivot":[0,12,0]},{"name":"head","pivot":[0,24,0],"cubes":[{"origin":[-4,24,-4],"size":[8,8,8],"uv":[0,0]}]},{"name":"rightArm","pivot":[-5,22,0],"cubes":[{"origin":[-8,12,-2],"size":[4,12,4],"uv":[40,16]}]},{"name":"leftArm","pivot":[5,22,0],"cubes":[{"origin":[4,12,-2],"size":[4,12,4],"uv":[32,48]}]},{"name":"rightLeg","pivot":[-1.9,12,0],"cubes":[{"origin":[-3.9,0,-2],"size":[4,12,4],"uv":[0,16]}]},{"name":"leftLeg","pivot":[1.9,12,0],"cubes":[{"origin":[-0.1,0,-2],"size":[4,12,4],"uv":[16,48]}]}]}]})";

void finalizeSkin(sculk::protocol::SerializedSkin& skin, std::string const& registryId) {
    npc_protocol::finalizeSkinIds(skin, registryId);
    if (skin.mGeometryData.empty()) {
        skin.mGeometryData                 = kStandardHumanoidGeometry;
        skin.mGeometryDataMinEngineVersion = "1.12.0";
        skin.mResourcePatch                = R"({"geometry":{"default":"geometry.humanoid.custom"}})";
    }
}

std::string colorToHexString(mce::Color const& color) {
    auto const toByte = [](float component) -> unsigned {
        return static_cast<unsigned>(std::round(255.0f * std::clamp(component, 0.0f, 1.0f)));
    };

    return std::format(
        "#{:02x}{:02x}{:02x}{:02x}",
        toByte(color.a), toByte(color.r), toByte(color.g), toByte(color.b));
}

std::string const& pieceTypeToString(::SharedTypes::persona::PieceType type) {
    static auto const reverse = [] {
        std::unordered_map<::SharedTypes::persona::PieceType, std::string> map;
        for (auto const& [str, pieceType] : ::persona::StringToPieceTypeMap()) {
            map.try_emplace(pieceType, str);
        }
        return map;
    }();
    static std::string const fallback = "unknown";
    auto                     it       = reverse.find(type);
    return it != reverse.end() ? it->second : fallback;
}

std::string parseGeometryIdentifier(std::string const& json) {

    auto oldKey = json.find("\"geometry.");
    if (oldKey != std::string::npos) {
        auto start = oldKey + 1;
        auto end   = json.find('"', start);
        if (end != std::string::npos && end > start)
            return json.substr(start, end - start);
    }

    std::size_t pos = 0;
    while ((pos = json.find("identifier", pos)) != std::string::npos) {
        auto colon = json.find(":", pos + 10);
        if (colon == std::string::npos)
            break;
        auto q1 = json.find("\"", colon + 1);
        if (q1 == std::string::npos)
            break;
        auto q2 = json.find("\"", q1 + 1);
        if (q2 == std::string::npos)
            break;
        auto id = json.substr(q1 + 1, q2 - q1 - 1);
        if (!id.empty())
            return id;
        pos = q2 + 1;
    }
    return {};
}

std::string parseGeometryVersion(std::string const& json) {
    auto key = json.find("\"format_version\"");
    if (key == std::string::npos)
        return "1.14.0";
    auto colon = json.find(':', key);
    if (colon == std::string::npos)
        return "1.14.0";
    auto q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos)
        return "1.14.0";
    auto q2 = json.find('"', q1 + 1);
    if (q2 == std::string::npos)
        return "1.14.0";
    auto v = json.substr(q1 + 1, q2 - q1 - 1);
    return v.empty() ? "1.14.0" : v;
}

// 皮肤缓存文件：magic + 版本 + 小端字段
constexpr char          kSkinFileMagic[4] = {'H', 'L', 'N', 'S'};
constexpr std::uint32_t kSkinFileVersion  = 3;

void putU32(sculk::protocol::BinaryStream& out, std::uint32_t v) {
    out.writeUnsignedInt(v);
}
void putU8(sculk::protocol::BinaryStream& out, std::uint8_t v) {
    out.writeByte(v);
}
void putF32(sculk::protocol::BinaryStream& out, float v) {
    out.writeFloat(v);
}

void putStr(sculk::protocol::BinaryStream& out, std::string const& s) {
    putU32(out, static_cast<std::uint32_t>(s.size()));
    out.writeBytes(s.data(), s.size());
}

std::string serializeSkin(std::string const& skinId, sculk::protocol::SerializedSkin const& skin) {
    std::vector<std::byte> buffer;
    buffer.reserve(64 * 1024 + 512);
    sculk::protocol::BinaryStream out{buffer};
    out.writeBytes(kSkinFileMagic, sizeof(kSkinFileMagic));
    putU32(out, kSkinFileVersion);
    putStr(out, skinId);

    putStr(out, skin.mId);
    putStr(out, skin.mPlayFabId);
    putStr(out, skin.mResourcePatch);
    putStr(out, skin.mFullId);

    putU32(out, skin.mSkinImageWidth);
    putU32(out, skin.mSkinImageHeight);
    putStr(out, skin.mSkinImageBytes);

    putU32(out, static_cast<std::uint32_t>(skin.mAnimations.size()));
    for (auto const& a : skin.mAnimations) {
        putU32(out, a.mWidth);
        putU32(out, a.mHeight);
        putStr(out, a.mSkinImageBytes);
        putU32(out, static_cast<std::uint32_t>(a.mAnimationType));
        putF32(out, a.mFrameCount);
        putU32(out, static_cast<std::uint32_t>(a.mAnimationExpression));
    }

    putU32(out, skin.mCapeImageWidth);
    putU32(out, skin.mCapeImageHeight);
    putStr(out, skin.mCapeImageBytes);

    putStr(out, skin.mGeometryData);
    putStr(out, skin.mGeometryDataMinEngineVersion);
    putStr(out, skin.mAnimationData);
    putStr(out, skin.mCapeId);
    putStr(out, skin.mArmSize);
    putStr(out, skin.mSkinColor);

    putU32(out, static_cast<std::uint32_t>(skin.mPersonaPieces.size()));
    for (auto const& p : skin.mPersonaPieces) {
        putStr(out, p.mPieceId);
        putStr(out, p.mPieceType);
        putStr(out, p.mPackId);
        putU8(out, p.mIsDefaultPiece ? 1 : 0);
        putStr(out, p.mProductId);
    }

    putU32(out, static_cast<std::uint32_t>(skin.mPieceTintColors.size()));
    for (auto const& t : skin.mPieceTintColors) {
        putStr(out, t.mPieceType);
        putU32(out, static_cast<std::uint32_t>(t.mPieceTintColors.size()));
        for (auto const& c : t.mPieceTintColors)
            putStr(out, c);
    }

    putU8(out, skin.mIsPremiumSkin ? 1 : 0);
    putU8(out, skin.mIsPersonaSkin ? 1 : 0);
    putU8(out, skin.mIsPersonaCapeOnClassicSkin ? 1 : 0);
    putU8(out, skin.mIsPrimaryUser ? 1 : 0);
    putU8(out, skin.mOverridesPlayerAppearance ? 1 : 0);
    return std::string{out.asStringView()};
}

struct SkinBlobReader {
    explicit SkinBlobReader(std::string const& blob)
        : stream(std::string_view{blob.data(), blob.size()}) {}

    sculk::protocol::ReadOnlyBinaryStream stream;
    bool                                  ok{true};

    bool readBytes(void* dst, std::size_t n) {
        if (!ok || !stream.readBytes(dst, n)) {
            ok = false;
            return false;
        }
        return true;
    }
    std::uint32_t u32() {
        std::uint32_t v{};
        readBytes(&v, sizeof(v));
        return v;
    }
    std::uint8_t u8() {
        std::uint8_t v{};
        readBytes(&v, 1);
        return v;
    }
    float f32() {
        float v{};
        readBytes(&v, sizeof(v));
        return v;
    }
    std::string str() {
        auto n = u32();
        if (!ok || n > (64u << 20)) {
            ok = false;
            return {};
        }
        std::string out(n, '\0');
        if (n > 0) {
            readBytes(out.data(), n);
        }
        return out;
    }
};

bool deserializeSkin(std::string const& blob, std::string& skinId, sculk::protocol::SerializedSkin& skin) {
    if (blob.size() < sizeof(kSkinFileMagic) + sizeof(std::uint32_t))
        return false;
    SkinBlobReader r{blob};
    char           magic[sizeof(kSkinFileMagic)]{};
    if (!r.readBytes(magic, sizeof(magic)) || std::memcmp(magic, kSkinFileMagic, sizeof(magic)) != 0)
        return false;
    if (r.u32() != kSkinFileVersion)
        return false;

    skinId = r.str();
    if (!r.ok)
        return false;

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
        a.mWidth               = r.u32();
        a.mHeight              = r.u32();
        a.mSkinImageBytes      = r.str();
        a.mAnimationType       = static_cast<sculk::protocol::AnimatedTextureType>(r.u32());
        a.mFrameCount          = r.f32();
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
        p.mPieceId        = r.str();
        p.mPieceType      = r.str();
        p.mPackId         = r.str();
        p.mIsDefaultPiece = r.u8() != 0;
        p.mProductId      = r.str();
        skin.mPersonaPieces.push_back(std::move(p));
    }

    auto tintCount = r.u32();
    for (std::uint32_t i = 0; i < tintCount && r.ok; ++i) {
        sculk::protocol::SerializedSkin::PieceTintColors t{};
        t.mPieceType    = r.str();
        auto colorCount = r.u32();
        for (std::uint32_t j = 0; j < colorCount && r.ok; ++j)
            t.mPieceTintColors.push_back(r.str());
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
    if (mGdiplusToken != nullptr)
        return;

    Gdiplus::GdiplusStartupInput startupInput{};
    ULONG_PTR                    token{};
    if (Gdiplus::GdiplusStartup(&token, &startupInput, nullptr) != Gdiplus::Ok) {
        HLIB_LOG_ERROR("[PlayerNpc] GDI+ init failed; PNG skin registration disabled");
        return;
    }
    mGdiplusToken = reinterpret_cast<void*>(token);
}

void NpcSkinRegistry::shutdown() {
    std::lock_guard lock(mMutex);
    if (mGdiplusToken == nullptr)
        return;
    Gdiplus::GdiplusShutdown(reinterpret_cast<ULONG_PTR>(mGdiplusToken));
    mGdiplusToken = nullptr;
    mSkins.clear();
}

bool NpcSkinRegistry::registerSkinFromPng(
    hologramlib::PlayerNpcSkin const& skin,
    std::string&                      error,
    bool                              forceOpaque,
    int                               alphaMode) {
    std::lock_guard lock(mMutex);
    if (mGdiplusToken == nullptr) {
        error = "GDI+ not initialized";
        return false;
    }

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

    auto const isPot = [](std::uint32_t v) {
        return v >= 64 && (v & (v - 1)) == 0;
    };
    if (width != height || !isPot(width)) {
        error = std::format("unsupported skin size: {}x{} (需要方形且边长是 64/128/256/512/1024)", width, height);
        return false;
    }

    Gdiplus::Rect       rect(0, 0, static_cast<INT>(width), static_cast<INT>(height));
    Gdiplus::BitmapData data{};
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok) {
        error = "cannot read skin pixels";
        return false;
    }

    std::string rgba;
    rgba.resize(static_cast<std::size_t>(width) * height * 4);
    auto* base = static_cast<std::uint8_t*>(data.Scan0);
    for (std::uint32_t y = 0; y < height; ++y) {
        auto* row = data.Stride >= 0
                        ? base + static_cast<std::ptrdiff_t>(y) * data.Stride
                        : base + static_cast<std::ptrdiff_t>(height - 1 - y) * (-data.Stride);
        for (std::uint32_t x = 0; x < width; ++x) {
            auto         source = static_cast<std::size_t>(x) * 4;
            auto         target = (static_cast<std::size_t>(y) * width + x) * 4;
            std::uint8_t r      = row[source + 2];
            std::uint8_t g      = row[source + 1];
            std::uint8_t b      = row[source];
            std::uint8_t a      = row[source + 3];
            if (alphaMode == 2) {

                auto const mix = [a](std::uint8_t c) {
                    return static_cast<std::uint8_t>((c * a + 255 * (255 - a)) / 255);
                };
                r = mix(r);
                g = mix(g);
                b = mix(b);
                a = 255;
            } else if (alphaMode == 1) {
                a = (a >= 128) ? 255 : 0;
            } else if (forceOpaque && a != 0) {
                a = 255;
            }
            rgba[target]     = static_cast<char>(r);
            rgba[target + 1] = static_cast<char>(g);
            rgba[target + 2] = static_cast<char>(b);
            rgba[target + 3] = static_cast<char>(a);
        }
    }
    bitmap.UnlockBits(&data);

    sculk::protocol::SerializedSkin proto;
    proto.mId              = "HoloLibNpcSkin_" + skinId;
    proto.mSkinImageWidth  = width;
    proto.mSkinImageHeight = height;
    proto.mSkinImageBytes  = std::move(rgba);
    if (!skin.geometryData.empty()) {

        auto identifier      = parseGeometryIdentifier(skin.geometryData);
        proto.mResourcePatch = std::format(
            R"({{"geometry":{{"default":"{}"}}}})",
            identifier.empty() ? skin.geometry : identifier);
        proto.mGeometryData = skin.geometryData;

        bool const oldStyle                 = skin.geometryData.find("\"minecraft:geometry\"") == std::string::npos;
        proto.mGeometryDataMinEngineVersion = oldStyle ? "0.0.0" : parseGeometryVersion(skin.geometryData);
    } else {

        proto.mResourcePatch                = std::format(R"({{"geometry":{{"default":"{}"}}}})", skin.geometry);
        proto.mGeometryDataMinEngineVersion = "1.21.100";
    }
    proto.mArmSize   = (skin.armSize == "slim") ? "slim" : "wide";
    proto.mSkinColor = "#00000000";

    proto.mIsPremiumSkin             = true;
    proto.mOverridesPlayerAppearance = false;
    finalizeSkin(proto, skinId);

    mSkins.insert_or_assign(std::move(skinId), std::move(proto));
    return true;
}

int NpcSkinRegistry::importSkinsFromDir(std::string const& dirPath, std::string& error) {
    namespace fs = std::filesystem;
    std::error_code ec;
    auto            root = fs::path(dirPath);
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        error = "not a directory: " + dirPath;
        return -1;
    }

    std::vector<fs::path> subs;
    for (auto const& entry : fs::directory_iterator(root, ec)) {
        if (ec)
            break;
        if (entry.is_directory(ec) && !ec)
            subs.push_back(entry.path());
    }
    std::sort(subs.begin(), subs.end());

    int imported = 0;
    for (auto const& sub : subs) {
        auto skinId = sub.filename().string();
        if (skinId.empty() || skinId == "." || skinId == "..")
            continue;

        std::string pngPath;
        std::string geometryData;
        {
            std::vector<fs::path> files;
            std::error_code       ec2;
            for (auto const& f : fs::directory_iterator(sub, ec2)) {
                if (ec2)
                    break;
                if (f.is_regular_file(ec2) && !ec2)
                    files.push_back(f.path());
            }
            std::sort(files.begin(), files.end());
            for (auto const& p : files) {
                auto ext = p.extension().string();
                for (char& c : ext)
                    c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
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
            HLIB_LOG_WARN("[PlayerNpc] import '{}': no .png found, skipped", skinId);
            continue;
        }

        hologramlib::PlayerNpcSkin skin;
        skin.pngPath      = pngPath;
        skin.skinId       = skinId;
        skin.geometryData = std::move(geometryData);
        std::string err;
        if (registerSkinFromPng(skin, err)) {
            ++imported;
        } else {
            HLIB_LOG_WARN("[PlayerNpc] import '{}': {}", skinId, err);
        }
    }
    return imported;
}

bool NpcSkinRegistry::captureSkin(std::string const& skinId, std::string const& playerName) {
    if (skinId.empty())
        return false;

    auto level = ll::service::getLevel();
    if (!level)
        return false;

    Player* player = level->getPlayer(playerName);
    if (!player)
        return false;

    auto const& skinRef = player->mSkin;
    if (!skinRef)
        return false;
    auto const& skinImpl = skinRef->mSkinImpl;
    if (!skinImpl)
        return false;
    ::SerializedSkinImpl const& impl = skinImpl->mObject;

    sculk::protocol::SerializedSkin proto;
    proto.mId = impl.mId;

    proto.mPlayFabId     = {};
    proto.mResourcePatch = impl.mResourcePatch;

    proto.mSkinImageWidth  = impl.mSkinImage.get().mWidth;
    proto.mSkinImageHeight = impl.mSkinImage.get().mHeight;
    proto.mSkinImageBytes  = blobToString(impl.mSkinImage.get().mImageBytes);

    for (auto const& anim : impl.mSkinAnimatedImages.get()) {
        sculk::protocol::SerializedSkin::Animation out{};
        out.mWidth          = anim.mImage.get().mWidth;
        out.mHeight         = anim.mImage.get().mHeight;
        out.mSkinImageBytes = blobToString(anim.mImage.get().mImageBytes);
        out.mAnimationType =
            static_cast<sculk::protocol::AnimatedTextureType>(static_cast<std::uint32_t>(anim.mType));
        out.mFrameCount = anim.mFrames;
        out.mAnimationExpression =
            static_cast<sculk::protocol::AnimationExpression>(static_cast<std::uint32_t>(anim.mAnimationExpression));
        proto.mAnimations.push_back(std::move(out));
    }

    proto.mCapeImageWidth  = impl.mCapeImage.get().mWidth;
    proto.mCapeImageHeight = impl.mCapeImage.get().mHeight;
    proto.mCapeImageBytes  = blobToString(impl.mCapeImage.get().mImageBytes);

    auto const& geoMutable              = impl.mGeometryDataMutable.get();
    auto const& geoRaw                  = impl.mGeometryData.get();
    auto const& geometry                = geoMutable.isNull() ? geoRaw : geoMutable;
    proto.mGeometryData                 = geometry.isNull() ? std::string{} : geometry.toStyledString();
    proto.mGeometryDataMinEngineVersion = impl.mGeometryDataMinEngineVersion.get().mSemVersion.get().asString();

    proto.mAnimationData = impl.mAnimationData;
    proto.mCapeId        = impl.mCapeId;
    proto.mArmSize       = (impl.mArmSizeType == ::SharedTypes::persona::ArmSizeType::Slim) ? "slim" : "wide";
    proto.mSkinColor     = colorToHexString(impl.mSkinColor.get());

    for (auto const& piece : impl.mPersonaPieces.get()) {
        sculk::protocol::SerializedSkin::PersonaPiece out{};
        out.mPieceId        = piece.mPieceId;
        out.mPieceType      = pieceTypeToString(piece.mPieceType);
        out.mPackId         = piece.mPackId.get().asString();
        out.mIsDefaultPiece = piece.mIsDefaultPiece;
        out.mProductId      = piece.mProductId;
        proto.mPersonaPieces.push_back(std::move(out));
    }

    for (auto const& [pieceType, tints] : impl.mPieceTintColors.get()) {
        sculk::protocol::SerializedSkin::PieceTintColors out{};
        out.mPieceType = pieceTypeToString(pieceType);
        out.mPieceTintColors.reserve(tints.colors.get().size());
        for (auto const& color : tints.colors.get()) {
            out.mPieceTintColors.push_back(colorToHexString(color));
        }
        proto.mPieceTintColors.push_back(std::move(out));
    }

    proto.mIsPremiumSkin              = impl.mIsPremium;
    proto.mIsPersonaSkin              = impl.mIsPersona;
    proto.mIsPersonaCapeOnClassicSkin = impl.mIsPersonaCapeOnClassicSkin;
    proto.mIsPrimaryUser              = impl.mIsPrimaryUser;
    proto.mOverridesPlayerAppearance  = impl.mOverridesPlayerAppearance;

    bool const isPersona = proto.mIsPersonaSkin || proto.mId.rfind("persona-", 0) == 0;
    if (isPersona) {
        std::string playerTag = proto.mId;
        if (playerTag.rfind("persona-", 0) == 0)
            playerTag.erase(0, 8);
        std::string const generic    = std::to_string(std::hash<std::string>{}(proto.mId) & 0x7FFFFFFF);
        auto              replaceAll = [](std::string& str, std::string const& from, std::string const& to) {
            if (from.empty() || from == to)
                return;
            for (std::size_t pos = 0; (pos = str.find(from, pos)) != std::string::npos; pos += to.size()) {
                str.replace(pos, from.size(), to);
            }
        };
        replaceAll(proto.mResourcePatch, playerTag, generic);
        replaceAll(proto.mGeometryData, playerTag, generic);
        replaceAll(proto.mAnimationData, playerTag, generic);
        replaceAll(proto.mCapeId, playerTag, generic);

        proto.mId = "hl_npc_" + generic;
        proto.mPersonaPieces.clear();
        proto.mPieceTintColors.clear();
        proto.mIsPersonaSkin = false;
        proto.mIsPrimaryUser = true;
    }

    finalizeSkin(proto, skinId);

    std::lock_guard lock(mMutex);
    mSkins.insert_or_assign(skinId, std::move(proto));
    return true;
}

bool NpcSkinRegistry::getSkinBlob(std::string const& skinId, std::string& out) const {
    std::lock_guard lock(mMutex);
    auto            it = mSkins.find(skinId);
    if (it == mSkins.end())
        return false;
    out = serializeSkin(it->first, it->second);
    return true;
}

bool NpcSkinRegistry::registerSkinFromBlob(std::string const& blob, std::string& error) {
    std::string                     skinId;
    sculk::protocol::SerializedSkin skin;
    if (!deserializeSkin(blob, skinId, skin)) {
        error = "invalid skin blob (bad magic/version/corrupted)";
        return false;
    }
    if (skinId.empty()) {
        error = "invalid skin blob (empty skinId)";
        return false;
    }
    finalizeSkin(skin, skinId);
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
    std::lock_guard          lock(mMutex);
    std::vector<std::string> ids;
    ids.reserve(mSkins.size());
    for (auto const& [id, skin] : mSkins)
        ids.push_back(id);
    return ids;
}

bool NpcSkinRegistry::getSkin(std::string const& skinId, sculk::protocol::SerializedSkin& out) const {
    std::lock_guard lock(mMutex);
    auto            it = mSkins.find(skinId);
    if (it == mSkins.end())
        return false;
    out = it->second;
    return true;
}

} // namespace debugshape_export
