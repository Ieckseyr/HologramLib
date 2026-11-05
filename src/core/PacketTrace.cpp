#include "core/PacketTrace.h"
#include "core/EventIds.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <ll/api/mod/NativeMod.h>
#include <ll/api/service/Bedrock.h>

#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>

#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace debugshape_export::wire {

namespace {

constexpr std::size_t kMaxEntries = 4096;
constexpr std::size_t kMaxStrings = 256;

struct Entry {
    std::uint64_t  seq{};
    std::uint64_t  timeMs{};
    std::uint32_t  id{};
    std::uint32_t  bytes{};
    std::uintptr_t playerKey{};
    std::uint16_t  nameSlot{0xFFFFu};
};

struct State {
    std::atomic<bool>          active{false};
    std::atomic<std::uint64_t> seq{0};

    std::mutex                     mutex;
    std::array<Entry, kMaxEntries> entries{};
    std::size_t                    count{};
    std::size_t                    head{};
    std::string                    tag;
    std::uint64_t                  startedAtMs{};

    std::array<std::string, kMaxStrings> strings{};
    std::size_t                          stringCount{};
};

State& state() {
    static State s;
    return s;
}

std::uint64_t nowMs() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// JSON 写入器
class JsonWriter {
public:
    explicit JsonWriter(std::string& out)
        : mOut(out) { mOut.clear(); }

    void raw(std::string_view text) { mOut.append(text); }

    void key(std::string_view name) {
        if (mNeedComma) {
            mOut.push_back(',');
        }
        mNeedComma = false;
        string(name);
        mOut.push_back(':');
        mAfterKey = true;
    }

    void value(std::string_view text) {
        comma();
        string(text);
    }

    void valueInt(std::uint64_t v) {
        comma();
        char buf[24];
        auto res = std::to_chars(buf, buf + sizeof(buf), v);
        mOut.append(buf, res.ptr);
    }

    void beginObject() {
        comma();
        mOut.push_back('{');
        mNeedComma = false;
    }

    void endObject() {
        mOut.push_back('}');
        mNeedComma = true;
    }

    void beginArray() {
        comma();
        mOut.push_back('[');
        mNeedComma = false;
    }

    void endArray() {
        mOut.push_back(']');
        mNeedComma = true;
    }

private:
    void comma() {
        if (mAfterKey) {
            mAfterKey  = false;
            mNeedComma = true;
            return;
        }
        if (mNeedComma) {
            mOut.push_back(',');
        }
        mNeedComma = true;
    }

    void string(std::string_view s) {
        mOut.push_back('"');
        for (char const c : s) {
            switch (c) {
            case '"':
                mOut.append("\\\"");
                break;
            case '\\':
                mOut.append("\\\\");
                break;
            case '\n':
                mOut.append("\\n");
                break;
            case '\r':
                mOut.append("\\r");
                break;
            case '\t':
                mOut.append("\\t");
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                    mOut.append(buf);
                } else {
                    mOut.push_back(c);
                }
            }
        }
        mOut.push_back('"');
    }

    std::string& mOut;
    bool         mNeedComma{false};
    bool         mAfterKey{false};
};

void logLine(std::string_view text) {
    if (auto mod = ll::mod::NativeMod::current(); mod) {
        mod->getLogger().info("[wire-trace] {}", text);
    }
}

// 名字按内容驻留，避免每条记录各自持有一份字符串
std::uint16_t internName(State& s, std::string_view text) {
    for (std::size_t i = 0; i < s.stringCount; ++i) {
        if (s.strings[i] == text) {
            return static_cast<std::uint16_t>(i);
        }
    }
    if (s.stringCount >= kMaxStrings) {
        return 0xFFFFu;
    }
    s.strings[s.stringCount] = text;
    return static_cast<std::uint16_t>(s.stringCount++);
}

std::string_view nameOf(Entry const& e, State const& s) {
    if (e.nameSlot < s.stringCount) {
        return s.strings[e.nameSlot];
    }
    return {};
}

