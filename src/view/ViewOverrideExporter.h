// ViewOverrideExporter.h - 客户端视图覆盖 LSE 导出声明
#pragma once

#include "hologramlib/HologramLib.h"

#include <string>

namespace debugshape_export {

class ViewOverrideExporter {
public:
    static void exportAll();
};

// spec 串 → EntityView（"type=minecraft:cow;name=§c奶牛;always=1"）。
// 供其它域复用（如 npcDialog 的 avatarViewSpec）—— 与 viewEntity 的 spec 语法同一份实现。
[[nodiscard]] hologramlib::EntityView parseEntitySpec(std::string const& spec);

} // namespace debugshape_export
