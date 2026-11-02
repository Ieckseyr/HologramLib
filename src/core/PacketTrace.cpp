// PacketTrace.cpp - 发包审计实现
#include "core/PacketTrace.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/mod/NativeMod.h>

#include <mc/world/actor/player/Player.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <mutex>
#include <vector>

namespace debugshape_export::wire {

namespace {

constexpr std::size_t kMaxEntries = 4096;

struct State {
    std::mutex               mutex;
    std::vector<TraceEntry>  entries;
    std::uint64_t            seq{0};
    bool                     active{false};
    std::string              tag;
    std::uint64_t            startedAtMs{0};
};

State& state() {
    static State s;
    return s;
}

std::uint64_t nowMs() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

std::string esc(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out.push_back(c);
        }
    }
    return out;
}

std::string buildJsonLocked(State& s, std::string_view reason) {
    std::map<std::string, int> byName;
    for (auto const& e : s.entries) {
        byName[e.name.empty() ? std::string{"<raw>"} : e.name] += 1;
    }

    std::string out;
    out += "{\n";
    out += std::format("  \"reason\": \"{}\",\n", esc(reason));
    out += std::format("  \"tag\": \"{}\",\n", esc(s.tag));
    out += std::format("  \"active\": {},\n", s.active ? "true" : "false");
    out += std::format("  \"startedAtMs\": {},\n", s.startedAtMs);
    out += std::format("  \"total\": {},\n", s.entries.size());

    out += "  \"byName\": {";
    bool first = true;
    for (auto const& [name, count] : byName) {
        if (!first) out += ", ";
        first = false;
        out += std::format("\"{}\": {}", esc(name), count);
    }
    out += "},\n";

    out += "  \"entries\": [\n";
    for (std::size_t i = 0; i < s.entries.size(); ++i) {
        auto const& e = s.entries[i];
        out += std::format(
            "    {{\"seq\": {}, \"t\": {}, \"id\": {}, \"name\": \"{}\", \"player\": \"{}\", \"bytes\": {}}}{}\n",
            e.seq,
            e.timeMs,
            e.id,
            esc(e.name),
            esc(e.player),
            e.bytes,
            i + 1 == s.entries.size() ? "" : ","
        );
    }
    out += "  ]\n}\n";
    return out;
}

void logLine(std::string const& text) {
    try {
        auto mod = ll::mod::NativeMod::current();
        if (mod) {
            mod->getLogger().info("[wire-trace] {}", text);
        }
    } catch (...) {
    }
}

// 断线时把"退出前发过的包"落盘
void dumpOnDisconnect(std::string const& playerName) {
    State& s = state();

    std::string json;
    std::size_t count = 0;
    {
        std::lock_guard lock(s.mutex);
        if (!s.active || s.entries.empty()) {
            return;
        }
        count = s.entries.size();
        json  = buildJsonLocked(s, "player-disconnect:" + playerName);
    }

    // 逐包统计（取最后的快照，避免持锁太久）
    std::map<std::string, int> byName;
    std::vector<std::string>   tail;
    {
        std::lock_guard lock(s.mutex);
        for (auto const& e : s.entries) {
            byName[e.name.empty() ? std::string{"<raw>"} : e.name] += 1;
        }
        std::size_t const from = s.entries.size() > 10 ? s.entries.size() - 10 : 0;
        for (std::size_t i = from; i < s.entries.size(); ++i) {
            auto const& e = s.entries[i];
            tail.push_back(std::format("#{} {} {}B -> {}", e.seq, e.name, e.bytes, e.player));
        }
    }

    logLine(std::format("玩家 '{}' 中途退出，退出前共发出 {} 个包", playerName, count));
    for (auto const& [name, c] : byName) {
        logLine(std::format("  {} x{}", name, c));
    }
    for (auto const& t : tail) {
        logLine(std::format("  tail {}", t));
    }

    try {
        std::filesystem::path dir{"plugins/HologramLib"};
        std::error_code       ec;
        std::filesystem::create_directories(dir, ec);
        auto const epoch = static_cast<long long>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count()
        );
        auto path = dir / std::format("wire-trace-{}-{}.json", playerName, epoch);
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (file) {
            file << json;
            logLine(std::format("已写入 {}", path.string()));
        }
    } catch (...) {
    }

    traceStop();
}

} // namespace

