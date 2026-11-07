#include "ModEntry.h"
#include "kernel/EventIds.h"
#include "core/HookRegistry.h"
#include "kernel/PlayerIndex.h"
#include "core/TickDriver.h"

#include "DiagLog.h"

#include "FloatingTextExporter.h"
#include "FloatingTextManager.h"
#include "GradientLineExporter.h"
#include "GradientLineManager.h"
#include "PacketDebugRenderer.h"
#include "RemoteCallExporter.h"

#include "InfoExporter.h"
#include "actor/ActorEffectManager.h"
#include "anchor/AnchorPool.h"
#include "block/BlockBatchExporter.h"
#include "block/BlockBatchManager.h"
#include "block/FakeBlockExporter.h"
#include "container/ContainerMenuExporter.h"
#include "container/ContainerMenuManager.h"
#include "core/PacketTrace.h"
#include "customentity/CustomEntityExporter.h"
#include "customentity/CustomEntityManager.h"
#include "fakeinv/FakeInventoryExporter.h"
#include "fakeinv/FakeInventoryManager.h"
#include "ghost/GhostInteractRouter.h"
#include "hud/BossBarManager.h"
#include "hud/HudExporter.h"
#include "hud/HudManager.h"
#include "itemactor/ItemActorExporter.h"
#include "itemactor/ItemActorManager.h"
#include "itemdetail/ItemDetailExporter.h"
#include "itemdisplay/ItemDisplayExporter.h"
#include "itemdisplay/ItemDisplayManager.h"
#include "lse/LseBridge.h"
#include "npcdialog/NpcDialogueExporter.h"
#include "npcdialog/NpcDialogueManager.h"
#include "particles/ParticleShapeExporter.h"
#include "particles/ParticleShapeManager.h"
#include "playernpc/PlayerNpcExporter.h"
#include "playernpc/PlayerNpcManager.h"
#include "scoreboard/SidebarManager.h"
#include "sensing/PlayerSensingExporter.h"
#include "sensing/PlayerSensingManager.h"
#include "sulfur/SulfurDisplayExporter.h"
#include "trade/TradeMenuExporter.h"
#include "trade/TradeMenuManager.h"
#include "ui/ClientUiExporter.h"
#include "ui/FormControlManager.h"
#include "view/ViewOverrideExporter.h"
#include "view/ViewOverrideManager.h"
#include "visibility/PlayerVisibilityManager.h"

#include "hologramlib/HologramLib.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/server/ServerStartedEvent.h>
#include <ll/api/mod/RegisterHelper.h>

#include <atomic>

