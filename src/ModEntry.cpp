#include "ModEntry.h"

#include "DiagLog.h"
#include "EventIdCompat.h"

#include "FloatingTextExporter.h"
#include "FloatingTextManager.h"
#include "GradientLineExporter.h"
#include "GradientLineManager.h"
#include "PacketDebugRenderer.h"
#include "RemoteCallExporter.h"

#include "container/ContainerMenuExporter.h"
#include "container/ContainerMenuManager.h"
#include "core/TickDriver.h"
#include "customentity/CustomEntityExporter.h"
#include "customentity/CustomEntityManager.h"
#include "fakeinv/FakeInventoryExporter.h"
#include "fakeinv/FakeInventoryManager.h"
#include "ghost/GhostInteractRouter.h"
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
#include "sensing/PlayerSensingExporter.h"
#include "sensing/PlayerSensingManager.h"
#include "sulfur/SulfurDisplayExporter.h"
#include "trade/TradeMenuExporter.h"
#include "trade/TradeMenuManager.h"
#include "view/ViewOverrideExporter.h"
#include "view/ViewOverrideManager.h"

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

// LSE 导出状态（attach 成功且已完成统一命名空间 HologramLib 导出）
std::atomic<bool> gLseExported{false};

// ── 域适配器 ─────────────────────────────────────────────────────────────────
// 一个域只描述三件事: 怎么起 / 怎么停 / 导出什么。
// 顺序由 ModEntry::enable() 里的 add 顺序决定（stop 自动反向），不再有三张需要同步维护的清单。
//
// 历史: 收敛前 enable()/disable()/exportLseFunctions() 各有一张手工清单，ghost 域的两个导出
// 曾长期漏在注册流程外（脚本 ll.import 得 null）。另外 ContainerMenu / NpcDialogue /
// PlayerSensing / FakeInventory 四个域的 shutdown() 写好了但从来没人调用 —— 现在在这里补上。

struct ItemDisplayDomain final : core::IDomain {
    char const* name() const noexcept override { return "itemdisplay"; }
    bool        start() override {
        ItemDisplayManager::getInstance().init();
        return true;
    }
    void stop() noexcept override { ItemDisplayManager::getInstance().shutdown(); }
    void exportLse() override { ItemDisplayExporter::exportAll(); }
};

struct CustomEntityDomain final : core::IDomain {
    char const* name() const noexcept override { return "customentity"; }
    bool        start() override {
        CustomEntityManager::getInstance().init();
        return true;
    }
    void stop() noexcept override { CustomEntityManager::getInstance().shutdown(); }
    void exportLse() override { CustomEntityExporter::exportAll(); }
};

struct ParticleShapeDomain final : core::IDomain {
    char const* name() const noexcept override { return "particles"; }
    bool        start() override {
        ParticleShapeManager::getInstance().init();
        return true;
    }
    void stop() noexcept override { ParticleShapeManager::getInstance().shutdown(); }
    void exportLse() override { ParticleShapeExporter::exportAll(); }
};

struct PlayerNpcDomain final : core::IDomain {
    char const* name() const noexcept override { return "playernpc"; }
    bool        start() override {
        PlayerNpcManager::getInstance().init();
        return true;
    }
    void stop() noexcept override { PlayerNpcManager::getInstance().shutdown(); }
    void exportLse() override { PlayerNpcExporter::exportAll(); }
};

struct TradeMenuDomain final : core::IDomain {
    char const* name() const noexcept override { return "trade"; }
    bool        start() override {
        // 挂掉线清理（关菜单 + 把真结算账本里暂存的付费还给玩家 —— 掉线瞬间玩家对象还在）
        TradeMenuManager::getInstance().init();
        return true;
    }
    void stop() noexcept override { TradeMenuManager::getInstance().shutdown(); }
    void exportLse() override { TradeMenuExporter::exportAll(); }
};

struct ViewOverrideDomain final : core::IDomain {
    char const* name() const noexcept override { return "view"; }
    bool        start() override {
        // 掉线时清掉运行时 id 索引（出站钩子在 OutboundViewHook.cpp 自注册）
        view::initViewOverrideEvents();
        return true;
    }
    void stop() noexcept override { view::ViewOverrideManager::getInstance().shutdown(); }
    void exportLse() override { ViewOverrideExporter::exportAll(); }
};

struct ShapeDomain final : core::IDomain {
    char const* name() const noexcept override { return "shape"; }
    bool        start() override {
        // 进服后重发全部可见形状（客户端不落盘, 重连必须补发）; 含 15s 周期兜底
        PacketDebugRenderer::getInstance().init();
        return true;
    }
    void stop() noexcept override {
        PacketDebugRenderer::getInstance().shutdown();
        PacketDebugRenderer::getInstance().destroyAll(); // 补发移除包, 避免客户端残留
    }
    void exportLse() override { RemoteCallExporter::exportAll(); }
};

struct HologramTextDomain final : core::IDomain {
    char const* name() const noexcept override { return "hologramtext"; }
    bool        start() override {
        // 动态行驱动（内容池轮播 / 变量刷新, 0.5s 节流）
        FloatingTextManager::getInstance().initDynamicDriver();
        return true;
    }
    void stop() noexcept override {
        FloatingTextManager::getInstance().shutdownDynamicDriver();
        FloatingTextManager::getInstance().destroyAll();
    }
    void exportLse() override { FloatingTextExporter::exportAll(); }
};

