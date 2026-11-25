// 形状编码的版本选形自测（不依赖服务端）。
//
// 为什么要这条：TextDataPayload 的 line_gap_height 只在 2193+ 的线里有；2168 的结构体没有这个成员。
// 编码侧一旦误用 2193/2225 的结构体，每个文本形状会多写 4 个字节，后面所有字段整体错位
// （客户端表现是悬浮字错位/花屏）。这条测试用**当前 kProtocolVersion 的读端**把 body 解回来：
//   1) 读端必须正好吃掉整个 body（有尾巴 = 写多了）
//   2) text 字段与变体 tag 对得上
//   3) line_gap_height 的存在性必须与 kProtocolVersion 对齐
//
// 跑法（Windows / clang-cl，路径按仓库布局）：
//   clang-cl /std:c++23 /EHsc /I . /I ..\..\.llsdk\endstone-proto\install\include ^
//       tests\shape_wire_version_test.cpp core\ShapeWire.cpp core\PacketCodec.cpp ^
//       /link /LIBPATH:..\..\.llsdk\endstone-proto\install\lib bedrock_protocol.lib

#include <cstdio>
#include <string>
#include <variant>

#include "core/ShapeWire.h"
#include "core/proto/Version.h"

namespace core = debugshape_export::core;
namespace holo = debugshape_export;
namespace proto = debugshape_export::core::proto;
namespace bp = ::bedrock::protocol;

// 选形自检：2168 的 TextDataPayload 不该有 line_gap_height，2193+ 才有
static_assert(
    proto::HasLineGapHeight<proto::TextDataPayload> == (proto::kProtocolVersion >= 2193),
    "TextDataPayload.line_gap_height 的选形与 kProtocolVersion 不一致"
);

namespace {

int gFailures = 0;

void check(bool ok, char const* what) {
    if (!ok) {
        std::printf("  FAIL %s\n", what);
        ++gFailures;
    } else {
        std::printf("  ok   %s\n", what);
    }
}

holo::DebugShape makeTextShape() {
    holo::DebugShape shape;
    shape.mNetworkId = 1'234'567'890'123ULL;
    shape.mType      = holo::DebugShapeType::Text;
    shape.mLocation  = holo::Vec3{100.5f, 64.25f, -30.75f};
    shape.mScale     = 2.0f;

    holo::DebugText text;
    text.mText             = "HologramLib 26.40";
    text.mUseRotation      = true;
    text.mLineGapHeight    = 0.75f; // 2168 线上没有这个字段，值必须被悄悄丢掉
    text.mDepthTest        = true;
    text.mShowBackface     = true;
    text.mShowTextBackface = true;
    text.mBackgroundColor  = 0x80112233; // ARGB: A=0x80 R=0x11 G=0x22 B=0x33
    shape.mShape           = text;
    return shape;
}

} // namespace

int main() {
    std::printf(
        "shape wire test @ protocol %d（%s）\n",
        proto::kProtocolVersion,
        proto::kProtocolVersion < 2193 ? "无 line_gap_height" : "有 line_gap_height"
    );

    auto const body = core::buildPrimitiveShapesBody({makeTextShape()});
    check(!body.empty(), "body 非空");

    bp::BinaryReader reader{body};
    auto             parsed = bp::deserialize<proto::PrimitiveShapesPacket>(reader);

    check(parsed.has_value(), "按当前协议版本解码成功");
    if (!parsed) {
        return 1;
    }

    auto const& packet = *parsed;
    check(packet.shapes.size() == 1, "解出 1 个形状");
    check(reader.getUnreadLength() == 0, "读端正好吃掉整个 body（没有多余字节）");

    if (packet.shapes.size() == 1) {
        auto const& shape = packet.shapes[0];
        check(shape.network_id == 1'234'567'890'123ULL, "network_id 往返一致");

        auto const* text = std::get_if<proto::TextDataPayload>(&shape.extra_data_payload);
        check(text != nullptr, "变体 tag 命中文本载荷");
        if (text != nullptr) {
            check(text->text == "HologramLib 26.40", "text 往返一致");
            check(text->use_rotation, "use_rotation 往返一致");
            check(text->depth_test, "depth_test 往返一致");
            check(text->show_backface, "show_backface 往返一致");
            check(text->show_text_backface, "show_text_backface 往返一致");
            check(text->background_color.has_value(), "background_color 往返一致");
            if (text->background_color) {
                check(
                    static_cast<std::int32_t>(*text->background_color) == 0x80112233,
                    "background_color 颜色值一致（ARGB）"
                );
            }
        }
    }

    // 删除包：只有 network_id，不带 shape_type / 载荷
    holo::DebugShape remove;
    remove.mNetworkId = 42;
    auto const removeBody = core::buildPrimitiveShapesBody({remove});
    bp::BinaryReader removeReader{removeBody};
    auto             removeParsed = bp::deserialize<proto::PrimitiveShapesPacket>(removeReader);
    check(removeParsed.has_value() && removeParsed->shapes.size() == 1, "删除包可解码");
    if (removeParsed && removeParsed->shapes.size() == 1) {
        check(!removeParsed->shapes[0].shape_type.has_value(), "删除包不带 shape_type");
        check(removeReader.getUnreadLength() == 0, "删除包无多余字节");
    }

    if (gFailures == 0) {
        std::printf("SHAPE-WIRE: PASS\n");
        return 0;
    }
    std::printf("SHAPE-WIRE: %d 项失败\n", gFailures);
    return 1;
}
