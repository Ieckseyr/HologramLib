// InfoExporter.cpp - 库自身的元信息导出
#include "InfoExporter.h"

#include "core/PacketSink.h"
#include "hologramlib/HologramLib.h"
#include "lse/LseBridge.h"

#include <format>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void InfoExporter::exportAll() {
    // wireStats() -> "encoded=N sent=N noPeer=N invalid=N"
    //   发包总账。**invalid 增长**说明某个包的字节过不了 BDS 回读校验（该包被静默丢弃），
    //   排查"调了接口但客户端没反应"先看这一项。
    hologramlib::lse::exportAs(NAMESPACE, "wireStats", []() -> std::string {
        auto& c = wire::counters();
        return std::format(
            "encoded={} sent={} noPeer={} invalid={}",
            c.encoded.load(std::memory_order_relaxed),
            c.sent.load(std::memory_order_relaxed),
            c.droppedNoPeer.load(std::memory_order_relaxed),
            c.droppedInvalid.load(std::memory_order_relaxed));
    });

    // wireResetStats() -> null
    hologramlib::lse::exportAs(NAMESPACE, "wireResetStats", []() {
        auto& c = wire::counters();
        c.encoded.store(0, std::memory_order_relaxed);
        c.sent.store(0, std::memory_order_relaxed);
        c.droppedNoPeer.store(0, std::memory_order_relaxed);
        c.droppedInvalid.store(0, std::memory_order_relaxed);
    });

    // holoInfo() -> JSON: {"apiVersion":"1.32.0","apiVersionNumber":73984,"abiStamp":1}
    //   脚本导入模块用它核对版本。
    hologramlib::lse::exportAs(NAMESPACE, "holoInfo", []() -> std::string {
        auto const number = static_cast<std::uint32_t>(HOLOGLIB_API_VERSION);
        auto const major  = number >> 16;
        auto const minor  = ((number >> 8) & 0xFF) - 6; // 与头文件注释的换算一致
        auto const patch  = number & 0xFF;
        return std::format(
            R"({{"apiVersion":"{}.{}.{}","apiVersionNumber":{},"abiStamp":{}}})",
            major,
            minor,
            patch,
            number,
            static_cast<std::uint32_t>(HOLOGLIB_ABI_STAMP));
    });

    // holoApiVersion() -> int，原始版本号
    hologramlib::lse::exportAs(NAMESPACE, "holoApiVersion", []() -> int64_t {
        return static_cast<int64_t>(HOLOGLIB_API_VERSION);
    });

    // ── 发包审计（测试用）──────────────────────────────────────────────
    // 只记录 trace 期间"真的交给 NetworkPeer"的包；测试期间玩家退出会自动落盘
    // plugins/HologramLib/wire-trace-<player>-<epoch>.json
    hologramlib::lse::exportAs(NAMESPACE, "wireTraceStart", [](std::string const& tag) -> bool {
        wire::traceStart(tag);
        return true;
    });
    hologramlib::lse::exportAs(NAMESPACE, "wireTraceStop", []() -> bool {
        wire::traceStop();
        return true;
    });
    hologramlib::lse::exportAs(NAMESPACE, "wireTraceClear", []() {
        wire::traceClear();
    });
    hologramlib::lse::exportAs(NAMESPACE, "wireTraceActive", []() -> bool {
        return wire::traceActive();
    });
    hologramlib::lse::exportAs(NAMESPACE, "wireTraceCount", []() -> int64_t {
        return static_cast<int64_t>(wire::traceCount());
    });
    hologramlib::lse::exportAs(NAMESPACE, "wireTraceDump", []() -> std::string {
        return wire::traceDumpJson();
    });
    hologramlib::lse::exportAs(NAMESPACE, "wireTraceMark", [](std::string const& tag) {
        wire::traceMark(tag);
    });
}

} // namespace debugshape_export
