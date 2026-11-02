// HudExporter.h - Boss 条 / HUD / 方位条扩展 / 实体表现事件的导出与适配（1.33.0）
#pragma once

#include "hologramlib/HologramLib.h" // IBossBar / IHud / ILocatorBarExt / IActorEffect

namespace debugshape_export {

class BossBarExporter {
public:
    static void exportAll();
};

class HudExporter {
public:
    static void exportAll();
};

class LocatorBarExtExporter {
public:
    static void exportAll();
};

class SidebarExporter {
public:
    static void exportAll();
};

class ActorEffectExporter {
public:
    static void exportAll();
};

// 适配器（IHologramLib 的访问器用）
hologramlib::IBossBar&       bossBarAdapter();
hologramlib::IHud&           hudAdapter();
hologramlib::ILocatorBarExt& locatorBarExtAdapter();
hologramlib::IActorEffect&   actorEffectAdapter();
hologramlib::ISidebar&       sidebarAdapter();

} // namespace debugshape_export
