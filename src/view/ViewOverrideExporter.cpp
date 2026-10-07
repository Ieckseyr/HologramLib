// ViewOverrideExporter.cpp - 客户端视图覆盖 LSE 导出实现
//
// 脚本侧用法:
//   viewEntity("Steve", 12345, "type=minecraft:cow;name=§c奶牛;always=1");
//   viewBlock("Steve", 100, 64, 100, "minecraft:diamond_block");
//   viewClearEntity("Steve", 12345);
//   viewClearBlock("Steve", 100, 64, 100);
//   viewClearAll("Steve");        // 玩家名传空串 = 全部玩家
//   var s = viewDescribe("Steve"); // "entities=1 blocks=2"
//
// spec 串（分号分隔, 每项 "键=值", 键不分大小写; 玩家名传空串 = 对所有玩家生效）:
//   type    换成该实体类型（"cow" 自动补成 "minecraft:cow"）; 对玩家实体 = 给他造代理生物
//   as      换成另一名在线玩家的样子（皮肤取自那名玩家; 只对玩家实体有效）
//   hidden  1/true = 对这名玩家隐藏这只实体
//   name    名字牌文字（出现这个键就改名字牌; 值给空串 = 清空名字）
//   always  1/true = 名字牌一直显示
#include "ViewOverrideExporter.h"

#include "ViewOverrideManager.h"

#include "lse/LseBridge.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::string trim(std::string const& text) {
    auto const begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    auto const end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

bool truthy(std::string const& value) {
    auto const v = lower(trim(value));
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

// "type=minecraft:cow;name=奶牛;always=1" → EntityView
hologramlib::EntityView parseEntitySpec(std::string const& spec) {
    hologramlib::EntityView view;
    std::string             part;
    auto                    flush = [&] {
        auto const eq = part.find('=');
        if (eq != std::string::npos) {
            auto const key   = lower(trim(part.substr(0, eq)));
            auto const value = trim(part.substr(eq + 1));
            if (key == "type") {
                view.identifier = value;
            } else if (key == "as") {
                view.asPlayer = value; // 换成另一名在线玩家的样子（皮肤）
            } else if (key == "hidden") {
                view.hidden = truthy(value);
            } else if (key == "name") {
                view.hasNametag = true;
                view.nametag    = value;
            } else if (key == "always") {
                view.nametagAlwaysShow = truthy(value);
            }
        }
        part.clear();
    };
    for (char const c : spec) {
        if (c == ';' || c == '\n') {
            flush();
        } else {
            part += c;
        }
    }
    flush();
    return view;
}

} // namespace

void ViewOverrideExporter::exportAll() {
    auto& manager = view::ViewOverrideManager::getInstance();

    // 实体: 换类型 / 换名字牌 / 隐藏（spec 见文件头）
    hologramlib::lse::exportAs(NAMESPACE, "viewEntity",
        [&manager](std::string const& playerName, int64_t uniqueId, std::string const& spec) -> bool {
            return manager.overrideEntity(playerName, uniqueId, parseEntitySpec(spec));
        });

    // 方块: 把 (x,y,z) 显示成另一种方块
    hologramlib::lse::exportAs(NAMESPACE, "viewBlock",
        [&manager](
            std::string const& playerName,
            int                x,
            int                y,
            int                z,
            std::string const& type
        ) -> bool {
            hologramlib::BlockView view;
            view.type = type;
            return manager.overrideBlock(playerName, x, y, z, view);
        });

    hologramlib::lse::exportAs(NAMESPACE, "viewClearEntity",
        [&manager](std::string const& playerName, int64_t uniqueId) -> bool {
            return manager.clearEntity(playerName, uniqueId);
        });

    hologramlib::lse::exportAs(NAMESPACE, "viewClearBlock",
        [&manager](std::string const& playerName, int x, int y, int z) -> bool {
            return manager.clearBlock(playerName, x, y, z);
        });

    hologramlib::lse::exportAs(NAMESPACE, "viewClearAll",
        [&manager](std::string const& playerName) -> void {
            manager.clearAll(playerName);
        });

    hologramlib::lse::exportAs(NAMESPACE, "viewDescribe",
        [&manager](std::string const& playerName) -> std::string {
            return manager.describeFor(playerName);
        });
}

} // namespace debugshape_export
