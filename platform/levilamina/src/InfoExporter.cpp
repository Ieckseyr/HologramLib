#include "InfoExporter.h"

#include "kernel/PacketSink.h"
#include "hologramlib/HologramLib.h"
#include "lse/LseBridge.h"

#include <format>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void InfoExporter::exportAll() {

    hologramlib::lse::exportAs(NAMESPACE, "wireStats", []() -> std::string {
        auto& c = wire::counters();
        return std::format(
            "encoded={} sent={} noPeer={} invalid={}",
            c.encoded.load(std::memory_order_relaxed),
            c.sent.load(std::memory_order_relaxed),
            c.droppedNoPeer.load(std::memory_order_relaxed),
            c.droppedInvalid.load(std::memory_order_relaxed));
    });

    hologramlib::lse::exportAs(NAMESPACE, "wireResetStats", []() {
        auto& c = wire::counters();
        c.encoded.store(0, std::memory_order_relaxed);
        c.sent.store(0, std::memory_order_relaxed);
        c.droppedNoPeer.store(0, std::memory_order_relaxed);
        c.droppedInvalid.store(0, std::memory_order_relaxed);
    });

    hologramlib::lse::exportAs(NAMESPACE, "holoInfo", []() -> std::string {
        auto const number = static_cast<std::uint32_t>(HOLOGLIB_API_VERSION);
        auto const major  = number >> 16;
        auto const minor  = ((number >> 8) & 0xFF) - 6;
        auto const patch  = number & 0xFF;
        return std::format(
            R"({{"apiVersion":"{}.{}.{}","apiVersionNumber":{},"abiStamp":{}}})",
            major,
            minor,
            patch,
            number,
            static_cast<std::uint32_t>(HOLOGLIB_ABI_STAMP));
    });

    hologramlib::lse::exportAs(NAMESPACE, "holoAbiVersion", []() -> std::string {
        return HOLOGLIB_ABI_VERSION;
    });

    hologramlib::lse::exportAs(NAMESPACE, "holoLseVersion", []() -> std::string {
        return HOLOGLIB_LSE_API_VERSION;
    });

    hologramlib::lse::exportAs(NAMESPACE, "holoEndstoneVersion", []() -> std::string {
        return HOLOGLIB_ENDSTONE_VERSION;
    });

    hologramlib::lse::exportAs(NAMESPACE, "holoApiVersion", []() -> int64_t {
        return static_cast<int64_t>(HOLOGLIB_API_VERSION);
    });

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
