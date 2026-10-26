// InfoExporter.cpp - 库自身的元信息导出
#include "InfoExporter.h"

#include "hologramlib/HologramLib.h"
#include "lse/LseBridge.h"

#include <format>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void InfoExporter::exportAll() {
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
}

} // namespace debugshape_export
