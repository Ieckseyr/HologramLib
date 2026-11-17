#include "SkinBlob.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

namespace debugshape_export::skin {

namespace proto = ::debugshape_export::core::proto;

namespace {

using proto::SerializedSkinRef;

constexpr char          kSkinFileMagic[4] = {'H', 'L', 'N', 'S'};
constexpr std::uint32_t kSkinFileVersion  = 4;
constexpr std::uint32_t kSkinFileVersionLegacy = 3;

// HLNS 皮肤缓存是自己定的小端二进制，与协议线格式无关。
struct BlobWriter {
    std::string out;

    void u8(std::uint8_t v) { out.push_back(static_cast<char>(v)); }
    void u32(std::uint32_t v) { out.append(reinterpret_cast<char const*>(&v), sizeof(v)); }
    void i32(std::int32_t v) { out.append(reinterpret_cast<char const*>(&v), sizeof(v)); }
    void f32(float v) { out.append(reinterpret_cast<char const*>(&v), sizeof(v)); }

    void raw(void const* data, std::size_t n) { out.append(static_cast<char const*>(data), n); }

    void str(std::string const& s) {
        u32(static_cast<std::uint32_t>(s.size()));
        out += s;
    }

    void uuid(proto::UUID const& id) {
        out.append(reinterpret_cast<char const*>(&id.most_significant_bits), sizeof(std::uint64_t));
        out.append(reinterpret_cast<char const*>(&id.least_significant_bits), sizeof(std::uint64_t));
    }
};

struct BlobReader {
    explicit BlobReader(std::string const& blob) : data(blob) {}

    std::string const& data;
    std::size_t        at{0};
    bool               ok{true};

