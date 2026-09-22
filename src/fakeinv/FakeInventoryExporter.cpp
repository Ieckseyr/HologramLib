// FakeInventoryExporter.cpp - 背包虚容器 LSE 导出实现
//
// 脚本侧用法:
//   fakeInvApply("Steve", "0|minecraft:diamond|64|§b钻石;;9|minecraft:paper|1|§7任务凭证");
//   // 槽位串格式: 槽位|物品id|数量|名字|描述行1~描述行2, 条目之间用 ';' 或换行分隔
//   ...
//   fakeInvSetSlot("Steve", 1, "minecraft:emerald", 3, 0, "§a奖励", "");
//   fakeInvClear("Steve");                       // 撤销, 恢复真实背包
//   for (const line of fakeInvPollClicks()) { }  // "player=Steve slot=0"
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
    if (begin == std::string::npos) return {};
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

// 描述行: '~' 或 '|' 分隔（'|' 留给字段分隔, 这里两种都认, 方便单行脚本写作）
std::vector<std::string> parseLore(std::string const& text) {
    std::vector<std::string> out;
    for (auto const& line : split(text, '~')) {
        if (!trim(line).empty()) out.push_back(line);
    }
    return out;
}

// 槽位串: 条目之间 ';' 或换行; 每条 "槽位|物品id|数量|名字|描述行~描述行"
std::vector<hologramlib::ContainerMenuItem> parseItems(std::string const& spec) {
    std::vector<hologramlib::ContainerMenuItem> items(
        static_cast<std::size_t>(hologramlib::kFakeInventorySlots)
    );
    std::string normalized = spec;
    for (auto& c : normalized) {
        if (c == '\n' || c == ';') c = ';';
    }
    for (auto const& raw : split(normalized, ';')) {
        auto const entry = trim(raw);
        if (entry.empty()) continue;
        auto const fields = split(entry, '|');
        if (fields.size() < 2) continue;
        int const slot = std::stoi(trim(fields[0]));
        if (slot < 0 || slot >= hologramlib::kFakeInventorySlots) continue;
        auto& item   = items[static_cast<std::size_t>(slot)];
        item.type    = trim(fields[1]);
        item.count   = fields.size() > 2 && !trim(fields[2]).empty() ? std::stoi(trim(fields[2])) : 1;
        item.name    = fields.size() > 3 ? trim(fields[3]) : std::string{};
        item.lore    = fields.size() > 4 ? parseLore(fields[4]) : std::vector<std::string>{};
    }
    return items;
}

} // namespace

void FakeInventoryExporter::exportAll() {
    auto& mgr = FakeInventoryManager::getInstance();

    // fakeInvApply(playerName, itemsSpec, refreshIntervalTicks) -> bool
    //   整份伪造背包下发; refreshIntervalTicks = 0 关掉周期重发（默认 20 tick）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "fakeInvApply",
        [&mgr](std::string const& playerName, std::string const& itemsSpec, int refreshIntervalTicks) -> bool {
            hologramlib::FakeInventorySpec spec;
            spec.items                = parseItems(itemsSpec);
            spec.refreshIntervalTicks = refreshIntervalTicks;
            return mgr.apply(playerName, spec);
        });

    // fakeInvSetSlot(playerName, slot, type, count, damage, name, loreCsv) -> bool
    //   单格改动（type 空 = 该格清空）
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
            std::string const& loreCsv
        ) -> bool {
            hologramlib::ContainerMenuItem item;
            item.type   = type;
            item.count  = count < 1 ? 1 : count;
            item.damage = damage;
            item.name   = name;
            item.lore   = parseLore(loreCsv);
            return mgr.setSlot(playerName, slot, item);
        });

    // fakeInvRefresh(playerName) -> bool（立即重发伪造内容; 打开界面后/发现被覆盖时用）
    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvRefresh", [&mgr](std::string const& playerName) -> bool {
            return mgr.refresh(playerName);
        });

    // fakeInvClear(playerName) -> bool（撤销伪造并恢复真实背包）
    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvClear", [&mgr](std::string const& playerName) -> bool {
            return mgr.clear(playerName);
        });

    // fakeInvClearAll() -> void
    hologramlib::lse::exportAs(NAMESPACE, "fakeInvClearAll", [&mgr]() -> void { mgr.clearAll(); });

    // fakeInvIsActive(playerName) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvIsActive", [&mgr](std::string const& playerName) -> bool {
            return mgr.isActive(playerName);
        });

    // fakeInvGetPlayers() -> [s]
    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvGetPlayers", [&mgr]() -> std::vector<std::string> { return mgr.getActivePlayers(); });

    // fakeInvPollClicks() -> [s]（取走并清空; 每条 "player=X slot=S"）
    hologramlib::lse::exportAs(NAMESPACE, "fakeInvPollClicks", []() -> std::vector<std::string> {
        auto                     events = FakeInventoryManager::getInstance().pollClicks();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events) out.push_back(FakeInventoryManager::formatClick(ev));
        return out;
    });

    // fakeInvClearClicks() -> void
    hologramlib::lse::exportAs(
        NAMESPACE, "fakeInvClearClicks", []() -> void { FakeInventoryManager::getInstance().clearClicks(); });
}

} // namespace debugshape_export
