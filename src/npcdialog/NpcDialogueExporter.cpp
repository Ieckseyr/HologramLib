#include "NpcDialogueExporter.h"
#include "NpcDialogueManager.h"

#include "lse/LseBridge.h"

#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

namespace {

std::vector<std::string> split(std::string const& text, char const sep) {
    std::vector<std::string> out;
    std::string              part;
    for (char const c : text) {
        if (c == sep) {
            out.push_back(part);
            part.clear();
        } else {
            part += c;
        }
    }
    out.push_back(part);
    return out;
}

std::string trim(std::string const& text) {
    auto const begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return {};
    auto const end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::vector<hologramlib::NpcDialogButton> parseButtons(std::string const& spec) {
    std::vector<hologramlib::NpcDialogButton> out;
    for (auto const& raw : split(spec, ';')) {
        auto const entry = trim(raw);
        if (entry.empty())
            continue;
        auto const                   fields = split(entry, '|');
        hologramlib::NpcDialogButton button;
        button.label = trim(fields[0]);
        if (fields.size() > 1)
            button.actionId = trim(fields[1]);
        if (fields.size() > 2 && !trim(fields[2]).empty())
            button.mode = std::stoi(trim(fields[2]));
        if (fields.size() > 3 && !trim(fields[3]).empty()) {
            for (auto const& cmd : split(trim(fields[3]), ',')) {
                if (!trim(cmd).empty())
                    button.commands.push_back(trim(cmd));
            }
        }
        if (button.label.empty())
            button.label = "按钮";
        out.push_back(std::move(button));
    }
    return out;
}

} // namespace

void NpcDialogueExporter::exportAll() {
    auto& mgr = NpcDialogueManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "npcDialogOpen",
        [&mgr](
            std::string const& playerName,
            std::string const& npcName,
            std::string const& sceneName,
            std::string const& dialogue,
            std::string const& buttonsSpec,
            std::string const& carrierIdentifier) -> int64_t {
            hologramlib::NpcDialogSpec spec;
            if (!npcName.empty())
                spec.npcName = npcName;
            if (!sceneName.empty())
                spec.sceneName = sceneName;
            spec.dialogue = dialogue;
            spec.buttons  = parseButtons(buttonsSpec);
            if (!carrierIdentifier.empty())
                spec.carrierIdentifier = carrierIdentifier;
            return mgr.open(playerName, spec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "npcDialogUpdate",
        [&mgr](int64_t id, std::string const& dialogue, std::string const& buttonsSpec) -> bool {
            hologramlib::NpcDialogSpec spec;
            if (!mgr.getSpec(id, spec))
                return false;
            spec.dialogue = dialogue;
            spec.buttons  = parseButtons(buttonsSpec);
            return mgr.update(id, spec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "npcDialogSetAvatar",
        [&mgr](int64_t id, int skinVariant, std::string const& avatarViewSpec) -> bool {
            return mgr.setAvatar(id, skinVariant, avatarViewSpec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "npcDialogClose", [&mgr](int64_t id) -> bool {
            return mgr.close(id);
        });

    hologramlib::lse::exportAs(NAMESPACE, "npcDialogCloseAll", [&mgr]() -> void {
        mgr.closeAll();
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "npcDialogIsOpen", [&mgr](int64_t id) -> bool {
            return mgr.isOpen(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "npcDialogGetIds", [&mgr]() -> std::vector<int64_t> {
            return mgr.getAllIds();
        });

    hologramlib::lse::exportAs(NAMESPACE, "npcDialogDroppedClicks", []() -> int64_t {
        return static_cast<int64_t>(NpcDialogueManager::getInstance().droppedClicks());
    });

    hologramlib::lse::exportAs(NAMESPACE, "npcDialogPollClicks", []() -> std::vector<std::string> {
        auto                     events = NpcDialogueManager::getInstance().pollClicks();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events)
            out.push_back(NpcDialogueManager::formatClick(ev));
        return out;
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "npcDialogClearClicks", []() -> void {
            NpcDialogueManager::getInstance().clearClicks();
        });
}

} // namespace debugshape_export
