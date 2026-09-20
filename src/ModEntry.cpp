#include "ModEntry.h"

#include "DiagLog.h"
#include "RemoteCallExporter.h"
#include "PacketDebugRenderer.h"
#include "FloatingTextExporter.h"
#include "GradientLineExporter.h"
#include "FloatingTextManager.h"
#include "GradientLineManager.h"
#include "itemdetail/ItemDetailExporter.h"
#include "itemdisplay/ItemDisplayExporter.h"
#include "itemdisplay/ItemDisplayManager.h"
#include "customentity/CustomEntityExporter.h"
#include "customentity/CustomEntityManager.h"
#include "particles/ParticleShapeExporter.h"
#include "particles/ParticleShapeManager.h"
#include "playernpc/PlayerNpcExporter.h"
#include "playernpc/PlayerNpcManager.h"
#include "trade/TradeMenuExporter.h"
#include "container/ContainerMenuExporter.h"
#include "npcdialog/NpcDialogueExporter.h"
#include "sensing/PlayerSensingExporter.h"
#include "lse/LseBridge.h"

#include "hologramlib/HologramLib.h"

#include "EventIdCompat.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/server/ServerStartedEvent.h>
#include <ll/api/mod/RegisterHelper.h>

#include <atomic>

namespace debugshape_export {

ModEntry& ModEntry::getInstance() {
    static ModEntry instance;
    return instance;
}

// LSE 导出状态（attach 成功且已完成统一命名空间 HologramLib 导出）
static std::atomic<bool> gLseExported{false};

static void exportLseFunctions() {
    if (gLseExported.exchange(true)) return; // 幂等
    RemoteCallExporter::exportAll();
    FloatingTextExporter::exportAll();
    GradientLineExporter::exportAll();
    ItemDetailExporter::exportAll();
    ItemDisplayExporter::exportAll();
    CustomEntityExporter::exportAll();
    ParticleShapeExporter::exportAll();
    PlayerNpcExporter::exportAll();
    TradeMenuExporter::exportAll();
    ContainerMenuExporter::exportAll();
    NpcDialogueExporter::exportAll();
    PlayerSensingExporter::exportAll();
}

bool ModEntry::load() {
    HLIB_LOG_INFO("HologramLib (unified hologram/shape/itemdetail library) loading...");
    return true;
}

bool ModEntry::enable() {
    // 构建时间戳: 与磁盘 DLL 的 LastWriteTime 对比即可确认部署的是否为当前构建
    HLIB_LOG_INFO("HologramLib enabling... (API 0x{:06X}, build {} {})", HOLOGLIB_API_VERSION, __DATE__, __TIME__);

    ItemDisplayManager::getInstance().init();
    CustomEntityManager::getInstance().init();
    ParticleShapeManager::getInstance().init();
    PlayerNpcManager::getInstance().init();
    // 玩家进服后重发全部可见形状(悬浮字/形状客户端不落盘, 重连必须补发)
    // 含周期兜底: 每 15s 全量重发, 与 PlayerJoinEvent 1s/5s 双保险
    PacketDebugRenderer::getInstance().init();

    // 运行时可选挂载 LegacyRemoteCall（无前置依赖）:
    // - lrca 已加载（顺序在前）→ 立即导出, LSE 可用
    // - lrca 未加载 → 监听 ServerStartedEvent 兜底（届时所有插件均已加载）
    if (hologramlib::lse::attach()) {
        exportLseFunctions();
        HLIB_LOG_INFO("LSE compat layer attached (LegacyRemoteCall detected).");
    } else {
        HLIB_LOG_INFO("LegacyRemoteCall not loaded yet; native C++ API active, will retry on ServerStarted.");
        ll::event::EventBus::getInstance().emplaceListener<ll::event::ServerStartedEvent>(
            [this](ll::event::ServerStartedEvent&) {
                if (hologramlib::lse::attach()) {
                    exportLseFunctions();
                    HLIB_LOG_INFO("LSE compat layer attached on ServerStarted.");
                } else {
                    HLIB_LOG_INFO(
                        "LegacyRemoteCall absent: LSE (ll.import) calls disabled; native C++ API unaffected."
                    );
                }
            }
        );
    }

    HLIB_LOG_INFO("HologramLib enabled successfully.");
    return true;
}

bool ModEntry::disable() {
    HLIB_LOG_INFO("HologramLib disabling...");

    // Destroy all shapes, release resources
    PacketDebugRenderer::getInstance().shutdown();
    PacketDebugRenderer::getInstance().destroyAll();
    FloatingTextManager::getInstance().destroyAll();
    GradientLineManager::getInstance().destroyAll();
    ItemDisplayManager::getInstance().shutdown();
    CustomEntityManager::getInstance().shutdown();
    ParticleShapeManager::getInstance().shutdown();
    PlayerNpcManager::getInstance().shutdown();

    HLIB_LOG_INFO("HologramLib disabled.");
    return true;
}

} // namespace debugshape_export

LL_REGISTER_MOD(debugshape_export::ModEntry, debugshape_export::ModEntry::getInstance());
