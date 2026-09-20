// ContainerMenuExporter.cpp - 虚拟容器（列表界面）LSE 导出实现
//
// 脚本侧的完整用法（打开 → 填格 → 关闭）:
//   const id = containerOpen("Steve", "§8任务列表", 3);      // 27 格（rows=6 = 54 格）
//   containerSetItem(id, 0, "minecraft:diamond_sword", 1, 0, "§b试炼", "§7点击领取\n§8第二行");
//   ...
//   在守护/定时里: const clicks = containerPollClicks();  // 每条 "player=.. menuId=.. slot=.. closed=.."
//
// 点击语义: 点击就是一次物品拾取, 任何输入设备都会发包（实测 27 格与大容器第二半区都正常）。
// 服务端并没有这个容器, 所以物品不会被真的拿走 —— 库只回传槽位号。
#include "ContainerMenuExporter.h"
#include "ContainerMenuManager.h"

#include "lse/LseBridge.h"

#include <string>
#include <vector>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

namespace {

// loreCsv: 每行一条（'\n' 分隔; 也接受 '|' 分隔, 便于在单行脚本里写）
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
    if (!line.empty()) out.push_back(line);
    return out;
}

} // namespace

void ContainerMenuExporter::exportAll() {
    auto& mgr = ContainerMenuManager::getInstance();

    // containerOpen(playerName, title, rows) -> id（打开一个空容器; 物品用 containerSetItem 逐格填）
    //   rows: 3 = 小容器 27 格; 6 = 大容器 54 格（两个配对方块）
    // 打开有固定延迟（默认 7 tick, 见 ContainerMenuSpec::openDelayTicks）—— 之后才能填格
    hologramlib::lse::exportAs(
        NAMESPACE,
        "containerOpen",
        [&mgr](std::string const& playerName, std::string const& title, int rows) -> int64_t {
            hologramlib::ContainerMenuSpec spec;
            spec.rows = rows;
            if (!title.empty()) spec.title = title;
            spec.items.resize(static_cast<std::size_t>(spec.rows >= 6 ? 54 : 27));
            return mgr.open(playerName, spec);
        });

    // containerSetItem(id, slot, type, count, damage, name, loreCsv) -> bool
    //   只发一条 InventorySlot（无延迟、无闪烁）; type 空 = 清空该槽; loreCsv 空 = 无描述行
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
            std::string const& loreCsv
        ) -> bool {
            hologramlib::ContainerMenuItem item;
            item.type   = type;
            item.count  = count < 1 ? 1 : count;
            item.damage = damage;
            item.name   = name;
            item.lore   = splitLore(loreCsv);
            return mgr.setItem(id, slot, item);
        });

    // containerSetTitle(id, title) -> bool（就地换标题 + 重发方块实体, 不重开界面）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "containerSetTitle",
        [&mgr](int64_t id, std::string const& title) -> bool { return mgr.setTitle(id, title); });

    // containerClose(id) -> bool
    hologramlib::lse::exportAs(NAMESPACE, "containerClose", [&mgr](int64_t id) -> bool { return mgr.close(id); });

    // containerCloseAll() -> void
    hologramlib::lse::exportAs(NAMESPACE, "containerCloseAll", [&mgr]() -> void { mgr.closeAll(); });

    // containerIsOpen(id) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "containerIsOpen", [&mgr](int64_t id) -> bool { return mgr.isOpen(id); });

    // containerGetIds() -> [i]
    hologramlib::lse::exportAs(
        NAMESPACE, "containerGetIds", [&mgr]() -> std::vector<int64_t> { return mgr.getAllIds(); });

    // containerPollClicks() -> [s]（取走并清空待处理点击/关闭; 条目见 formatClick）
    hologramlib::lse::exportAs(NAMESPACE, "containerPollClicks", []() -> std::vector<std::string> {
        auto                     events = ContainerMenuManager::getInstance().pollClicks();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events) out.push_back(ContainerMenuManager::formatClick(ev));
        return out;
    });

    // containerClearClicks() -> void（丢弃队列里全部待处理点击）
    hologramlib::lse::exportAs(
        NAMESPACE, "containerClearClicks", []() -> void { ContainerMenuManager::getInstance().clearClicks(); });
}

} // namespace debugshape_export