namespace debugshape_export {

ModEntry& ModEntry::getInstance() {
    static ModEntry instance;
    return instance;
}

namespace {

std::atomic<bool> gLseExported{false};

core::Domain const kDomains[] = {
    {"itemdisplay",
     +[] {
         ItemDisplayManager::getInstance().init();
         return true;
     },
     +[] {
         ItemDisplayManager::getInstance().shutdown();
     },
     +[] {
         ItemDisplayExporter::exportAll();
     }},
    {"customentity",
     +[] {
         CustomEntityManager::getInstance().init();
         return true;
     },
     +[] {
         CustomEntityManager::getInstance().shutdown();
     },
     +[] {
         CustomEntityExporter::exportAll();
     }},
    {"particles",
     +[] {
         ParticleShapeManager::getInstance().init();
         return true;
     },
     +[] {
         ParticleShapeManager::getInstance().shutdown();
     },
     +[] {
         ParticleShapeExporter::exportAll();
     }},
    {"playernpc",
     +[] {
         PlayerNpcManager::getInstance().init();
         return true;
     },
     +[] {
         PlayerNpcManager::getInstance().shutdown();
     },
     +[] {
         PlayerNpcExporter::exportAll();
     }},
    {"trade",
     +[] {
         TradeMenuManager::getInstance().init();
         return true;
     },
     +[] {
         TradeMenuManager::getInstance().shutdown();
     },
     +[] {
         TradeMenuExporter::exportAll();
     }},
    {"view",
     +[] {
         view::initViewOverrideEvents();
         return true;
     },
     +[] {
         view::ViewOverrideManager::getInstance().shutdown();
     },
     +[] {
         ViewOverrideExporter::exportAll();
     }},
    {"shape",
     +[] {
         PacketDebugRenderer::getInstance().init();
         return true;
     },
     +[] {
         PacketDebugRenderer::getInstance().shutdown();
         PacketDebugRenderer::getInstance().destroyAll();
     },
     +[] {
         RemoteCallExporter::exportAll();
     }},
    {"hologramtext",
     +[] {
         FloatingTextManager::getInstance().initDynamicDriver();
         return true;
     },
     +[] {
         FloatingTextManager::getInstance().shutdownDynamicDriver();
         FloatingTextManager::getInstance().destroyAll();
     },
     +[] {
         FloatingTextExporter::exportAll();
     }},
    {"gradientline", nullptr,
     +[] {
         GradientLineManager::getInstance().destroyAll();
     },
     +[] {
         GradientLineExporter::exportAll();
     }},
    {"itemdetail", nullptr, nullptr, +[] {
         ItemDetailExporter::exportAll();
     }},
    {"container", nullptr, +[] {
         ContainerMenuManager::getInstance().shutdown();
     },
     +[] {
         ContainerMenuExporter::exportAll();
    FakeBlockExporter::exportAll();
     }},
    {"npcdialog", nullptr, +[] {
         NpcDialogueManager::getInstance().shutdown();
     },
     +[] {
         NpcDialogueExporter::exportAll();
     }},
    {"sensing", nullptr, +[] {
         PlayerSensingManager::getInstance().shutdown();
     },
     +[] {
         PlayerSensingExporter::exportAll();
     }},
    {"fakeinv", nullptr, +[] {
         FakeInventoryManager::getInstance().shutdown();
     },
     +[] {
         FakeInventoryExporter::exportAll();
     }},
    {"blockbatch",
     +[] {
         blockbatch::BlockBatchManager::getInstance().init();
         return true;
     },
     +[] {
         blockbatch::BlockBatchManager::getInstance().shutdown();
     },
     +[] {
         blockbatch::BlockBatchExporter::exportAll();
     }},
    {"sulfur", nullptr, nullptr, +[] {
         SulfurDisplayExporter::exportAll();
     }},
    {"ghost", nullptr, nullptr, +[] {
         GhostInteractRouter::exportLse();
     }},
    {"info", nullptr, nullptr, +[] {
         InfoExporter::exportAll();
     }},
    {"bossbar", nullptr, +[] {
         BossBarManager::getInstance().shutdown();
     },
     +[] {
         BossBarExporter::exportAll();
     }},
    {"hud", nullptr, nullptr, +[] {
         HudExporter::exportAll();
     }},
    {"actoreffect", nullptr, nullptr, +[] {
         ActorEffectExporter::exportAll();
     }},
    {"anchor", nullptr, +[] {
         AnchorPool::getInstance().shutdown();
     },
     nullptr},
    {"sidebar", nullptr, +[] {
         SidebarManager::getInstance().shutdown();
     },
     +[] {
         SidebarExporter::exportAll();
     }},
    {"itemactor", nullptr, +[] {
         ItemActorManager::getInstance().shutdown();
     },
     +[] {
         ItemActorExporter::exportAll();
     }},
    {"form", nullptr, nullptr, +[] {
         FormControlExporter::exportAll();
     }},
    {"visibility", nullptr, +[] {
         PlayerVisibilityManager::getInstance().shutdown();
     },
     +[] {
         PlayerVisibilityExporter::exportAll();
     }},
};

// 掉线清理挂在组合根：各域只提供 onPlayerLeave，不自己注册监听。
// 不接这些的效果分别是：容器菜单占满 101..199 的 id（此后所有箱子都开不了）、
// 对话条目与合成载体留在表里、Boss 条 / 侧边栏 / 玩家可见性 每条留一个死条目。
core::PlayerLeaveHook const gContainerLeaveHook{[](std::string const& playerName) {
    ContainerMenuManager::getInstance().onPlayerLeave(playerName);
}};

core::PlayerLeaveHook const gNpcDialogLeaveHook{[](std::string const& playerName) {
    NpcDialogueManager::getInstance().onPlayerLeave(playerName);
}};

core::PlayerLeaveHook const gBossBarLeaveHook{[](std::string const& playerName) {
    BossBarManager::getInstance().onPlayerLeave(playerName);
}};

core::PlayerLeaveHook const gSidebarLeaveHook{[](std::string const& playerName) {
    SidebarManager::getInstance().onPlayerLeave(playerName);
}};

core::PlayerLeaveHook const gBlockBatchLeaveHook{[](std::string const& playerName) {
    blockbatch::BlockBatchManager::getInstance().onPlayerLeave(playerName);
}};

core::PlayerLeaveHook const gPlayerVisibilityLeaveHook{[](std::string const& playerName) {
    PlayerVisibilityManager::getInstance().onPlayerLeave(playerName);
}};

void exportLseFunctions(core::ServiceHost& host) {
    if (gLseExported.exchange(true))
        return;
    host.exportAll();
}

} // namespace

bool ModEntry::load() {
    HLIB_LOG_INFO("HologramLib (unified hologram/shape/itemdetail library) loading...");
    return true;
}

bool ModEntry::enable() {

    HLIB_LOG_INFO("HologramLib enabling... (API 0x{:06X}, build {} {})", HOLOGLIB_API_VERSION, __DATE__, __TIME__);

    HLIB_LOG_INFO("[tick] order: {}", tick::TickDriver::getInstance().describe());

    for (auto const& domain : kDomains) {
        mHost.add(domain);
    }

    if (!mHost.startAll()) {
        HLIB_LOG_ERROR("[host] domain start failed; HologramLib stays disabled");
        return false;
    }
    HLIB_LOG_INFO("[host] {} domains: {}", mHost.size(), mHost.describe());

    if (hologramlib::lse::attach()) {
        exportLseFunctions(mHost);
        HLIB_LOG_INFO("LSE compat layer attached (LegacyRemoteCall detected).");
    } else {
        HLIB_LOG_INFO("LegacyRemoteCall not loaded yet; native C++ API active, will retry on ServerStarted.");
        debugshape_export::core::listen<ll::event::ServerStartedEvent>(core::kServerStartedId,
                                                                       [this](ll::event::ServerStartedEvent&) {
                                                                           if (hologramlib::lse::attach()) {
                                                                               exportLseFunctions(mHost);
                                                                               HLIB_LOG_INFO("LSE compat layer attached on ServerStarted.");
                                                                           } else {
                                                                               HLIB_LOG_INFO(
                                                                                   "LegacyRemoteCall absent: LSE (ll.import) calls disabled; native C++ API unaffected.");
                                                                           }
                                                                       });
    }

    core::setLive(true);
    core::hookAll(); // 上一次 disable 摘下的 detour 在这里重新挂上（首次 enable 是空操作）
    if (auto mod = ll::mod::NativeMod::current(); mod) {
        mod->getLogger().info("[hooks] {} 个 detour 已挂载（disable 时统一摘下）", core::hookCount());
    }
    tick::installDriver();
    core::PlayerIndex::getInstance().install();

    wire::traceInit();

    HLIB_LOG_INFO("HologramLib enabled successfully.");
    return true;
}

bool ModEntry::disable() {
    HLIB_LOG_INFO("HologramLib disabling...");
    // 顺序有讲究：先让 detour 透传（live=false），再停域，最后摘钩。反过来会在"域已清空、钩子仍在"的窗口里被别的线程回调，正是 0xC0000005 的来源。
    core::setLive(false);
    // 不摘钩：unhook 会释放蹦床，正在执行中的 detour 走到 origin() 就会跳进已释放的内存；（实测崩在 detour 的 origin 调用点）。所有 detour 都由 core::live() 门控，disable 后一律透传。
    mHost.stopAll();
    core::PlayerIndex::getInstance().shutdown();
    core::removeAllListeners();
    HLIB_LOG_INFO("HologramLib disabled.");
    return true;
}

} // namespace debugshape_export

LL_REGISTER_MOD(debugshape_export::ModEntry, debugshape_export::ModEntry::getInstance());
