// ClientUiExporter.cpp - 表单控制 / 方位条 LSE 导出 + 公开接口适配
#include "ClientUiExporter.h"

#include "FormControlManager.h"
#include "LocatorBarManager.h"

#include "lse/LseBridge.h"

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

} // namespace debugshape_export
