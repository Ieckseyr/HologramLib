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
#include "ViewPackets.h"

#include "lse/LseBridge.h"

#include <algorithm>
#include <format>
#include <cctype>
#include <string>

namespace debugshape_export {

static constexpr const char* NAMESPACE = "HologramLib";

// 以下四个从匿名命名空间移出: parseEntitySpec 供其它域复用（如 npcDialog 的 avatarViewSpec）——
// 声明见 ViewOverrideExporter.h, spec 语法与 viewEntity 保持同一份实现。
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

// 数值解析: 解析不出来就当"这一项没给"（不设 hasXxx）, 而不是抛异常穿过 LSE 边界
bool intOf(std::string const& value, int& out) {
    try {
        std::size_t used = 0;
        auto const  v    = std::stoi(trim(value), &used);
        if (used == 0) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

bool floatOf(std::string const& value, float& out) {
    try {
        std::size_t used = 0;
        auto const  v    = std::stof(trim(value), &used);
        if (used == 0) return false;
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

// "type=minecraft:cow;name=奶牛;always=1;variant=2" → EntityView
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
            } else if (key == "skin") {
                // 26.40.8/1.27.0: 换成**注册表里的那张皮肤**（非玩家实体 = 同 id 换模型;
                // npcDialog 的头像载体 = 玩家模型载体）。皮肤来自 IPlayerNpc 注册表。
                view.skinId = value;
            }
            // ── 26.40.10 / 1.29.0: 变成生物之后的姿态 / 缩放 / 变种 / 颜色（逐字段, 设了才发）──
            else if (key == "pose") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasPose = true;
                    view.pose    = v; // 越界由库内 clampPose 钳进 0..13
                }
            } else if (key == "scale") {
                // ⚠ 这条是**元数据缩放**（客户端实体缩放 + 碰撞箱, 0.0625~10 外整项忽略）,
                //   与 `skin=` 那档的"皮肤模型缩放"不是一回事（后者是结构体里另一个字段）。
                float v = 1.0f;
                if (floatOf(value, v)) {
                    view.hasMetaScale = true;
                    view.metaScale    = v;
                }
            } else if (key == "variant") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasVariant = true;
                    view.variant    = v; // 河豚的鼓胀档位、热带鱼体型、羊的颜色档……都在这一项
                }
            } else if (key == "mark" || key == "markvariant") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasMarkVariant = true;
                    view.markVariant    = v; // 热带鱼花纹 0..5
                }
            } else if (key == "color" || key == "colorindex") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasColorIndex = true;
                    view.colorIndex    = v; // 主色 0..15（越界整项忽略）
                }
            } else if (key == "color2") {
                int v = 0;
                if (intOf(value, v)) {
                    view.hasColor2 = true;
                    view.color2    = v; // 花纹色 0..15（热带鱼用）
                }
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

// 玩家名 → 实体 uniqueId。**不在线统一返回 -1 只作为对 JS 的约定**（-1 是 BDS 的
// INVALID 常量, 真实在线玩家不会是它）; 库内部一律走返回值判断, 不做"负数 = 没找到"。
//
// 这一条是实测踩出来的: 玩家 uniqueId 本身是负数（实测 = -25769803775 = -0x5FFFFFFFF）,
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

    // 元数据探针（无客户端也能验）: 把 spec 折成的元数据真序列化一遍, 回读成 items/bytes/hex。
    // 用的是和真正发包完全同一条 `pushMetaItems` —— 所以它绿=线上那一串 id/type 就是这个。
    hologramlib::lse::exportAs(NAMESPACE, "viewMetaProbe",
        [](std::string const& spec) -> std::string {
            return view::describeMetaItems(parseEntitySpec(spec));
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

    // 能力协商（26.40.10 / 1.28.0 新增）: 本库的视图覆盖是否连 **AddItemActor**(id 15) 一起拦。
    // 为什么脚本需要它: 掉落物(item 实体)的出生包走 AddItemActor, 老库只拦 AddActor —— 那种库上
    // "把掉落物换类型/隐藏"只做一半（引擎随后自己发的出生包会把物品图标塞回去）, 脚本必须周期性重套;
    // 新库拦住了这条, 脚本就不必重套（也就没有那个周期性 remove+respawn 的闪一下）。
    // 返回 true / false, 恒可用（老库没有这个名字 → 脚本 ll.hasExported 判 false, 视为不支持）。
    hologramlib::lse::exportAs(NAMESPACE, "viewItemSpawnHooked",
        []() -> bool {
            return true;
        });
}

} // namespace debugshape_export