std::string buildJsonLocked(State const& s, std::string_view reason, std::unordered_map<std::uintptr_t, std::string> const& names) {
    std::string out;
    out.reserve(160 + s.count * 110);

    JsonWriter w(out);
    w.beginObject();
    w.key("reason");
    w.value(reason);
    w.key("tag");
    w.value(s.tag);
    w.key("active");
    w.value(s.active.load(std::memory_order_relaxed) ? "true" : "false");
    w.key("startedAtMs");
    w.valueInt(s.startedAtMs);
    w.key("total");
    w.valueInt(s.count);

    std::array<std::pair<std::string_view, std::uint32_t>, 64> byName{};
    std::size_t                                                kinds = 0;
    for (std::size_t i = 0; i < s.count; ++i) {
        std::size_t const idx = (s.head + kMaxEntries - s.count + i) % kMaxEntries;
        auto              n   = nameOf(s.entries[idx], s);
        std::size_t       k   = 0;
        for (; k < kinds; ++k) {
            if (byName[k].first == n) {
                ++byName[k].second;
                break;
            }
        }
        if (k == kinds && kinds < byName.size()) {
            byName[kinds++] = {n, 1};
        }
    }
    w.key("byName");
    w.beginObject();
    for (std::size_t k = 0; k < kinds; ++k) {
        w.key(byName[k].first.empty() ? std::string_view{"<raw>"} : byName[k].first);
        w.valueInt(byName[k].second);
    }
    w.endObject();

    w.key("entries");
    w.beginArray();
    for (std::size_t i = 0; i < s.count; ++i) {
        std::size_t const idx = (s.head + kMaxEntries - s.count + i) % kMaxEntries;
        Entry const&      e   = s.entries[idx];
        w.beginObject();
        w.key("seq");
        w.valueInt(e.seq);
        w.key("t");
        w.valueInt(e.timeMs);
        w.key("id");
        w.valueInt(e.id);
        w.key("name");
        w.value(nameOf(e, s));
        w.key("player");
        auto const it = names.find(e.playerKey);
        w.value(it == names.end() ? std::string_view{} : std::string_view{it->second});
        w.key("bytes");
        w.valueInt(e.bytes);
        w.endObject();
    }
    w.endArray();
    w.endObject();
    out.push_back('\n');
    return out;
}

std::unordered_map<std::uintptr_t, std::string> collectNames(::Player const* extra, std::string const& extraName) {
    std::unordered_map<std::uintptr_t, std::string> names;
    if (auto level = ::ll::service::getLevel()) {
        level->forEachPlayer([&](::Player& p) -> bool {
            names.emplace(reinterpret_cast<std::uintptr_t>(&p), p.getRealName());
            return true;
        });
    }
    if (extra != nullptr) {
        names.emplace(reinterpret_cast<std::uintptr_t>(extra), extraName);
    }
    return names;
}

// 只有账本里出现过这个玩家才落盘，否则无关掉线会截断追踪
void dumpOnDisconnect(::Player* player, std::string const& playerName) {
    State& s = state();
    if (!s.active.load(std::memory_order_relaxed)) {
        return;
    }

    std::string          json;
    std::size_t          count = 0;
    std::uintptr_t const key   = reinterpret_cast<std::uintptr_t>(player);
    {
        std::lock_guard lock(s.mutex);

        bool relevant = false;
        for (std::size_t i = 0; i < s.count && !relevant; ++i) {
            std::size_t const idx = (s.head + kMaxEntries - s.count + i) % kMaxEntries;
            relevant              = s.entries[idx].playerKey == key;
        }
        if (!relevant) {
            return;
        }

        count = s.count;
        json  = buildJsonLocked(s, "player-disconnect", collectNames(player, playerName));
        s.active.store(false, std::memory_order_relaxed);
    }

    logLine("玩家 '" + playerName + "' 中途退出，共记录 " + std::to_string(count) + " 条发包");

    std::error_code       ec;
    std::filesystem::path dir{"plugins/HologramLib"};
    std::filesystem::create_directories(dir, ec);

    auto const epoch = static_cast<long long>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    auto path = dir / ("wire-trace-" + playerName + '-' + std::to_string(epoch) + ".json");
    if (auto* fp = std::fopen(path.string().c_str(), "wb"); fp != nullptr) {
        std::fwrite(json.data(), 1, json.size(), fp);
        std::fclose(fp);
        logLine("已写入 " + path.string());
    } else {
        logLine("落盘失败: " + path.string());
    }
}

} // namespace

