// ClientUiExporter.cpp - 表单控制 / 方位条 LSE 导出 + 公开接口适配
#include "ClientUiExporter.h"

#include "../visibility/PlayerVisibilityManager.h"
#include "FormControlManager.h"
#include "LocatorBarManager.h"

#include "lse/LseBridge.h"

#include <format>

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void FormControlExporter::exportAll() {
    auto& mgr = FormControlManager::getInstance();

    // formClose(playerName) -> bool（强制关掉该玩家当前打开的客户端表单）
    hologramlib::lse::exportAs(
        NAMESPACE, "formClose", [&mgr](std::string const& playerName) -> bool {
            return mgr.closeForm(playerName);
        });

    // formCloseAll() -> i（发给所有在线玩家, 返回发给了几个）
    hologramlib::lse::exportAs(NAMESPACE, "formCloseAll", [&mgr]() -> int {
        return mgr.closeAllForms();
    });
}

void LocatorBarExporter::exportAll() {
    auto& mgr = LocatorBarManager::getInstance();

    // locatorBarSet(playerName, key, x, y, z, dim, color, visible) -> bool
    //   key 自己起名, 同一个 key 再调就是更新位置; color = ARGB, -1 = 客户端默认
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarSet",
        [&mgr](
            std::string const& playerName,
            std::string const& key,
            float              x,
            float              y,
            float              z,
            int                dim,
            int                color,
            bool               visible) -> bool {
            return mgr.setWaypoint(playerName, key, x, y, z, dim, color, visible);
        });

    // locatorBarRemove(playerName, key) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "locatorBarRemove", [&mgr](std::string const& playerName, std::string const& key) -> bool {
            return mgr.removeWaypoint(playerName, key);
        });

    // locatorBarClear(playerName) -> i（清掉该玩家全部航点, 返回清掉了几个）
    hologramlib::lse::exportAs(
        NAMESPACE, "locatorBarClear", [&mgr](std::string const& playerName) -> int {
            return mgr.clearWaypoints(playerName);
        });

    // locatorBarHideFromOthers(playerName, hidden) -> bool
    //   把该玩家从其他人的客户端上藏起来（本人照常看得到自己）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarHideFromOthers",
        [&mgr](std::string const& playerName, bool hidden) -> bool {
            return mgr.hideFromOthers(playerName, hidden);
        });

    // locatorBarIsHiddenFromOthers(playerName) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "locatorBarIsHiddenFromOthers",
        [&mgr](std::string const& playerName) -> bool {
            return mgr.isHiddenFromOthers(playerName);
        });

    // locatorBarCount(playerName) -> i
    hologramlib::lse::exportAs(
        NAMESPACE, "locatorBarCount", [&mgr](std::string const& playerName) -> int {
            return mgr.waypointCount(playerName);
        });
}

namespace {

class FormControlAdapter final : public hologramlib::IFormControl {
public:
    bool closeForm(std::string const& playerName) override {
        return FormControlManager::getInstance().closeForm(playerName);
    }
    int closeAllForms() override { return FormControlManager::getInstance().closeAllForms(); }
};

class LocatorBarAdapter final : public hologramlib::ILocatorBar {
public:
    bool setWaypoint(
        std::string const& playerName,
        std::string const& key,
        float              x,
        float              y,
        float              z,
        int                dimension,
        int                color,
        bool               visible) override {
        return LocatorBarManager::getInstance().setWaypoint(playerName, key, x, y, z, dimension, color, visible);
    }
    bool removeWaypoint(std::string const& playerName, std::string const& key) override {
        return LocatorBarManager::getInstance().removeWaypoint(playerName, key);
    }
    int clearWaypoints(std::string const& playerName) override {
        return LocatorBarManager::getInstance().clearWaypoints(playerName);
    }
    [[nodiscard]] int waypointCount(std::string const& playerName) const override {
        return LocatorBarManager::getInstance().waypointCount(playerName);
    }
    bool hideFromOthers(std::string const& playerName, bool hidden) override {
        return LocatorBarManager::getInstance().hideFromOthers(playerName, hidden);
    }
    [[nodiscard]] bool isHiddenFromOthers(std::string const& playerName) const override {
        return LocatorBarManager::getInstance().isHiddenFromOthers(playerName);
    }
};

} // namespace

