#include "ClientUiExporter.h"

#include "../visibility/PlayerVisibilityManager.h"
#include "FormControlManager.h"

#include "lse/LseBridge.h"

#include <format>

#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

void FormControlExporter::exportAll() {
    auto& mgr = FormControlManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE, "formClose", [&mgr](std::string const& playerName) -> bool {
            return mgr.closeForm(playerName);
        });

    hologramlib::lse::exportAs(NAMESPACE, "formCloseAll", [&mgr]() -> int {
        return mgr.closeAllForms();
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

} // namespace

hologramlib::IFormControl& formControlAdapter() {
    static FormControlAdapter adapter;
    return adapter;
}

void PlayerVisibilityExporter::exportAll() {
    auto& mgr = PlayerVisibilityManager::getInstance();

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

    hologramlib::lse::exportAs(
        NAMESPACE, "playerHideSetHidden", [&mgr](std::string const& playerName, bool hidden) -> bool {
            return mgr.setHidden(playerName, hidden);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "playerHideIsHidden", [&mgr](std::string const& playerName) -> bool {
            return mgr.isHidden(playerName);
        });

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
