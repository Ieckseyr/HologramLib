// Wire26.cpp - 26.60（2225）与 sculk 2168（26.40）有差异的包的 26.60 编码实现
#include "compat/Wire26.h"

#include <cstddef>

namespace debugshape_export::wire26 {

std::string skinBody(sculk::protocol::SerializedSkin const& skin) {
    // 26.60 的 SerializedSkin 去掉了 PlayFabId（原先是 ID 之后的第二个字段），
    // 其余字段顺序/编码与 26.40 一致（见 protocol-docs 的 SerializedSkinRef）。
    sculk::protocol::SerializedSkin copy = skin;
    copy.mPlayFabId.clear();

    std::string body = sculkBody(copy);

    // sculk 仍会写一个空的 PlayFabId（1 字节 0x00），位置 = ID 的长度前缀 + ID 本身
    std::size_t const off = uvLen(static_cast<std::uint32_t>(copy.mId.size())) + copy.mId.size();
    if (off < body.size() && body[off] == '\0') {
        body.erase(off, 1);
    }
    return body;
}

std::string playerListBody(sculk::protocol::PlayerListPacket const& packet) {
    using Action = sculk::protocol::PlayerListPacket::ActionType;

    bool const isAdd = packet.mAction == Action::Add;

    std::string out;
    putUv(out, static_cast<std::uint32_t>(packet.mPlayerEntryList.size()));

    for (auto const& entry : packet.mPlayerEntryList) {
        // 元素是 variant<RemoveEntry, AddEntry>：先写 variant 下标
        putUv(out, isAdd ? 1u : 0u);
        // 每个 case 的第一个字段 Action(PlayerListPacketType)：Add=0 / Remove=1
        putU8(out, static_cast<std::uint8_t>(packet.mAction));

        // UUID：两个 uint64
        putU64(out, entry.mUUID.mMostSignificantBits);
        putU64(out, entry.mUUID.mLeastSignificantBits);

        if (!isAdd) {
            continue;
        }

        putVi64(out, entry.mActorUniqueId);
        putStr(out, entry.mPlayerName);
        putStr(out, entry.mXuid);
        putStr(out, {}); // 26.60 新增 PlayFab ID（这里发空串）
        putStr(out, entry.mPlatformChatId);
        putS32(out, entry.mBuildPlatform);

        sculk::protocol::SerializedSkin skin = entry.mSerializedSkin;
        skin.mTrustedSkinFlag                = entry.mSkinTrusted ? 2 : 1; // 与 sculk 的条目语义一致
        out += skinBody(skin);

        putBool(out, entry.mIsTeacher);
        putBool(out, entry.mIsHost);
        putBool(out, entry.mIsSubClient);
        putS32(out, entry.mColor);
    }
    return out;
}

std::string playerSkinBody(sculk::protocol::PlayerSkinPacket const& packet) {
    std::string out;
    putU64(out, packet.mUUID.mMostSignificantBits);
    putU64(out, packet.mUUID.mLeastSignificantBits);
    out += skinBody(packet.mSerializedSkin);
    putStr(out, packet.mNewSkinName);
    putStr(out, packet.mOldSkinName);
    return out;
}

std::string addPlayerBody(sculk::protocol::AddPlayerPacket const& packet) {
    std::string body = sculkBody(packet);

    // 26.60：Actor Links 之后、Device Id 之前插入可选字段 PassengerBlockData。
    // body 末尾是 DeviceId(string) + BuildPlatform(uint32)，据此反推插入点。
    std::size_t const deviceLen = uvLen(static_cast<std::uint32_t>(packet.mDeviceId.size()))
                                + packet.mDeviceId.size();
    std::size_t const insertAt = body.size() - 4 /*BuildPlatform*/ - deviceLen;
    body.insert(body.begin() + static_cast<std::ptrdiff_t>(insertAt), '\0'); // 可选字段不存在
    return body;
}

std::string addActorBody(sculk::protocol::AddActorPacket const& packet) {
    // 26.60：Actor Links 之后（包尾）多了可选字段 PassengerBlockData
    std::string body = sculkBody(packet);
    body.push_back('\0');
    return body;
}

std::string bossEventBody(sculk::protocol::BossEventPacket const& packet) {
    // 26.60：去掉 PlayerID；EventType / Color / Overlay 都是 uint8
    std::string out;
    putVi64(out, packet.mTargetActorID);
    putU8(out, static_cast<std::uint8_t>(packet.mType));
    putStr(out, packet.mName);
    putStr(out, packet.mFilteredName);
    putF32(out, packet.mPercentage);
    putU8(out, static_cast<std::uint8_t>(packet.mColor));
    putU8(out, static_cast<std::uint8_t>(packet.mOverlay));
    return out;
}

} // namespace debugshape_export::wire26

namespace debugshape_export::wire26 {

std::string body26(sculk::protocol::AddPlayerPacket const& packet) { return addPlayerBody(packet); }
std::string body26(sculk::protocol::AddActorPacket const& packet) { return addActorBody(packet); }
std::string body26(sculk::protocol::BossEventPacket const& packet) { return bossEventBody(packet); }
std::string body26(sculk::protocol::PlayerListPacket const& packet) { return playerListBody(packet); }
std::string body26(sculk::protocol::PlayerSkinPacket const& packet) { return playerSkinBody(packet); }

} // namespace debugshape_export::wire26