    bool bytes(void* dst, std::size_t n) {
        if (!ok || at + n > data.size()) {
            ok = false;
            return false;
        }
        std::memcpy(dst, data.data() + at, n);
        at += n;
        return true;
    }
    std::uint8_t u8() {
        std::uint8_t v{};
        bytes(&v, sizeof(v));
        return v;
    }
    std::uint32_t u32() {
        std::uint32_t v{};
        bytes(&v, sizeof(v));
        return v;
    }
    std::int32_t i32() {
        std::int32_t v{};
        bytes(&v, sizeof(v));
        return v;
    }
    float f32() {
        float v{};
        bytes(&v, sizeof(v));
        return v;
    }
    proto::UUID uuid() {
        proto::UUID v{};
        bytes(&v.most_significant_bits, sizeof(std::uint64_t));
        bytes(&v.least_significant_bits, sizeof(std::uint64_t));
        return v;
    }
    std::string str() {
        auto n = u32();
        if (!ok || n > (64u << 20) || at + n > data.size()) {
            ok = false;
            return {};
        }
        std::string out(data.data() + at, n);
        at += n;
        return out;
    }
};

inline proto::bp::ArmSizeType armSizeOf(std::string const& name) {
    return name == "slim" ? proto::bp::ArmSizeType::Slim : proto::bp::ArmSizeType::Wide;
}

inline std::string armSizeName(proto::bp::ArmSizeType size) {
    return size == proto::bp::ArmSizeType::Slim ? "slim" : "wide";
}

inline std::string serializeSkinImpl(std::string const& skinId, SerializedSkinRef const& skin) {
    BlobWriter w;
    w.raw(kSkinFileMagic, sizeof(kSkinFileMagic));
    w.u32(kSkinFileVersion);
    w.str(skinId);

    w.str(skin.id);
    w.str(skin.resource_patch);
    w.str(skin.full_id);


    std::uint32_t imgW = 0;
    std::uint32_t imgH = 0;
    std::string   imgBytes;

    proto::getSkinImage(skin, imgW, imgH, imgBytes);
    w.u32(imgW);
    w.u32(imgH);
    w.str(imgBytes);

    w.u32(static_cast<std::uint32_t>(skin.animated_image_data.size()));
    for (auto const& a : skin.animated_image_data) {
        std::uint32_t aw = 0;
        std::uint32_t ah = 0;
        std::string   ab;
        proto::getAnimImage(a, aw, ah, ab);
        w.u32(aw);
        w.u32(ah);
        w.str(ab);
        w.u32(static_cast<std::uint32_t>(a.type));
        w.f32(a.frames);
        w.u32(static_cast<std::uint32_t>(a.animation_expression));
    }

    proto::getSkinCapeImage(skin, imgW, imgH, imgBytes);
    w.u32(imgW);
    w.u32(imgH);
    w.str(imgBytes);

    w.str(skin.geometry_data);
    w.str(skin.geometry_data_min_engine_version);
    w.str(skin.animation_data);
    w.str(skin.cape_id);
    w.str(armSizeName(skin.arm_size));
    w.i32(static_cast<std::int32_t>(proto::getSkinColor(skin)));

    // persona/tint 两代结构不同，NPC 皮肤用不到：固定写 0，读取侧遇非 0 判为不兼容缓存。
    w.u32(0);
    w.u32(0);

    w.u8(skin.is_premium ? 1 : 0);
    w.u8(skin.is_persona ? 1 : 0);
    w.u8(skin.is_persona_cape_on_classic_skin ? 1 : 0);
    w.u8(skin.is_primary_user ? 1 : 0);
    w.u8(skin.overrides_player_appearance ? 1 : 0);
    w.u8(static_cast<std::uint8_t>(proto::getSkinTrusted(skin)));
    w.str({}); // profile_hash 占位
    return w.out;
}

bool readLegacyV3(BlobReader& r, std::string& skinId, SerializedSkinRef& skin) {
    skinId = r.str();
    if (!r.ok)
        return false;

    skin.id             = r.str();
    (void)r.str(); // legacy mPlayFabId：新版本线不再带这个字段
    skin.resource_patch = r.str();
    skin.full_id        = r.str();

    proto::setSkinImage(skin, r.u32(), r.u32(), r.str());

    auto animCount = r.u32();
    for (std::uint32_t i = 0; i < animCount && r.ok; ++i) {
        proto::AnimatedImageData a{};
        proto::setAnimImage(a, r.u32(), r.u32(), r.str());
        a.type                 = static_cast<decltype(a.type)>(r.u32());
        a.frames               = r.f32();
        a.animation_expression = static_cast<decltype(a.animation_expression)>(r.u32());
        skin.animated_image_data.push_back(std::move(a));
    }

    proto::setSkinCapeImage(skin, r.u32(), r.u32(), r.str());

    skin.geometry_data                    = r.str();
    skin.geometry_data_min_engine_version = r.str();
    skin.animation_data                   = r.str();
    skin.cape_id                          = r.str();
    skin.arm_size                         = armSizeOf(r.str());
    (void)r.str(); // legacy hex skin color：v3 值丢弃，走默认色

    auto pieceCount = r.u32();
    if (pieceCount != 0)
        return false; // 结构不兼容的缓存，直接丢弃让上层重新生成
    auto tintCount = r.u32();
    if (tintCount != 0)
        return false;
    for (std::uint32_t i = 0; i < pieceCount && r.ok; ++i) {
        (void)r.str();
        (void)r.str();
        (void)r.str();
        (void)r.u8();
        (void)r.str();
    }
    for (std::uint32_t i = 0; i < tintCount && r.ok; ++i) {
        (void)r.str();
        auto colorCount = r.u32();
        for (std::uint32_t j = 0; j < colorCount && r.ok; ++j) {
            (void)r.str();
        }
    }

    skin.is_premium                      = r.u8() != 0;
    skin.is_persona                      = r.u8() != 0;
    skin.is_persona_cape_on_classic_skin = r.u8() != 0;
    skin.is_primary_user                 = r.u8() != 0;
    skin.overrides_player_appearance     = r.u8() != 0;
    return r.ok;
}

bool readV4(BlobReader& r, std::string& skinId, SerializedSkinRef& skin) {
    skinId = r.str();
    if (!r.ok)
        return false;

    skin.id             = r.str();
    skin.resource_patch = r.str();
    skin.full_id        = r.str();

    proto::setSkinImage(skin, r.u32(), r.u32(), r.str());

    auto animCount = r.u32();
    for (std::uint32_t i = 0; i < animCount && r.ok; ++i) {
        proto::AnimatedImageData a{};
        proto::setAnimImage(a, r.u32(), r.u32(), r.str());
        a.type                 = static_cast<decltype(a.type)>(r.u32());
        a.frames               = r.f32();
        a.animation_expression = static_cast<decltype(a.animation_expression)>(r.u32());
        skin.animated_image_data.push_back(std::move(a));
    }

    proto::setSkinCapeImage(skin, r.u32(), r.u32(), r.str());

    skin.geometry_data                    = r.str();
    skin.geometry_data_min_engine_version = r.str();
    skin.animation_data                   = r.str();
    skin.cape_id                          = r.str();
    skin.arm_size                         = armSizeOf(r.str());
    proto::setSkinColor(skin, static_cast<std::uint32_t>(r.i32()));

    auto pieceCount = r.u32();
    if (pieceCount != 0)
        return false; // 同上：结构不兼容
    auto tintCount = r.u32();
    if (tintCount != 0)
        return false;

    skin.is_premium                      = r.u8() != 0;
    skin.is_persona                      = r.u8() != 0;
    skin.is_persona_cape_on_classic_skin = r.u8() != 0;
    skin.is_primary_user                 = r.u8() != 0;
    skin.overrides_player_appearance     = r.u8() != 0;
    proto::setSkinTrusted(skin, static_cast<proto::TrustedSkinFlag>(r.u8()));
    (void)r.str(); // profile_hash 占位
    return r.ok;
}

} // namespace

std::string serializeSkin(std::string const& skinId, SerializedSkinRef const& skin) {
    return serializeSkinImpl(skinId, skin);
}

bool deserializeSkin(std::string const& blob, std::string& skinId, SerializedSkinRef& skin) {
    if (blob.size() < sizeof(kSkinFileMagic) + sizeof(std::uint32_t))
        return false;

    BlobReader r{blob};
    char       magic[sizeof(kSkinFileMagic)]{};
    if (!r.bytes(magic, sizeof(magic)) || std::memcmp(magic, kSkinFileMagic, sizeof(magic)) != 0)
        return false;

    auto const version = r.u32();
    if (version == kSkinFileVersion)
        return readV4(r, skinId, skin);
    if (version == kSkinFileVersionLegacy)
        return readLegacyV3(r, skinId, skin);
    return false;
}

} // namespace debugshape_export::skin