void traceInit() {
    static std::once_flag once;
    std::call_once(once, [] {
        try {
            ll::event::EventBus::getInstance().emplaceListener<ll::event::player::PlayerDisconnectEvent>(
                [](ll::event::player::PlayerDisconnectEvent& ev) {
                    try {
                        dumpOnDisconnect(ev.self().getRealName());
                    } catch (...) {
                    }
                }
            );
        } catch (...) {
        }
    });
}

void traceStart(std::string tag) {
    State& s = state();
    {
        std::lock_guard lock(s.mutex);
        s.entries.clear();
        s.seq        = 0;
        s.active     = true;
        s.tag        = std::move(tag);
        s.startedAtMs = nowMs();
    }
    logLine(std::format("trace 开始: {}", tag));
}

void traceStop() {
    State& s = state();
    std::lock_guard lock(s.mutex);
    s.active = false;
}

void traceClear() {
    State& s = state();
    std::lock_guard lock(s.mutex);
    s.entries.clear();
    s.seq = 0;
}

bool traceActive() {
    State& s = state();
    std::lock_guard lock(s.mutex);
    return s.active;
}

std::size_t traceCount() {
    State& s = state();
    std::lock_guard lock(s.mutex);
    return s.entries.size();
}

std::string traceDumpJson() {
    State& s = state();
    std::lock_guard lock(s.mutex);
    return buildJsonLocked(s, "manual-dump");
}

void traceMark(std::string_view text) {
    State& s = state();
    std::lock_guard lock(s.mutex);
    if (!s.active) {
        return;
    }
    TraceEntry e;
    e.seq    = ++s.seq;
    e.timeMs = nowMs();
    e.id     = 0;
    e.bytes  = 0;
    e.name   = "== " + std::string{text} + " ==";
    e.player = "";
    s.entries.push_back(std::move(e));
}

void noteSent(std::string_view name, std::uint32_t id, ::Player& player, std::size_t bytes) {
    State& s = state();
    std::lock_guard lock(s.mutex);
    if (!s.active) {
        return;
    }
    if (s.entries.size() >= kMaxEntries) {
        s.entries.erase(s.entries.begin(), s.entries.begin() + static_cast<std::ptrdiff_t>(kMaxEntries / 4));
    }
    TraceEntry e;
    e.seq    = ++s.seq;
    e.timeMs = nowMs();
    e.id     = id;
    e.bytes  = bytes;
    e.name.assign(name);
    try {
        e.player = player.getRealName();
    } catch (...) {
        e.player = "?";
    }
    s.entries.push_back(std::move(e));
}

} // namespace debugshape_export::wire

// ── 对外 C++ API（HOLOGLIB_API 导出，供原生消费方/测试插件用）──
#include "hologramlib/HologramLib.h"

namespace hologramlib {

bool wireTraceStart(std::string const& tag) {
    debugshape_export::wire::traceStart(tag);
    return true;
}

void wireTraceStop() { debugshape_export::wire::traceStop(); }

void wireTraceClear() { debugshape_export::wire::traceClear(); }

bool wireTraceActive() { return debugshape_export::wire::traceActive(); }

std::size_t wireTraceCount() { return debugshape_export::wire::traceCount(); }

std::string wireTraceDumpJson() { return debugshape_export::wire::traceDumpJson(); }

void wireTraceMark(std::string const& tag) { debugshape_export::wire::traceMark(tag); }

} // namespace hologramlib
