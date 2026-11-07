#include "ContainerMenuExporter.h"
#include "ContainerMenuManager.h"
#include "ContainerPreview.h"

#include "lse/LseBridge.h"

#include <string>
#include <exception>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

namespace {

class ContainerPreviewAdapter final : public hologramlib::IContainerPreview {
public:
    int64_t preview(
        std::string const& playerName,
        int                dim,
        int                x,
        int                y,
        int                z,
        std::string const& title) override {
        return container::previewAt(playerName, dim, x, y, z, title);
    }
};

} // namespace

hologramlib::IContainerPreview& containerPreviewAdapter() {
    static ContainerPreviewAdapter adapter;
    return adapter;
}

namespace {

std::vector<std::string> splitLore(std::string const& loreCsv) {
    std::vector<std::string> out;
    std::string              line;
    for (char const c : loreCsv) {
        if (c == '\n' || c == '|') {
            out.push_back(line);
            line.clear();
        } else {
            line += c;
        }
    }
    if (!line.empty())
        out.push_back(line);
    return out;
}

std::vector<std::pair<int, int>> splitEnch(std::string const& enchCsv) {
    std::vector<std::pair<int, int>> out;
    std::string                      item;
    auto                             flush = [&]() {
        if (item.empty())
            return;
        auto const colon = item.find(':');
        if (colon != std::string::npos) {
            try {
                int const id  = std::stoi(item.substr(0, colon));
                int const lvl = std::stoi(item.substr(colon + 1));
                if (id >= 0 && id <= 42 && lvl >= 1 && lvl <= 255)
                    out.emplace_back(id, lvl);
            } catch (std::exception const&) {
            }
        }
        item.clear();
    };
    for (char const c : enchCsv) {
        if (c == ',' || c == '|' || c == ';' || c == ' ')
            flush();
        else
            item += c;
    }
    flush();
    return out;
}

} // namespace

void ContainerMenuExporter::exportAll() {
    auto& mgr = ContainerMenuManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "containerOpen",
        [&mgr](std::string const& playerName, std::string const& title, int rows) -> int64_t {
            hologramlib::ContainerMenuSpec spec;
            spec.rows = rows;
            if (!title.empty())
                spec.title = title;
            spec.items.resize(static_cast<std::size_t>(spec.rows >= 6 ? 54 : 27));
            return mgr.open(playerName, spec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "containerSetItem",
        [&mgr](
            int64_t            id,
            int                slot,
            std::string const& type,
            int                count,
            int                damage,
            std::string const& name,
            std::string const& loreCsv) -> bool {
            hologramlib::ContainerMenuItem item;
            item.type   = type;
            item.count  = count < 1 ? 1 : count;
            item.damage = damage;
            item.name   = name;
            item.lore   = splitLore(loreCsv);
            return mgr.setItem(id, slot, item);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "containerSetItemEnchanted",
        [&mgr](
            int64_t            id,
            int                slot,
            std::string const& type,
            int                count,
            int                damage,
            std::string const& name,
            std::string const& loreCsv,
            std::string const& enchCsv) -> bool {
            hologramlib::ContainerMenuItem item;
            item.type   = type;
            item.count  = count < 1 ? 1 : count;
            item.damage = damage;
            item.name   = name;
            item.lore   = splitLore(loreCsv);
            return mgr.setItemEnchanted(id, slot, item, splitEnch(enchCsv));
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "containerSetTitle",
        [&mgr](int64_t id, std::string const& title) -> bool {
            return mgr.setTitle(id, title);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "containerSetInteractive", [&mgr](int64_t id, bool on) -> bool {
            return mgr.setInteractive(id, on);
        });

    hologramlib::lse::exportAs(NAMESPACE, "containerClose", [&mgr](int64_t id) -> bool {
        return mgr.close(id);
    });

    hologramlib::lse::exportAs(NAMESPACE, "containerCloseAll", [&mgr]() -> void {
        mgr.closeAll();
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "containerIsOpen", [&mgr](int64_t id) -> bool {
            return mgr.isOpen(id);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "containerGetIds", [&mgr]() -> std::vector<int64_t> {
            return mgr.getAllIds();
        });

    hologramlib::lse::exportAs(NAMESPACE, "containerDroppedClicks", []() -> int64_t {
        return static_cast<int64_t>(ContainerMenuManager::getInstance().droppedClicks());
    });

    hologramlib::lse::exportAs(NAMESPACE, "containerPollClicks", []() -> std::vector<std::string> {
        auto                     events = ContainerMenuManager::getInstance().pollClicks();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events)
            out.push_back(ContainerMenuManager::formatClick(ev));
        return out;
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "containerBlockEvent", [&mgr](int64_t menuId, int b0, int b1) -> bool {
            return mgr.blockEvent(menuId, b0, b1);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "containerPreview",
        [](std::string const& playerName, int dim, int x, int y, int z) -> int64_t {
            return container::previewAt(playerName, dim, x, y, z, "");
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "containerPreviewTitled",
        [](std::string const& playerName, int dim, int x, int y, int z, std::string const& title) -> int64_t {
            return container::previewAt(playerName, dim, x, y, z, title);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "containerUpdate",
        [&mgr](int64_t id, std::string const& title) -> bool {
            hologramlib::ContainerMenuSpec spec;
            if (!mgr.getSpec(id, spec))
                return false;
            if (!title.empty())
                spec.title = title;
            return mgr.update(id, spec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "containerClearClicks", []() -> void {
            ContainerMenuManager::getInstance().clearClicks();
        });
}

} // namespace debugshape_export
