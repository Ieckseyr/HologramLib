// ItemActorExporter.h - 协议层掉落物 LSE 导出
#pragma once

#include "hologramlib/HologramLib.h" // hologramlib::IItemActor

namespace debugshape_export {

class ItemActorExporter {
public:
    static void exportAll();
};

// 公开接口适配（IHologramLib::itemActors()）
hologramlib::IItemActor& itemActorAdapter();

} // namespace debugshape_export
