#include "NpcSkinRegistry.h"

#include "core/proto/Version.h"

#include "DiagLog.h"
#include "NpcPlayerList.h"
#include "skin/SkinBlob.h"
#include "skin/SkinGeometry.h"
#include "skin/SkinImage.h"

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

#include <fstream>

#include <algorithm>

#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <unordered_map>
#include <utility>

namespace debugshape_export {

namespace proto = ::debugshape_export::core::proto;

namespace {

std::string blobToString(mce::Blob const& blob) {
    return std::string(reinterpret_cast<char const*>(blob.data()), blob.size());
}

// 打包成 ARGB（与协议里 Color 的 int32 语义一致）
std::int32_t toPackedColor(mce::Color const& color) {
    auto const toByte = [](float component) -> std::uint32_t {
        return static_cast<std::uint32_t>(std::round(255.0f * std::clamp(component, 0.0f, 1.0f)));
    };

    return static_cast<std::int32_t>(
        (toByte(color.a) << 24) | (toByte(color.r) << 16) | (toByte(color.g) << 8) | toByte(color.b));
}

proto::PieceType toProtoPieceType(::SharedTypes::persona::PieceType type) {
    auto const raw = static_cast<std::uint32_t>(type);
    if (raw > static_cast<std::uint32_t>(proto::PieceType::Unsupported)) {
        return proto::PieceType::Unsupported;
    }
    return static_cast<proto::PieceType>(raw);
}

} // namespace

NpcSkinRegistry& NpcSkinRegistry::getInstance() {
    static NpcSkinRegistry instance;
    return instance;
}

void NpcSkinRegistry::init() {
    std::string error;
    if (!skin::startup(error))
        HLIB_LOG_ERROR("[PlayerNpc] {}; PNG skin registration disabled", error);
}

void NpcSkinRegistry::shutdown() {
    skin::shutdown();
    std::lock_guard lock(mMutex);
    mSkins.clear();
}

bool NpcSkinRegistry::registerSkinFromPng(
    hologramlib::PlayerNpcSkin const& skin,
    std::string&                      error,
    bool                              forceOpaque,
    int                               alphaMode) {
    std::string skinId = skin.skinId;
    if (skinId.empty()) {
        auto name = std::filesystem::path(skin.pngPath).stem().string();
        if (name.empty()) {
            error = "empty skinId and pngPath has no stem";
            return false;
        }
        skinId = std::move(name);
    }

    auto const mode = alphaMode == 2   ? skin::AlphaMode::BlendWhite
                      : alphaMode == 1 ? skin::AlphaMode::Binary
                                       : skin::AlphaMode::Keep;

    // 解码在锁外做：文件 I/O + GDI+ 都慢，不能挡住其他注册表调用。
    skin::RgbaImage image;
    if (!skin::loadPng(skin.pngPath, mode, forceOpaque, image, error))
        return false;

    proto::SerializedSkinRef protoSkin;
    protoSkin.id                 = "HoloLibNpcSkin_" + skinId;
    protoSkin.image_data.width   = image.width;
    protoSkin.image_data.height  = image.height;
    protoSkin.image_data.image_bytes = std::move(image.rgba);

    if (!skin.geometryData.empty()) {
        auto const identifier = skin::parseGeometryIdentifier(skin.geometryData);
        protoSkin.resource_patch = std::format(
            R"({{"geometry":{{"default":"{}"}}}})",
            identifier.empty() ? skin.geometry : identifier);
        protoSkin.geometry_data = skin.geometryData;

        bool const oldStyle = skin.geometryData.find("\"minecraft:geometry\"") == std::string::npos;
        protoSkin.geometry_data_min_engine_version =
            oldStyle ? "0.0.0" : skin::parseGeometryVersion(skin.geometryData);
    } else {
        protoSkin.resource_patch = std::format(R"({{"geometry":{{"default":"{}"}}}})", skin.geometry);
        protoSkin.geometry_data_min_engine_version = "1.21.100";
    }
    protoSkin.arm_size =
        (skin.armSize == "slim") ? proto::bp::ArmSizeType::Slim : proto::bp::ArmSizeType::Wide;
    protoSkin.skin_color = proto::Color{0};

    protoSkin.is_premium                = true;
    protoSkin.overrides_player_appearance = false;
    skin::finalizeSkin(protoSkin, skinId);

    {
        std::lock_guard lock(mMutex);
        mSkins.insert_or_assign(std::move(skinId), std::move(protoSkin));
    }
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

    proto::SerializedSkinRef protoSkin;
    protoSkin.id             = impl.mId;
    protoSkin.resource_patch = impl.mResourcePatch;

    protoSkin.image_data.width       = impl.mSkinImage.get().mWidth;
    protoSkin.image_data.height      = impl.mSkinImage.get().mHeight;
    protoSkin.image_data.image_bytes = blobToString(impl.mSkinImage.get().mImageBytes);

    for (auto const& anim : impl.mSkinAnimatedImages.get()) {
        proto::AnimatedImageData out{};
        out.image.width          = anim.mImage.get().mWidth;
        out.image.height         = anim.mImage.get().mHeight;
        out.image.image_bytes    = blobToString(anim.mImage.get().mImageBytes);
        out.type                 = static_cast<decltype(out.type)>(static_cast<std::uint32_t>(anim.mType));
        out.frames               = anim.mFrames;
        out.animation_expression = static_cast<decltype(out.animation_expression)>(
            static_cast<std::uint32_t>(anim.mAnimationExpression));
        protoSkin.animated_image_data.push_back(std::move(out));
    }

    protoSkin.cape_image_data.width       = impl.mCapeImage.get().mWidth;
    protoSkin.cape_image_data.height      = impl.mCapeImage.get().mHeight;
    protoSkin.cape_image_data.image_bytes = blobToString(impl.mCapeImage.get().mImageBytes);

    auto const& geoMutable    = impl.mGeometryDataMutable.get();
    auto const& geoRaw        = impl.mGeometryData.get();
    auto const& geometry      = geoMutable.isNull() ? geoRaw : geoMutable;
    protoSkin.geometry_data   = geometry.isNull() ? std::string{} : geometry.toStyledString();
    protoSkin.geometry_data_min_engine_version = impl.mGeometryDataMinEngineVersion.get().mSemVersion.get().asString();

    protoSkin.animation_data = impl.mAnimationData;
    protoSkin.cape_id        = impl.mCapeId;
    protoSkin.arm_size       = (impl.mArmSizeType == ::SharedTypes::persona::ArmSizeType::Slim)
                                   ? proto::bp::ArmSizeType::Slim
                                   : proto::bp::ArmSizeType::Wide;
    protoSkin.skin_color     = proto::Color{toPackedColor(impl.mSkinColor.get())};

    for (auto const& piece : impl.mPersonaPieces.get()) {
        proto::SerializedPersonaPiece out{};
        out.piece_id        = piece.mPieceId;
        out.piece_type      = toProtoPieceType(piece.mPieceType);
        out.pack_id         = proto::UUID{piece.mPackId.get().a, piece.mPackId.get().b};
        out.is_default_piece = piece.mIsDefaultPiece;
        out.product_id      = piece.mProductId;
        protoSkin.persona_pieces.push_back(std::move(out));
    }

    for (auto const& [pieceType, tints] : impl.mPieceTintColors.get()) {
        proto::TintMapColor out{};
        auto const&         src = tints.colors.get();
        for (std::size_t i = 0; i < out.colors.size() && i < src.size(); ++i) {
            out.colors[i] = proto::Color{toPackedColor(src[i])};
        }
        protoSkin.piece_tint_colors.emplace(toProtoPieceType(pieceType), out);
    }

    protoSkin.is_premium                      = impl.mIsPremium;
    protoSkin.is_persona                      = impl.mIsPersona;
    protoSkin.is_persona_cape_on_classic_skin = impl.mIsPersonaCapeOnClassicSkin;
    protoSkin.is_primary_user                 = impl.mIsPrimaryUser;
    protoSkin.overrides_player_appearance     = impl.mOverridesPlayerAppearance;

    bool const isPersona = protoSkin.is_persona || protoSkin.id.rfind("persona-", 0) == 0;
    if (isPersona) {
        std::string playerTag = protoSkin.id;
        if (playerTag.rfind("persona-", 0) == 0)
            playerTag.erase(0, 8);
        std::string const generic    = std::to_string(std::hash<std::string>{}(protoSkin.id) & 0x7FFFFFFF);
        auto              replaceAll = [](std::string& str, std::string const& from, std::string const& to) {
            if (from.empty() || from == to)
                return;
            for (std::size_t pos = 0; (pos = str.find(from, pos)) != std::string::npos; pos += to.size()) {
                str.replace(pos, from.size(), to);
            }
        };
        replaceAll(protoSkin.resource_patch, playerTag, generic);
        replaceAll(protoSkin.geometry_data, playerTag, generic);
        replaceAll(protoSkin.animation_data, playerTag, generic);
        replaceAll(protoSkin.cape_id, playerTag, generic);

        protoSkin.id = "hl_npc_" + generic;
        protoSkin.persona_pieces.clear();
        protoSkin.piece_tint_colors.clear();
        protoSkin.is_persona     = false;
        protoSkin.is_primary_user = true;
    }

    skin::finalizeSkin(protoSkin, skinId);

    std::lock_guard lock(mMutex);
    mSkins.insert_or_assign(skinId, std::move(protoSkin));
    return true;
}

bool NpcSkinRegistry::getSkinBlob(std::string const& skinId, std::string& out) const {
    std::lock_guard lock(mMutex);
    auto            it = mSkins.find(skinId);
    if (it == mSkins.end())
        return false;
    out = skin::serializeSkin(it->first, it->second);
    return true;
}

bool NpcSkinRegistry::registerSkinFromBlob(std::string const& blob, std::string& error) {
    std::string              skinId;
    proto::SerializedSkinRef skin;
    if (!skin::deserializeSkin(blob, skinId, skin)) {
        error = "invalid skin blob (bad magic/version/corrupted)";
        return false;
    }
    if (skinId.empty()) {
        error = "invalid skin blob (empty skinId)";
        return false;
    }
    skin::finalizeSkin(skin, skinId);
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

bool NpcSkinRegistry::getSkin(std::string const& skinId, proto::SerializedSkinRef& out) const {
    std::lock_guard lock(mMutex);
    auto            it = mSkins.find(skinId);
    if (it == mSkins.end())
        return false;
    out = it->second;
    return true;
}

} // namespace debugshape_export