void traceInit() {
    static std::once_flag once;
    std::call_once(once, [] {
        debugshape_export::core::listen<ll::event::player::PlayerDisconnectEvent>(
            debugshape_export::core::kPlayerDisconnectId,
            [](ll::event::player::PlayerDisconnectEvent& ev) {
                if (state().active.load(std::memory_order_relaxed)) {
                    dumpOnDisconnect(&ev.self(), ev.self().getRealName());
                }
            },
            debugshape_export::core::ListenerScope::Process);
    });
}

void traceStart(std::string tag) {
    State&          s = state();
    std::lock_guard lock(s.mutex);
    s.count       = 0;
    s.head        = 0;
    s.stringCount = 0;
    s.tag         = std::move(tag);
    s.startedAtMs = nowMs();
    s.seq.store(0, std::memory_order_relaxed);
    s.active.store(true, std::memory_order_release);
}

void traceStop() {
    state().active.store(false, std::memory_order_release);
}

void traceClear() {
    State&          s = state();
    std::lock_guard lock(s.mutex);
    s.count       = 0;
    s.head        = 0;
    s.stringCount = 0;
    s.seq.store(0, std::memory_order_relaxed);
}

bool traceActive() {
    return state().active.load(std::memory_order_acquire);
}

std::size_t traceCount() {
    State&          s = state();
    std::lock_guard lock(s.mutex);
    return s.count;
}

std::string traceDumpJson() {
    State&          s = state();
    std::lock_guard lock(s.mutex);
    auto const      names = collectNames(nullptr, {});
    return buildJsonLocked(s, "manual-dump", names);
}

void traceMark(std::string_view text) {
    if (!state().active.load(std::memory_order_relaxed)) {
        return;
    }
    State&          s = state();
    std::lock_guard lock(s.mutex);

    Entry& e    = s.entries[s.head];
    e.seq       = s.seq.fetch_add(1, std::memory_order_relaxed) + 1;
    e.timeMs    = nowMs();
    e.id        = 0;
    e.bytes     = 0;
    e.playerKey = 0;
    e.nameSlot  = internName(s, text);
    s.head      = (s.head + 1) % kMaxEntries;
    if (s.count < kMaxEntries) {
        ++s.count;
    }
}

void noteSent(std::string_view name, std::uint32_t id, ::Player& player, std::size_t bytes) {
    if (!state().active.load(std::memory_order_relaxed)) {
        return;
    }
    State&          s = state();
    std::lock_guard lock(s.mutex);

    Entry& e    = s.entries[s.head];
    e.seq       = s.seq.fetch_add(1, std::memory_order_relaxed) + 1;
    e.timeMs    = nowMs();
    e.id        = id;
    e.bytes     = bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<std::uint32_t>(bytes);
    e.playerKey = reinterpret_cast<std::uintptr_t>(&player);
    e.nameSlot  = internName(s, name);
    s.head      = (s.head + 1) % kMaxEntries;
    if (s.count < kMaxEntries) {
        ++s.count;
    }
}

} // namespace debugshape_export::wire

#include "hologramlib/HologramLib.h"

namespace hologramlib {

bool wireTraceStart(std::string const& tag) {
    debugshape_export::wire::traceStart(tag);
    return true;
}

void wireTraceStop() {
    debugshape_export::wire::traceStop();
}
void wireTraceClear() {
    debugshape_export::wire::traceClear();
}
bool wireTraceActive() {
    return debugshape_export::wire::traceActive();
}
std::size_t wireTraceCount() {
    return debugshape_export::wire::traceCount();
}
std::string wireTraceDumpJson() {
    return debugshape_export::wire::traceDumpJson();
}
void wireTraceMark(std::string const& tag) {
    debugshape_export::wire::traceMark(tag);
}

} // namespace hologramlib
