// PlayerNpcExporter.h - 假玩家 NPC LSE 导出层（1.16.0）
#pragma once

#include "hologramlib/HologramLib.h"

namespace debugshape_export {

class PlayerNpcExporter {
public:
    static void exportAll();
};

// IPlayerNpc 适配器单例（HologramLibImpl::playerNpcs() 返回引用）
hologramlib::IPlayerNpc& playerNpcAdapter();

} // namespace debugshape_export
