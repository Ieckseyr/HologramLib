#pragma once

#include "hologramlib/HologramLib.h"

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

hologramlib::IBossBar&           bossBarAdapter();
hologramlib::IHud&               hudAdapter();
hologramlib::ILocatorBarExt&     locatorBarExtAdapter();
hologramlib::IActorEffect&       actorEffectAdapter();
hologramlib::ISidebar&           sidebarAdapter();
hologramlib::IPlayerRespawn&     playerRespawnAdapter();
hologramlib::IActorEffectFilter& actorEffectFilterAdapter();

} // namespace debugshape_export
