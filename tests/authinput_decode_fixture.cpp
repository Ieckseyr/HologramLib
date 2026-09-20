// authinput_decode_fixture.cpp - 用协议库离线解码 AuthInput 抓包（logs/authinput.hex → packets.bin）
//
// 目的: 回答"点击事件是不是在 AuthInput 里、在哪个字段"。不靠人肉猜字节偏移 —— 直接调
// sculk::protocol::PlayerAuthInputPacket::read() 把每个包解成字段, 再由 Python 逐包 diff
// 解码结果（见 tests/analyze-authinput.py）。
//
// 输出: 每个包一行 TSV, 只把**运动学字段**（旋转/位置/移动向量/相机）留在原位不打印,
// 其余字段全部原样打印（含全部 65 个输入位、可选部分的存在性与内部动作明细）。
// 解码失败或未完全消费的包会标 DECODE_FAIL, 那种包不能当依据。
//
// 用法: authinput_decode_fixture <packets.bin>
#include <sculk/protocol/codec/packet/PlayerAuthInputPacket.hpp>
#include <sculk/protocol/utility/ReadOnlyBinaryStream.hpp>

#include <cstring>
#include <span>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

using sculk::protocol::PlayerAuthInputPacket;
using sculk::protocol::ReadOnlyBinaryStream;

// 读 varint 长度前缀的包流（由 analyze-authinput.py 从 authinput.hex 生成）
bool readPackets(std::string const& path, std::vector<std::vector<std::byte>>& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::string const raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<std::byte> all(raw.size());
    if (!raw.empty()) std::memcpy(all.data(), raw.data(), raw.size());
    std::size_t at = 0;
    auto        readVarInt = [&](std::uint32_t& value) {
        value     = 0;
        int shift = 0;
        while (at < all.size()) {
            auto const b = static_cast<std::uint8_t>(all[at++]);
            value |= static_cast<std::uint32_t>(b & 0x7F) << shift;
            if (!(b & 0x80)) return true;
            shift += 7;
            if (shift > 28) return false;
        }
        return false;
    };
    while (at < all.size()) {
        std::uint32_t len = 0;
        if (!readVarInt(len) || len == 0 || at + len > all.size()) return false;
        out.emplace_back(all.begin() + static_cast<std::ptrdiff_t>(at), all.begin() + static_cast<std::ptrdiff_t>(at + len));
        at += len;
    }
    return !out.empty();
}

std::string bitsOf(std::bitset<65> const& bits) {
    std::string out;
    for (std::size_t i = 0; i < bits.size(); ++i) {
        if (bits.test(i)) {
            if (!out.empty()) out += ',';
            out += std::to_string(i);
        }
    }
    return out.empty() ? "-" : out;
}

// 内嵌物品请求的动作明细（"点击"最可能的载体）: 动作类型 + src/dst 容器与槽位。
// 用字符串拼接而不是 std::format: 这里字段类型是协议库的 TypedStorage 包装, 拼接更少踩坑。
std::string describeRequestActions(sculk::protocol::ItemStackRequestData const& req) {
    std::string out;
    for (auto const& action : req.mActions) {
        out += " actionType=" + std::to_string(static_cast<int>(action.mActionType));
        std::visit(
            [&](auto const& data) {
                if constexpr (requires { data.mSource; }) {
                    out += " src(c=" + std::to_string(static_cast<int>(data.mSource.mFullContainerName.mContainerEnumName))
                         + " slot=" + std::to_string(static_cast<int>(data.mSource.mSlot)) + ")";
                }
                if constexpr (requires { data.mDestination; }) {
                    out += " dst(c=" + std::to_string(static_cast<int>(data.mDestination.mFullContainerName.mContainerEnumName))
                         + " slot=" + std::to_string(static_cast<int>(data.mDestination.mSlot)) + ")";
                }
                if constexpr (requires { data.mAmount; }) {
                    out += " amount=" + std::to_string(static_cast<int>(data.mAmount));
                }
                if constexpr (requires { data.mRecipeNetworkIdOrCreativeId; }) {
                    out += " recipeNetId=" + std::to_string(data.mRecipeNetworkIdOrCreativeId);
                }
            },
            action.mVariant
        );
    }
    return out;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: authinput_decode_fixture <packets.bin>\n";
        return 2;
    }
    std::vector<std::vector<std::byte>> packets;
    if (!readPackets(argv[1], packets)) {
        std::cerr << "cannot read packet stream: " << argv[1] << '\n';
        return 1;
    }

    for (std::size_t i = 0; i < packets.size(); ++i) {
        auto const& bytes = packets[i];
        ReadOnlyBinaryStream stream{std::span<const std::byte>{bytes.data(), bytes.size()}};

        PlayerAuthInputPacket packet;
        auto const           result = packet.read(stream);
        bool const           consumedAll =
            stream.getPosition() == bytes.size();

        if (!result || !consumedAll) {
            std::cout << std::format(
                "#{}\tDECODE_FAIL\tlen={}\tconsumed={}\n",
                i,
                bytes.size(),
                stream.getPosition()
            );
            continue;
        }

        auto const useTxnActionCount =
            packet.mItemUseTransaction.mItemUseTransaction.mTransaction.mActions.size();
        std::cout << std::format(
            "#{}\tlen={}\tbits=[{}]\tinputType={}\tplayMode={}\tmodel={}\ttick={}\t"
            "interactRot=({:.2f},{:.2f})\tuseTxnActions={}\tuseTxnLegacySlots={}\t"
            "reqActions={}\tblockActions={}\tpredictedVehicle={}\n",
            i,
            bytes.size(),
            bitsOf(packet.mInputData),
            packet.mInputType,
            packet.mPlayMode,
            packet.mNewInteractionModel,
            packet.mClientTick,
            packet.mInteractRotation.mX,
            packet.mInteractRotation.mY,
            useTxnActionCount,
            packet.mItemUseTransaction.mLegacySetItemSlots.size(),
            packet.mItemStackRequestData.mActions.size(),
            packet.mPlayerBlockActions.mActions.size(),
            packet.mClientPredictedVihicle
        );
        if (!packet.mItemStackRequestData.mActions.empty()) {
            std::cout << std::format(
                "#{}  >> 内嵌请求: requestId={} 动作:{}\n",
                i,
                packet.mItemStackRequestData.mClientRequestId,
                describeRequestActions(packet.mItemStackRequestData)
            );
        }
    }
    return 0;
}