hologramlib::IFormControl& formControlAdapter() {
    static FormControlAdapter adapter;
    return adapter;
}

hologramlib::ILocatorBar& locatorBarAdapter() {
    static LocatorBarAdapter adapter;
    return adapter;
}

void PlayerVisibilityExporter::exportAll() {
    auto& mgr = PlayerVisibilityManager::getInstance();

    // playerHideSetMask(playerName, spawn, tab, motion, state, skin, alsoSelf) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE,
        "playerHideSetMask",
        [&mgr](
            std::string const& playerName,
            bool               spawn,
            bool               tab,
            bool               motion,
            bool               state,
            bool               skin,
            bool               alsoSelf) -> bool {
            hologramlib::PlayerPacketMask mask;
            mask.spawn  = spawn;
            mask.tab    = tab;
            mask.motion = motion;
            mask.state  = state;
            mask.skin   = skin;
            return mgr.setMask(playerName, mask, alsoSelf);
        });

    // playerHideGetMask(playerName) -> "spawn=0 tab=1 motion=0 state=0 skin=1 hidden=0"
    hologramlib::lse::exportAs(
        NAMESPACE, "playerHideGetMask", [&mgr](std::string const& playerName) -> std::string {
            auto const mask = mgr.maskOf(playerName);
            return std::format(
                "spawn={} tab={} motion={} state={} skin={} hidden={}",
                mask.spawn ? 1 : 0,
                mask.tab ? 1 : 0,
                mask.motion ? 1 : 0,
                mask.state ? 1 : 0,
                mask.skin ? 1 : 0,
                mgr.isHidden(playerName) ? 1 : 0);
        });

    // playerHideSetHidden(playerName, hidden) -> bool（整体隐身; 已出生在别人屏幕上的也会消失）
    hologramlib::lse::exportAs(
        NAMESPACE, "playerHideSetHidden", [&mgr](std::string const& playerName, bool hidden) -> bool {
            return mgr.setHidden(playerName, hidden);
        });

    // playerHideIsHidden(playerName) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "playerHideIsHidden", [&mgr](std::string const& playerName) -> bool {
            return mgr.isHidden(playerName);
        });

    // playerHideRestore(playerName) -> bool（清掩码 + 清隐身 + 立刻补发出生包）
    hologramlib::lse::exportAs(
        NAMESPACE, "playerHideRestore", [&mgr](std::string const& playerName) -> bool {
            return mgr.restore(playerName);
        });
}

namespace {

class PlayerVisibilityAdapter final : public hologramlib::IPlayerVisibility {
public:
    bool setMask(std::string const& playerName, hologramlib::PlayerPacketMask const& mask, bool alsoSelf) override {
        return PlayerVisibilityManager::getInstance().setMask(playerName, mask, alsoSelf);
    }
    bool setHidden(std::string const& playerName, bool hidden) override {
        return PlayerVisibilityManager::getInstance().setHidden(playerName, hidden);
    }
    bool restore(std::string const& playerName) override {
        return PlayerVisibilityManager::getInstance().restore(playerName);
    }
    [[nodiscard]] hologramlib::PlayerPacketMask maskOf(std::string const& playerName) const override {
        return PlayerVisibilityManager::getInstance().maskOf(playerName);
    }
    [[nodiscard]] bool isHidden(std::string const& playerName) const override {
        return PlayerVisibilityManager::getInstance().isHidden(playerName);
    }
};

} // namespace

hologramlib::IPlayerVisibility& playerVisibilityAdapter() {
    static PlayerVisibilityAdapter adapter;
    return adapter;
}

} // namespace debugshape_export
