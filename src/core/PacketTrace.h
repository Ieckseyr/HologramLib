#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

class Player;

namespace debugshape_export::wire {

struct TraceEntry {
    std::uint64_t seq{};
    std::uint64_t timeMs{};
    std::uint32_t id{};
    std::size_t   bytes{};
    std::string   name;
    std::string   player;
};

void traceInit();

void        traceStart(std::string tag);
void        traceStop();
void        traceClear();
bool        traceActive();
std::size_t traceCount();
std::string traceDumpJson();

void noteSent(std::string_view name, std::uint32_t id, ::Player& player, std::size_t bytes);

void traceMark(std::string_view text);

} // namespace debugshape_export::wire
