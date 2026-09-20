// NpcDialogueExporter.cpp - NPC 对话框 LSE 导出实现
//
// 脚本侧用法:
//   const id = npcDialogOpen("Steve", "§e村长", "main", "要来点任务吗？",
//                            "§a接受|main#0|0|say 你好;;§c离开|main#1|1|");
//   // 在守护/定时里取点击:
//   const clicks = npcDialogPollClicks();   // "player=.. dialogId=.. scene=.. button=.. actionId=.. closed=.. commands=.."
//   npcDialogOpen("Steve", "§e村长", "accept", "任务已接取。", "");   // 点完按钮换下一层（必须走 open）
//
// 按钮串格式: 按钮之间用 ';' 分隔（写成 ';;' 或 ';' 都行, 空段跳过）, 字段用 '|' 分隔:
//   label|actionId|mode|命令1,命令2
//   mode: 0=普通按钮 1=关闭 2=打开（默认 0）; 命令与 actionId 可留空
//
// 重要实测限制: **点任意按钮时客户端就会自行收起界面**, 而 update() 只重发一次
// NpcDialoguePacket(Open) —— 客户端不会因此重新弹出。所以"点按钮就地换页"不可行:
// 要让客户端重新显示必须再调一次 npcDialogOpen（它会删旧载体、建新载体、再发 Open）。
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
    if (begin == std::string::npos) return {};
    auto const end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

// "label|actionId|mode|cmd1,cmd2" → NpcDialogButton（字段缺失时按默认值）
std::vector<hologramlib::NpcDialogButton> parseButtons(std::string const& spec) {
    std::vector<hologramlib::NpcDialogButton> out;
    for (auto const& raw : split(spec, ';')) {
        auto const entry = trim(raw);
        if (entry.empty()) continue;
        auto const fields = split(entry, '|');
        hologramlib::NpcDialogButton button;
        button.label    = trim(fields[0]);
        if (fields.size() > 1) button.actionId = trim(fields[1]);
        if (fields.size() > 2 && !trim(fields[2]).empty()) button.mode = std::stoi(trim(fields[2]));
        if (fields.size() > 3 && !trim(fields[3]).empty()) {
            for (auto const& cmd : split(trim(fields[3]), ',')) {
                if (!trim(cmd).empty()) button.commands.push_back(trim(cmd));
            }
        }
        if (button.label.empty()) button.label = "按钮";
        out.push_back(std::move(button));
    }
    return out;
}

} // namespace

void NpcDialogueExporter::exportAll() {
    auto& mgr = NpcDialogueManager::getInstance();

    // npcDialogOpen(playerName, npcName, sceneName, dialogue, buttonsSpec, carrierIdentifier) -> id
    //   打开对话（同一玩家重复调用会先关掉上一层）; carrierIdentifier 空 = 默认 minecraft:npc
    hologramlib::lse::exportAs(
        NAMESPACE,
        "npcDialogOpen",
        [&mgr](
            std::string const& playerName,
            std::string const& npcName,
            std::string const& sceneName,
            std::string const& dialogue,
            std::string const& buttonsSpec,
            std::string const& carrierIdentifier
        ) -> int64_t {
            hologramlib::NpcDialogSpec spec;
            if (!npcName.empty()) spec.npcName = npcName;
            if (!sceneName.empty()) spec.sceneName = sceneName;
            spec.dialogue = dialogue;
            spec.buttons  = parseButtons(buttonsSpec);
            if (!carrierIdentifier.empty()) spec.carrierIdentifier = carrierIdentifier;
            return mgr.open(playerName, spec);
        });

    // npcDialogUpdate(id, dialogue, buttonsSpec) -> bool
    // 就地换内容（**只适合界面仍开着时的微调**; 点按钮后的换页必须重新 open, 见文件头）
    hologramlib::lse::exportAs(
        NAMESPACE,
        "npcDialogUpdate",
        [&mgr](int64_t id, std::string const& dialogue, std::string const& buttonsSpec) -> bool {
            // 先读出当前规格再改: update 是整体替换, 只传正文会把 npcName / sceneName / 载体类型冲掉
            hologramlib::NpcDialogSpec spec;
            if (!mgr.getSpec(id, spec)) return false;
            spec.dialogue = dialogue;
            spec.buttons  = parseButtons(buttonsSpec);
            return mgr.update(id, spec);
        });

    // npcDialogClose(id) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "npcDialogClose", [&mgr](int64_t id) -> bool { return mgr.close(id); });

    // npcDialogCloseAll() -> void
    hologramlib::lse::exportAs(NAMESPACE, "npcDialogCloseAll", [&mgr]() -> void { mgr.closeAll(); });

    // npcDialogIsOpen(id) -> bool
    hologramlib::lse::exportAs(
        NAMESPACE, "npcDialogIsOpen", [&mgr](int64_t id) -> bool { return mgr.isOpen(id); });

    // npcDialogGetIds() -> [i]
    hologramlib::lse::exportAs(
        NAMESPACE, "npcDialogGetIds", [&mgr]() -> std::vector<int64_t> { return mgr.getAllIds(); });

    // npcDialogPollClicks() -> [s]（取走并清空待处理点击/关闭; 条目见 formatClick）
    hologramlib::lse::exportAs(NAMESPACE, "npcDialogPollClicks", []() -> std::vector<std::string> {
        auto                     events = NpcDialogueManager::getInstance().pollClicks();
        std::vector<std::string> out;
        out.reserve(events.size());
        for (auto const& ev : events) out.push_back(NpcDialogueManager::formatClick(ev));
        return out;
    });

    // npcDialogClearClicks() -> void
    hologramlib::lse::exportAs(
        NAMESPACE, "npcDialogClearClicks", []() -> void { NpcDialogueManager::getInstance().clearClicks(); });
}

} // namespace debugshape_export
