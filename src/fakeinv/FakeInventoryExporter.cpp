#include "FakeInventoryExporter.h"
#include "FakeInventoryManager.h"

#include "lse/LseBridge.h"

#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

namespace {

std::string trim(std::string const& text) {
    auto const begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return {};
    auto const end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

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

std::vector<std::string> parseLore(std::string const& text) {
    std::vector<std::string> out;
    for (auto const& line : split(text, '~')) {
        if (!trim(line).empty())
            out.push_back(line);
    }
    return out;
}

std::vector<hologramlib::ContainerMenuItem> parseItems(std::string const& spec) {
    std::vector<hologramlib::ContainerMenuItem> items(
        static_cast<std::size_t>(hologramlib::kFakeInventorySlots));
    std::string normalized = spec;
    for (auto& c : normalized) {
        if (c == '\n' || c == ';')
            c = ';';
    }
    for (auto const& raw : split(normalized, ';')) {
        auto const entry = trim(raw);
        if (entry.empty())
            continue;
        auto const fields = split(entry, '|');
        if (fields.size() < 2)
            continue;
        int const slot = std::stoi(trim(fields[0]));
        if (slot < 0 || slot >= hologramlib::kFakeInventorySlots)
            continue;
        auto& item = items[static_cast<std::size_t>(slot)];
        item.type  = trim(fields[1]);
        item.count = fields.size() > 2 && !trim(fields[2]).empty() ? std::stoi(trim(fields[2])) : 1;
        item.name  = fields.size() > 3 ? trim(fields[3]) : std::string{};
        item.lore  = fields.size() > 4 ? parseLore(fields[4]) : std::vector<std::string>{};
    }
    return items;
}

} // namespace

void FakeInventoryExporter::exportAll() {
    auto& mgr = FakeInventoryManager::getInstance();

    hologramlib::lse::exportAs(
        NAMESPACE,
        "fakeInvApply",
        [&mgr](std::string const& playerName, std::string const& itemsSpec, int refreshIntervalTicks) -> bool {
            hologramlib::FakeInventorySpec spec;
            spec.items                = parseItems(itemsSpec);
            spec.refreshIntervalTicks = refreshIntervalTicks;
            return mgr.apply(playerName, spec);
        });

    hologramlib::lse::exportAs(
        NAMESPACE,
        "fakeInvSetSlot",
        [&mgr](
            std::string const& playerName,
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
            item.lore   = parseLore(loreCsv);
            return mgr.setSlot(playerName, slot, item);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvRefresh", [&mgr](std::string const& playerName) -> bool {
            return mgr.refresh(playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvClear", [&mgr](std::string const& playerName) -> bool {
            return mgr.clear(playerName);
        });

    hologramlib::lse::exportAs(NAMESPACE, "fakeInvClearAll", [&mgr]() -> void {
        mgr.clearAll();
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvIsActive", [&mgr](std::string const& playerName) -> bool {
            return mgr.isActive(playerName);
        });

    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvGetPlayers", [&mgr]() -> std::vector<std::string> {
            return mgr.getActivePlayers();
        });

    hologramlib::lse::exportAs(NAMESPACE, "fakeInvDroppedClicks", []() -> int64_t {
        return static_cast<int64_t>(FakeInventoryManager::getInstance().droppedClicks());
    });

    hologramlib::lse::exportAs(NAMESPACE, "fakeInvPollClicks", []() -> std::vector<std::string> {
        auto                     events = FakeInventoryManager::getInstance().pollClicks();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events)
            out.push_back(FakeInventoryManager::formatClick(ev));
        return out;
    });

    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvClearClicks", []() -> void {
            FakeInventoryManager::getInstance().clearClicks();
        });
}

} // namespace debugshape_export
