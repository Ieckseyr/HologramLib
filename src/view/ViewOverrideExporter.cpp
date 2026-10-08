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
#include <format>
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

// 玩家名 → 实体 uniqueId。**不在线统一返回 -1 只作为对 JS 的约定**（-1 是 BDS 的
// INVALID 常量, 真实在线玩家不会是它）; 库内部一律走返回值判断, 不做"负数 = 没找到"。
//
// 这一条是实测踩出来的: 玩家 uniqueId 本身是负数（某服玩家 = -25769803775 = -0x5FFFFFFFF）,
// 首版用 `id < 0` 当"没找到" → 玩家明明在线却直接返回 false, C++ 结构体直传那条路不经过
// 这个判断, 所以只有 LSE 字符串那条路翻车。
static bool playerIdOf(std::string const& playerName, std::int64_t& uniqueId) {
    return view::ViewOverrideManager::getInstance().uniqueIdOfPlayer(playerName, uniqueId);
}

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

    // 按名字取该玩家实体的 uniqueId（-1 = 不在线）—— JS 侧拿不到 id, 用这个
    hologramlib::lse::exportAs(NAMESPACE, "viewUniqueIdOf",
        [](std::string const& playerName) -> int64_t {
            std::int64_t id = 0;
            if (!playerIdOf(playerName, id)) return -1; // 不在线（-1 只作为对 JS 的约定）
            return id;
        });

    // 给"自己"套覆盖（对所有玩家生效; spec 同 viewEntity）—— 管理员自我伪装用
    hologramlib::lse::exportAs(NAMESPACE, "viewSelf",
        [&manager](std::string const& playerName, std::string const& spec) -> bool {
            std::int64_t id = 0;
            if (!playerIdOf(playerName, id)) return false;
            return manager.overrideEntity({}, id, parseEntitySpec(spec)); // 空玩家名 = 所有玩家都看得见
        });
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfClear",
        [](std::string const& playerName) -> bool {
            std::int64_t id = 0;
            if (!playerIdOf(playerName, id)) return false;
            return view::ViewOverrideManager::getInstance().clearEntity({}, id);
        });

    // 逐字段版本（不解析 spec 字符串）—— JS 侧专用, 绕开"字符串 spec 解析"这条路。
    // 每个函数只收一个字段, 与 C++ 结构体直传等价: 类型 / 皮肤 / 隐藏 / 名字牌 / 给别人 / 撤销。
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfType",
        [&manager](std::string const& playerName, std::string const& type) -> bool {
            std::int64_t id = 0;
            if (!playerIdOf(playerName, id)) return false;
            hologramlib::EntityView view;
            view.identifier = type;
            return manager.overrideEntity({}, id, view);
        });
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfSkin",
        [&manager](std::string const& playerName, std::string const& skinOfPlayer) -> bool {
            std::int64_t id = 0;
            if (!playerIdOf(playerName, id)) return false;
            hologramlib::EntityView view;
            view.asPlayer = skinOfPlayer;
            return manager.overrideEntity({}, id, view);
        });
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfName",
        [&manager](std::string const& playerName, std::string const& nametag, bool alwaysShow) -> bool {
            std::int64_t id = 0;
            if (!playerIdOf(playerName, id)) return false;
            hologramlib::EntityView view;
            view.hasNametag        = true;
            view.nametag           = nametag;
            view.nametagAlwaysShow = alwaysShow;
            return manager.overrideEntity({}, id, view);
        });
    hologramlib::lse::exportAs(NAMESPACE, "viewSelfHide",
        [&manager](std::string const& playerName, bool hidden) -> bool {
            std::int64_t id = 0;
            if (!playerIdOf(playerName, id)) return false;
            hologramlib::EntityView view;
            view.hidden = hidden;
            return manager.overrideEntity({}, id, view);
        });
    hologramlib::lse::exportAs(NAMESPACE, "viewTargetType",
        [&manager](std::int64_t uniqueId, std::string const& type) -> bool {
            hologramlib::EntityView view;
            view.identifier = type;
            return manager.overrideEntity({}, uniqueId, view); // 空玩家名 = 所有玩家
        });
    hologramlib::lse::exportAs(NAMESPACE, "viewClearId",
        [](std::int64_t uniqueId) -> bool {
            return view::ViewOverrideManager::getInstance().clearEntity({}, uniqueId);
        });

    // 诊断: 把收到的字符串原样回传（带上长度与每个字节的十六进制前 16 个）——
    // 用来判断 LSE 传进来的字符串有没有被串码/截断（spec 解析失败时先看它）。
    hologramlib::lse::exportAs(NAMESPACE, "viewEcho",
        [](std::string const& text) -> std::string {
            std::string hex;
            for (std::size_t i = 0; i < text.size() && i < 16; ++i) {
                hex += std::format("{:02x}", static_cast<unsigned char>(text[i]));
            }
            return std::format("len={} hex={} text=[{}]", text.size(), hex, text);
        });

    hologramlib::lse::exportAs(NAMESPACE, "viewDescribe",
        [&manager](std::string const& playerName) -> std::string {
            return manager.describeFor(playerName);
        });
}

} // namespace debugshape_export