// 渐变线没有启动动作（线段批由消费方按需驱动），只有关停清理
struct GradientLineDomain final : core::IDomain {
    char const* name() const noexcept override { return "gradientline"; }
    void        stop() noexcept override { GradientLineManager::getInstance().destroyAll(); }
    void        exportLse() override { GradientLineExporter::exportAll(); }
};

// 以下四个域此前只有导出、没有生命周期入口; shutdown() 存在但从未被调用（见文件头说明）
struct ItemDetailDomain final : core::IDomain {
    char const* name() const noexcept override { return "itemdetail"; }
    void        exportLse() override { ItemDetailExporter::exportAll(); }
};

struct ContainerMenuDomain final : core::IDomain {
    char const* name() const noexcept override { return "container"; }
    void        stop() noexcept override { ContainerMenuManager::getInstance().shutdown(); }
    void        exportLse() override { ContainerMenuExporter::exportAll(); }
};

struct NpcDialogueDomain final : core::IDomain {
    char const* name() const noexcept override { return "npcdialog"; }
    void        stop() noexcept override { NpcDialogueManager::getInstance().shutdown(); }
    void        exportLse() override { NpcDialogueExporter::exportAll(); }
};

struct PlayerSensingDomain final : core::IDomain {
    char const* name() const noexcept override { return "sensing"; }
    void        stop() noexcept override { PlayerSensingManager::getInstance().shutdown(); }
    void        exportLse() override { PlayerSensingExporter::exportAll(); }
};

struct FakeInventoryDomain final : core::IDomain {
    char const* name() const noexcept override { return "fakeinv"; }
    void        stop() noexcept override { FakeInventoryManager::getInstance().shutdown(); }
    void        exportLse() override { FakeInventoryExporter::exportAll(); }
};

struct SulfurDisplayDomain final : core::IDomain {
    char const* name() const noexcept override { return "sulfur"; }
    void        exportLse() override { SulfurDisplayExporter::exportAll(); }
};

// ghost 域: 钩子在静态初始化期自注册, 这里只需要它的两个 LSE 导出（曾漏过一次）
struct GhostDomain final : core::IDomain {
    char const* name() const noexcept override { return "ghost"; }
    void        exportLse() override { GhostInteractRouter::exportLse(); }
};

void exportLseFunctions(core::ServiceHost& host) {
    if (gLseExported.exchange(true)) return; // 幂等
    host.exportAll();
}

} // namespace

bool ModEntry::load() {
    HLIB_LOG_INFO("HologramLib (unified hologram/shape/itemdetail library) loading...");
    return true;
}

bool ModEntry::enable() {
    // 构建时间戳: 与磁盘 DLL 的 LastWriteTime 对比即可确认部署的是否为当前构建
    HLIB_LOG_INFO("HologramLib enabling... (API 0x{:06X}, build {} {})", HOLOGLIB_API_VERSION, __DATE__, __TIME__);

    // 每 tick 驱动的执行顺序在这里可见（顺序由 core/TickDriver.h 的 Order 显式决定,
    // 不再依赖各 TU 的静态初始化顺序）
    HLIB_LOG_INFO("[tick] order: {}", tick::TickDriver::getInstance().describe());

    // 声明顺序 = 启动顺序；关停时 ServiceHost 反向遍历
    mHost.add<ItemDisplayDomain>();
    mHost.add<CustomEntityDomain>();
    mHost.add<ParticleShapeDomain>();
    mHost.add<PlayerNpcDomain>();
    mHost.add<TradeMenuDomain>();
    mHost.add<ViewOverrideDomain>();
    mHost.add<ShapeDomain>();
    mHost.add<HologramTextDomain>();
    mHost.add<GradientLineDomain>();
    mHost.add<ItemDetailDomain>();
    mHost.add<ContainerMenuDomain>();
    mHost.add<NpcDialogueDomain>();
    mHost.add<PlayerSensingDomain>();
    mHost.add<FakeInventoryDomain>();
    mHost.add<SulfurDisplayDomain>();
    mHost.add<GhostDomain>();

    if (!mHost.startAll()) {
        HLIB_LOG_ERROR("[host] domain start failed; HologramLib stays disabled");
        return false;
    }
    HLIB_LOG_INFO("[host] {} domains: {}", mHost.size(), mHost.describe());

    // 运行时可选挂载 LegacyRemoteCall（无前置依赖）:
    // - lrca 已加载（顺序在前）→ 立即导出, LSE 可用
    // - lrca 未加载 → 监听 ServerStartedEvent 兜底（届时所有插件均已加载）
    if (hologramlib::lse::attach()) {
        exportLseFunctions(mHost);
        HLIB_LOG_INFO("LSE compat layer attached (LegacyRemoteCall detected).");
    } else {
        HLIB_LOG_INFO("LegacyRemoteCall not loaded yet; native C++ API active, will retry on ServerStarted.");
        ll::event::EventBus::getInstance().emplaceListener<ll::event::ServerStartedEvent>(
            [this](ll::event::ServerStartedEvent&) {
                if (hologramlib::lse::attach()) {
                    exportLseFunctions(mHost);
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
    mHost.stopAll(); // 反向关停, 并清空注册表（下次 enable 重新注册）
    HLIB_LOG_INFO("HologramLib disabled.");
    return true;
}

} // namespace debugshape_export

LL_REGISTER_MOD(debugshape_export::ModEntry, debugshape_export::ModEntry::getInstance());
